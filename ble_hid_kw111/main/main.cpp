#include "esp_log.h"
#include "fpm.h"                       // header adattato
#include "transport/espidf_uart_stream.h"

static const char* TAG = "APP";

bool searchDatabase(FPM & finger) {
  FPMStatus status;

  /* Take a snapshot of the input finger */
  ESP_LOGI(TAG, "Place a finger.");

  do {
    status = finger.getImage();

    switch (status) {
      case FPMStatus::OK:
        ESP_LOGI(TAG, "Image taken");
        break;

      case FPMStatus::NOFINGER:
        // ESP_LOGI(TAG, ".");
        vTaskDelay(200 / portTICK_PERIOD_MS);
        break;

      default:
        /* allow retries even when an error happens */        
        ESP_LOGE(TAG, "getImage(): error 0x%X", static_cast<uint16_t>(status));
        break;
    }

    taskYIELD();
  } while (status != FPMStatus::OK);

  /* Extract the fingerprint features */
  status = finger.image2Tz();

  switch (status) {
    case FPMStatus::OK:
      ESP_LOGE(TAG, "Image converted");
      break;

    default:      
      ESP_LOGE(TAG, "image2Tz(): error 0x%X", static_cast<uint16_t>(status));
      return false;
  }

  /* Search the database for the converted print */
  uint16_t fid, score;
  status = finger.searchDatabase(&fid, &score);

  switch (status) {
    case FPMStatus::OK:
      ESP_LOGI(TAG,  "Found a match at ID #%u with confidence %u", fid, score);      
      break;

    case FPMStatus::NOTFOUND:
      ESP_LOGI(TAG, "Did not find a match.");
      return false;

    default:
      ESP_LOGE(TAG, "searchDatabase(): error 0x%X", static_cast<uint16_t>(status));      
      return false;
  }

  /* Now wait for the finger to be removed, though not necessary. 
       This was moved here after the Search operation because of the R503 sensor, 
       whose searches oddly fail if they happen after the image buffer is cleared  */
  ESP_LOGI(TAG, "Remove finger.");
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  do {
    status = finger.getImage();
    vTaskDelay(200 / portTICK_PERIOD_MS);
  } while (status != FPMStatus::NOFINGER);

  return true;
}


extern "C" void app_main(void) {

    // Configura UART1: cambiare i GPIO secondo la propria scheda
    EspIdfUartStream uart{UART_NUM_1, /*TX*/4, /*RX*/3, /*baud*/57600};
    if (uart.begin() != ESP_OK) {
        ESP_LOGE(TAG, "UART init failed");
        return;
    }

    // Inizializza FPM usando il trasporto UART di ESP-IDF
    FPM fpm(&uart);

    if (!fpm.begin()) {
        ESP_LOGE(TAG, "FPM begin() failed (verifica cavi, baud, alimentazione)");
        return;
    }

    // Lettura parametri prodotto 
    FPMSystemParams params{};
    if (fpm.readParams(&params) == FPMStatus::LIB_OK) {
        ESP_LOGI(TAG, "Found fingerprint sensor!");
        ESP_LOGI(TAG, "Capacity: %u", params.capacity);
        ESP_LOGI(TAG, "Packet length: %u", FPM::packetLengths[static_cast<uint8_t>(params.packetLen)]);
    } else {
        ESP_LOGE(TAG, "readParams non eseguito correttamente");
    }

    // Leggi informazioni sul numero di fingerprint memorizzati
    int16_t count = 0;
    if (fpm.getLastIndex(&count) == FPMStatus::LIB_OK) {
        ESP_LOGI(TAG, "Numero di fingerprint nel database: %u", (unsigned)count + 1); // +1 perché l'indice parte da 0
    } else {
        ESP_LOGW(TAG, "Impossibile leggere il numero di fingerprint");
    }

    searchDatabase(fpm);
}


