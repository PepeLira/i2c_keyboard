# Bit-0 Keyboard Firmware

This firmware drives the RP2040-based power and input controller for the Bit-0 keyboard. It manages the power latch, power
switch timing, the modifier key, and the RGB status LED.

## Behavior overview
- **Power latch (GPIO 29):** Starts as a pull-up input to close the latch and keep the system powered. The latch is opened by
  returning the pin to a floating input (no pulls). A long press after boot opens the latch to cut power.
- **Switch timing:**
  - `FIRST_SWITCH_PRESS`: Press begins during the first second of runtime and is held for at least one second. When detected,
    the latch remains closed; this initial press never counts as a long press.
  - `SWITCH_LONG_PRESS`: Any subsequent press held for at least three seconds. Opens the latch.
  - `SWITCH_SHORT_PRESS`: Presses held for at least one second but less than three seconds. Triggers visual feedback only.
- **Debounce and timing:** A 1 ms repeating timer drives debouncing and state machines. Buttons use a 30 ms debounce window.
- **Modifier button (GPIO 24):** Pulldown input that forces the LED to the modifier color while held.
- **RGB LED (GPIO 28):**
  - Idle: solid green.
  - Power button held: blink red at 1 Hz (50% duty cycle).
  - Modifier held: solid orange overrides other indications.
  - Short press feedback: brief pulse without changing latch state.

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
