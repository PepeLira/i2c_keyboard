#include "fifo.h"

void fifo_init(fifo_t *fifo) {
    fifo->head = 0;
    fifo->tail = 0;
    fifo->count = 0;
}

bool fifo_is_empty(const fifo_t *fifo) {
    return fifo->count == 0;
}

bool fifo_is_full(const fifo_t *fifo) {
    return fifo->count >= FIFO_SIZE;
}

bool fifo_push(fifo_t *fifo, const fifo_event_t *event) {
    if (fifo_is_full(fifo)) {
        return false;
    }
    fifo->buffer[fifo->head] = *event;
    fifo->head = (fifo->head + 1) % FIFO_SIZE;
    fifo->count++;
    return true;
}

bool fifo_pop(fifo_t *fifo, fifo_event_t *event) {
    if (fifo_is_empty(fifo)) {
        return false;
    }
    *event = fifo->buffer[fifo->tail];
    fifo->tail = (fifo->tail + 1) % FIFO_SIZE;
    fifo->count--;
    return true;
}
