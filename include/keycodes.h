#ifndef KEYCODES_H
#define KEYCODES_H

#include <stdint.h>

// Logical keycodes aligned with USB HID where applicable.
// 0x00 reserved as NO_EVENT.
#define KC_NONE        0x00
#define KC_A           0x04
#define KC_B           0x05
#define KC_C           0x06
#define KC_D           0x07
#define KC_E           0x08
#define KC_F           0x09
#define KC_G           0x0A
#define KC_H           0x0B
#define KC_I           0x0C
#define KC_J           0x0D
#define KC_K           0x0E
#define KC_L           0x0F
#define KC_M           0x10
#define KC_N           0x11
#define KC_O           0x12
#define KC_P           0x13
#define KC_Q           0x14
#define KC_R           0x15
#define KC_S           0x16
#define KC_T           0x17
#define KC_U           0x18
#define KC_V           0x19
#define KC_W           0x1A
#define KC_X           0x1B
#define KC_Y           0x1C
#define KC_Z           0x1D

#define KC_1           0x1E
#define KC_2           0x1F
#define KC_3           0x20
#define KC_4           0x21
#define KC_5           0x22
#define KC_6           0x23
#define KC_7           0x24
#define KC_8           0x25
#define KC_9           0x26
#define KC_0           0x27

#define KC_ENTER       0x28
#define KC_ESCAPE      0x29
#define KC_BACKSPACE   0x2A
#define KC_TAB         0x2B
#define KC_SPACE       0x2C

#define KC_MINUS       0x2D
#define KC_EQUAL       0x2E
#define KC_LBRACKET    0x2F
#define KC_RBRACKET    0x30
#define KC_BSLASH      0x31
#define KC_NONUS_HASH  0x32
#define KC_SCOLON      0x33
#define KC_QUOTE       0x34
#define KC_GRAVE       0x35
#define KC_COMMA       0x36
#define KC_DOT         0x37
#define KC_SLASH       0x38

#define KC_CAPSLOCK    0x39

#define KC_F1          0x3A
#define KC_F2          0x3B
#define KC_F3          0x3C
#define KC_F4          0x3D
#define KC_F5          0x3E
#define KC_F6          0x3F
#define KC_F7          0x40
#define KC_F8          0x41
#define KC_F9          0x42
#define KC_F10         0x43
#define KC_F11         0x44
#define KC_F12         0x45

#define KC_PSCREEN     0x46
#define KC_SCROLL      0x47
#define KC_PAUSE       0x48

#define KC_INSERT      0x49
#define KC_HOME        0x4A
#define KC_PGUP        0x4B
#define KC_DELETE      0x4C
#define KC_END         0x4D
#define KC_PGDN        0x4E

#define KC_RIGHT       0x4F
#define KC_LEFT        0x50
#define KC_DOWN        0x51
#define KC_UP          0x52

#define KC_LCTRL       0xE0
#define KC_LSHIFT      0xE1
#define KC_LALT        0xE2
#define KC_LGUI        0xE3
#define KC_RCTRL       0xE4
#define KC_RSHIFT      0xE5
#define KC_RALT        0xE6
#define KC_RGUI        0xE7

#define KC_FN          0xF0
#define KC_LAYER_TOGGLE 0xF1
#define KC_MACRO0      0xF2
#define KC_MACRO1      0xF3

#endif // KEYCODES_H
