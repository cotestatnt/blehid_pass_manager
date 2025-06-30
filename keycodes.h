extern BLEHidAdafruit blehid;

// Ridefinizione parziale per layout ITALIANO
// #define HID_ASCII_TO_KEYCODE_IT \
//     {0, 0                     }, /* 0x00 Null      */ \
//     {0, 0                     }, /* 0x01           */ \
//     {0, 0                     }, /* 0x02           */ \
//     {0, 0                     }, /* 0x03           */ \
//     {0, 0                     }, /* 0x04           */ \
//     {0, 0                     }, /* 0x05           */ \
//     {0, 0                     }, /* 0x06           */ \
//     {0, 0                     }, /* 0x07           */ \
//     {0, HID_KEY_BACKSPACE     }, /* 0x08 Backspace */ \
//     {0, HID_KEY_TAB           }, /* 0x09 Tab       */ \
//     {0, HID_KEY_ENTER         }, /* 0x0A Line Feed */ \
//     {0, 0                     }, /* 0x0B           */ \
//     {0, 0                     }, /* 0x0C           */ \
//     {0, HID_KEY_ENTER         }, /* 0x0D CR        */ \
//     {0, 0                     }, /* 0x0E           */ \
//     {0, 0                     }, /* 0x0F           */ \
//     {0, 0                     }, /* 0x10           */ \
//     {0, 0                     }, /* 0x11           */ \
//     {0, 0                     }, /* 0x12           */ \
//     {0, 0                     }, /* 0x13           */ \
//     {0, 0                     }, /* 0x14           */ \
//     {0, 0                     }, /* 0x15           */ \
//     {0, 0                     }, /* 0x16           */ \
//     {0, 0                     }, /* 0x17           */ \
//     {0, 0                     }, /* 0x18           */ \
//     {0, 0                     }, /* 0x19           */ \
//     {0, 0                     }, /* 0x1A           */ \
//     {0, HID_KEY_ESCAPE        }, /* 0x1B Escape    */ \
//     {0, 0                     }, /* 0x1C           */ \
//     {0, 0                     }, /* 0x1D           */ \
//     {0, 0                     }, /* 0x1E           */ \
//     {0, 0                     }, /* 0x1F           */ \
//                                                       \
//     {0, HID_KEY_SPACE         }, /* 0x20           */ \
//     {1, HID_KEY_2             }, /* 0x21 !         */ \
//     {1, HID_KEY_APOSTROPHE    }, /* 0x22 "         */ \
//     {2, HID_KEY_KEYPAD_LEFT_BRACE    }, /* 0x23 #  [ALTGR + è]*/ \
//     {1, HID_KEY_4             }, /* 0x24 $         */ \
//     {1, HID_KEY_5             }, /* 0x25 %         */ \
//     {1, HID_KEY_6             }, /* 0x26 &         */ \
//     {0, HID_KEY_APOSTROPHE    }, /* 0x27 '         */ \
//     {1, HID_KEY_8             }, /* 0x28 (         */ \
//     {1, HID_KEY_9             }, /* 0x29 )         */ \
//     {1, HID_KEY_7             }, /* 0x2A *         */ \
//     {1, HID_KEY_EQUAL         }, /* 0x2B +         */ \
//     {0, HID_KEY_COMMA         }, /* 0x2C ,         */ \
//     {0, HID_KEY_MINUS         }, /* 0x2D -         */ \
//     {0, HID_KEY_PERIOD        }, /* 0x2E .         */ \
//     {1, HID_KEY_COMMA         }, /* 0x2F / (SHIFT + ,) */ \
//     {0, HID_KEY_0             }, /* 0x30 0         */ \
//     {0, HID_KEY_1             }, /* 0x31 1         */ \
//     {0, HID_KEY_2             }, /* 0x32 2         */ \
//     {0, HID_KEY_3             }, /* 0x33 3         */ \
//     {0, HID_KEY_4             }, /* 0x34 4         */ \
//     {0, HID_KEY_5             }, /* 0x35 5         */ \
//     {0, HID_KEY_6             }, /* 0x36 6         */ \
//     {0, HID_KEY_7             }, /* 0x37 7         */ \
//     {0, HID_KEY_8             }, /* 0x38 8         */ \
//     {0, HID_KEY_9             }, /* 0x39 9         */ \
//     {1, HID_KEY_PERIOD        }, /* 0x3A : (SHIFT + .) */ \
//     {0, HID_KEY_SEMICOLON     }, /* 0x3B ;         */ \
//     {1, HID_KEY_PERIOD        }, /* 0x3C < (SHIFT + ,)*/ \
//     {1, HID_KEY_0             }, /* 0x3D = (SHIFT + 0)*/ \
//     {1, HID_KEY_SEMICOLON     }, /* 0x3E > (SHIFT + ;)*/ \
//     {1, HID_KEY_COMMA         }, /* 0x3F ? (SHIFT + -)*/ \                                                      
//     {2, HID_KEY_SEMICOLON     }, /* 0x40 @ [ALTGR + ò]*/ \
//     {1, HID_KEY_A             }, /* 0x41 A         */ \
//     {1, HID_KEY_B             }, /* 0x42 B         */ \
//     {1, HID_KEY_C             }, /* 0x43 C         */ \
//     {1, HID_KEY_D             }, /* 0x44 D         */ \
//     {1, HID_KEY_E             }, /* 0x45 E         */ \
//     {1, HID_KEY_F             }, /* 0x46 F         */ \
//     {1, HID_KEY_G             }, /* 0x47 G         */ \
//     {1, HID_KEY_H             }, /* 0x48 H         */ \
//     {1, HID_KEY_I             }, /* 0x49 I         */ \
//     {1, HID_KEY_J             }, /* 0x4A J         */ \
//     {1, HID_KEY_K             }, /* 0x4B K         */ \
//     {1, HID_KEY_L             }, /* 0x4C L         */ \
//     {1, HID_KEY_M             }, /* 0x4D M         */ \
//     {1, HID_KEY_N             }, /* 0x4E N         */ \
//     {1, HID_KEY_O             }, /* 0x4F O         */ \
//     {1, HID_KEY_P             }, /* 0x50 P         */ \
//     {1, HID_KEY_Q             }, /* 0x51 Q         */ \
//     {1, HID_KEY_R             }, /* 0x52 R         */ \
//     {1, HID_KEY_S             }, /* 0x53 S         */ \
//     {1, HID_KEY_T             }, /* 0x54 T         */ \
//     {1, HID_KEY_U             }, /* 0x55 U         */ \
//     {1, HID_KEY_V             }, /* 0x56 V         */ \
//     {1, HID_KEY_W             }, /* 0x57 W         */ \
//     {1, HID_KEY_X             }, /* 0x58 X         */ \
//     {1, HID_KEY_Y             }, /* 0x59 Y         */ \
//     {1, HID_KEY_Z             }, /* 0x5A Z         */ \
//     {2, HID_KEY_BRACKET_LEFT  }, /* 0x5B [ [ALTGR + è]*/ \
//     {2, HID_KEY_BRACKET_RIGHT }, /* 0x5C \ [ALTGR + +]*/ \
//     {2, HID_KEY_BRACKET_RIGHT }, /* 0x5D ] [ALTGR + +]*/ \
//     {1, HID_KEY_9             }, /* 0x5E ^ (SHIFT + 9)*/ \
//     {1, HID_KEY_MINUS         }, /* 0x5F _ (SHIFT + -)*/ \
//     {0, HID_KEY_GRAVE         }, /* 0x60 `         */ \
//     {0, HID_KEY_A             }, /* 0x61 a         */ \
//     {0, HID_KEY_B             }, /* 0x62 b         */ \
//     {0, HID_KEY_C             }, /* 0x63 c         */ \
//     {0, HID_KEY_D             }, /* 0x64 d         */ \
//     {0, HID_KEY_E             }, /* 0x65 e         */ \
//     {0, HID_KEY_F             }, /* 0x66 f         */ \
//     {0, HID_KEY_G             }, /* 0x67 g         */ \
//     {0, HID_KEY_H             }, /* 0x68 h         */ \
//     {0, HID_KEY_I             }, /* 0x69 i         */ \
//     {0, HID_KEY_J             }, /* 0x6A j         */ \
//     {0, HID_KEY_K             }, /* 0x6B k         */ \
//     {0, HID_KEY_L             }, /* 0x6C l         */ \
//     {0, HID_KEY_M             }, /* 0x6D m         */ \
//     {0, HID_KEY_N             }, /* 0x6E n         */ \
//     {0, HID_KEY_O             }, /* 0x6F o         */ \
//     {0, HID_KEY_P             }, /* 0x70 p         */ \
//     {0, HID_KEY_Q             }, /* 0x71 q         */ \
//     {0, HID_KEY_R             }, /* 0x72 r         */ \
//     {0, HID_KEY_S             }, /* 0x73 s         */ \
//     {0, HID_KEY_T             }, /* 0x74 t         */ \
//     {0, HID_KEY_U             }, /* 0x75 u         */ \
//     {0, HID_KEY_V             }, /* 0x76 v         */ \
//     {0, HID_KEY_W             }, /* 0x77 w         */ \
//     {0, HID_KEY_X             }, /* 0x78 x         */ \
//     {0, HID_KEY_Y             }, /* 0x79 y         */ \
//     {0, HID_KEY_Z             }, /* 0x7A z         */ \
//     {2, HID_KEY_BRACKET_LEFT  }, /* 0x7B { [ALTGR + è]*/ \
//     {2, HID_KEY_BACKSLASH     }, /* 0x7C | [ALTGR + <]*/ \
//     {2, HID_KEY_BRACKET_RIGHT }, /* 0x7D } [ALTGR + +]*/ \
//     {1, HID_KEY_GRAVE         }, /* 0x7E ~ (SHIFT + `)*/ \
//     {0, HID_KEY_DELETE        }  /* 0x7F Delete    */

