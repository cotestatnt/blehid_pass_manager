#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_hmac.h"
#include "mbedtls/aes.h"

#include "hid_device_prf.h"
#include "user_list.h"
#include "display_oled.h"


user_entry_t user_list[MAX_USERS];  // Lista degli account
size_t user_count = 0;              // Numero totale degli account memorizzati
int user_index = -1;                // Indice dell'account attualmente selezionato


#define NVS_NAMESPACE "userdb"
#define NVS_KEY "users"
#define AES_BLOCK_SIZE 16
uint8_t decrypt_key[16];

static const char *TAG = "USER_DB";


void send_ble_message(const char* message, uint8_t type);

// Derives a secure 128-bit key from eFuse HMAC_KEY0
esp_err_t get_device_key_hmac(uint8_t out_key[16]) {
    const char* context = "userdb-password-key";
    return esp_hmac_calculate(HMAC_KEY0, (const uint8_t*)context, strlen(context), out_key);
}

int userdb_encrypt_password(const char* plain, uint8_t* out_encrypted) {
    size_t len = strlen(plain);
    size_t padded_len = ((len + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    if (padded_len > MAX_PASSWORD_LEN) 
        return -1;

    uint8_t iv[16] = {0};  // Static IV; for more security it can be randomized
    uint8_t input[128] = {0};
    memcpy(input, plain, len);  // padding with 0 (you can use PKCS#7 if you prefer)
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, decrypt_key, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_len, iv, input, out_encrypted);
    mbedtls_aes_free(&aes);
    return padded_len;
}

int userdb_decrypt_password(const uint8_t* encrypted, size_t len, char* out_plain) {
    if (len % AES_BLOCK_SIZE != 0 || len > MAX_PASSWORD_LEN) 
        return -1;
    uint8_t iv[16] = {0};
    uint8_t output[128] = {0};
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, decrypt_key, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, len, iv, encrypted, output);
    mbedtls_aes_free(&aes);
    memcpy(out_plain, output, len);
    out_plain[len] = '\0';
    return 0;
}


void user_print(user_entry_t* user) {    
    for (size_t i = 0; i < user_count; ++i) {
        printf(" [%d] User: %s\n", (int)i, user->label);
        #if DEBUG_PASSWD
        printf("     Password (hex): ");
        for (size_t j = 0; j < user->password_len; ++j) {
            printf("%02X ", user->password_enc[j]);
        }
        printf("\n");
        #endif
        printf("     Usage count: %lu\n", user->usage_count);
        printf("     Fingerprint ID: %02d\n", user->fingerprint_id);
        printf("     MagicFinder: %s\n", user->magicfinger ? "enabled" : "disabled");
        printf("     Winlogin: %s\n", user->winlogin ? "enabled" : "disabled");
        printf("     Login type: %d\n", user->login_type);
    }
}

void userdb_dump() {
    printf("=== USER LIST (%d users) ===\n", (int)user_count);
    for (size_t i = 0; i < user_count; ++i) {
        user_print(&user_list[i]);
    }
    printf("============================\n");
}

// Saves the user list and count to NVS
void userdb_save() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving users list");
        return;
    }
    nvs_set_blob(handle, NVS_KEY, user_list, sizeof(user_entry_t) * user_count);
    nvs_set_u32(handle, "count", user_count);
    nvs_commit(handle);
    nvs_close(handle);
    printf("Users list saved (%d records)\n", user_count);
}

// Loads the user list and count from NVS
void userdb_load() {
    if (get_device_key_hmac(decrypt_key) != ESP_OK)
        ESP_LOGE(TAG, "Error getting decrypt key");

    nvs_handle_t handle;
    size_t required_size = sizeof(user_list);
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        user_count = 0;
        ESP_LOGE(TAG, "Error loading users list");
        return;
    }
    
    user_count = 0;
    memset(user_list, 0, sizeof(user_list));

    nvs_get_blob(handle, NVS_KEY, user_list, &required_size);
    uint32_t count = 0;
    nvs_get_u32(handle, "count", &count);
    user_count = count;
    nvs_close(handle);
    user_index = -1;  // Reset index at startup
}

// Adds a new user 
int userdb_add(user_entry_t* user) {
    if (user_count >= MAX_USERS) 
        return -1;
    user_print(user);
    user_list[user_count] = *user; // Copy the complete structure
    user_list[user_count].usage_count = 0; // Initialize usage counter
    user_count++;
    userdb_save();
    userdb_load();
    display_oled_post_info("User added");
    ESP_LOGI(TAG, "User added: %s", user->label);
    userdb_dump();
    return user_count;
}


void userdb_edit(int index, user_entry_t* user){
    if (index < 0 || index >= user_count) 
        return ;

    user_print(user);
    user_list[index] = *user; // Copy the complete structure
    userdb_save();
    userdb_load();
    display_oled_post_info("User update");
    ESP_LOGI(TAG, "User updated: %s", user->label);    
}


// Removes a user given the index
int userdb_remove(int index) {    

    display_oled_post_info("rem user");    
    if (index < 0 || index >= user_count) return -1;
    for (size_t i = index; i < user_count - 1; ++i) {
        user_list[i] = user_list[i + 1];
    }
    user_count--;
    userdb_save();
    userdb_load(); 
    display_oled_post_info("User removed");
    ESP_LOGI(TAG, "User removed at index: %d", index);    
    return 0;
}

