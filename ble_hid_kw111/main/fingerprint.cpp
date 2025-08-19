#include "esp_log.h"

#include "fingerprint.h"
#include "oled.h"
#include "user_list.h"
#include "ble_device_main.h"
#include "ble_userlist_auth.h"

FPM* fpm;
int16_t num_fingerprints = 0;
bool enrolling_in_progress = false;


static const char *TAG = "FPM TASK";
#define NUM_SNAPSHOTS 5

bool enrollFinger() 
{
    FPMStatus status;    
    TickType_t now = xTaskGetTickCount();

    enrolling_in_progress = true;
    oled_write_text("Set new FP", true);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    /* Take snapshots of the finger, and extract the fingerprint features from each image */
    for (int i = 0; i < NUM_SNAPSHOTS; i++)
    {
        ESP_LOGI(TAG, "Place a finger");
        oled_write_text("Place finger", true);

        do {
            status = fpm->getImage();    
            switch (status) 
            {
                case FPMStatus::OK:
                    ESP_LOGI(TAG, "Image taken");
                    oled_write_text("Image taken", true);
                    vTaskDelay(pdMS_TO_TICKS(200));
                    now = xTaskGetTickCount();
                    break;
                    
                case FPMStatus::NOFINGER:
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    break;
                    
                default:
                    /* allow retries even when an error happens */
                    ESP_LOGE(TAG, "getImage(): error 0x%X", static_cast<uint16_t>(status));                    
                    break;
            }
            
            if (xTaskGetTickCount() - now > pdMS_TO_TICKS(10000)) {
                ESP_LOGE(TAG, "Timeout waiting for finger");
                oled_write_text("Timeout", true);
                vTaskDelay(pdMS_TO_TICKS(1000));
                enrolling_in_progress = false;
                return false;                
            }

            yield();
        } while (status != FPMStatus::OK);

        status = fpm->image2Tz(i+1);
        
        switch (status) 
        {
            case FPMStatus::OK:
                ESP_LOGI(TAG,"Image converted");
                break;
                
            default:
                ESP_LOGI(TAG, "image2Tz(%d): error 0x%X", i+1, static_cast<uint16_t>(status));        
                oled_write_text("Image error", true);                
                vTaskDelay(pdMS_TO_TICKS(2000));        
                return false;
        }

        ESP_LOGI(TAG, "Remove finger");
        oled_write_text("Lift finger", true);  
        vTaskDelay(500 / portTICK_PERIOD_MS);
        do {
            status = fpm->getImage();
            vTaskDelay(200 / portTICK_PERIOD_MS);
        } while (status != FPMStatus::NOFINGER);

    }
    
    /* Images have been taken and converted into features a.k.a character files.
     * Now, need to create a model/template from these features and store it. */     
    status = fpm->generateTemplate();
    switch (status)
    {
        case FPMStatus::OK:
            ESP_LOGI(TAG, "Template created from matching prints!");
            break;
            
        case FPMStatus::ENROLLMISMATCH:
            ESP_LOGI(TAG, "The prints do not match!");
            return false;
            
        default:
            ESP_LOGE(TAG, "createModel(): error 0x%X", static_cast<uint16_t>(status));            
            return false;
    }
    
    status = fpm->storeTemplate(num_fingerprints);
    switch (status)
    {
        case FPMStatus::OK:
            ESP_LOGI(TAG, "Template stored at ID %d!", num_fingerprints);       
            oled_write_text("Template stored", true);    
            vTaskDelay(pdMS_TO_TICKS(1000));
            break;
            
        case FPMStatus::BADLOCATION:
            ESP_LOGE(TAG, "Could not store in that location %d!", num_fingerprints);
            oled_write_text("Store error", true);
            vTaskDelay(pdMS_TO_TICKS(2000));
            return false;
            
        default:
            ESP_LOGE(TAG, "storeModel(): error 0x%X", static_cast<uint16_t>(status));
            oled_write_text("Template error", true);
            vTaskDelay(pdMS_TO_TICKS(2000));
            return false;
    }
        
    
    num_fingerprints += 1; 
    ESP_LOGI(TAG, " >> Enroll process completed successfully!\n");
    char message[20];
    snprintf(message, sizeof(message), "Enroll %02d", num_fingerprints);
    oled_write_text(message, true);
    enrolling_in_progress = false;
    
    return true;
}

