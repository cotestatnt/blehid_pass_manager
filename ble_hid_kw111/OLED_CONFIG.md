# Configurazione Display OLED

## Tipi di Display Supportati

Il progetto supporta due tipi di display OLED:

1. **OLED 96x16 pixel** - Display originale, singola linea
2. **OLED 128x32 pixel** - Display più grande, supporta 2 linee di testo

## Configurazione

Per configurare il tipo di display, modificare il file `main/include/config.h`:

```c
// Configure the OLED display type here
#define OLED_TYPE OLED_96x16    // Per display 96x16
// oppure
#define OLED_TYPE OLED_128x32   // Per display 128x32
```

## Parametri Automatici

In base al tipo di display selezionato, vengono automaticamente configurati:

### Per OLED_96x16:
- Risoluzione: 96x16 pixel
- Numero massimo di linee: 1
- Caratteri per linea: 12

### Per OLED_128x32:
- Risoluzione: 128x32 pixel  
- Numero massimo di linee: 2
- Caratteri per linea: 16

## Funzionalità

Il codice attuale supporta automaticamente il word wrapping e il supporto multilinea per il display 128x32. Non sono necessarie modifiche al codice applicativo esistente.

### Icone e Status Bar (128x32 solamente)

Il display 128x32 supporta una status bar nella prima riga con:
- Icona BLE (connesso/disconnesso)
- Icona USB (quando connesso)
- Icona batteria con percentuale (allineata a destra)

La seconda riga è disponibile per il testo principale.

### Funzioni Icone Disponibili

- `oled_update_status()` - Status completo con icone + testo
- `oled_show_status_bar()` - Solo barra di stato
- `oled_draw_icon()` - Disegna icone singole

Per dettagli completi vedere `ICONS_DOCUMENTATION.md`.