// Increments the usage counter of a user
void userdb_increment_usage(int index) {
    if (index < 0 || index >= user_count) return;
    user_list[index].usage_count++;
    userdb_sort_by_usage();
    userdb_save();
}

// Sorts the user list by usage frequency (descending)
void userdb_sort_by_usage() {
    for (size_t i = 0; i < user_count - 1; ++i) {
        for (size_t j = i + 1; j < user_count; ++j) {
            if (user_list[j].usage_count > user_list[i].usage_count) {
                user_entry_t tmp = user_list[i];
                user_list[i] = user_list[j];
                user_list[j] = tmp;
            }
        }
    }
}

// Cancella tutto il DB degli utenti dalla flash
void userdb_clear() {
    display_oled_post_info("clear users");
    user_count = 0;
    user_index = -1;
    memset(user_list, 0, sizeof(user_list));
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE("userdb", "Errore apertura NVS: %s", esp_err_to_name(err));
        return;
    }
    nvs_erase_key(handle, NVS_KEY);     // "users"
    nvs_erase_key(handle, "count");     // contatore utenti
    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI("userdb", "Database utenti cancellato");
    display_oled_post_info("DB cleared!");
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

int send_user_entry(int8_t index) {
    if (index < 0 || index >= user_count) {
        ESP_LOGW(TAG, "Index not valid or end of list (%d)\n", user_count);
    }
    
    size_t payload_size = 0;
    uint8_t payload_data[128] = {0};

    if (index < MAX_USERS && user_count ) {
        user_entry_t entry = user_list[index];
        size_t size = sizeof(entry);

        payload_data[payload_size++] = 0xA1; // Command to send a user
        payload_data[payload_size++] = index; // Current user index
        if (size > sizeof(payload_data) - 2) {
            ESP_LOGE(TAG, "User entry size exceeds maximum payload size");
            return -1;
        }

        // Set label and password in the payload
        memcpy(&payload_data[payload_size], entry.label, MAX_LABEL_LEN);
        payload_size += MAX_LABEL_LEN;

        char plain[128] = {0};
        userdb_decrypt_password(entry.password_enc, entry.password_len, plain);
        memcpy(&payload_data[payload_size], plain, MAX_PASSWORD_LEN);
        payload_size += MAX_PASSWORD_LEN;

        payload_data[payload_size++] = entry.winlogin ? 1 : 0; 
        payload_data[payload_size++] = entry.magicfinger ? 1 : 0; 
        payload_data[payload_size++] = entry.fingerprint_id;
        payload_data[payload_size++] = entry.login_type;

        memset(plain, 0, sizeof(plain));        
    }
    
    esp_ble_gatts_send_indicate(
        hidd_le_env.gatt_if,
        user_mgmt_conn_id,
        user_mgmt_handle[USER_MGMT_IDX_VAL],
        payload_size,
        (uint8_t *)&payload_data,
        true
    );

    return (index >= user_count) ? -1 : user_count;
}


void send_authenticated(bool auth) {
    user_mgmt_payload_t payload = {0};
    payload.cmd = 0x99;  // Command to indicate that the user is not authenticated
    payload.index = 0;   // Doesn't make sense in this context, but needed to maintain structure
    payload.data[0] = auth ? 1 : 0; // 1 if authenticated, 0 otherwise

    esp_ble_gatts_send_indicate(
        hidd_le_env.gatt_if,
        user_mgmt_conn_id,
        user_mgmt_handle[USER_MGMT_IDX_VAL],
        sizeof(payload),
        (uint8_t *)&payload,
        true
    );

    ESP_LOGI(TAG, "Authenticated message: %s", auth ? "true" : "false");
    ESP_LOGI(TAG, "gatt_if: %d, conn_id: %d, handle: %d\n", hidd_le_env.gatt_if, user_mgmt_conn_id, user_mgmt_handle[USER_MGMT_IDX_VAL]);
    auth ? display_oled_set_text("Auth!") : display_oled_set_text("No auth!");
}


void send_ble_message(const char* message, uint8_t type) {
    user_mgmt_payload_t payload = {0};
    payload.cmd = 0xAA;  // Command to send a generic message
    payload.index = type; // 0x00 Info, 0x01 Warning, 0x02 Error

    strncpy(payload.data, message, sizeof(payload.data) - 1);        
    ESP_LOGI(TAG, "[%s] Message: %s", type ? "error": "info", (char*) payload.data);

    esp_ble_gatts_send_indicate(
        hidd_le_env.gatt_if,
        user_mgmt_conn_id,
        user_mgmt_handle[USER_MGMT_IDX_VAL],
        sizeof(payload),
        (uint8_t *)&payload,
        true
    );

    ESP_LOGI(TAG, "gatt_if: %d, conn_id: %d, handle: %d\n",
             hidd_le_env.gatt_if, user_mgmt_conn_id, user_mgmt_handle[USER_MGMT_IDX_VAL]);
}

void send_db_cleared() {
    user_mgmt_payload_t payload = {0};
    payload.cmd = 0xFF;  // Command to indicate that the db has been cleared
    payload.index = 0;   // Doesn't make sense in this context, but needed to maintain structure

    strcpy(payload.data, "User DB cleared");

    esp_ble_gatts_send_indicate(
        hidd_le_env.gatt_if,
        user_mgmt_conn_id,
        user_mgmt_handle[USER_MGMT_IDX_VAL],
        sizeof(payload),
        (uint8_t *)&payload,
        false
    );

    ESP_LOGI(TAG, "User DB cleared");
}
