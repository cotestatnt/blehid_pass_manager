// Esempio di utilizzo delle nuove funzioni icone OLED
// Da integrare nel codice esistente

#include "oled.h"
#include "battery.h"
#include "hid_device_ble.h"

void update_oled_with_status() {
    // Ottieni i valori di stato attuali
    bool ble_connected = ble_is_connected();
    bool usb_connected = is_usb_connected_simple();
    int battery_percent = get_battery_percentage();
    
    // Aggiorna il display con status completo (per display 128x32)
    oled_update_status(ble_connected, usb_connected, battery_percent, "BLE PassMan");
}

void show_user_info_with_status(const char* username) {
    // Ottieni i valori di stato attuali
    bool ble_connected = ble_is_connected();
    bool usb_connected = is_usb_connected_simple();
    int battery_percent = get_battery_percentage();
    
    // Mostra informazioni utente con status bar
    oled_update_status(ble_connected, usb_connected, battery_percent, username);
}

void show_only_status_bar() {
    // Mostra solo la barra di stato senza testo principale
    bool ble_connected = ble_is_connected();
    bool usb_connected = is_usb_connected_simple();
    int battery_percent = get_battery_percentage();
    
    oled_show_status_bar(ble_connected, usb_connected, battery_percent);
}

void draw_individual_icons_example() {
    // Esempio per disegnare icone singole
    oled_draw_icon(0, 0, ICON_BLE_CONNECTED);      // Icona BLE connesso in alto a sinistra
    oled_draw_icon(10, 0, ICON_USB_CONNECTED);     // Icona USB connesso
    oled_draw_icon(20, 0, ICON_BATTERY_FULL);      // Icona batteria piena
}

// Esempio di integrazione nel loop principale:
/*
void main_loop_with_status_update() {
    while(1) {
        // Aggiorna il display con status ogni secondo
        update_oled_with_status();
        
        // Resto del codice...
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
*/
