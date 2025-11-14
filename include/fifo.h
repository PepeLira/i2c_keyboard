#ifndef FIFO_H
#define FIFO_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"
#include "protocol.h"

typedef struct {
    keyboard_event_t buffer[FIFO_SIZE];
    size_t head;
    size_t tail;
    bool full;
} event_fifo_t;

void fifo_init(event_fifo_t *fifo);
bool fifo_push(event_fifo_t *fifo, const keyboard_event_t *event);
bool fifo_pop(event_fifo_t *fifo, keyboard_event_t *event);
size_t fifo_count(const event_fifo_t *fifo);
bool fifo_is_empty(const event_fifo_t *fifo);
bool fifo_is_full(const event_fifo_t *fifo);

#endif // FIFO_H
