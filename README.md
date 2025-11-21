# Bit-0 Keyboard Firmware

This firmware drives the RP2040-based power and input controller for the Bit-0 keyboard. It manages the power latch, power switch timing, a 6x7 keyboard matrix, 11 independent function keys, modifier keys with sticky/lock behavior, digital mouse control, and I2C slave communication for event reporting.

## Hardware Configuration

### GPIO Assignments
- **Power latch switch:** GPIO 29 (pull-up input)
- **RGB LED (WS2812B):** GPIO 28
- **I2C interface:** GPIO 0 (SDA), GPIO 1 (SCL)
- **I2C interrupt output:** GPIO 26 (active-low event signaling)

### Keyboard Matrix (6 rows × 7 columns)
- **Rows:** GPIO 7, 8, 9, 10, 11, 2
- **Columns:** GPIO 12 (A), 13 (B), 14 (C), 15 (D), 16 (E), 17 (F), 18 (G)

### Independent Function Keys
- **FN1-FN6:** GPIO 19, 20, 21, 22, 3, 4
- **FN8-FN12:** GPIO 5, 6, 23, 24, 25 (FN7 is skipped)

### Modifier Keys (from matrix)
- **FN:** C6 (Column C, Row 6)
- **ALT:** C5 (Column C, Row 5)
- **LSHIFT:** E4 (Column E, Row 4)

## Behavior Overview

### Power Management
- **Power latch (GPIO 29):** Starts as a pull-up input to close the latch and keep the system powered. The latch is opened by returning the pin to a floating input (no pulls). A long press after boot opens the latch to cut power.
- **Switch timing:**
  - `FIRST_SWITCH_PRESS`: Press begins during the first second of runtime and is held for at least one second. When detected, the latch remains closed; this initial press never counts as a long press.
  - `SWITCH_LONG_PRESS`: Any subsequent press held for at least three seconds. Opens the latch.
  - `SWITCH_SHORT_PRESS`: Presses held for at least one second but less than three seconds. Triggers visual feedback only.

### Keyboard Scanning
- **Matrix scanning:** All 42 matrix keys (6×7) are scanned with column-driven, row-read architecture
- **FN1-FN6 (keyboard keys):** Generate keyboard events, pushed to FIFO for I2C retrieval
- **FN8 (action button):** Generates click events, pushed to FIFO
- **FN9-FN12 (mouse control):** Control mouse movement, do NOT generate FIFO events
- **Debouncing:** 30ms debounce time for all keys
- **Event generation:** Press, Hold (500ms threshold), and Release events for keyboard keys
- **Event FIFO:** 64-entry circular buffer stores key events for I2C master retrieval

### Modifier Behavior
Modifier keys (FN, ALT, SHIFT) support sticky and locked modes:
- **Single press:** Modifier activates and remains active until another non-modifier key is pressed (sticky mode)
- **Double press (within 300ms):** Modifier locks and remains active until pressed again
- **LED indication:** Active modifier shows its color on the RGB LED

### Digital Mouse Control
FN keys 9-12 provide digital mouse movement control:
- **FN12:** Move left (continuous while held)
- **FN11:** Move right (continuous while held)
- **FN10:** Move up (continuous while held)
- **FN9:** Move down (continuous while held)
- **FN8:** Mouse click action (generates FIFO event)
- **Movement speed:** 2 pixels per 20ms update interval
- **Note:** Mouse movement keys (FN9-12) do NOT generate FIFO events; only position deltas are reported via I2C registers

### RGB LED Status
- **Idle:** Solid green (0x001400)
- **Power button held:** Blink red at 1 Hz (0x140000)
- **FN modifier active:** Solid orange (0x200C00)
- **ALT modifier active:** Solid yellow-green (0x0C2000)
- **SHIFT modifier active:** Solid cyan (0x00200C)
- **Short press feedback:** Brief orange pulse (0x200400)
- **Priority:** Power press > Modifier > Short press pulse > Idle

### Timing Configuration
- **Tick interval:** 1 ms
- **Debounce time:** 30 ms
- **Startup window:** 1000 ms
- **First press hold threshold:** 500 ms
- **Long press threshold:** 3000 ms
- **Hold event threshold:** 500 ms
- **Modifier double-press window:** 300 ms
- **Mouse update interval:** 20 ms

## I2C Slave Interface

The RP2040 operates as an I2C slave at address **0x20** (configurable), exposing a register-based interface for the I2C master to read key events and mouse status.

### Register Map

| Address | Name | Access | Description |
|---------|------|--------|-------------|
| 0x00 | Key Status | R | Bits[3:0]: Active modifiers bitmap (FN, ALT, SHIFT, reserved)<br>Bits[7:4]: FIFO level (0-15, clamped) |
| 0x01 | FIFO Access | R | Pop one key event from FIFO<br>Bits[1:0]: Event type (00=none, 01=press, 10=hold, 11=release)<br>Bits[7:2]: Key code (0-52 for all keys)<br>Returns 0x00 if FIFO empty |
| 0x02 | Mouse X | R | Signed 8-bit X position delta<br>Reading clears the accumulated value |
| 0x03 | Mouse Y | R | Signed 8-bit Y position delta<br>Reading clears the accumulated value |

### Key Code Mapping
- **Matrix keys:** Row × 7 + Column (0-41)
  - Example: Row 0, Col 0 = 0; Row 5, Col 6 = 41
- **FN1-FN6, FN8:** 42-47, 48 (keyboard/action keys that generate FIFO events)
- **FN9-FN12:** 49-52 (mouse movement keys - do NOT generate FIFO events, only control position deltas)

### Interrupt Signaling
- **GPIO 26** serves as an active-low interrupt output
- **Asserted (LOW):** When key events are available in the FIFO (FIFO transitions from empty to non-empty)
- **Cleared (HIGH):** When the FIFO becomes empty after the master reads all events
- The interrupt line is automatically managed by the firmware based on FIFO state

### I2C Communication Protocol
1. Master writes register address to select the register
2. Master reads one or more bytes (auto-increment not supported, address must be rewritten for each register)
3. Reading FIFO Access (0x01) pops one event and decrements the FIFO count
4. Reading Mouse X/Y (0x02/0x03) returns and clears the delta value
5. When FIFO becomes empty, the interrupt line is automatically deasserted

**Note:** Mouse click actions (FN8) generate FIFO events just like keyboard keys. Mouse movement keys (FN9-12) only affect the X/Y position deltas reported via I2C registers 0x02 and 0x03.

## Building
1. Fetch the pico-sdk submodule (uses HTTPS):
   ```bash
   git submodule update --init --recursive
   ```
2. Configure and build:
   ```bash
   cmake -B build -S .
   cmake --build build
   ```
3. Build artifacts (UF2/ELF/map) are generated in `build/` via `pico_add_extra_outputs`.

> If network access is restricted, the pico-sdk fetch may need to be performed in an environment with access to
> `https://github.com/raspberrypi/pico-sdk.git`.

## Tests and simulation hooks
The switch timing logic is hardware-agnostic and covered by a lightweight unit test:
```bash
cd build
ctest
```
The test exercises the first-press guard, short press detection, and long-press detection timelines without requiring
hardware access.
