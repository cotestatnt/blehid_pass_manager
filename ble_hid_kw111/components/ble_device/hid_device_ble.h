#pragma once
#ifndef HID_DEVICE_BLE_H
#define HID_DEVICE_BLE_H

#include "esp_bt_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MTU_SIZE                128

esp_err_t ble_device_init(void);

void ble_userlist_set_authenticated(bool value);
bool ble_userlist_is_authenticated();

void ble_send_char(wint_t chr);
void ble_send_key_combination(uint8_t modifiers, uint8_t key);
void ble_send_string(const char* str);

bool ble_is_connected(void);
// True if connected and HID keyboard notifications are enabled on CCCD
bool ble_can_send(void);

esp_err_t ble_force_disconnect(void);
esp_err_t ble_force_disconnect_and_clear(void);  // New function for forced cleanup
void ble_clear_blacklist(void);                  // Clear blacklist manually
void ble_check_blacklist_expiry(void);           // Check if blacklist has expired

esp_err_t ble_remove_all_bonded_devices(void);
void ble_get_connection_info(esp_bd_addr_t *remote_addr, bool *is_bonded);


/*
Logica di funzionamento.
All'avvio: whitelist disabilitata (whitelist_enabled = false)
hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
Accetta connessioni da qualsiasi dispositivo (bonded e non bonded)

Connessione di un dispositivo non bonded: Il dispositivo può connettersi e fare pairing
Dopo il pairing completato con successo (ESP_GAP_BLE_AUTH_CMPL_EVT), se la whitelist è attiva, viene aggiornata

Pressione dei pulsanti (3 secondi): Il dispositivo corrente viene blacklistato
Si attiva la whitelist con tutti i dispositivi bonded ECCETTO quello blacklistato
hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST
Solo i dispositivi in whitelist possono connettersi

Dopo 30 secondi: La blacklist scade automaticamente
Si disattiva la whitelist e si torna alle condizioni iniziali (accetta tutti i dispositivi)
*/

// Blacklist management functions
bool is_device_blacklisted(const esp_bd_addr_t addr);
void blacklist_device(const esp_bd_addr_t addr);

// Whitelist management functions
void update_whitelist_from_bonded_devices(void);
esp_err_t update_advertising_filter_policy(void);
void enable_whitelist_filtering(void);
void disable_whitelist_filtering(void);

// Public function to check and update blacklist/whitelist status
void ble_check_blacklist_expiry(void);

#ifdef __cplusplus
}
#endif

#endif