bool clearFingerprintDB() 
{
    bool confirmed = false;

    oled_write_text("Clear FPs DB", true);
    vTaskDelay(pdMS_TO_TICKS(1000));
    oled_write_text("Put finger", true);

    uint32_t timeout = xTaskGetTickCount() + pdMS_TO_TICKS(8000);
    while (!confirmed && xTaskGetTickCount() < timeout) { 
        confirmed = searchDatabase();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (!confirmed) {
        ESP_LOGE(TAG, "No finger detected or no match found. Aborting clear operation.");
        oled_write_text("No match found", true);
        return false;
    } 
    else {
        FPMStatus status = fpm->emptyDatabase();        
        switch (status) {
            case FPMStatus::OK:
                ESP_LOGI(TAG, "Database empty.");
                return true;                
                
            case FPMStatus::DBCLEARFAIL:
                ESP_LOGE(TAG, "Could not clear sensor database.");
                return false;
                
            default:
                ESP_LOGE(TAG, "emptyDatabase(): error 0x%X", static_cast<uint16_t>(status));
                return false;
        }                
    }
    return false;
}


int searchDatabase() {
    FPMStatus status;
    uint16_t fid = 0xFFFF, score = 0;

    /* Take a snapshot of the input finger */
    ESP_LOGI(TAG, "Place a finger");
    oled_write_text("Searching...", true);
    TickType_t now = xTaskGetTickCount();

    do {
        status = fpm->getImage();        

        switch (status) {
        case FPMStatus::OK:
            ESP_LOGI(TAG, "Image taken");
            break;

        case FPMStatus::NOFINGER:            
            vTaskDelay(200 / portTICK_PERIOD_MS);
            break;

        default:
            /* allow retries even when an error happens */
            ESP_LOGE(TAG, "getImage(): error 0x%X", static_cast<uint16_t>(status));
            break;
        }

        if (xTaskGetTickCount() - now > pdMS_TO_TICKS(5000)) {
            ESP_LOGE(TAG, "Timeout waiting for finger");
            return fid; // Timeout, return invalid index
        }

        taskYIELD();
    } while (status != FPMStatus::OK);

    /* Extract the fingerprint features */
    status = fpm->image2Tz();

    switch (status) {
    case FPMStatus::OK:
        ESP_LOGI(TAG, "Image converted");
        break;

    default:
        ESP_LOGE(TAG, "image2Tz(): error 0x%X", static_cast<uint16_t>(status));
        return fid;
    }

    /* Search the database for the converted print */
    status = fpm->searchDatabase(&fid, &score);

    switch (status){
    case FPMStatus::OK:
        ESP_LOGI(TAG, "Found a match at ID #%u with confidence %u", fid, score);        
        char message[20];
        snprintf(message, sizeof(message), "Match ID %02d", fid);
        oled_write_text(message, true);        
        vTaskDelay(500 / portTICK_PERIOD_MS);
        break;

    case FPMStatus::NOTFOUND:
        ESP_LOGI(TAG, "Did not find a match.");
        oled_write_text("No match", true);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        break;

    default:
        ESP_LOGE(TAG, "searchDatabase(): error 0x%X", static_cast<uint16_t>(status));
        return fid; // Return invalid index
    }

    /* Now wait for the finger to be removed */
    // ESP_LOGI(TAG, "Remove finger");
    // oled_write_text("Lift finger", true);  
    // vTaskDelay(500 / portTICK_PERIOD_MS);

    do {
        status = fpm->getImage();
        vTaskDelay(200 / portTICK_PERIOD_MS);
    } while (status != FPMStatus::NOFINGER);

    return fid;
}


void fingerprint_task(void *pvParameters) {

    // Configura FP_TOUCH come input per il segnale "touch"
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << FP_TOUCH);
    io_conf.pull_down_en = PULLDOWN_TYPE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Configura FP_ACTIVATE come output per il segnale "activate"
    gpio_config_t fp_conf = {};
    fp_conf.intr_type = GPIO_INTR_DISABLE;
    fp_conf.mode = GPIO_MODE_OUTPUT;
    fp_conf.pin_bit_mask = (1ULL << FP_ACTIVATE);
    fp_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    fp_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&fp_conf);

    // Configura UART1: cambiare i GPIO secondo la propria scheda
    EspIdfUartStream uart{UART_NUM_1, /*TX*/ FP_TX, /*RX*/ FP_RX, /*baud*/ 57600};
    if (uart.begin() != ESP_OK) {
        ESP_LOGE(TAG, "UART init failed");
        return;
    }

    // Attiva il modulo fingerprint
    gpio_set_level((gpio_num_t)FP_ACTIVATE, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Inizializza FPM usando il trasporto UART di ESP-IDF
    
    fpm = new FPM(&uart);

    if (!fpm->begin()) {
        ESP_LOGE(TAG, "FPM begin() failed (verifica cavi, baud, alimentazione)");
        return;
    }

    // Lettura parametri prodotto
    FPMSystemParams params{};
    if (fpm->readParams(&params) == FPMStatus::OK) {
        ESP_LOGI(TAG, "Found fingerprint sensor!");
        ESP_LOGI(TAG, "Capacity: %u", params.capacity);
        ESP_LOGI(TAG, "Packet length: %u", FPM::packetLengths[static_cast<uint8_t>(params.packetLen)]);
    }
    else {
        ESP_LOGE(TAG, "readParams non eseguito correttamente");
    }

    // Leggi informazioni sul numero di fingerprint memorizzati    
    if (fpm->getLastIndex(&num_fingerprints) == FPMStatus::OK) {
        ESP_LOGI(TAG, "Numero di fingerprint nel database: %u", (unsigned)num_fingerprints);
    }
    else {
        ESP_LOGW(TAG, "Impossibile leggere il numero di fingerprint registrati");
    }

    // E' necessario attendere che venga registrata almeno un'impronta "root"
    while (!num_fingerprints) {
        fpm->getLastIndex(&num_fingerprints);
        vTaskDelay(pdMS_TO_TICKS(100));

        if (num_fingerprints == 0) {
            ESP_LOGW(TAG, "No templates found in the library. Please enroll a finger.");
            oled_write_text("No root FP", true);            
            vTaskDelay(pdMS_TO_TICKS(1000));
            enrollFinger();
        }
    }

    // Avvio del task ripetitivo per il controllo del fingerprint
    while(1) {
        if (gpio_get_level((gpio_num_t)FP_TOUCH) == ACTIVE_LEVEL && !enrolling_in_progress) {

            last_interaction_time = xTaskGetTickCount();
           
            ESP_LOGI(TAG, "Touch detected, starting fingerprint search...");
            last_interaction_time = xTaskGetTickCount() - pdMS_TO_TICKS(10000);
            display_reset_pending = 1;            
                       
            // Esegui la ricerca del fingerprint
            uint16_t finger_index = searchDatabase();

            if (finger_index != 0xFFFF) {
                // Autenticazione biometrica riuscita: abilita accesso BLE user list
                ble_userlist_set_authenticated(true);
                ESP_LOGI(TAG, "Autenticazione biometrica riuscita: accesso BLE abilitato");
                ESP_LOGI(TAG, "User index: %d", user_index);

                if (user_index != -1 && user_list[user_index].winlogin) {  // CTRL+ALT+DELETE
                    send_key_combination(HID_MODIFIER_LEFT_CTRL | HID_MODIFIER_LEFT_ALT, 0x4C);
                }

                // Cerca nel database utenti se esiste un fingerprint_id equivalente con l'opzione magicfinger
                if (user_index == -1) {
                    for (int i = 0; i < MAX_USERS; ++i) {
                        if (user_list[i].fingerprint_id == finger_index && user_list[i].magicfinger) {
                            user_index = i;
                            ESP_LOGI(TAG, "User %s with fingerprint ID %d found at index %d\n", user_list[i].label, finger_index, i);
                            break;
                        }
                    }                
                }

                if (user_index != -1) {
                    // Invia la password corrispondente all'indice attuale
                    uint8_t* encoded = user_list[user_index].password_enc;
                    size_t len = user_list[user_index].password_len;

                    char plain[128];
                    if (userdb_decrypt_password(encoded, len, plain) == 0) {
                        // printf("Password (decrypted): %s\n", plain);
                        send_string(plain);
                        userdb_increment_usage(user_index);

                        char message[20];
                        snprintf(message, sizeof(message), "Finger ID: %02d", finger_index);
                        oled_write_text(message, true);
                    }  
                    else {
                        ESP_LOGE(TAG, "Decrypt error");
                        oled_write_text("Decrypt err", true);
                    }               
                }
                else {
                    ESP_LOGW(TAG, "No account selected");
                    oled_write_text("No account", true);
                }
            } 
            else {
                ESP_LOGI(TAG, "No matching finger found.");                
            }
        }

        // Attendi un po' prima di ripetere la ricerca
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
