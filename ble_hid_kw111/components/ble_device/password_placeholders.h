// Password placeholder definitions (1-byte markers inside the stored password)
// Byte values 0x80-0x8F are chosen because they are UTF-8 continuation bytes
// and never appear standalone in valid UTF-8, so we can safely repurpose them.
// They are inserted BEFORE cifratura (encryption) e restano invariati.
#pragma once

#define PW_PH_ENTER          0x80  // Invio (ENTER)
#define PW_PH_TAB            0x81  // Tab
#define PW_PH_ESC            0x82  // Escape
#define PW_PH_BACKSPACE      0x83  // Backspace
#define PW_PH_DELAY_500MS    0x84  // Pausa 500 ms
#define PW_PH_DELAY_1000MS   0x85  // Pausa 1000 ms
#define PW_PH_CTRL_ALT_DEL   0x86  // Sequenza Ctrl+Alt+Del
#define PW_PH_SHIFT_TAB      0x87  // Shift+Tab
#define PW_PH_SLEEP          0x88  // Deep sleep
// 0x88-0x8F liberi per azioni future (ALT+TAB ecc.)

// Utility macro per riconoscere rapidamente placeholder
#define PW_IS_PLACEHOLDER(b) ((uint8_t)(b) >= 0x80 && (uint8_t)(b) <= 0x8F)
