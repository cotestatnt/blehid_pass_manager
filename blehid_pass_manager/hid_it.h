#pragma once
#include <bluefruit.h>
extern BLEHidAdafruit blehid;
#define IT_MOD_NONE 0x00
#define IT_MOD_SHIFT 0x02
#define IT_MOD_ALTGR 0x40

//--------------------------------------------------------------------+
// HID KEYCODE
//--------------------------------------------------------------------+
#define HID_KEY_NONE                        0x00
#define HID_KEY_A                           0x04
#define HID_KEY_B                           0x05
#define HID_KEY_C                           0x06
#define HID_KEY_D                           0x07
#define HID_KEY_E                           0x08
#define HID_KEY_F                           0x09
#define HID_KEY_G                           0x0A
#define HID_KEY_H                           0x0B
#define HID_KEY_I                           0x0C
#define HID_KEY_J                           0x0D
#define HID_KEY_K                           0x0E
#define HID_KEY_L                           0x0F
#define HID_KEY_M                           0x10
#define HID_KEY_N                           0x11
#define HID_KEY_O                           0x12
#define HID_KEY_P                           0x13
#define HID_KEY_Q                           0x14
#define HID_KEY_R                           0x15
#define HID_KEY_S                           0x16
#define HID_KEY_T                           0x17
#define HID_KEY_U                           0x18
#define HID_KEY_V                           0x19
#define HID_KEY_W                           0x1A
#define HID_KEY_X                           0x1B
#define HID_KEY_Y                           0x1C
#define HID_KEY_Z                           0x1D
#define HID_KEY_1                           0x1E
#define HID_KEY_2                           0x1F
#define HID_KEY_3                           0x20
#define HID_KEY_4                           0x21
#define HID_KEY_5                           0x22
#define HID_KEY_6                           0x23
#define HID_KEY_7                           0x24
#define HID_KEY_8                           0x25
#define HID_KEY_9                           0x26
#define HID_KEY_0                           0x27
#define HID_KEY_ENTER                       0x28
#define HID_KEY_ESCAPE                      0x29
#define HID_KEY_BACKSPACE                   0x2A
#define HID_KEY_TAB                         0x2B
#define HID_KEY_SPACE                       0x2C
#define HID_KEY_MINUS                       0x2D
#define HID_KEY_EQUAL                       0x2E
#define HID_KEY_BRACKET_LEFT                0x2F
#define HID_KEY_BRACKET_RIGHT               0x30
#define HID_KEY_BACKSLASH                   0x31
#define HID_KEY_EUROPE_1                    0x32
#define HID_KEY_SEMICOLON                   0x33
#define HID_KEY_APOSTROPHE                  0x34
#define HID_KEY_GRAVE                       0x35
#define HID_KEY_COMMA                       0x36
#define HID_KEY_PERIOD                      0x37
#define HID_KEY_SLASH                       0x38
#define HID_KEY_CAPS_LOCK                   0x39
#define HID_KEY_F1                          0x3A
#define HID_KEY_F2                          0x3B
#define HID_KEY_F3                          0x3C
#define HID_KEY_F4                          0x3D
#define HID_KEY_F5                          0x3E
#define HID_KEY_F6                          0x3F
#define HID_KEY_F7                          0x40
#define HID_KEY_F8                          0x41
#define HID_KEY_F9                          0x42
#define HID_KEY_F10                         0x43
#define HID_KEY_F11                         0x44
#define HID_KEY_F12                         0x45
#define HID_KEY_PRINT_SCREEN                0x46
#define HID_KEY_SCROLL_LOCK                 0x47
#define HID_KEY_PAUSE                       0x48
#define HID_KEY_INSERT                      0x49
#define HID_KEY_HOME                        0x4A
#define HID_KEY_PAGE_UP                     0x4B
#define HID_KEY_DELETE                      0x4C
#define HID_KEY_END                         0x4D
#define HID_KEY_PAGE_DOWN                   0x4E
#define HID_KEY_ARROW_RIGHT                 0x4F
#define HID_KEY_ARROW_LEFT                  0x50
#define HID_KEY_ARROW_DOWN                  0x51
#define HID_KEY_ARROW_UP                    0x52
#define HID_KEY_NUM_LOCK                    0x53
#define HID_KEY_KEYPAD_DIVIDE               0x54
#define HID_KEY_KEYPAD_MULTIPLY             0x55
#define HID_KEY_KEYPAD_SUBTRACT             0x56
#define HID_KEY_KEYPAD_ADD                  0x57
#define HID_KEY_KEYPAD_ENTER                0x58
#define HID_KEY_KEYPAD_1                    0x59
#define HID_KEY_KEYPAD_2                    0x5A
#define HID_KEY_KEYPAD_3                    0x5B
#define HID_KEY_KEYPAD_4                    0x5C
#define HID_KEY_KEYPAD_5                    0x5D
#define HID_KEY_KEYPAD_6                    0x5E
#define HID_KEY_KEYPAD_7                    0x5F
#define HID_KEY_KEYPAD_8                    0x60
#define HID_KEY_KEYPAD_9                    0x61
#define HID_KEY_KEYPAD_0                    0x62
#define HID_KEY_KEYPAD_DECIMAL              0x63
#define HID_KEY_EUROPE_2                    0x64
#define HID_KEY_APPLICATION                 0x65
#define HID_KEY_POWER                       0x66
#define HID_KEY_KEYPAD_EQUAL                0x67
#define HID_KEY_F13                         0x68
#define HID_KEY_F14                         0x69
#define HID_KEY_F15                         0x6A
#define HID_KEY_F16                         0x6B
#define HID_KEY_F17                         0x6C
#define HID_KEY_F18                         0x6D
#define HID_KEY_F19                         0x6E
#define HID_KEY_F20                         0x6F
#define HID_KEY_F21                         0x70
#define HID_KEY_F22                         0x71
#define HID_KEY_F23                         0x72
#define HID_KEY_F24                         0x73
#define HID_KEY_EXECUTE                     0x74
#define HID_KEY_HELP                        0x75
#define HID_KEY_MENU                        0x76
#define HID_KEY_SELECT                      0x77
#define HID_KEY_STOP                        0x78
#define HID_KEY_AGAIN                       0x79
#define HID_KEY_UNDO                        0x7A
#define HID_KEY_CUT                         0x7B
#define HID_KEY_COPY                        0x7C
#define HID_KEY_PASTE                       0x7D
#define HID_KEY_FIND                        0x7E
#define HID_KEY_MUTE                        0x7F
#define HID_KEY_VOLUME_UP                   0x80
#define HID_KEY_VOLUME_DOWN                 0x81
#define HID_KEY_LOCKING_CAPS_LOCK           0x82
#define HID_KEY_LOCKING_NUM_LOCK            0x83
#define HID_KEY_LOCKING_SCROLL_LOCK         0x84
#define HID_KEY_KEYPAD_COMMA                0x85
#define HID_KEY_KEYPAD_EQUAL_SIGN           0x86
#define HID_KEY_KANJI1                      0x87
#define HID_KEY_KANJI2                      0x88
#define HID_KEY_KANJI3                      0x89
#define HID_KEY_KANJI4                      0x8A
#define HID_KEY_KANJI5                      0x8B
#define HID_KEY_KANJI6                      0x8C
#define HID_KEY_KANJI7                      0x8D
#define HID_KEY_KANJI8                      0x8E
#define HID_KEY_KANJI9                      0x8F
#define HID_KEY_LANG1                       0x90
#define HID_KEY_LANG2                       0x91
#define HID_KEY_LANG3                       0x92
#define HID_KEY_LANG4                       0x93
#define HID_KEY_LANG5                       0x94
#define HID_KEY_LANG6                       0x95
#define HID_KEY_LANG7                       0x96
#define HID_KEY_LANG8                       0x97
#define HID_KEY_LANG9                       0x98
#define HID_KEY_ALTERNATE_ERASE             0x99
#define HID_KEY_SYSREQ_ATTENTION            0x9A
#define HID_KEY_CANCEL                      0x9B
#define HID_KEY_CLEAR                       0x9C
#define HID_KEY_PRIOR                       0x9D
#define HID_KEY_RETURN                      0x9E
#define HID_KEY_SEPARATOR                   0x9F
#define HID_KEY_OUT                         0xA0
#define HID_KEY_OPER                        0xA1
#define HID_KEY_CLEAR_AGAIN                 0xA2
#define HID_KEY_CRSEL_PROPS                 0xA3
#define HID_KEY_EXSEL                       0xA4
// RESERVED					                        0xA5-AF
#define HID_KEY_KEYPAD_00                   0xB0
#define HID_KEY_KEYPAD_000                  0xB1
#define HID_KEY_THOUSANDS_SEPARATOR         0xB2
#define HID_KEY_DECIMAL_SEPARATOR           0xB3
#define HID_KEY_CURRENCY_UNIT               0xB4
#define HID_KEY_CURRENCY_SUBUNIT            0xB5
#define HID_KEY_KEYPAD_LEFT_PARENTHESIS     0xB6
#define HID_KEY_KEYPAD_RIGHT_PARENTHESIS    0xB7
#define HID_KEY_KEYPAD_LEFT_BRACE           0xB8
#define HID_KEY_KEYPAD_RIGHT_BRACE          0xB9
#define HID_KEY_KEYPAD_TAB                  0xBA
#define HID_KEY_KEYPAD_BACKSPACE            0xBB
#define HID_KEY_KEYPAD_A                    0xBC
#define HID_KEY_KEYPAD_B                    0xBD
#define HID_KEY_KEYPAD_C                    0xBE
#define HID_KEY_KEYPAD_D                    0xBF
#define HID_KEY_KEYPAD_E                    0xC0
#define HID_KEY_KEYPAD_F                    0xC1
#define HID_KEY_KEYPAD_XOR                  0xC2
#define HID_KEY_KEYPAD_CARET                0xC3
#define HID_KEY_KEYPAD_PERCENT              0xC4
#define HID_KEY_KEYPAD_LESS_THAN            0xC5
#define HID_KEY_KEYPAD_GREATER_THAN         0xC6
#define HID_KEY_KEYPAD_AMPERSAND            0xC7
#define HID_KEY_KEYPAD_DOUBLE_AMPERSAND     0xC8
#define HID_KEY_KEYPAD_VERTICAL_BAR         0xC9
#define HID_KEY_KEYPAD_DOUBLE_VERTICAL_BAR  0xCA
#define HID_KEY_KEYPAD_COLON                0xCB
#define HID_KEY_KEYPAD_HASH                 0xCC
#define HID_KEY_KEYPAD_SPACE                0xCD
#define HID_KEY_KEYPAD_AT                   0xCE
#define HID_KEY_KEYPAD_EXCLAMATION          0xCF
#define HID_KEY_KEYPAD_MEMORY_STORE         0xD0
#define HID_KEY_KEYPAD_MEMORY_RECALL        0xD1
#define HID_KEY_KEYPAD_MEMORY_CLEAR         0xD2
#define HID_KEY_KEYPAD_MEMORY_ADD           0xD3
#define HID_KEY_KEYPAD_MEMORY_SUBTRACT      0xD4
#define HID_KEY_KEYPAD_MEMORY_MULTIPLY      0xD5
#define HID_KEY_KEYPAD_MEMORY_DIVIDE        0xD6
#define HID_KEY_KEYPAD_PLUS_MINUS           0xD7
#define HID_KEY_KEYPAD_CLEAR                0xD8
#define HID_KEY_KEYPAD_CLEAR_ENTRY          0xD9
#define HID_KEY_KEYPAD_BINARY               0xDA
#define HID_KEY_KEYPAD_OCTAL                0xDB
#define HID_KEY_KEYPAD_DECIMAL_2            0xDC
#define HID_KEY_KEYPAD_HEXADECIMAL          0xDD
// RESERVED					                        0xDE-DF
#define HID_KEY_CONTROL_LEFT                0xE0
#define HID_KEY_SHIFT_LEFT                  0xE1
#define HID_KEY_ALT_LEFT                    0xE2
#define HID_KEY_GUI_LEFT                    0xE3
#define HID_KEY_CONTROL_RIGHT               0xE4
#define HID_KEY_SHIFT_RIGHT                 0xE5
#define HID_KEY_ALT_RIGHT                   0xE6
#define HID_KEY_GUI_RIGHT                   0xE7

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

    // Serial.print(" ");
    // Serial.print(c, HEX);

    hid_keyboard_report_t report;
    varclr(&report);

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
