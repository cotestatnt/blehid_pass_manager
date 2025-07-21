#include "r503.h"
#include "oled.h"
#include "user_list.h"
#include "ble_device_main.h"
#include "ble_userlist_auth.h"

static const char *TAG = "FingerprintTask";
static TaskHandle_t fingerprintTaskHandle = nullptr;
uint16_t num_templates = 0;

// static void printMenu()
// {
//     printf("\n[Fingerprint Menu]\n");
//     printf("e - Enroll Finger\n");
//     printf("s - Search Finger\n");
//     printf("d - Delete All Templates\n");
//     printf("m - Show Menu\n");
//     printf("--------------------------\n\n");
// }


 void enrollFinger()
{
    int ret;
    uint8_t featureCount = 5;

    // char inputBuf[8]
    // uint16_t location = 0;
    // printf("How many features do you want to extract? (1-6): ");
    // fgets(inputBuf, sizeof(inputBuf), stdin);
    // featureCount = atoi(inputBuf);
    // if (featureCount < 1 || featureCount > 6) featureCount = 2;

    // printf("Where do you want to store the fingerprint (ID)? ");
    // fgets(inputBuf, sizeof(inputBuf), stdin);
    // location = atoi(inputBuf);

    printf("Follow the steps below to enroll a new finger\n");
    oled_write_text("Enroll new FP", true);
    vTaskDelay(pdMS_TO_TICKS(2000));

    for (int i = 1; i <= featureCount; i++) {
        fps.setAuraLED(aLEDBreathing, aLEDBlue, 50, 255);
        printf("\n>> Place finger on sensor...\n");
        oled_write_text("Place finger", true);

        while (true) {
            ret = fps.takeImage();
            if (ret == R503_NO_FINGER) {
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            } else if (ret == R503_OK) {
                printf(" >> Image %d of %d taken\n", i, featureCount);
                oled_write_text("Image taken", true);
                fps.setAuraLED(aLEDBreathing, aLEDYellow, 255, 255);
            } else {
                printf("[X] Could not take image (code: 0x%02X)\n", ret);
                oled_write_text("Image error", true);
                fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            ret = fps.extractFeatures(i);
            if (ret != R503_OK) {
                printf("[X] Failed to extract features (code: 0x%02X)\n", ret);
                oled_write_text("Feature error", true);
                fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            fps.setAuraLED(aLEDBreathing, aLEDGreen, 255, 255);
            printf(" >> Features %d of %d extracted\n", i, featureCount);
            oled_write_text("Features extracted", true);
            vTaskDelay(pdMS_TO_TICKS(500));
            break;
        }

        printf(" >> Lift your finger from the sensor!\n");
        oled_write_text("Lift finger", true);
        while (fps.takeImage() != R503_NO_FINGER) {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    printf(" >> Creating template...\n");
    oled_write_text("Create template", true);
    fps.setAuraLED(aLEDBreathing, aLEDPurple, 100, 255);
    vTaskDelay(pdMS_TO_TICKS(100));
    ret = fps.createTemplate();

    if (ret != R503_OK) {
        printf("[X] Failed to create a template (code: 0x%02X)\n", ret);
        oled_write_text("Template error", true);
        fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
        return;
    }

    printf(" >> Template created\n");
    oled_write_text("Template created", true);
    ret = fps.storeTemplate(1, num_templates);
    if (ret != R503_OK) {
        printf("[X] Failed to store the template (code: 0x%02X)\n", ret);
        oled_write_text ("Store error", true);
        fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    fps.setAuraLED(aLEDBreathing, aLEDGreen, 255, 1);
    printf(" >> Template stored at location: %d\n", num_templates); 
    num_templates += 1; 
    printf(" >> Enroll process completed successfully!\n");
    oled_write_text("Enroll done!", true);
}

bool searchFinger()
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
        send_ble_message("Timeout: No finger detected", 0x02);
        return false;
    }

    ret = fps.extractFeatures(1);
    if (ret != R503_OK) {
        printf("[X] Could not extract features (code: 0x%02X)\n", ret);
        fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
        return false;
    }

    uint16_t location = 0, confidence = 0;
    ret = fps.searchFinger(1, location, confidence);

    if (ret == R503_NO_MATCH_IN_LIBRARY) {
        printf(" >> No matching finger found\n");
        fps.setAuraLED(aLEDBreathing, aLEDRed, 255, 1);
        send_ble_message("No matching fingerprint", 0x02);
    } 
    else if (ret == R503_OK) {
        printf(" >> Found finger\n");
        printf("    Finger ID: %d\n", location);
        printf("    Confidence: %d\n", confidence);
        fps.setAuraLED(aLEDBreathing, aLEDGreen, 255, 1);
        return true;
    } 
    else {
        printf("[X] Could not search library (code: 0x%02X)\n", ret);
        fps.setAuraLED(aLEDFlash, aLEDRed, 50, 3);
    }
    return false;
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
        if (gpio_get_level((gpio_num_t)TOUCH_GPIO) == 0) {
            ESP_LOGI(TAG, "Touch detected, starting fingerprint search...");
            last_interaction_time = xTaskGetTickCount() - pdMS_TO_TICKS(10000);
            display_reset_pending = 1;
            oled_write_text("Search...", true);

            if (searchFinger()) {
                // Autenticazione biometrica riuscita: abilita accesso BLE user list
                ble_userlist_set_authenticated(true);
                printf("[FINGERPRINT] Autenticazione biometrica riuscita: accesso BLE abilitato\n");

                // Invia la password corrispondente all'indice attuale
                uint8_t* encoded = user_list[user_index].password_enc;
                size_t len = user_list[user_index].password_len;

                char plain[128];
                if (userdb_decrypt_password(encoded, len, plain) == 0) {
                    // printf("Password (decrypted): %s\n", plain);
                    send_string(plain);
                    userdb_increment_usage(user_index);
                    oled_write_text("Match found", true);
                }
                else {
                    printf("Decrypt error");
                    oled_write_text("No account", true);
                }
            } else {
                ESP_LOGI(TAG, "No matching finger found.");
                send_string("No matching fingerprint");
                oled_write_text("No match", true);
            }

            while (gpio_get_level((gpio_num_t)TOUCH_GPIO) == 0) {
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }

        // // Gestione menu da seriale
        // size_t buffered_size;
        // uart_get_buffered_data_len(UART_PORT, &buffered_size);
        // if (buffered_size > 0) {
        //     int ch = getchar();
        //     printf("Received command: %c\n", ch);
        //     switch (ch) {
        //         case 'e':
        //             enrollFinger();
        //             break;
        //         case 's':
        //             searchFinger();
        //             break;
        //         case 'd':
        //             clearLibrary();
        //             printf("[Delete] All templates deleted\n");
        //             break;
        //         case 'm':
        //             printMenu();
        //             break;
        //         default:
        //             break;
        //     }
        // }

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
