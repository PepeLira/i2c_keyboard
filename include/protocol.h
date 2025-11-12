#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define REG_DEV_ID          0x00
#define REG_FW_VERSION_MAJOR 0x01
#define REG_FW_VERSION_MINOR 0x02
#define REG_STATUS          0x03
#define REG_MOD_MASK        0x04
#define REG_FIFO_COUNT      0x05
#define REG_FIFO_POP        0x06
#define REG_CFG_FLAGS       0x07
#define REG_CURSOR          0x08
#define REG_LED_STATE       0x09
#define REG_SCAN_RATE       0x0C

#define STATUS_FIFO_EMPTY   (1u << 0)
#define STATUS_FIFO_FULL    (1u << 1)
#define STATUS_MOD_VALID    (1u << 2)
#define STATUS_ERROR        (1u << 7)

#define EVENT_NOP       0x00
#define EVENT_KEY_DOWN  0x01
#define EVENT_KEY_UP    0x02
#define EVENT_MOD_CHANGE 0x03
#define EVENT_CURSOR    0x04
#define EVENT_SPECIAL   0x05

#define CURSOR_UP     1
#define CURSOR_DOWN   2
#define CURSOR_LEFT   3
#define CURSOR_RIGHT  4
#define CURSOR_CENTER 5

#define CURSOR_BIT_UP     (1u << 0)
#define CURSOR_BIT_DOWN   (1u << 1)
#define CURSOR_BIT_LEFT   (1u << 2)
#define CURSOR_BIT_RIGHT  (1u << 3)
#define CURSOR_BIT_CENTER (1u << 4)

#define MOD_CTRL   (1u << 0)
#define MOD_SHIFT  (1u << 1)
#define MOD_ALT    (1u << 2)
#define MOD_GUI    (1u << 3)
#define MOD_FN     (1u << 4)

typedef struct {
    uint8_t type;
    uint8_t code;
    uint8_t mods;
    uint8_t timestamp;
} fifo_event_t;

#endif // PROTOCOL_H
