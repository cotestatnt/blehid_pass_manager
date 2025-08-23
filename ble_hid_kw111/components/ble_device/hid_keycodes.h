#pragma once
#ifndef HID_KEYCODES_H
#define HID_KEYCODES_H  

#include "tinyusb.h"
#include "class/hid/hid_device.h"

#define KEYBOARD_MODIFIER_NONE 0

#define HID_IT_IT_ASCII_TO_KEYCODE \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x00 Null      */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x01           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x02           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x03           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x04           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x05           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x06           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x07           */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_BACKSPACE          }, /* 0x08 Backspace */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_TAB                }, /* 0x09 Tab       */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_ENTER              }, /* 0x0A Line Feed */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x0B           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x0C           */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_ENTER              }, /* 0x0D CR        */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x0E           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x0F           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x10           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x11           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x12           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x13           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x14           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x15           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x16           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x17           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x18           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x19           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x1A           */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_ESCAPE             }, /* 0x1B Escape    */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x1C           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x1D           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x1E           */ \
  {KEYBOARD_MODIFIER_NONE, 0                          }, /* 0x1F           */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_SPACE              }, /* 0x20           */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_1             }, /* 0x21 !         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_2             }, /* 0x22 "         */ \
  {KEYBOARD_MODIFIER_RIGHTALT, HID_KEY_APOSTROPHE     }, /* 0x23 #         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_4             }, /* 0x24 $         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_5             }, /* 0x25 %         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_6             }, /* 0x26 &         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_MINUS              }, /* 0x27 '         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_8             }, /* 0x28 (         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_9             }, /* 0x29 )         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_BRACKET_RIGHT }, /* 0x2A *         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_BRACKET_RIGHT      }, /* 0x2B +         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_COMMA              }, /* 0x2C ,         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_SLASH              }, /* 0x2D -         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_PERIOD             }, /* 0x2E .         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_7             }, /* 0x2F /         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_0                  }, /* 0x30 0         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_1                  }, /* 0x31 1         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_2                  }, /* 0x32 2         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_3                  }, /* 0x33 3         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_4                  }, /* 0x34 4         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_5                  }, /* 0x35 5         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_6                  }, /* 0x36 6         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_7                  }, /* 0x37 7         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_8                  }, /* 0x38 8         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_9                  }, /* 0x39 9         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_PERIOD        }, /* 0x3A :         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_COMMA         }, /* 0x3B ;         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_EUROPE_2           }, /* 0x3C <         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_0             }, /* 0x3D =         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_EUROPE_2      }, /* 0x3E >         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_MINUS         }, /* 0x3F ?         */ \
  {KEYBOARD_MODIFIER_RIGHTALT, HID_KEY_SEMICOLON      }, /* 0x40 @         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_A             }, /* 0x41 A         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_B             }, /* 0x42 B         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_C             }, /* 0x43 C         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_D             }, /* 0x44 D         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_E             }, /* 0x45 E         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_F             }, /* 0x46 F         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_G             }, /* 0x47 G         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_H             }, /* 0x48 H         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_I             }, /* 0x49 I         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_J             }, /* 0x4A J         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_K             }, /* 0x4B K         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_L             }, /* 0x4C L         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_M             }, /* 0x4D M         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_N             }, /* 0x4E N         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_O             }, /* 0x4F O         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_P             }, /* 0x50 P         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_Q             }, /* 0x51 Q         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_R             }, /* 0x52 R         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_S             }, /* 0x53 S         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_T             }, /* 0x54 T         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_U             }, /* 0x55 U         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_V             }, /* 0x56 V         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_W             }, /* 0x57 W         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_X             }, /* 0x58 X         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_Y             }, /* 0x59 Y         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_Z             }, /* 0x5A Z         */ \
  {KEYBOARD_MODIFIER_RIGHTALT, HID_KEY_BRACKET_LEFT   }, /* 0x5B [         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_GRAVE              }, /* 0x5C '\'       */ \
  {KEYBOARD_MODIFIER_RIGHTALT, HID_KEY_BRACKET_RIGHT  }, /* 0x5D ]         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_EQUAL         }, /* 0x5E ^         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_SLASH         }, /* 0x5F _         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_GRAVE              }, /* 0x60 `         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_A                  }, /* 0x61 a         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_B                  }, /* 0x62 b         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_C                  }, /* 0x63 c         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_D                  }, /* 0x64 d         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_E                  }, /* 0x65 e         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_F                  }, /* 0x66 f         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_G                  }, /* 0x67 g         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_H                  }, /* 0x68 h         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_I                  }, /* 0x69 i         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_J                  }, /* 0x6A j         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_K                  }, /* 0x6B k         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_L                  }, /* 0x6C l         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_M                  }, /* 0x6D m         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_N                  }, /* 0x6E n         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_O                  }, /* 0x6F o         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_P                  }, /* 0x70 p         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_Q                  }, /* 0x71 q         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_R                  }, /* 0x72 r         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_S                  }, /* 0x73 s         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_T                  }, /* 0x74 t         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_U                  }, /* 0x75 u         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_V                  }, /* 0x76 v         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_W                  }, /* 0x77 w         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_X                  }, /* 0x78 x         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_Y                  }, /* 0x79 y         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_Z                  }, /* 0x7A z         */ \
  {KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTSHIFT, \
                                HID_KEY_BRACKET_LEFT  }, /* 0x7B {         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_GRAVE         }, /* 0x7C |         */ \
  {KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTSHIFT, \
                                HID_KEY_BRACKET_RIGHT }, /* 0x7D }         */ \
  {KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_GRAVE         }, /* 0x7E ~         */ \
  {KEYBOARD_MODIFIER_NONE, HID_KEY_DELETE             }  /* 0x7F Delete    */

  #endif /* HID_KEYCODES_H */


