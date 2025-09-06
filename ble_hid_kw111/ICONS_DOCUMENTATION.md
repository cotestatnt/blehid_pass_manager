# Icone OLED - Documentazione

## Icone Disponibili

Il sistema supporta le seguenti icone 8x8 pixel:

### Connessione BLE
- `ICON_BLE_CONNECTED` - Simbolo Bluetooth con indicatore di connessione
- `ICON_BLE_DISCONNECTED` - Simbolo Bluetooth con X (disconnesso)

### Connessione USB
- `ICON_USB_CONNECTED` - Simbolo USB

### Batteria (5 livelli)
- `ICON_BATTERY_FULL` - Batteria piena (90-100%)
- `ICON_BATTERY_HIGH` - Batteria alta (70-89%)
- `ICON_BATTERY_MEDIUM` - Batteria media (40-69%)
- `ICON_BATTERY_LOW` - Batteria bassa (15-39%)
- `ICON_BATTERY_EMPTY` - Batteria scarica (0-14%)

## Funzioni Disponibili

### Funzioni Base
```c
void oled_draw_icon(int x, int y, oled_icon_t icon);
```
Disegna una singola icona alle coordinate specificate.

### Status Bar (solo per display 128x32)
```c
void oled_show_status_bar(bool ble_connected, bool usb_connected, int battery_percent);
```
Mostra solo la barra di stato con icone nella prima riga.

### Status Completo (raccomandato)
```c
void oled_update_status(bool ble_connected, bool usb_connected, int battery_percent, const char* main_text);
```
Mostra status bar nella prima riga e testo principale nella seconda riga.
Per display 96x16, mostra solo il testo principale.

## Layout Status Bar (128x32)

```
[BLE] [USB] ........................ [BAT] [XX%]
[           Testo principale                   ]
```

- Prima riga (0-15px): Icone di stato
- Seconda riga (16-31px): Testo principale

## Posizionamento Icone

- **BLE**: Sempre presente, posizione 0,0
- **USB**: Solo se connesso, posizione 10,0 
- **Batteria**: Sempre presente, allineata a destra
- **Percentuale**: Accanto all'icona batteria

## Compatibilità Display

- **96x16**: Solo testo principale (le funzioni status vengono ignorate)
- **128x32**: Status bar completa + testo principale

## Esempio d'uso

```c
// Aggiornamento status completo
bool ble_conn = ble_is_connected();
bool usb_conn = is_usb_connected_simple();
int battery = get_battery_percentage();

oled_update_status(ble_conn, usb_conn, battery, "BLE PassMan");
```
