#ifndef FIFO_H
#define FIFO_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "protocol.h"

typedef struct {
    fifo_event_t buffer[FIFO_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} fifo_t;

void fifo_init(fifo_t *fifo);
bool fifo_is_empty(const fifo_t *fifo);
bool fifo_is_full(const fifo_t *fifo);
bool fifo_push(fifo_t *fifo, const fifo_event_t *event);
bool fifo_pop(fifo_t *fifo, fifo_event_t *event);

#endif // FIFO_H
