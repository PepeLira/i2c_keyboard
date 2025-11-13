#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define REG_DEV_ID        0x00
#define REG_FW_VERSION    0x01
#define REG_STATUS        0x02
#define REG_MOD_MASK      0x03
#define REG_FIFO_COUNT    0x04
#define REG_FIFO_POP      0x05
#define REG_CFG_FLAGS     0x06
#define REG_CURSOR_STATE  0x07
#define REG_LED_STATE     0x08
#define REG_SCAN_RATE     0x09
#define REG_TS_HIGH       0x0A

#define DEV_ID            0xB0

#define STATUS_FIFO_EMPTY   0x01
#define STATUS_FIFO_FULL    0x02
#define STATUS_MOD_VALID    0x04
#define STATUS_ERROR        0x80

#define EVENT_NOP        0x00
#define EVENT_KEY_DOWN   0x01
#define EVENT_KEY_UP     0x02
#define EVENT_MOD_CHANGE 0x03
#define EVENT_CURSOR     0x04
#define EVENT_SPECIAL    0x05

#define CURSOR_UP        0x01
#define CURSOR_DOWN      0x02
#define CURSOR_LEFT      0x03
#define CURSOR_RIGHT     0x04
#define CURSOR_CENTER    0x05
#define CURSOR_RELEASE_FLAG 0x80

#define MOD_LEFT_CTRL    0x01
#define MOD_LEFT_SHIFT   0x02
#define MOD_LEFT_ALT     0x04
#define MOD_LEFT_GUI     0x08
#define MOD_RIGHT_CTRL   0x10
#define MOD_RIGHT_SHIFT  0x20
#define MOD_RIGHT_ALT    0x40
#define MOD_RIGHT_GUI    0x80

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t code;
    uint8_t mod_mask;
    uint8_t ts_low;
} keyboard_event_t;

#endif // PROTOCOL_H
