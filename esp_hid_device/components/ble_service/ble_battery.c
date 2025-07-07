#include "host/ble_hs.h"
#include "ble_battery.h"
#include "esp_log.h"

static const char *TAG = "BLE_BATT";
static uint8_t battery_level = 100;
static uint16_t batt_level_val_handle = 0;

// Funzione di accesso caratteristica (read)
static int battery_level_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg) {
    // Valore 1 byte: livello batteria %
    int rc = os_mbuf_append(ctxt->om, &battery_level, 1);
    if (rc != 0) {
        ESP_LOGE(TAG, "Errore appending battery level");
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return 0; // success
}

// Definizione GATT del servizio batteria
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x180F), // Battery Service
        .characteristics = (struct ble_gatt_chr_def[]) { {
            .uuid = BLE_UUID16_DECLARE(0x2A19), // Battery Level
            .access_cb = battery_level_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            .val_handle = &batt_level_val_handle,
        }, {
            0, // Terminatore
        } }
    },
    { 0 }
};

void ble_battery_init(void) {
    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg failed: %d", rc);
        return;
    }
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs failed: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "Battery service GATT registered");
}

void ble_battery_set_level(uint8_t percent) {
    battery_level = percent;
    if (batt_level_val_handle)
        ble_gatts_chr_updated(batt_level_val_handle);
}

void ble_battery_notify_level(uint8_t percent) {
    battery_level = percent;
    // Notifica a tutti i peer connessi (va passato il conn_handle, qui usiamo NONE per broadcast)
    if (batt_level_val_handle) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(&battery_level, 1);
        ble_gattc_notify_custom(BLE_HS_CONN_HANDLE_NONE, batt_level_val_handle, om);
        ESP_LOGI(TAG, "Battery level notified: %d%%", battery_level);
    }
}