// ChatGPT
#define HID_ASCII_TO_KEYCODE_IT \
    {0, 0},                   /* 0x00 */ \
    {0, 0},                   /* 0x01 */ \
    {0, 0},                   /* 0x02 */ \
    {0, 0},                   /* 0x03 */ \
    {0, 0},                   /* 0x04 */ \
    {0, 0},                   /* 0x05 */ \
    {0, 0},                   /* 0x06 */ \
    {0, 0},                   /* 0x07 */ \
    {0, HID_KEY_BACKSPACE},   /* 0x08 Backspace */ \
    {0, HID_KEY_TAB},         /* 0x09 Tab */ \
    {0, HID_KEY_ENTER},       /* 0x0A Line Feed */ \
    {0, 0},                   /* 0x0B */ \
    {0, 0},                   /* 0x0C */ \
    {0, HID_KEY_ENTER},       /* 0x0D Carriage Return */ \
    {0, 0},                   /* 0x0E */ \
    {0, 0},                   /* 0x0F */ \
    {0, 0},                   /* 0x10 */ \
    {0, 0},                   /* 0x11 */ \
    {0, 0},                   /* 0x12 */ \
    {0, 0},                   /* 0x13 */ \
    {0, 0},                   /* 0x14 */ \
    {0, 0},                   /* 0x15 */ \
    {0, 0},                   /* 0x16 */ \
    {0, 0},                   /* 0x17 */ \
    {0, 0},                   /* 0x18 */ \
    {0, 0},                   /* 0x19 */ \
    {0, 0},                   /* 0x1A */ \
    {0, HID_KEY_ESCAPE},      /* 0x1B Escape */ \
    {0, 0},                   /* 0x1C */ \
    {0, 0},                   /* 0x1D */ \
    {0, 0},                   /* 0x1E */ \
    {0, 0},                   /* 0x1F */ \
    {0, HID_KEY_SPACE},       /* 0x20 SPACE */ \
    {1, HID_KEY_2},           /* 0x21 ! */ \
    {1, HID_KEY_APOSTROPHE},  /* 0x22 " */ \
    {2, HID_KEY_BRACKET_LEFT}, /* 0x23 # */ \
    {1, HID_KEY_4},           /* 0x24 $ */ \
    {1, HID_KEY_5},           /* 0x25 % */ \
    {1, HID_KEY_6},           /* 0x26 & */ \
    {0, HID_KEY_APOSTROPHE},  /* 0x27 ' */ \
    {1, HID_KEY_8},           /* 0x28 ( */ \
    {1, HID_KEY_9},           /* 0x29 ) */ \
    {1, HID_KEY_7},           /* 0x2A * */ \
    {1, HID_KEY_EQUAL},       /* 0x2B + */ \
    {0, HID_KEY_COMMA},       /* 0x2C , */ \
    {0, HID_KEY_MINUS},       /* 0x2D - */ \
    {0, HID_KEY_PERIOD},      /* 0x2E . */ \
    {1, HID_KEY_COMMA},       /* 0x2F / */ \
    {0, HID_KEY_0},           /* 0x30 0 */ \
    {0, HID_KEY_1},           /* 0x31 1 */ \
    {0, HID_KEY_2},           /* 0x32 2 */ \
    {0, HID_KEY_3},           /* 0x33 3 */ \
    {0, HID_KEY_4},           /* 0x34 4 */ \
    {0, HID_KEY_5},           /* 0x35 5 */ \
    {0, HID_KEY_6},           /* 0x36 6 */ \
    {0, HID_KEY_7},           /* 0x37 7 */ \
    {0, HID_KEY_8},           /* 0x38 8 */ \
    {0, HID_KEY_9},           /* 0x39 9 */ \
    {1, HID_KEY_PERIOD},      /* 0x3A : */ \
    {0, HID_KEY_SEMICOLON},   /* 0x3B ; */ \
    {1, HID_KEY_COMMA},       /* 0x3C < */ \
    {0, HID_KEY_EQUAL},       /* 0x3D = */ \
    {1, HID_KEY_SEMICOLON},   /* 0x3E > */ \
    {1, HID_KEY_MINUS},       /* 0x3F ? */ \
    {2, HID_KEY_SEMICOLON},   /* 0x40 @ */ \
    {1, HID_KEY_A},           /* 0x41 A */ \
    {1, HID_KEY_B},           /* 0x42 B */ \
    {1, HID_KEY_C},           /* 0x43 C */ \
    {1, HID_KEY_D},           /* 0x44 D */ \
    {1, HID_KEY_E},           /* 0x45 E */ \
    {1, HID_KEY_F},           /* 0x46 F */ \
    {1, HID_KEY_G},           /* 0x47 G */ \
    {1, HID_KEY_H},           /* 0x48 H */ \
    {1, HID_KEY_I},           /* 0x49 I */ \
    {1, HID_KEY_J},           /* 0x4A J */ \
    {1, HID_KEY_K},           /* 0x4B K */ \
    {1, HID_KEY_L},           /* 0x4C L */ \
    {1, HID_KEY_M},           /* 0x4D M */ \
    {1, HID_KEY_N},           /* 0x4E N */ \
    {1, HID_KEY_O},           /* 0x4F O */ \
    {1, HID_KEY_P},           /* 0x50 P */ \
    {1, HID_KEY_Q},           /* 0x51 Q */ \
    {1, HID_KEY_R},           /* 0x52 R */ \
    {1, HID_KEY_S},           /* 0x53 S */ \
    {1, HID_KEY_T},           /* 0x54 T */ \
    {1, HID_KEY_U},           /* 0x55 U */ \
    {1, HID_KEY_V},           /* 0x56 V */ \
    {1, HID_KEY_W},           /* 0x57 W */ \
    {1, HID_KEY_X},           /* 0x58 X */ \
    {1, HID_KEY_Y},           /* 0x59 Y */ \
    {1, HID_KEY_Z},           /* 0x5A Z */ \
    {2, HID_KEY_BRACKET_LEFT}, /* 0x5B [ */ \
    {2, HID_KEY_BRACKET_RIGHT},/* 0x5C \ */ \
    {2, HID_KEY_BRACKET_RIGHT},/* 0x5D ] */ \
    {1, HID_KEY_9},           /* 0x5E ^ */ \
    {1, HID_KEY_MINUS},       /* 0x5F _ */ \
    {0, HID_KEY_GRAVE},       /* 0x60 ` */ \
    {0, HID_KEY_A},           /* 0x61 a */ \
    {0, HID_KEY_B},           /* 0x62 b */ \
    {0, HID_KEY_C},           /* 0x63 c */ \
    {0, HID_KEY_D},           /* 0x64 d */ \
    {0, HID_KEY_E},           /* 0x65 e */ \
    {0, HID_KEY_F},           /* 0x66 f */ \
    {0, HID_KEY_G},           /* 0x67 g */ \
    {0, HID_KEY_H},           /* 0x68 h */ \
    {0, HID_KEY_I},           /* 0x69 i */ \
    {0, HID_KEY_J},           /* 0x6A j */ \
    {0, HID_KEY_K},           /* 0x6B k */ \
    {0, HID_KEY_L},           /* 0x6C l */ \
    {0, HID_KEY_M},           /* 0x6D m */ \
    {0, HID_KEY_N},           /* 0x6E n */ \
    {0, HID_KEY_O},           /* 0x6F o */ \
    {0, HID_KEY_P},           /* 0x70 p */ \
    {0, HID_KEY_Q},           /* 0x71 q */ \
    {0, HID_KEY_R},           /* 0x72 r */ \
    {0, HID_KEY_S},           /* 0x73 s */ \
    {0, HID_KEY_T},           /* 0x74 t */ \
    {0, HID_KEY_U},           /* 0x75 u */ \
    {0, HID_KEY_V},           /* 0x76 v */ \
    {0, HID_KEY_W},           /* 0x77 w */ \
    {0, HID_KEY_X},           /* 0x78 x */ \
    {0, HID_KEY_Y},           /* 0x79 y */ \
    {0, HID_KEY_Z},           /* 0x7A z */ \
    {2, HID_KEY_BRACKET_LEFT}, /* 0x7B { */ \
    {2, HID_KEY_BRACKET_RIGHT},   /* 0x7C | */ \
    {2, HID_KEY_BRACKET_LEFT},/* 0x7D } */ \
    {1, HID_KEY_GRAVE},       /* 0x7E ~ */ \
    {0, HID_KEY_DELETE}       /* 0x7F Delete */


