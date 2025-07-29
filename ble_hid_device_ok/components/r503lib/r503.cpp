#include "r503.h"
#include "oled.h"
#include "user_list.h"
#include "ble_device_main.h"
#include "ble_userlist_auth.h"

static const char *TAG = "FingerprintTask";
static TaskHandle_t fingerprintTaskHandle = nullptr;
uint16_t num_templates = 0;
bool enrolling_in_progress = false;


void enrollFinger()
{
    int ret;
    uint8_t featureCount = 5;
    enrolling_in_progress = true;

    printf("\nFollow the steps below to enroll a new finger\n");
    oled_write_text("Set new FP", true);
    vTaskDelay(pdMS_TO_TICKS(2000));

    TickType_t now = xTaskGetTickCount();

    for (int i = 1; i <= featureCount; i++) {
        fps.setAuraLED(aLEDBreathing, aLEDBlue, 50, 255);
        printf("\n>> Place finger on sensor...\n");
        oled_write_text("Place finger", true);

        while (true) {
            ret = fps.takeImage();
            if (ret == R503_NO_FINGER) {
                vTaskDelay(pdMS_TO_TICKS(100));

                // Check for timeout
                if (xTaskGetTickCount() - now > pdMS_TO_TICKS(10000)) {
                    printf("[X] Enrollment timed out\n");
                    oled_write_text("Timeout", true);
                    fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    enrolling_in_progress = false;
                    return;
                }
                continue;
            } 

            // Image taken successfully
            else if (ret == R503_OK) {
                printf(" >> Image %d of %d taken\n", i, featureCount);
                oled_write_text("Image taken", true);                
                fps.setAuraLED(aLEDBreathing, aLEDYellow, 255, 255);
                vTaskDelay(pdMS_TO_TICKS(200));                             
            } 
            else {
                printf("[X] Could not take image (code: 0x%02X)\n", ret);
                oled_write_text("Image error", true);
                fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
                vTaskDelay(pdMS_TO_TICKS(2000));
                continue;
            }

            // Get features from the image
            ret = fps.extractFeatures(i);
            if (ret != R503_OK) {
                printf("[X] Failed to extract features (code: 0x%02X)\n", ret);
                oled_write_text("Feature error", true);
                fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
                vTaskDelay(pdMS_TO_TICKS(2000));
                continue;
            }

            // Features extracted successfully
            fps.setAuraLED(aLEDBreathing, aLEDGreen, 255, 255);
            printf(" >> Features %d of %d extracted\n", i, featureCount);
            oled_write_text("Extract features", true);
            vTaskDelay(pdMS_TO_TICKS(500));                       
            break;
        }

        printf(" >> Lift your finger from the sensor!\n");
        oled_write_text("Lift finger", true);  
        now = xTaskGetTickCount();       

        while (fps.takeImage() != R503_NO_FINGER) {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    // Create the template from the extracted features
    printf(" >> Creating template...\n");
    oled_write_text("Create template", true);
    fps.setAuraLED(aLEDBreathing, aLEDPurple, 100, 255);
    vTaskDelay(pdMS_TO_TICKS(100));
    ret = fps.createTemplate();

    if (ret != R503_OK) {
        printf("[X] Failed to create a template (code: 0x%02X)\n", ret);
        oled_write_text("Template error", true);
        fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }

    // Store the template in the library
    printf(" >> Template created\n");
    oled_write_text("Template created", true);
    ret = fps.storeTemplate(1, num_templates);
    if (ret != R503_OK) {
        printf("[X] Failed to store the template (code: 0x%02X)\n", ret);
        oled_write_text ("Store error", true);
        fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    fps.setAuraLED(aLEDBreathing, aLEDGreen, 255, 1);
    printf(" >> Template stored at location: %d\n", num_templates); 
    num_templates += 1; 
    printf(" >> Enroll process completed successfully!\n");
    char message[20];
    snprintf(message, sizeof(message), "Enroll %02d", num_templates);
    oled_write_text(message, true);
    enrolling_in_progress = false;
}

int searchFinger()
{
    int ret;
    uint64_t start = esp_timer_get_time() / 1000; // in ms

    printf(" >> Place your finger on the sensor...\n\n");
    fps.setAuraLED(aLEDBreathing, aLEDBlue, 50, 255);

    while ((esp_timer_get_time() / 1000 - start) < 15000) {
        ret = fps.takeImage();

        if (ret == R503_NO_FINGER) {
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        } else if (ret == R503_OK) {
            printf(" >> Image taken\n");
            fps.setAuraLED(aLEDFlash, aLEDWhite, 150, 255);
            break;
        } else {
            printf("[X] Could not take image (code: 0x%02X)\n", ret);
            fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
    }

    if ((esp_timer_get_time() / 1000 - start) >= 10000) {
        printf("[X] Could not take image (timeout)\n");
        fps.setAuraLED(aLEDFlash, aLEDRed, 100, 2);
        // send_ble_message("Timeout: No finger detected", 0x02);
        return -1;
    }

    ret = fps.extractFeatures(1);
    if (ret != R503_OK) {
        printf("[X] Could not extract features (code: 0x%02X)\n", ret);
        fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
        return -1;
    }

    uint16_t location = 0, confidence = 0;
    ret = fps.searchFinger(1, location, confidence);

    if (ret == R503_NO_MATCH_IN_LIBRARY) {
        printf(" >> No matching finger found\n");
        fps.setAuraLED(aLEDBreathing, aLEDRed, 255, 1);
        // send_ble_message("No matching fingerprint", 0x02);
    } 
    else if (ret == R503_OK) {
        printf(" >> Found finger\n");
        printf("    Finger ID: %d\n", location);
        printf("    Confidence: %d\n", confidence);
        fps.setAuraLED(aLEDBreathing, aLEDGreen, 255, 1);
        return location;
    } 
    else {
        printf("[X] Could not search library (code: 0x%02X)\n", ret);
        fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    }
    return -1;
}

void clearFingerprintDB()
{   
    oled_write_text("Clear FPs DB", true);
    vTaskDelay(pdMS_TO_TICKS(1000));
    oled_write_text("Put finger", true);
    bool confirmed = false;
    uint32_t timeout = xTaskGetTickCount() + pdMS_TO_TICKS(8000);
    while (!confirmed && xTaskGetTickCount() < timeout) { 
        confirmed = searchFinger();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (!confirmed) {
        printf("No finger detected or no match found. Aborting clear operation.\n");
        oled_write_text("No match found", true);
        return;
    } 
    else {
        int ret = fps.emptyLibrary();
        if (ret == R503_OK) {
            printf("The fingerprint library has been cleared!\n");
            fps.setAuraLED(aLEDBreathing, aLEDGreen, 50, 2);
        } else {
            printf("[X] Failed to clear library (code: 0x%02X)\n", ret);
            fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
        }
    }
}


void printFingerprintTable() {
    printf("\nReading fingerprint table...\n");
    R503Lib* fp = &fps;
    if (!fp) {
        printf("Fingerprint library not initialized!\n");
        return;
    }        

    uint16_t count;
    int ret = fp->getTemplateCount(count);
    if (ret == R503_OK)
        printf("- Amount of templates: %d\n", count);
    else
        printf("- Amount of templates: could not read (code: 0x%02X)\n", ret);    

    R503Parameters params;
    ret = fp->readParameters(params);
    if (ret == R503_OK)
        printf("- Fingerprint library capacity: %d\n", params.fingerLibrarySize);    
    else    
        printf("- Fingerprint library capacity: could not read (code: 0x%02X)\n", ret);
    

    // only print first page, since we know library size is less than 256
    uint8_t table[32];
    ret = fp->readIndexTable(table);

    if (ret == R503_OK) {
        printf("- Fingerprints stored at locations (ID): ");
        for (int i = 0; i < 32; i++)
            for (int b = 0; b < 8; b++)
                if (table[i] >> b & 0x01)
                    printf("%d  ", i * 8 + b);
        printf("\n");
    }
    else  {
        printf("- Fingerprints stored at locations (ID): could not read (code: 0x%02X)\n", ret);
    }
}

static void fingerprint_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting fingerprint task...");
    if (fps.begin(57600, PASSWORD) != R503_OK) {
        ESP_LOGE(TAG, "Sensor not found!");
        vTaskDelete(nullptr);
        return;
    }

    // Configura GPIO 4 come input per il segnale "touch"
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << TOUCH_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    fps.setAuraLED(aLEDBreathing, aLEDBlue, 255, 1);
    fps.printDeviceInfo();
    fps.printParameters();
    // printMenu();
    
    while (!num_templates) {
        fps.getTemplateCount(num_templates);
        vTaskDelay(pdMS_TO_TICKS(100));

        if (num_templates == 0) {
            printf("No templates found in the library. Please enroll a finger.\n");
            oled_write_text("No root FP", true);
            fps.setAuraLED(aLEDBreathing, aLEDRed, 255, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
            enrollFinger();
        }
    }

    while (true) {
        // Controlla il segnale "touch" su GPIO 4
        if (gpio_get_level((gpio_num_t)TOUCH_GPIO) == 0 && !enrolling_in_progress) {
            last_interaction_time = xTaskGetTickCount();
           
            ESP_LOGI(TAG, "Touch detected, starting fingerprint search...");
            last_interaction_time = xTaskGetTickCount() - pdMS_TO_TICKS(10000);
            display_reset_pending = 1;
            oled_write_text("Search...", true);

            int finger_index = searchFinger();
            if (finger_index != -1) {

                // Autenticazione biometrica riuscita: abilita accesso BLE user list
                ble_userlist_set_authenticated(true);
                printf("[FINGERPRINT] Autenticazione biometrica riuscita: accesso BLE abilitato\n");

                if (user_list[user_index].winlogin && user_index != -1 ) {  // CTRL+ALT+DELETE
                    send_key_combination(HID_MODIFIER_LEFT_CTRL | HID_MODIFIER_LEFT_ALT, 0x4C);
                }

                // Cerca nel database utenti se esiste un fingerprint_id equivalente con l'opzione magicfinger
                if (user_index == -1) {
                    for (int i = 0; i < MAX_USERS; ++i) {
                        if (user_list[i].fingerprint_id == finger_index && user_list[i].magicfinger) {
                            user_index = i;
                            printf("User %s with fingerprint ID %d found at index %d\n", user_list[i].label, finger_index, i);
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

                        char message[16];
                        snprintf(message, sizeof(message), "Finger ID: %02d", finger_index);
                        oled_write_text(message, true);
                    }  
                    else {
                        printf("Decrypt error");
                        oled_write_text("Decrypt err", true);
                    }               
                }
                else {
                    printf("No account selected");
                    oled_write_text("No account", true);
                }
            } 
            else {
                ESP_LOGI(TAG, "No matching finger found.");
                // send_string("No matching fingerprint");
                oled_write_text("No match", true);
            }

            while (gpio_get_level((gpio_num_t)TOUCH_GPIO) == 0) {
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }        

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void fingerprint_task_start(void)
{
    if (fingerprintTaskHandle == nullptr) {
        xTaskCreate(
            fingerprint_task,
            "FingerprintTask",
            4096,
            nullptr,
            5,
            &fingerprintTaskHandle
        );
    }
}
