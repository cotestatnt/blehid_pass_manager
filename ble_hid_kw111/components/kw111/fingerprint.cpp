#include "fingerprint.h"
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
        fps.LEDcontrol(FINGERPRINT_LED_BREATHING, 100, FINGERPRINT_LED_BLUE);
        printf("\n>> Place finger on sensor...\n");
        oled_write_text("Place finger", true);

        while (true) {
            ret = fps.getImage();
            if (ret == FINGERPRINT_NOFINGER) {
                vTaskDelay(pdMS_TO_TICKS(100));

                // Check for timeout
                if (xTaskGetTickCount() - now > pdMS_TO_TICKS(10000)) {
                    printf("[X] Enrollment timed out\n");
                    oled_write_text("Timeout", true);
                    fps.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    enrolling_in_progress = false;
                    return;
                }
                continue;
            } 

            // Image taken successfully
            else if (ret == FINGERPRINT_OK) {
                printf(" >> Image %d of %d taken\n", i, featureCount);
                oled_write_text("Image taken", true);                
                fps.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 10);
                vTaskDelay(pdMS_TO_TICKS(200));                             
            } 
            else {
                printf("[X] Could not take image (code: 0x%02X)\n", ret);
                oled_write_text("Image error", true);
                fps.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
                vTaskDelay(pdMS_TO_TICKS(2000));
                continue;
            }

            // Get features from the image
            ret = fps.image2Tz(i);
            if (ret != FINGERPRINT_OK) {
                printf("[X] Failed to extract features (code: 0x%02X)\n", ret);
                oled_write_text("Feature error", true);
                fps.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
                vTaskDelay(pdMS_TO_TICKS(2000));
                continue;
            }

            // Features extracted successfully
            fps.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 10);
            printf(" >> Features %d of %d extracted\n", i, featureCount);
            oled_write_text("Extract features", true);
            vTaskDelay(pdMS_TO_TICKS(500));                       
            break;
        }

        printf(" >> Lift your finger from the sensor!\n");
        oled_write_text("Lift finger", true);  
        now = xTaskGetTickCount();       

        while (fps.getImage() != FINGERPRINT_NOFINGER) {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    // Create the template from the extracted features
    printf(" >> Creating template...\n");
    oled_write_text("Create template", true);
    fps.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_PURPLE);
    vTaskDelay(pdMS_TO_TICKS(100));
    ret = fps.createModel();

    if (ret != FINGERPRINT_OK) {
        printf("[X] Failed to create a template (code: 0x%02X)\n", ret);
        oled_write_text("Template error", true);
        fps.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }

    // Store the template in the library
    printf(" >> Template created\n");
    oled_write_text("Template created", true);
    ret = fps.storeModel(num_templates);
    if (ret != FINGERPRINT_OK) {
        printf("[X] Failed to store the template (code: 0x%02X)\n", ret);
        oled_write_text ("Store error", true);
        fps.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    fps.LEDcontrol(FINGERPRINT_LED_BREATHING, 100, FINGERPRINT_LED_PURPLE, 50);
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
    fps.LEDcontrol(FINGERPRINT_LED_BREATHING, 100, FINGERPRINT_LED_BLUE);

    while ((esp_timer_get_time() / 1000 - start) < 15000) {
        ret = fps.getImage();

        if (ret == FINGERPRINT_NOFINGER) {
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        } else if (ret == FINGERPRINT_OK) {
            printf(" >> Image taken\n");
            // fps.setAuraLED(aLEDFlash, aLEDWhite, 150, 255);
            break;
        } else {
            printf("[X] Could not take image (code: 0x%02X)\n", ret);
            // fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
    }

    if ((esp_timer_get_time() / 1000 - start) >= 10000) {
        printf("[X] Could not take image (timeout)\n");
        // fps.setAuraLED(aLEDFlash, aLEDRed, 100, 2);
        // send_ble_message("Timeout: No finger detected", 0x02);
        return -1;
    }

    // ret = fps.fingerSearch();
    // if (ret != FINGERPRINT_OK) {
    //     printf("[X] Could not extract features (code: 0x%02X)\n", ret);
    //     // fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    //     return -1;
    // }

    ret = fps.fingerSearch();
    if (ret == FINGERPRINT_NOMATCH) {
        printf(" >> No matching finger found\n");
        // fps.setAuraLED(aLEDBreathing, aLEDRed, 255, 1);
        // send_ble_message("No matching fingerprint", 0x02);
    } 
    else if (ret == FINGERPRINT_OK) {
        printf(" >> Found finger\n");
        printf("    Finger ID: %d\n", fps.fingerID);
        printf("    Confidence: %d\n", fps.confidence);
        // fps.setAuraLED(aLEDBreathing, aLEDGreen, 255, 1);
        return fps.fingerID;
    } 
    else {
        printf("[X] Could not search library (code: 0x%02X)\n", ret);
        // fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
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
        int ret = fps.emptyDatabase();
        if (ret == FINGERPRINT_OK) {
            printf("The fingerprint library has been cleared!\n");
            // fps.setAuraLED(aLEDBreathing, aLEDGreen, 50, 2);
        } else {
            printf("[X] Failed to clear library (code: 0x%02X)\n", ret);
            // fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
        }
    }
}


void printFingerprintTable() {
    printf("\nReading fingerprint table...\n");
    FingerprintSensor* fp = &fps;
    if (!fp) {
        printf("Fingerprint library not initialized!\n");
        return;
    }        

    int count = fp->getTemplateCount();
    if (count)
        printf("- Amount of templates: %d\n", count);
    else
        printf("- Amount of templates: could not read");    

}


static void fingerprint_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting fingerprint task...");
    if (fps.begin(57600) != FINGERPRINT_OK) {
        ESP_LOGE(TAG, "Sensor not found!");
        vTaskDelete(nullptr);
        return;
    }

    // Configura GPIO come input per il segnale "touch"
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << FP_TOUCH);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    fps.LEDcontrol(FINGERPRINT_LED_BREATHING, 100, FINGERPRINT_LED_BLUE);
    fps.printDeviceInfo();
    // fps.printParameters();
    // printMenu();
    
    while (!num_templates) {
        num_templates = fps.getTemplateCount();
        vTaskDelay(pdMS_TO_TICKS(100));

        if (num_templates == 0) {
            printf("No templates found in the library. Please enroll a finger.\n");
            oled_write_text("No root FP", true);
            fps.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 10);
            vTaskDelay(pdMS_TO_TICKS(1000));
            enrollFinger();
        }
    }

    while (true) {
        // Controlla il segnale "touch" su GPIO 4
        if (gpio_get_level((gpio_num_t)FP_TOUCH) && !enrolling_in_progress) {
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

            while (gpio_get_level((gpio_num_t)FP_TOUCH)) {
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }        

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void fingerprint_task_start(void) {
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
