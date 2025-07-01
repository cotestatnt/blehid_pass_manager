#pragma once
#include <bluefruit.h>
extern BLEHidAdafruit blehid;
#define IT_MOD_NONE 0x00
#define IT_MOD_SHIFT 0x02
#define IT_MOD_ALTGR 0x40

// Tabella di conversione ASCII -> {modificatore, keycode} per layout IT
const uint8_t hid_ascii_to_keycode_it[128][2] = {
  // 0x00 - 0x1F (non stampabili)
  { IT_MOD_NONE, 0 },               // 0x00
  { IT_MOD_NONE, 0 },               // 0x01
  { IT_MOD_NONE, 0 },               // 0x02
  { IT_MOD_NONE, 0 },               // 0x03
  { IT_MOD_NONE, 0 },               // 0x04
  { IT_MOD_NONE, 0 },               // 0x05
  { IT_MOD_NONE, 0 },               // 0x06
  { IT_MOD_NONE, 0 },               // 0x07
  { IT_MOD_NONE, 0 },               // 0x08
  { IT_MOD_NONE, HID_KEY_TAB },     // 0x09   \t
  { IT_MOD_NONE, HID_KEY_ENTER },   // 0x0A   \n
  { IT_MOD_NONE, 0 },               // 0x0B
  { IT_MOD_NONE, 0 },               // 0x0C
  { IT_MOD_NONE, 0 },               // 0x0D
  { IT_MOD_NONE, 0 },               // 0x0E
  { IT_MOD_NONE, 0 },               // 0x0F
  { IT_MOD_NONE, 0 },               // 0x10
  { IT_MOD_NONE, 0 },               // 0x11
  { IT_MOD_NONE, 0 },               // 0x12
  { IT_MOD_NONE, 0 },               // 0x13
  { IT_MOD_NONE, 0 },               // 0x14
  { IT_MOD_NONE, 0 },               // 0x15
  { IT_MOD_NONE, 0 },               // 0x16
  { IT_MOD_NONE, 0 },               // 0x17
  { IT_MOD_NONE, 0 },               // 0x18
  { IT_MOD_NONE, 0 },               // 0x19
  { IT_MOD_NONE, 0 },               // 0x1A
  { IT_MOD_NONE, 0 },               // 0x1B
  { IT_MOD_NONE, HID_KEY_ESCAPE },  // 0x1C
  { IT_MOD_NONE, 0 },               // 0x1D
  { IT_MOD_NONE, 0 },               // 0x1E
  { IT_MOD_NONE, 0 },               // 0x1F

  // 0x20 - 0x2F
  { IT_MOD_NONE, HID_KEY_SPACE },           // 0x20   ' '
  { IT_MOD_SHIFT, HID_KEY_1 },              // 0x21   '!'
  { IT_MOD_SHIFT, HID_KEY_2 },              // 0x22   '"'
  { IT_MOD_ALTGR, HID_KEY_APOSTROPHE },     // 0x23   '#'
  { IT_MOD_SHIFT, HID_KEY_4 },              // 0x24   '$'
  { IT_MOD_SHIFT, HID_KEY_5 },              // 0x25   '%'
  { IT_MOD_SHIFT, HID_KEY_6 },              // 0x26   '&'
  { IT_MOD_NONE, HID_KEY_MINUS },           // 0x27   '''
  { IT_MOD_SHIFT, HID_KEY_8 },              // 0x28   '('
  { IT_MOD_SHIFT, HID_KEY_9 },              // 0x29   ')'
  { IT_MOD_SHIFT, HID_KEY_BRACKET_RIGHT },  // 0x2A   '*'
  { IT_MOD_NONE, HID_KEY_BRACKET_RIGHT },   // 0x2B   '+'
  { IT_MOD_NONE, HID_KEY_COMMA },           // 0x2C   ','
  { IT_MOD_NONE, HID_KEY_SLASH },           // 0x2D   '-'
  { IT_MOD_NONE, HID_KEY_PERIOD },          // 0x2E   '.'
  { IT_MOD_SHIFT, HID_KEY_7 },              // 0x2F   '/'

  // 0x30 - 0x39
  { IT_MOD_NONE, HID_KEY_0 },  // 0x30   '0'
  { IT_MOD_NONE, HID_KEY_1 },  // 0x31   '1'
  { IT_MOD_NONE, HID_KEY_2 },  // 0x32   '2'
  { IT_MOD_NONE, HID_KEY_3 },  // 0x33   '3'
  { IT_MOD_NONE, HID_KEY_4 },  // 0x34   '4'
  { IT_MOD_NONE, HID_KEY_5 },  // 0x35   '5'
  { IT_MOD_NONE, HID_KEY_6 },  // 0x36   '6'
  { IT_MOD_NONE, HID_KEY_7 },  // 0x37   '7'
  { IT_MOD_NONE, HID_KEY_8 },  // 0x38   '8'
  { IT_MOD_NONE, HID_KEY_9 },  // 0x39   '9'

  // 0x3A - 0x40
  { IT_MOD_SHIFT, HID_KEY_PERIOD },     // 0x3A   ':'
  { IT_MOD_SHIFT, HID_KEY_COMMA },      // 0x3B   ';'
  { IT_MOD_NONE, HID_KEY_EUROPE_2 },    // 0x3C   '<'
  { IT_MOD_SHIFT, HID_KEY_0 },          // 0x3D   '='
  { IT_MOD_SHIFT, HID_KEY_EUROPE_2 },   // 0x3E   '>'
  { IT_MOD_SHIFT, HID_KEY_MINUS },      // 0x3F   '?'
  { IT_MOD_ALTGR, HID_KEY_SEMICOLON },  // 0x40   '@'

  // 0x41 - 0x5A (A-Z)
  { IT_MOD_SHIFT, HID_KEY_A },  // 0x41   'A'
  { IT_MOD_SHIFT, HID_KEY_B },  // 0x42   'B'
  { IT_MOD_SHIFT, HID_KEY_C },  // 0x43   'C'
  { IT_MOD_SHIFT, HID_KEY_D },  // 0x44   'D'
  { IT_MOD_SHIFT, HID_KEY_E },  // 0x45   'E'
  { IT_MOD_SHIFT, HID_KEY_F },  // 0x46   'F'
  { IT_MOD_SHIFT, HID_KEY_G },  // 0x47   'G'
  { IT_MOD_SHIFT, HID_KEY_H },  // 0x48   'H'
  { IT_MOD_SHIFT, HID_KEY_I },  // 0x49   'I'
  { IT_MOD_SHIFT, HID_KEY_J },  // 0x4A   'J'
  { IT_MOD_SHIFT, HID_KEY_K },  // 0x4B   'K'
  { IT_MOD_SHIFT, HID_KEY_L },  // 0x4C   'L'
  { IT_MOD_SHIFT, HID_KEY_M },  // 0x4D   'M'
  { IT_MOD_SHIFT, HID_KEY_N },  // 0x4E   'N'
  { IT_MOD_SHIFT, HID_KEY_O },  // 0x4F   'O'
  { IT_MOD_SHIFT, HID_KEY_P },  // 0x50   'P'
  { IT_MOD_SHIFT, HID_KEY_Q },  // 0x51   'Q'
  { IT_MOD_SHIFT, HID_KEY_R },  // 0x52   'R'
  { IT_MOD_SHIFT, HID_KEY_S },  // 0x53   'S'
  { IT_MOD_SHIFT, HID_KEY_T },  // 0x54   'T'
  { IT_MOD_SHIFT, HID_KEY_U },  // 0x55   'U'
  { IT_MOD_SHIFT, HID_KEY_V },  // 0x56   'V'
  { IT_MOD_SHIFT, HID_KEY_W },  // 0x57   'W'
  { IT_MOD_SHIFT, HID_KEY_X },  // 0x58   'X'
  { IT_MOD_SHIFT, HID_KEY_Y },  // 0x59   'Y'
  { IT_MOD_SHIFT, HID_KEY_Z },  // 0x5A   'Z'

  // 0x5B - 0x60
  { IT_MOD_ALTGR, HID_KEY_BRACKET_LEFT },   // 0x5B   '['
  { IT_MOD_NONE, HID_KEY_GRAVE },           // 0x5C   '\'
  { IT_MOD_ALTGR, HID_KEY_BRACKET_RIGHT },  // 0x5D   ']'
  { IT_MOD_SHIFT, HID_KEY_EQUAL },          // 0x5E   '^'
  { IT_MOD_SHIFT, HID_KEY_SLASH },          // 0x5F   '_'
  { IT_MOD_NONE, HID_KEY_GRAVE },           // 0x60   '`'

  // 0x61 - 0x7A (a-z)
  { IT_MOD_NONE, HID_KEY_A },  // 0x61   'a'
  { IT_MOD_NONE, HID_KEY_B },  // 0x62   'b'
  { IT_MOD_NONE, HID_KEY_C },  // 0x63   'c'
  { IT_MOD_NONE, HID_KEY_D },  // 0x64   'd'
  { IT_MOD_NONE, HID_KEY_E },  // 0x65   'e'
  { IT_MOD_NONE, HID_KEY_F },  // 0x66   'f'
  { IT_MOD_NONE, HID_KEY_G },  // 0x67   'g'
  { IT_MOD_NONE, HID_KEY_H },  // 0x68   'h'
  { IT_MOD_NONE, HID_KEY_I },  // 0x69   'i'
  { IT_MOD_NONE, HID_KEY_J },  // 0x6A   'j'
  { IT_MOD_NONE, HID_KEY_K },  // 0x6B   'k'
  { IT_MOD_NONE, HID_KEY_L },  // 0x6C   'l'
  { IT_MOD_NONE, HID_KEY_M },  // 0x6D   'm'
  { IT_MOD_NONE, HID_KEY_N },  // 0x6E   'n'
  { IT_MOD_NONE, HID_KEY_O },  // 0x6F   'o'
  { IT_MOD_NONE, HID_KEY_P },  // 0x70   'p'
  { IT_MOD_NONE, HID_KEY_Q },  // 0x71   'q'
  { IT_MOD_NONE, HID_KEY_R },  // 0x72   'r'
  { IT_MOD_NONE, HID_KEY_S },  // 0x73   's'
  { IT_MOD_NONE, HID_KEY_T },  // 0x74   't'
  { IT_MOD_NONE, HID_KEY_U },  // 0x75   'u'
  { IT_MOD_NONE, HID_KEY_V },  // 0x76   'v'
  { IT_MOD_NONE, HID_KEY_W },  // 0x77   'w'
  { IT_MOD_NONE, HID_KEY_X },  // 0x78   'x'
  { IT_MOD_NONE, HID_KEY_Y },  // 0x79   'y'
  { IT_MOD_NONE, HID_KEY_Z },  // 0x7A   'z'

  // 0x7B - 0x7F
  { IT_MOD_ALTGR, HID_KEY_7 },      // 0x7B   '{'
  { IT_MOD_SHIFT, HID_KEY_GRAVE },  // 0x7C   '|'
  { IT_MOD_ALTGR, HID_KEY_0 },      // 0x7D   '}'
  { IT_MOD_ALTGR, HID_KEY_MINUS },  // 0x7E   '~'
  { IT_MOD_NONE, 0 }                // 0x7F   DEL
};


