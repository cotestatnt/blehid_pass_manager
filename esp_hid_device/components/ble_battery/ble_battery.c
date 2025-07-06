#include "ble_battery.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_hidd.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

static const char *TAG = "BLE_BATT";

#define BAS_SERVICE_UUID      0x180F
#define BAS_CHAR_UUID_BATT_LVL 0x2A19

static uint8_t battery_level = 100;  // valore iniziale

// Handle della caratteristica (per notifiche)
static uint16_t batt_level_val_handle = 0;

// Access handler per lettura da client
static int battery_level_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        int rc = os_mbuf_append(ctxt->om, &battery_level, sizeof(battery_level));
        ESP_LOGI(TAG, "Battery level read: %u%%", battery_level);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

// Definizione della caratteristica
static const struct ble_gatt_chr_def battery_characteristics[] = {
    {
        .uuid = BLE_UUID16_DECLARE(BAS_CHAR_UUID_BATT_LVL),
        .access_cb = battery_level_access_cb,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
        .val_handle = &batt_level_val_handle
    },
    { 0 } // terminatore
};

// Definizione del servizio
static const struct ble_gatt_svc_def battery_service[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BAS_SERVICE_UUID),
        .characteristics = battery_characteristics
    },
    { 0 } // terminatore
};

void ble_battery_init(void)
{
    int rc;

    rc = ble_gatts_count_cfg(battery_service);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg failed: %d", rc);
        return;
    }

    rc = ble_gatts_add_svcs(battery_service);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs failed: %d", rc);
        return;
    }

    ESP_LOGI(TAG, "Battery service initialized");
}


void ble_battery_notify_level(uint8_t level)
{
    battery_level = level;

    struct os_mbuf *om = ble_hs_mbuf_from_flat(&battery_level, sizeof(battery_level));
    if (!om) {
        ESP_LOGE(TAG, "Failed to allocate mbuf for battery notification");
        return;
    }

    ble_gatts_notify_custom(0, batt_level_val_handle, om);
    ESP_LOGI(TAG, "Battery level notified: %u%%", battery_level);
}
