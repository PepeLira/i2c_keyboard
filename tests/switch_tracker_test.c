#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "switch_tracker.h"

static switch_event_t step_until(switch_tracker_t *tracker, bool pressed, uint32_t *now_ms, uint32_t end_ms) {
    switch_event_t last = SWITCH_EVENT_NONE;
    while (*now_ms < end_ms) {
        last = switch_tracker_tick(tracker, pressed, *now_ms);
        (*now_ms)++;
        if (last != SWITCH_EVENT_NONE) {
            break;
        }
    }
    return last;
}

int main() {
    switch_tracker_t tracker;
    switch_tracker_init(&tracker, 1000, 1000, 3000);

    uint32_t now_ms = 0;

    // First press inside startup window held long enough to latch, but should not emit long press
    switch_event_t evt = switch_tracker_tick(&tracker, true, now_ms);
    assert(evt == SWITCH_EVENT_NONE);
    now_ms = 1;
    evt = step_until(&tracker, true, &now_ms, 1200);
    assert(evt == SWITCH_EVENT_FIRST_PRESS);
    // Continue to hold long but must not emit long press for the first interaction
    evt = step_until(&tracker, true, &now_ms, 3200);
    assert(evt == SWITCH_EVENT_NONE);

    // Release
    evt = switch_tracker_tick(&tracker, false, now_ms);
    assert(evt == SWITCH_EVENT_NONE);

    // After startup window, a long press should be detected
    now_ms = 4000;
    evt = switch_tracker_tick(&tracker, true, now_ms);
    assert(evt == SWITCH_EVENT_NONE);
    now_ms += 1;
    evt = step_until(&tracker, true, &now_ms, 8000);
    assert(evt == SWITCH_EVENT_LONG_PRESS);

    // Short press after long press
    evt = switch_tracker_tick(&tracker, false, now_ms);
    assert(evt == SWITCH_EVENT_NONE);
    now_ms += 100;
    evt = switch_tracker_tick(&tracker, true, now_ms);
    assert(evt == SWITCH_EVENT_NONE);
    now_ms = 9150;
    evt = switch_tracker_tick(&tracker, false, now_ms);
    assert(evt == SWITCH_EVENT_SHORT_PRESS);

    // Latch should have remained closed after the first press
    assert(switch_tracker_should_hold_latch(&tracker));

    return 0;
}