void sendString(const char* str) {
  size_t i = 0;
  while (str[i]) {
    uint8_t c = (uint8_t)str[i];
    uint32_t codepoint = 0;

    if (c < 0x80) {
      codepoint = c;
      i += 1;
    } else if ((c & 0xE0) == 0xC0 && str[i+1]) {
      codepoint = ((c & 0x1F) << 6) | ((uint8_t)str[i+1] & 0x3F);
      i += 2;
    } else if ((c & 0xF0) == 0xE0 && str[i+1] && str[i+2]) {
      codepoint = ((c & 0x0F) << 12) | (((uint8_t)str[i+1] & 0x3F) << 6) | ((uint8_t)str[i+2] & 0x3F);
      i += 3;
    } else {
      Serial.print(" NU ");
      Serial.print(c, HEX);
      i += 1;
      continue;
    }

    Serial.print(" ");
    Serial.print(c, HEX);

    hid_keyboard_report_t report;
    varclr(&report);

    // àèéìòù
    // òèèà\ù

    switch (codepoint) {
      case 0x00B0: report.modifier = IT_MOD_SHIFT; report.keycode[0] = HID_KEY_APOSTROPHE; break;   // °
      case 0x00E7: report.modifier = IT_MOD_SHIFT; report.keycode[0] = HID_KEY_GRAVE; break;        // ç
      case 0x00E0: report.keycode[0] = HID_KEY_APOSTROPHE; break;                                   // à
      case 0x00E8: report.keycode[0] = HID_KEY_BRACKET_LEFT; break;                                 // è
      case 0x00E9: report.modifier = IT_MOD_SHIFT; report.keycode[0] = HID_KEY_BRACKET_LEFT; break; // é
      case 0x00EC: report.keycode[0] = HID_KEY_EQUAL; break;                                        // ì
      case 0x00F2: report.keycode[0] = HID_KEY_SEMICOLON; break;                                    // ò
      case 0x00F9: report.keycode[0] = HID_KEY_BACKSLASH; break;                                    // ù
      default:
        report.modifier = hid_ascii_to_keycode_it[(uint8_t)codepoint][0];
        report.keycode[0] = hid_ascii_to_keycode_it[(uint8_t)codepoint][1];
        break;
    }

    if (report.keycode[0] == 0) continue;
    blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, &report);
    delay(2);
    blehid.keyRelease();
    delay(1);
  }
}


// void sendString(const char* str) {
//   while (*str) {
//     hid_keyboard_report_t report;
//     varclr(&report);

//     char ch = *str++;
//     report.modifier = hid_ascii_to_keycode_it[(uint8_t)ch][0];
//     report.keycode[0] = hid_ascii_to_keycode_it[(uint8_t)ch][1];

//     Serial.print(" ");
//     Serial.print(ch, HEX);

//     if (report.keycode[0] == 0) 
//       continue;
    
//     blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, &report);
//     delay(2);
//     blehid.keyRelease();
//     delay(1);
//   }
// }