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

#include "hid_device_le_prf.h"
#include "user_list.h"
#include "oled.h"


user_entry_t user_list[MAX_USERS]; // Lista degli account
size_t user_count = 0;             // Numero totale degli account memorizzati
volatile int user_index = 0;       // Indice dell'account attualmente selezionato

// Oled handling
int display_reset_pending = 0;      // Reset del display dopo 15 secondi
uint32_t last_interaction_time = 0; // Memorizza il momento in cui viene scritto qualcosa sul display

#define NVS_NAMESPACE "userdb"
#define NVS_KEY "users"
#define AES_BLOCK_SIZE 16
uint8_t decrypt_key[16];

static const char *TAG = "USER_DB";


void send_ble_message(const char* message, uint8_t type);

// Deriva una chiave sicura a 128 bit da eFuse HMAC_KEY0
esp_err_t get_device_key_hmac(uint8_t out_key[16]) {
    const char* context = "userdb-password-key";
    return esp_hmac_calculate(HMAC_KEY0, (const uint8_t*)context, strlen(context), out_key);
}

int userdb_encrypt_password(const char* plain, uint8_t* out_encrypted, size_t* out_len) {
    size_t len = strlen(plain);
    size_t padded_len = ((len + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    if (padded_len > MAX_PASSWORD_LEN) 
        return -1;

    uint8_t iv[16] = {0};  // IV statico; per più sicurezza si può randomizzare
    uint8_t input[128] = {0};
    memcpy(input, plain, len);  // padding con 0 (puoi usare PKCS#7 se preferisci)
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, decrypt_key, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_len, iv, input, out_encrypted);
    mbedtls_aes_free(&aes);
    *out_len = padded_len;
    return 0;
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

// void userdb_init_test_data() {
//     if (get_device_key_hmac(decrypt_key) != ESP_OK)
//         ESP_LOGE(TAG, "Error getting decrypt key");

//     user_count = 0;

//     uint8_t pswd[MAX_PASSWORD_LEN];
//     size_t len;

//     userdb_encrypt_password("123456789", pswd, &len);
//     userdb_add("PIN Banca", pswd, len);

//     userdb_encrypt_password("XTA?=XXcts))", pswd, &len);
//     userdb_add("CSR Tolentino", pswd, len);

//     userdb_encrypt_password("Ndirondirondello?", pswd, &len);
//     userdb_add("Windows11", pswd, len);

//     // userdb_increment_usage(2);
//     // userdb_increment_usage(2);
//     // userdb_increment_usage(0);
//     user_index = -1;  // Reset index all'avvio
// }

void userdb_dump() {
    printf("=== USER LIST (%d users) ===\n", (int)user_count);
    for (size_t i = 0; i < user_count; ++i) {
        printf(" [%d] Label: %s\n", (int)i, user_list[i].label);
        #if DEBUG_PASSWD
        printf("      Password (hex): ");
        for (size_t j = 0; j < user_list[i].password_len; ++j) {
            printf("%02X ", user_list[i].password_enc[j]);
        }
        printf("\n");
        #endif
        printf("      Usage count: %lu\n", user_list[i].usage_count);
    }
    printf("=============================\n");
}

// Salva la lista utenti e il conteggio su NVS
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

// Carica la lista utenti e il conteggio da NVS
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
    user_index = -1;
    memset(user_list, 0, sizeof(user_list));

    nvs_get_blob(handle, NVS_KEY, user_list, &required_size);
    uint32_t count = 0;
    nvs_get_u32(handle, "count", &count);
    user_count = count;
    nvs_close(handle);
    user_index = -1;  // Reset index all'avvio
}

// Aggiunge un nuovo utente (label e password in chiaro, da cifrare prima di chiamare questa funzione)
// int userdb_add(const char* label, const uint8_t* password_enc, size_t enc_len) {
//     if (user_count >= MAX_USERS) 
//         return -1;
    
//     strncpy(user_list[user_count].label, label, MAX_LABEL_LEN - 1);
//     user_list[user_count].label[MAX_LABEL_LEN - 1] = 0;

//     if (enc_len > MAX_PASSWORD_LEN) 
//         enc_len = MAX_PASSWORD_LEN;
    
//     memcpy(user_list[user_count].password_enc, password_enc, enc_len);
//     user_list[user_count].password_len = enc_len;
//     user_list[user_count].usage_count = 0;
//     user_count++;
//     userdb_save();

//     oled_write_text("User added", true);
//     ESP_LOGI(TAG, "User added: %s", label);
//     return user_count;
// }


// Aggiorna label e password di un utente
// int userdb_update(int index, const char* new_label, const char* new_password_enc) {
//     if (index < 0 || index >= user_count) return -1;
//     strncpy(user_list[index].label, new_label, MAX_LABEL_LEN - 1);
//     user_list[index].label[MAX_LABEL_LEN - 1] = 0;
//     size_t len = strlen(new_password_enc);
//     if (len > MAX_PASSWORD_LEN) 
//         len = MAX_PASSWORD_LEN;
//     memcpy(user_list[index].password_enc, new_password_enc, len);
//     user_list[index].password_len = len;
//     userdb_save();
//     send_ble_message("User updated", 0x00);
//     return 0;
// }


int userdb_add(user_entry_t* user) {
    if (user_count >= MAX_USERS) 
        return -1;

    // strncpy(user_list[user_count].label, user->label, MAX_LABEL_LEN - 1);
    

    // uint8_t encoded[MAX_PASSWORD_LEN];
    // size_t len = 0;
    // userdb_encrypt_password((const char*)pswd, encoded, &len);

    // memcpy(user_list[user_count].password_enc, encoded, len);
    // user_list[user_count].password_len = len;
    // user_list[user_count].usage_count = 0;
    // user_count++;

    user_list[user_count] = *user; // Copia la struttura completa
    user_list[user_count].usage_count = 0; // Inizializza il contatore di utilizzo
    userdb_save();

    oled_write_text("User added", true);
    ESP_LOGI(TAG, "User added: %s", user->label);
    return user_count;
}


void userdb_edit(int index, user_entry_t* user){
    if (index < 0 || index >= user_count) 
        return ;
    // strncpy(user_list[index].label, user->label, MAX_LABEL_LEN - 1);
    // user_list[index].label[MAX_LABEL_LEN - 1] = 0;
    
    // size_t len = 0;
    // uint8_t encoded[MAX_PASSWORD_LEN];
    // userdb_encrypt_password((const char*)pswd, encoded, &len);

    // memcpy(user_list[index].password_enc, encoded, len);
    // user_list[index].password_len = user->password_len;

    user_list[index] = *user; // Copia la struttura completa
    userdb_save();
    oled_write_text("User updated", true);
    ESP_LOGI(TAG, "User updated: %s", user->label);
}


// Rimuove un utente dato l'indice
int userdb_remove(int index) {
    if (index < 0 || index >= user_count) return -1;
    for (size_t i = index; i < user_count - 1; ++i) {
        user_list[i] = user_list[i + 1];
    }
    user_count--;
    userdb_save();

    userdb_load(); 
    oled_write_text("User removed", true);
    ESP_LOGI(TAG, "User removed at index: %d", index);
    return 0;
}

// Incrementa il contatore di utilizzo di un utente
void userdb_increment_usage(int index) {
    if (index < 0 || index >= user_count) return;
    user_list[index].usage_count++;
    userdb_sort_by_usage();
    userdb_save();
}

// Ordina la lista utenti per frequenza di utilizzo (decrescente)
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
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void send_user(uint8_t index) {
    user_mgmt_payload_t payload = {0};
    payload.cmd = 0x04;
    payload.index = index;
    
    if (index < MAX_USERS && user_count ) {
        strcpy(payload.data, user_list[index].label);        
    }  

    ESP_LOGI(TAG, "Username: %s\n", (char*) payload.data);
   
    esp_ble_gatts_send_indicate(
        hidd_le_env.gatt_if,
        user_mgmt_conn_id,
        user_mgmt_handle[USER_MGMT_IDX_VAL],
        sizeof(payload),
        (uint8_t *)&payload,
        true
    );
}

void send_password(uint8_t index) {
    user_mgmt_payload_t payload = {0};
    payload.cmd = 0x05;
    payload.index = index;

    if (index < MAX_USERS && user_count ) {
        char plain[128];
        userdb_decrypt_password(user_list[index].password_enc, user_list[index].password_len, plain);        
        strcpy(payload.data, plain);        
        #if DEBUG_PASSWD
        ESP_LOGI(TAG, "User password: %s\n", plain);
        #endif
    }
   
    esp_ble_gatts_send_indicate(
        hidd_le_env.gatt_if,
        user_mgmt_conn_id,
        user_mgmt_handle[USER_MGMT_IDX_VAL],
        sizeof(payload),
        (uint8_t *)&payload,
        true
    );
}

void send_winlogin(uint8_t index) {
    user_mgmt_payload_t payload = {0};
    payload.cmd = 0x06;
    payload.index = index;

    if (index < MAX_USERS && user_count ) {
        payload.data[0] = user_list[index].winlogin ? 1 : 0; // 1 se è un login Windows, 0 altrimenti
        ESP_LOGI(TAG, "Winlogin: %d\n", payload.data[0]);
    }  

    esp_ble_gatts_send_indicate(
        hidd_le_env.gatt_if,
        user_mgmt_conn_id,
        user_mgmt_handle[USER_MGMT_IDX_VAL],
        sizeof(payload),
        (uint8_t *)&payload,
        true
    );
}



void send_user_entry(int8_t index) {
    if (index < 0 || index >= user_count) {
        ESP_LOGW(TAG, "Invalid user index: %d (end of list)\n", index);
    }
    
    size_t payload_size = 0;
    uint8_t payload_data[128] = {0};

    if (index < MAX_USERS && user_count ) {
        user_entry_t entry = user_list[index];
        size_t size = sizeof(entry);

        payload_data[payload_size++] = 0xA1; // Comando per inviare un utente
        payload_data[payload_size++] = index; // Indice dell'utente corrente
        if (size > sizeof(payload_data) - 2) {
            ESP_LOGE(TAG, "User entry size exceeds maximum payload size");
            return;
        }

        // Set label and password in the payload
        memcpy(&payload_data[payload_size], entry.label, MAX_LABEL_LEN);
        payload_size += MAX_LABEL_LEN;

        char plain[128] = {0};
        userdb_decrypt_password(entry.password_enc, entry.password_len, plain);
        memcpy(&payload_data[payload_size], plain, MAX_PASSWORD_LEN);
        payload_size += MAX_PASSWORD_LEN;

        payload_data[payload_size++] = entry.winlogin ? 1 : 0; 
        payload_data[payload_size++] = entry.auto_fingerprint ? 1 : 0; 
        payload_data[payload_size++] = entry.footprintIndex;
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
}


void send_authenticated(bool auth) {
    user_mgmt_payload_t payload = {0};
    payload.cmd = 0x99;  // Comando per indicare che l'utente non è autenticato
    payload.index = 0;   // Non ha senso in questo contesto, ma serve per mantenere la struttura

    payload.data[0] = auth ? 1 : 0; // 1 se autenticato, 0 altrimenti

    esp_ble_gatts_send_indicate(
        hidd_le_env.gatt_if,
        user_mgmt_conn_id,
        user_mgmt_handle[USER_MGMT_IDX_VAL],
        sizeof(payload),
        (uint8_t *)&payload,
        true
    );

    ESP_LOGI(TAG, "Authenticated message: %s", auth ? "true" : "false");
    ESP_LOGI(TAG, "gatt_if: %d, conn_id: %d, handle: %d\n",
             hidd_le_env.gatt_if, user_mgmt_conn_id, user_mgmt_handle[USER_MGMT_IDX_VAL]);
}


void send_ble_message(const char* message, uint8_t type) {
    user_mgmt_payload_t payload = {0};
    payload.cmd = 0xAA;  // Comando per inviare un messaggio generico
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
    payload.cmd = 0xFF;  // Comando per indicare che il db è stato cancellato
    payload.index = 0;   // Non ha senso in questo contesto, ma serve per mantenere la struttura

    strcpy(payload.data, "Userd DB cleared");

    esp_ble_gatts_send_indicate(
        hidd_le_env.gatt_if,
        user_mgmt_conn_id,
        user_mgmt_handle[USER_MGMT_IDX_VAL],
        sizeof(payload),
        (uint8_t *)&payload,
        false
    );

    ESP_LOGI(TAG, "Userd DB cleared");
}


void userdb_set_username(int index, const char* username, size_t len) {

    printf("Setting username for index %d: %.*s\n", index, (int)len, username);

    if (index >= 0 && index < user_count) {
        // Modifica: aggiorna username esistente
        memset(user_list[index].label, 0, MAX_LABEL_LEN);
        strncpy(user_list[index].label, username, len < MAX_LABEL_LEN ? len : MAX_LABEL_LEN - 1);
        user_list[index].label[MAX_LABEL_LEN - 1] = '\0';
        userdb_save();
    } else if (index == user_count && user_count < MAX_USERS) {
        // Nuovo inserimento: crea nuovo utente con solo username
        memset(&user_list[user_count], 0, sizeof(user_entry_t));
        strncpy(user_list[user_count].label, username, len < MAX_LABEL_LEN ? len : MAX_LABEL_LEN - 1);
        user_list[user_count].label[MAX_LABEL_LEN - 1] = '\0';
        user_list[user_count].usage_count = 0;
        user_list[user_count].password_len = 0;
        user_count++;
        userdb_save();
    }
}

void userdb_set_password(int index, const char* password, size_t len) {
    printf("Setting password for index %d\n", index);

    if (index >= 0 && index < user_count) {
        // Modifica: aggiorna password esistente (criptata)
        uint8_t encrypted[MAX_PASSWORD_LEN];
        size_t enc_len = 0;
        userdb_encrypt_password(password, encrypted, &enc_len);
        if (enc_len > MAX_PASSWORD_LEN) enc_len = MAX_PASSWORD_LEN;

        // Pulisci la password precedente
        memset(user_list[index].password_enc, 0, MAX_PASSWORD_LEN);
        memcpy(user_list[index].password_enc, encrypted, enc_len);
        user_list[index].password_len = enc_len;
        userdb_save();
    } else if (index == user_count && user_count < MAX_USERS) {
        // Nuovo inserimento: crea nuovo utente con solo password
        memset(&user_list[user_count], 0, sizeof(user_entry_t));
        uint8_t encrypted[MAX_PASSWORD_LEN];
        size_t enc_len = 0;
        userdb_encrypt_password(password, encrypted, &enc_len);
        if (enc_len > MAX_PASSWORD_LEN) enc_len = MAX_PASSWORD_LEN;
        memcpy(user_list[user_count].password_enc, encrypted, enc_len);
        user_list[user_count].password_len = enc_len;
        user_list[user_count].usage_count = 0;
        user_count++;
        userdb_save();
    }
}

void userdb_set_winlogin(int index, bool winlogin) {
    printf("Setting winlogin for index %d: %d\n", index, winlogin);

    if (index >= 0 && index < user_count) {
        user_list[index].winlogin = winlogin;
        userdb_save();
    }
}