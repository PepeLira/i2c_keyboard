#include "fifo.h"

#include <string.h>

static size_t advance(const event_fifo_t *fifo, size_t index) {
    (void)fifo;
    return (index + 1) % FIFO_SIZE;
}

void fifo_init(event_fifo_t *fifo) {
    fifo->head = 0;
    fifo->tail = 0;
    fifo->full = false;
    memset(fifo->buffer, 0, sizeof(fifo->buffer));
}

bool fifo_push(event_fifo_t *fifo, const keyboard_event_t *event) {
    if (fifo->full) {
        return false;
    }
    fifo->buffer[fifo->head] = *event;
    fifo->head = advance(fifo, fifo->head);
    if (fifo->head == fifo->tail) {
        fifo->full = true;
    }
    return true;
}

bool fifo_pop(event_fifo_t *fifo, keyboard_event_t *event) {
    if (fifo_is_empty(fifo)) {
        return false;
    }
    *event = fifo->buffer[fifo->tail];
    fifo->tail = advance(fifo, fifo->tail);
    fifo->full = false;
    return true;
}

size_t fifo_count(const event_fifo_t *fifo) {
    if (fifo->full) {
        return FIFO_SIZE;
    }
    if (fifo->head >= fifo->tail) {
        return fifo->head - fifo->tail;
    }
    return FIFO_SIZE - fifo->tail + fifo->head;
}

bool fifo_is_empty(const event_fifo_t *fifo) {
    return (!fifo->full && (fifo->head == fifo->tail));
}

bool fifo_is_full(const event_fifo_t *fifo) {
    return fifo->full;
}
