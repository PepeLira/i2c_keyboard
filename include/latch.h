#ifndef LATCH_H
#define LATCH_H

#include "event.h"

typedef enum {
    LATCH_OPEN = 0,
    LATCH_CLOSED,
    LATCH_TRANSITION
} latch_status_t;

void latch_init(void);
void latch_set_status(latch_status_t status, uint32_t timestamp_ms);
latch_status_t latch_get_status(void);

#endif
