#include "event.h"

#include <stddef.h>

#include "config.h"

#ifndef EVENT_QUEUE_LENGTH
#define EVENT_QUEUE_LENGTH 32
#endif

static event_handler_t subscribers[8];
static size_t subscriber_count = 0;

static event_t queue[EVENT_QUEUE_LENGTH];
static size_t queue_head = 0;
static size_t queue_tail = 0;

static bool queue_is_full(void) {
    return ((queue_tail + 1) % EVENT_QUEUE_LENGTH) == queue_head;
}

static bool queue_is_empty(void) {
    return queue_head == queue_tail;
}

void event_init(void) {
    subscriber_count = 0;
    queue_head = 0;
    queue_tail = 0;
}

bool event_subscribe(event_handler_t handler) {
    if (subscriber_count >= (sizeof(subscribers) / sizeof(subscribers[0])) || handler == NULL) {
        return false;
    }

    subscribers[subscriber_count++] = handler;
    return true;
}

bool event_publish(const event_t *event) {
    if (event == NULL || queue_is_full()) {
        return false;
    }

    queue[queue_tail] = *event;
    queue_tail = (queue_tail + 1) % EVENT_QUEUE_LENGTH;
    return true;
}

void event_process_pending(void) {
    while (!queue_is_empty()) {
        const event_t ev = queue[queue_head];
        queue_head = (queue_head + 1) % EVENT_QUEUE_LENGTH;

        for (size_t i = 0; i < subscriber_count; i++) {
            if (subscribers[i]) {
                subscribers[i](&ev);
            }
        }
    }
}
