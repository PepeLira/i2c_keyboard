# Firmware RP2040 – Power Latch Switch

This firmware has the objective of implementing a power cicle for a power latch switch and to scan a 6x7 keyboard matrix plus additional function keys. The wiring for this project is:

- Single WS2812B RGB led: gpio 28
- Power latch switch (pull up input): gpio 29
- I2C interface: GPIO 0 (SCL), GPIO 1 (SDA)
- Keyboard matrix columns:
  - Col A: GPIO 12
  - Col B: GPIO 13
  - Col C: GPIO 14
  - Col D: GPIO 15
  - Col E: GPIO 16
  - Col F: GPIO 17
  - Col G: GPIO 18
- Keyboard matrix rows:
  - Row 1: GPIO 7
  - Row 2: GPIO 8
  - Row 3: GPIO 9
  - Row 4: GPIO 10
  - Row 5: GPIO 11
  - Row 6: GPIO 2
- Independent FN keys:
  - FN1: GPIO 19
  - FN2: GPIO 20
  - FN3: GPIO 21
  - FN4: GPIO 22
  - FN5: GPIO 3
  - FN6: GPIO 4
  - FN8: GPIO 5
  - FN9: GPIO 6
  - FN10: GPIO 23
  - FN11: GPIO 24
  - FN12: GPIO 25
- Modifier buttons: a list of keys that are considered modifiers (these keys come from the 6x7 keyboard matrix and FN keys), they are keys like Shift, FN or ALT.
- Modifier Keys = [C6, C5, E4]

The FN1,2,3,4,5,6 are part of the keyboard, the FN8,9,10,11,12 are part of the mouse (up, down, left, right, left_click).

First define some states:

### Power Latch Switch
1. FIRST_SWITCH_PRESS: if the switch is pressed for more than one second at the start of the program. 
2. SWITCH_LONG_PRESS: after the first switch press, if the switch was pressed for more than 3 seconds.
3. SWITCH_SHORT_PRESS: the switch was pressed for less than 3 seconds. 

### Latch Gpio 
1. CLOSE_LATCH: the latch is close (letting the power pass throw) if the gpio is a pull up input. 
2. OPEN_LATCH: the latch is open (not letting the power pass throw) if the gpio is in their default state 
> // default state:  
> gpio_init(pin);             // sets it up as a GPIO  
> gpio_set_dir(pin, GPIO_IN); // make it an input  
> gpio_disable_pulls(pin); // no pull-up, no pull-down (floating) 
### RGB LED
1. IDLE_COLOR: The default color that is solid to indicate that the firmware is running (Green).
2. POWER_PRESS_COLOR: The blinking color to show that the power button is pressed (red).
3. MODIFIER_COLOR_: The individual color for each modifier. 

## General Flow:

- the firmware starts with the powert latch gpio as a pull up input CLOSE_LATCH (with that opening the power path to the other devices).
- If the firmware detects a FIRST_SWITCH_PRESS at the start of the program, then and only the latch should stay close CLOSE_LATCH. 
- If the firmware wont detect a FIRST_SWITCH_PRESS then OPEN_LATCH.
- IF the power switch is press the POWER_PRESS_COLOR should blink for one second periods (half on half off), till the switch is low again. 
- If a SWITCH_LONG_PRESS is detected then the power latch moves to OPEN_LATCH. 
- The modifier keys acts as a sticky key, when press one time the modifier stay active till a new key is pressed, if the modifier key is pressed two times in a row then the modifier sticks and is active till is pressed one more time. 
- While the modifier is active the led should be solid MODIFIER_COLOR_, then it goes back to idle.

## I2C Communication
* GPIO 0 (SDA) and GPIO 1 (SCL) are reserved for I2C communication.
* The RP2040 will act as an I2C slave device.
* The I2C interface will be used to report the current states of all keys in the keyboard matrix (6 rows × 7 columns), the 11 additional independent keys (FN1–FN12, skipping FN7), and the digital mouse position to an I2C master.
* The GPIO assignments for all keys are defined in the keyboard_layout.json file under the gpios property.
* The I2C slave address should be configurable (default value can be specified, e.g., 0x20).

### Register Map and Behaviour
The I2C protocol exposes a small set of registers that the master can read and write to obtain key events and mouse status:

* **Interrupt / Event signalling**
  * The firmware shall provide an interrupt-capable output (e.g. a dedicated GPIO line) that is asserted when new keyboard/digital mouse(5 buttons) events are available (for example, when the FIFO goes from empty to non‑empty).
  * The interrupt line is cleared when the master has read all pending events from the FIFO (FIFO becomes empty) and any pending status has been acknowledged.

* **Key Status Register** (read‑only)
  * Encodes the current state of modifier keys and the FIFO fill level.
  * Bits [3:0]: modifier status (implementation‑defined bitmap of active modifiers such as Shift, FN, ALT, Ctrl).
  * Bits [7:4]: FIFO level (number of key events currently waiting to be read, up to the maximum FIFO depth).
  * Remaining bits are reserved for future use and must be read as 0.

* **FIFO Access Register** (read‑only)
  * Reading this register pops one entry from the key event FIFO.
  * Each entry encodes both the key state and the key code:
    * One bit indicates whether the event is a key press, key press and held or key release.
    * The remaining bits encode the key code, which uniquely identifies each key in the 6x7 matrix and the 11 independent FN keys.
  * If the FIFO is empty, reading this register returns a defined "no event" value and must not underflow the FIFO.

* **Mouse X Position Register** (read‑only)
  * Returns the current X position or delta of the Mouse, in an implementation‑defined format (e.g. signed 8‑bit delta or absolute 16‑bit position).

* **Mouse Y Position Register** (read‑only)
  * Returns the current Y position or delta of the Mouse, in an implementation‑defined format.

* The I2C logic must ensure reliable, debounced key state reporting for all keys and consistent Mouse readings.
* The I2C implementation must not interfere with the timing or reliability of the power latch and LED logic.


## Indications
- the tick for the timer must consider 1 ms.
- The implementation should follow good practices, a good abstraction and architecture.
- implement debounce times, focusing on reliability and low latency.
- It must consider that later the modifier inputs could drive other events and be expanded to more inputs.
- It must use optimum implementations for the manage of timers to permite a long term execution with low latency. 
- If the first press is more than 1000ms or 3000ms it should not be considered to be a SWITCH_LONG_PRESS. 
- The SWITCH_SHORT_PRESS should be indicated with the led but for now it wont trigger other changes of state (for now, later maybe not).
