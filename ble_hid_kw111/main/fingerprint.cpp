#include "esp_log.h"

#include "fingerprint.h"
#include "display_oled.h"
#include "user_list.h"
#include "battery.h"

#include "hid_dev.h"
#include "hid_device_ble.h"
#include "buzzer.h"

#if CONFIG_IDF_TARGET_ESP32S3
#include "hid_device_usb.h"
#include "class/hid/hid.h"
#endif

FPM* fpm;
int16_t num_fingerprints = 0;
bool enrolling_in_progress = false;
extern uint32_t last_interaction_time;

static const char *TAG = "FPM TASK";
#define NUM_SNAPSHOTS 10

bool enrollFinger() 
{
    FPMStatus status;    
    TickType_t now = xTaskGetTickCount();

    enrolling_in_progress = true;
    display_oled_post_info("Set new FP");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    /* Take snapshots of the finger, and extract the fingerprint features from each image */
    for (int i = 0; i < NUM_SNAPSHOTS; i++)
    {
        ESP_LOGI(TAG, "Place a finger");
        display_oled_post_info("Place finger");

        do {
            status = fpm->getImage();    
            switch (status) 
            {
                case FPMStatus::OK:
                    ESP_LOGI(TAG, "Image taken");
                    display_oled_post_info("Image taken");
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
                display_oled_post_error("Timeout");
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
                display_oled_post_error("Image error");  
                buzzer_feedback_fail();
                vTaskDelay(pdMS_TO_TICKS(2000));        
                return false;
        }

        ESP_LOGI(TAG, "Remove finger");
        buzzer_feedback_lift();
        display_oled_post_info("Lift finger");  
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
            display_oled_post_info("Template stored");   
            buzzer_feedback_success();
            vTaskDelay(pdMS_TO_TICKS(1000));
            break;
            
        case FPMStatus::BADLOCATION:
            ESP_LOGE(TAG, "Could not store in that location %d!", num_fingerprints);
            display_oled_post_error("Store error");
            buzzer_feedback_fail();
            vTaskDelay(pdMS_TO_TICKS(2000));
            return false;
            
        default:
            ESP_LOGE(TAG, "storeModel(): error 0x%X", static_cast<uint16_t>(status));
            display_oled_post_error("Template error");
            buzzer_feedback_fail();
            vTaskDelay(pdMS_TO_TICKS(2000));
            return false;
    }
        
    
    num_fingerprints += 1; 
    ESP_LOGI(TAG, " >> Enroll process completed successfully!\n");
    display_oled_post_info("Enroll %02d", num_fingerprints);
    enrolling_in_progress = false;
    return true;
}

bool clearFingerprintDB() 
{
    bool confirmed = false;

    display_oled_post_info("Clear FPs DB");
    vTaskDelay(pdMS_TO_TICKS(1000));
    display_oled_post_info("Put finger");
    buzzer_feedback_lift();

    uint32_t timeout = xTaskGetTickCount() + pdMS_TO_TICKS(8000);
    while (!confirmed && xTaskGetTickCount() < timeout) { 
        confirmed = searchDatabase();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (!confirmed) {
        ESP_LOGE(TAG, "No finger detected or no match found. Aborting clear operation.");
        display_oled_post_error("No match found");
        return false;
    } 
    else {
        FPMStatus status = fpm->emptyDatabase();        
        switch (status) {
            case FPMStatus::OK:
                ESP_LOGI(TAG, "Database empty.");
                buzzer_feedback_success();
                return true;                
                
            case FPMStatus::DBCLEARFAIL:
                ESP_LOGE(TAG, "Could not clear sensor database.");
                buzzer_feedback_fail();
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
    display_oled_post_info("Searching...");
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
        display_oled_post_info("Match ID %02d", fid);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        buzzer_feedback_success();
        break;

    case FPMStatus::NOTFOUND:
        ESP_LOGI(TAG, "Did not find a match.");
        display_oled_post_info("No match");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        buzzer_feedback_fail();
        break;

    default:
        ESP_LOGE(TAG, "searchDatabase(): error 0x%X", static_cast<uint16_t>(status));
        return fid; // Return invalid index
    }

    // Now wait for the finger to be removed 
    do {
        status = fpm->getImage();
        vTaskDelay(200 / portTICK_PERIOD_MS);
    } while (status != FPMStatus::NOFINGER);

    return fid;
}


void fingerprint_task(void *pvParameters) {

    // Configure FP_TOUCH as input for the "touch" signal
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << FP_TOUCH);
    io_conf.pull_down_en = PULLDOWN_TYPE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Configure FP_ACTIVATE as output for the "activate" signal
    gpio_config_t fp_conf = {};
    fp_conf.intr_type = GPIO_INTR_DISABLE;
    fp_conf.mode = GPIO_MODE_OUTPUT;
    fp_conf.pin_bit_mask = (1ULL << FP_ACTIVATE);
    fp_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    fp_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&fp_conf);

    // Configure UART1: change the GPIOs according to your board
    EspIdfUartStream uart{UART_NUM_1, /*TX*/ FP_TX, /*RX*/ FP_RX, /*baud*/ 57600};
    if (uart.begin() != ESP_OK) {
        ESP_LOGE(TAG, "UART init failed");
        return;
    }

    // Activate the fingerprint module
    gpio_set_level((gpio_num_t)FP_ACTIVATE, 0);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Initialize FPM using ESP-IDF UART transport
    fpm = new FPM(&uart);
    if (!fpm->begin()) {
        ESP_LOGE(TAG, "FPM begin() failed (check cables, baud, power supply)");
        return;
    }

    // Read product parameters
    FPMSystemParams params{};
    if (fpm->readParams(&params) == FPMStatus::OK) {
        ESP_LOGI(TAG, "Found fingerprint sensor!");
        ESP_LOGI(TAG, "Capacity: %u", params.capacity);
        ESP_LOGI(TAG, "Packet length: %u", FPM::packetLengths[static_cast<uint8_t>(params.packetLen)]);
    }
    else {
        ESP_LOGE(TAG, "readParams not executed correctly");
    }

    // Read information about the number of stored fingerprints
    if (fpm->getLastIndex(&num_fingerprints) == FPMStatus::OK) {
        ESP_LOGI(TAG, "Number of fingerprints in database: %u", (unsigned)num_fingerprints);
    }
    else {
        ESP_LOGW(TAG, "Unable to read the number of registered fingerprints");
    }

    // It's necessary to wait until at least one "root" fingerprint is registered
    while (!num_fingerprints) {
        fpm->getLastIndex(&num_fingerprints);
        vTaskDelay(pdMS_TO_TICKS(100));

        if (num_fingerprints == 0) {
            ESP_LOGW(TAG, "No templates found in the library. Please enroll a finger.");
            display_oled_post_info("No root FP");            
            vTaskDelay(pdMS_TO_TICKS(1000));
            enrollFinger();
        }
    }

    // Start of the repetitive task for fingerprint control
    while(1) {
        if (gpio_get_level((gpio_num_t)FP_TOUCH) == ACTIVE_LEVEL && !enrolling_in_progress) {            
                       
            ESP_LOGI(TAG, "Touch detected, starting fingerprint search...");
            last_interaction_time = xTaskGetTickCount() - pdMS_TO_TICKS(10000);            
                       
            // Execute fingerprint search
            uint16_t finger_index = searchDatabase();

            if (finger_index != 0xFFFF) {
                // Biometric authentication successful: enable BLE user list access
                ble_userlist_set_authenticated(true);
                ESP_LOGI(TAG, "Biometric authentication successful: BLE access enabled");
                ESP_LOGI(TAG, "User index: %d", user_index);
                
                #if CONFIG_IDF_TARGET_ESP32S3
                // Check if we need USB HID also
                bool usb_available = is_usb_connected_simple();                
                #endif

                // Search in the user database if there is an equivalent fingerprint_id with the magicfinger option
                if (user_index == -1) {
                    for (int i = 0; i < MAX_USERS; ++i) {
                        if (user_list[i].fingerprint_id == finger_index && user_list[i].magicfinger) {
                            user_index = i;
                            ESP_LOGI(TAG, "User %s with fingerprint ID %d found at index %d\n", user_list[i].label, finger_index, i);
                            break;
                        }
                    }                
                }
                
                // Index selected with buttons or magic finder, let's check if is a windows login (CTRL+ALT+DEL)
                user_entry_t user = user_list[user_index];
                if (user_index != -1 && user.winlogin) {  // CTRL+ALT+DELETE
                    ESP_LOGI(TAG, "Sending CTRL+ALT+DEL combination for user %s...", user.label);
                    switch (user.login_type)  {
                        case 0:  // BLE only
                            ESP_LOGI(TAG, "Sending via BLE: modifiers=0x%02X, key=0x%02X", 
                                     HID_MODIFIER_LEFT_CTRL | HID_MODIFIER_LEFT_ALT, 0x4C);
                            ble_send_key_combination(HID_MODIFIER_LEFT_CTRL | HID_MODIFIER_LEFT_ALT, 0x4C);
                            break;
                        
                        #if CONFIG_IDF_TARGET_ESP32S3
                        case 1:  // USB only
                            ESP_LOGI(TAG, "Sending via USB: modifiers=0x%02X, key=0x%02X", 
                                     HID_MODIFIER_LEFT_CTRL | HID_MODIFIER_LEFT_ALT, 0x4C);
                            usb_send_key_combination(HID_MODIFIER_LEFT_CTRL | HID_MODIFIER_LEFT_ALT, 0x4C);
                            break;
                        case 2:  // Both
                            if (usb_available) {
                                ESP_LOGI(TAG, "Sending via USB (both mode): modifiers=0x%02X, key=0x%02X", 
                                         HID_MODIFIER_LEFT_CTRL | HID_MODIFIER_LEFT_ALT, 0x4C);
                                usb_send_key_combination(HID_MODIFIER_LEFT_CTRL | HID_MODIFIER_LEFT_ALT, 0x4C);
                            } else {
                                ESP_LOGI(TAG, "Sending via BLE (both mode): modifiers=0x%02X, key=0x%02X", 
                                         HID_MODIFIER_LEFT_CTRL | HID_MODIFIER_LEFT_ALT, 0x4C);
                                ble_send_key_combination(HID_MODIFIER_LEFT_CTRL | HID_MODIFIER_LEFT_ALT, 0x4C);
                            }
                            break;
                        #endif
                    }
                    ESP_LOGI(TAG, "CTRL+ALT+DEL combination sent successfully");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }

                if (user_index != -1) {
                    // Send the password corresponding to the current index
                    uint8_t* encoded = user.password_enc;
                    size_t len = user.password_len;

                    char plain[128];
                    if (userdb_decrypt_password(encoded, len, plain) == 0) {                        
                        switch (user.login_type)  {
                            case 0:  // BLE only
                                ble_send_string(plain);
                                break;
                            
                            #if CONFIG_IDF_TARGET_ESP32S3
                            case 1:  // USB only
                                usb_send_string(plain);
                                break;
                            case 2:  // Both
                                if (usb_available) {
                                    usb_send_string(plain);
                                } else {
                                    ble_send_string(plain);
                                }
                                break;
                            #endif
                        }

                        if (user.sendEnter) {
                            ESP_LOGI(TAG, "Sending ENTER key");
                            // Send an ENTER key
                            switch (user.login_type)  {
                                case 0:  // BLE only
                                    ble_send_key_combination(KEYBOARD_MODIFIER_NONE, HID_KEY_ENTER);
                                    break;
                                
                                #if CONFIG_IDF_TARGET_ESP32S3
                                case 1:  // USB only
                                    usb_send_key_combination(KEYBOARD_MODIFIER_NONE, HID_KEY_ENTER);
                                    break;
                                case 2:  // Both
                                    if (usb_available) {
                                        usb_send_key_combination(KEYBOARD_MODIFIER_NONE, HID_KEY_ENTER);
                                    } else {
                                        ble_send_key_combination(KEYBOARD_MODIFIER_NONE, HID_KEY_ENTER);
                                    }
                                    break;
                                #endif
                            }
                        }

                        userdb_increment_usage(user_index);                        
                        display_oled_post_info("Finger ID: %02d", finger_index);
                        user_index = -1;
                    }  
                    else {
                        ESP_LOGE(TAG, "Decrypt error");
                        display_oled_post_error("Decrypt err");
                    }               
                }
                else {
                    ESP_LOGW(TAG, "No account selected");
                    display_oled_post_error("No account");
                    buzzer_feedback_noauth();
                }
            } 
            else {
                ESP_LOGI(TAG, "No matching finger found.");                
            }
        }

        // Wait a bit before repeating the search
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