// // Ridefinizione corretta e verificata per layout ITALIANO (Gemini)
// #define HID_ASCII_TO_KEYCODE_IT \
//     {0, 0                            }, /* 0x00 Null      */ \
//     {0, 0                            }, /* 0x01           */ \
//     {0, 0                            }, /* 0x02           */ \
//     {0, 0                            }, /* 0x03           */ \
//     {0, 0                            }, /* 0x04           */ \
//     {0, 0                            }, /* 0x05           */ \
//     {0, 0                            }, /* 0x06           */ \
//     {0, 0                            }, /* 0x07           */ \
//     {0, HID_KEY_BACKSPACE            }, /* 0x08 Backspace */ \
//     {0, HID_KEY_TAB                  }, /* 0x09 Tab       */ \
//     {0, HID_KEY_ENTER                }, /* 0x0A Line Feed */ \
//     {0, 0                            }, /* 0x0B           */ \
//     {0, 0                            }, /* 0x0C           */ \
//     {0, HID_KEY_ENTER                }, /* 0x0D CR        */ \
//     {0, 0                            }, /* 0x0E           */ \
//     {0, 0                            }, /* 0x0F           */ \
//     {0, 0                            }, /* 0x10           */ \
//     {0, 0                            }, /* 0x11           */ \
//     {0, 0                            }, /* 0x12           */ \
//     {0, 0                            }, /* 0x13           */ \
//     {0, 0                            }, /* 0x14           */ \
//     {0, 0                            }, /* 0x15           */ \
//     {0, 0                            }, /* 0x16           */ \
//     {0, 0                            }, /* 0x17           */ \
//     {0, 0                            }, /* 0x18           */ \
//     {0, 0                            }, /* 0x19           */ \
//     {0, 0                            }, /* 0x1A           */ \
//     {0, HID_KEY_ESCAPE               }, /* 0x1B Escape    */ \
//     {0, 0                            }, /* 0x1C           */ \
//     {0, 0                            }, /* 0x1D           */ \
//     {0, 0                            }, /* 0x1E           */ \
//     {0, 0                            }, /* 0x1F           */ \
//     {0, HID_KEY_SPACE                }, /* 0x20           */ \
//     {1, HID_KEY_1                    }, /* 0x21 !  (SHIFT + 1) */ \
//     {1, HID_KEY_2                    }, /* 0x22 "  (SHIFT + 2) */ \
//     {2, HID_KEY_KEYPAD_RIGHT_BRACE   }, /* 0x23 #  (ALTGR + à) */ \
//     {1, HID_KEY_4                    }, /* 0x24 $  (SHIFT + 4) */ \
//     {1, HID_KEY_5                    }, /* 0x25 %  (SHIFT + 5) */ \
//     {1, HID_KEY_6                    }, /* 0x26 &  (SHIFT + 6) */ \
//     {0, HID_KEY_SLASH                }, /* 0x27 '  (Tasto ')  */ \
//     {1, HID_KEY_8                    }, /* 0x28 (  (SHIFT + 8) */ \
//     {1, HID_KEY_9                    }, /* 0x29 )  (SHIFT + 9) */ \
//     {1, HID_KEY_EQUAL                }, /* 0x2A * (SHIFT + +) */ \
//     {0, HID_KEY_EQUAL                }, /* 0x2B +  (Tasto +)  */ \
//     {0, HID_KEY_COMMA                }, /* 0x2C ,  (Tasto ,)  */ \
//     {0, HID_KEY_MINUS                }, /* 0x2D -  (Tasto -)  */ \
//     {0, HID_KEY_PERIOD               }, /* 0x2E .  (Tasto .)  */ \
//     {1, HID_KEY_7                    }, /* 0x2F /  (SHIFT + 7) */ \
//     {0, HID_KEY_0                    }, /* 0x30 0         */ \
//     {0, HID_KEY_1                    }, /* 0x31 1         */ \
//     {0, HID_KEY_2                    }, /* 0x32 2         */ \
//     {0, HID_KEY_3                    }, /* 0x33 3         */ \
//     {0, HID_KEY_4                    }, /* 0x34 4         */ \
//     {0, HID_KEY_5                    }, /* 0x35 5         */ \
//     {0, HID_KEY_6                    }, /* 0x36 6         */ \
//     {0, HID_KEY_7                    }, /* 0x37 7         */ \
//     {0, HID_KEY_8                    }, /* 0x38 8         */ \
//     {0, HID_KEY_9                    }, /* 0x39 9         */ \
//     {1, HID_KEY_PERIOD               }, /* 0x3A :  (SHIFT + .) */ \
//     {1, HID_KEY_COMMA                }, /* 0x3B ;  (SHIFT + ,) */ \
//     {0, HID_KEY_BACKSLASH            }, /* 0x3C <  (Tasto <)  */ \
//     {1, HID_KEY_0                    }, /* 0x3D =  (SHIFT + 0) */ \
//     {1, HID_KEY_BACKSLASH            }, /* 0x3E >  (SHIFT + <) */ \
//     {1, HID_KEY_COMMA                }, /* 0x3F ?  (SHIFT + ') */ \
//     {2, HID_KEY_SEMICOLON            }, /* 0x40 @  (ALTGR + ò) */ \
//     {1, HID_KEY_A                    }, /* 0x41 A         */ \
//     {1, HID_KEY_B                    }, /* 0x42 B         */ \
//     {1, HID_KEY_C                    }, /* 0x43 C         */ \
//     {1, HID_KEY_D                    }, /* 0x44 D         */ \
//     {1, HID_KEY_E                    }, /* 0x45 E         */ \
//     {1, HID_KEY_F                    }, /* 0x46 F         */ \
//     {1, HID_KEY_G                    }, /* 0x47 G         */ \
//     {1, HID_KEY_H                    }, /* 0x48 H         */ \
//     {1, HID_KEY_I                    }, /* 0x49 I         */ \
//     {1, HID_KEY_J                    }, /* 0x4A J         */ \
//     {1, HID_KEY_K                    }, /* 0x4B K         */ \
//     {1, HID_KEY_L                    }, /* 0x4C L         */ \
//     {1, HID_KEY_M                    }, /* 0x4D M         */ \
//     {1, HID_KEY_N                    }, /* 0x4E N         */ \
//     {1, HID_KEY_O                    }, /* 0x4F O         */ \
//     {1, HID_KEY_P                    }, /* 0x50 P         */ \
//     {1, HID_KEY_Q                    }, /* 0x51 Q         */ \
//     {1, HID_KEY_R                    }, /* 0x52 R         */ \
//     {1, HID_KEY_S                    }, /* 0x53 S         */ \
//     {1, HID_KEY_T                    }, /* 0x54 T         */ \
//     {1, HID_KEY_U                    }, /* 0x55 U         */ \
//     {1, HID_KEY_V                    }, /* 0x56 V         */ \
//     {1, HID_KEY_W                    }, /* 0x57 W         */ \
//     {1, HID_KEY_X                    }, /* 0x58 X         */ \
//     {1, HID_KEY_Y                    }, /* 0x59 Y         */ \
//     {1, HID_KEY_Z                    }, /* 0x5A Z         */ \
//     {2, HID_KEY_BRACKET_LEFT         }, /* 0x5B [  (ALTGR + è) */ \
//     {0, HID_KEY_APOSTROPHE           }, /* 0x5C \  (Tasto \)  */ \
//     {2, HID_KEY_EQUAL                }, /* 0x5D ]  (ALTGR + +) */ \
//     {1, HID_KEY_GRAVE                }, /* 0x5E ^  (SHIFT + ì) */ \
//     {1, HID_KEY_MINUS                }, /* 0x5F _  (SHIFT + -) */ \
//     {2, HID_KEY_GRAVE                }, /* 0x60 `  (ALTGR + ì) */ \
//     {0, HID_KEY_A                    }, /* 0x61 a         */ \
//     {0, HID_KEY_B                    }, /* 0x62 b         */ \
//     {0, HID_KEY_C                    }, /* 0x63 c         */ \
//     {0, HID_KEY_D                    }, /* 0x64 d         */ \
//     {0, HID_KEY_E                    }, /* 0x65 e         */ \
//     {0, HID_KEY_F                    }, /* 0x66 f         */ \
//     {0, HID_KEY_G                    }, /* 0x67 g         */ \
//     {0, HID_KEY_H                    }, /* 0x68 h         */ \
//     {0, HID_KEY_I                    }, /* 0x69 i         */ \
//     {0, HID_KEY_J                    }, /* 0x6A j         */ \
//     {0, HID_KEY_K                    }, /* 0x6B k         */ \
//     {0, HID_KEY_L                    }, /* 0x6C l         */ \
//     {0, HID_KEY_M                    }, /* 0x6D m         */ \
//     {0, HID_KEY_N                    }, /* 0x6E n         */ \
//     {0, HID_KEY_O                    }, /* 0x6F o         */ \
//     {0, HID_KEY_P                    }, /* 0x70 p         */ \
//     {0, HID_KEY_Q                    }, /* 0x71 q         */ \
//     {0, HID_KEY_R                    }, /* 0x72 r         */ \
//     {0, HID_KEY_S                    }, /* 0x73 s         */ \
//     {0, HID_KEY_T                    }, /* 0x74 t         */ \
//     {0, HID_KEY_U                    }, /* 0x75 u         */ \
//     {0, HID_KEY_V                    }, /* 0x76 v         */ \
//     {0, HID_KEY_W                    }, /* 0x77 w         */ \
//     {0, HID_KEY_X                    }, /* 0x78 x         */ \
//     {0, HID_KEY_Y                    }, /* 0x79 y         */ \
//     {0, HID_KEY_Z                    }, /* 0x7A z         */ \
//     {1, HID_KEY_BRACKET_LEFT         }, /* 0x7B {  (SHIFT + [) */ \
//     {1, HID_KEY_APOSTROPHE           }, /* 0x7C |  (SHIFT + \) */ \
//     {1, HID_KEY_EQUAL                }, /* 0x7D }  (SHIFT + ]) */ \
//     {2, HID_KEY_APOSTROPHE           }, /* 0x7E ~  (ALTGR + ù) */ \
//     {0, HID_KEY_DELETE               }  /* 0x7F Delete    */

