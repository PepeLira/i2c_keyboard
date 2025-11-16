#ifndef EVENT_H
#define EVENT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    EVENT_TICK_1MS = 0,
    EVENT_BUTTON_PRESSED,
    EVENT_BUTTON_RELEASED,
    EVENT_BUTTON_HOLD_1S,
    EVENT_BUTTON_HOLD_3S,
    EVENT_POWER_MODE_CHANGED,
    EVENT_LATCH_CHANGED
} event_type_t;

typedef struct {
    event_type_t type;
    uint32_t timestamp_ms;
    union {
        uint32_t u32;
        int32_t i32;
    } data;
} event_t;

typedef void (*event_handler_t)(const event_t *event);

void event_init(void);
bool event_subscribe(event_handler_t handler);
bool event_publish(const event_t *event);
void event_process_pending(void);

#endif