const uint8_t hid_ascii_to_keycode_it[128][2] = { HID_ASCII_TO_KEYCODE_IT };


bool keyPress(uint32_t ch) {
  hid_keyboard_report_t report;
  varclr(&report);

  Serial.print(" ");
  Serial.print(ch, HEX);

  switch (ch) {
    // Lettere accentate UTF-8 (2 byte)
    case 0xC3A0: report.keycode[0] = 0x34; break; // à
    case 0xC3A8: report.keycode[0] = 0x2F; break; // è
    case 0xC3A9: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x2F; break; // é
    case 0xC3AC: report.keycode[0] = 0x2e; break; // ì
    case 0xC3B2: report.keycode[0] = 0x33; break; // ò
    case 0xC3B9: report.keycode[0] = 0x31; break; // ù
    // Grado UTF-8 (2 byte)
    // case 0xC2B0: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x31; break; // °
    // case 0xC2A3: report.modifier = KEYBOARD_MODIFIER_RIGHTALT;  report.keycode[0] = 0x20; break; // £
    // // Euro UTF-8 (3 byte)
    // case 0xE282AC: report.modifier = KEYBOARD_MODIFIER_RIGHTALT; report.keycode[0] = 0x08; break; // €
    // // Simboli password e speciali (ASCII, un solo byte)
    // case 0x21: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x1E; break; // !
    // case 0x22: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x1F; break; // "
    // case 0x23: report.modifier = KEYBOARD_MODIFIER_RIGHTALT;  report.keycode[0] = 0x34; break; // #
    // case 0x24: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x21; break; // $
    // case 0x25: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x22; break; // %
    // case 0x26: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x23; break; // &
    // case 0x27: report.keycode[0] = 0x32; break; // '
    // case 0x28: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x25; break; // (
    // case 0x29: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x26; break; // )
    // case 0x2A: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x27; break; // *
    // case 0x2B: report.keycode[0] = 0x30; break; // +
    // case 0x2C: report.keycode[0] = 0x36; break; // ,
    // case 0x2D: report.keycode[0] = 0x38; break; // -
    // case 0x2E: report.keycode[0] = 0x37; break; // .
    // case 0x2F: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x24; break; // /
    // case 0x3A: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x37; break; // :
    // case 0x3B: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x36; break; // ;
    // case 0x3C: report.keycode[0] = 0x64; break; // <
    // case 0x3D: report.keycode[0] = 0x2E; break; // =
    // case 0x3E: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x64; break; // >
    // // case 0x3F: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x36; break; // ?_
    // // case 0x40: report.modifier = KEYBOARD_MODIFIER_RIGHTALT;  report.keycode[0] = 0x40; break; // @
    // case 0x5B: report.modifier = KEYBOARD_MODIFIER_RIGHTALT;  report.keycode[0] = 0x2F; break; // [
    // case 0x5C: report.modifier = KEYBOARD_MODIFIER_RIGHTALT;  report.keycode[0] = 0x31; break; // "\"
    // case 0x5D: report.modifier = KEYBOARD_MODIFIER_RIGHTALT;  report.keycode[0] = 0x30; break; // ]
    // case 0x5E: report.modifier = KEYBOARD_MODIFIER_RIGHTALT;  report.keycode[0] = 0x30; break; // ^
    // case 0x5F: report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x38; break; // _
    // case 0x60: report.modifier = KEYBOARD_MODIFIER_RIGHTALT;  report.keycode[0] = 0x31; break; // `
    // case 0x7B: report.modifier = KEYBOARD_MODIFIER_RIGHTALT | KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x2F; break; // {
    // case 0x7C: report.modifier = KEYBOARD_MODIFIER_RIGHTALT;  report.keycode[0] = 0x64; break; // |
    // case 0x7D: report.modifier = KEYBOARD_MODIFIER_RIGHTALT | KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x30; break; // }
    // case 0x7E: report.modifier = KEYBOARD_MODIFIER_RIGHTALT | KEYBOARD_MODIFIER_LEFTSHIFT; report.keycode[0] = 0x30; break; // ~
    default:
      report.modifier = ( hid_ascii_to_keycode_it[(uint8_t)ch][0] ) ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
      report.keycode[0] = hid_ascii_to_keycode_it[(uint8_t)ch][1];
      break;
  }
  return blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, &report);
}



// bool keyPress(char ch) {
//   hid_keyboard_report_t report;
//   varclr(&report);

//   Serial.print(" ");
//   Serial.print(ch, HEX);

//   // Gestione lettere accentate italiane
//   switch ((uint8_t)ch) {
//     case 0xE0: // à
//       report.keycode[0] = 0x34; // tasto 'à' su IT
//       break;
//     case 0xE8: // è
//       report.keycode[0] = 0x2F; // tasto 'è' su IT
//       break;
//     case 0xE9: // é
//       report.modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
//       report.keycode[0] = 0x2F; // SHIFT + 'è' = 'é'
//       break;
//     case 0xEC: // ì
//       report.keycode[0] = 0x2E; // tasto 'ì' su IT
//       break;
//     case 0xF2: // ò
//       report.keycode[0] = 0x33; // tasto 'ò' su IT
//       break;
//     case 0xF9: // ù
//       report.keycode[0] = 0x31; // tasto 'ù' su IT
//       break;
      
//     default:
//       report.modifier = ( hid_ascii_to_keycode_it[(uint8_t)ch][0] ) ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
//       report.keycode[0] = hid_ascii_to_keycode_it[(uint8_t)ch][1];
//       break;
//   }
//   return blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, &report);
// }

void sendString(const String& str) {
  for (size_t i = 0; i < str.length(); ) {
    uint8_t c = (uint8_t)str[i];
    uint32_t codepoint = 0;

    if (c < 0x80) {
      // ASCII
      codepoint = c;
      i += 1;
    } else if ((c & 0xE0) == 0xC0 && i + 1 < str.length()) {
      // 2-byte UTF-8
      codepoint = (c << 8) | (uint8_t)str[i + 1];
      i += 2;
    } else if ((c & 0xF0) == 0xE0 && i + 2 < str.length()) {
      // 3-byte UTF-8
      codepoint = (c << 16) | ((uint8_t)str[i + 1] << 8) | (uint8_t)str[i + 2];
      i += 3;
    } else {
      // Carattere non gestito, salto      
      Serial.print(" NU ");
      Serial.print(c, HEX);
      i += 1;
      continue;
    }

    keyPress(codepoint);
    delay(5);
    blehid.keyRelease();
    delay(2);
  }
}

