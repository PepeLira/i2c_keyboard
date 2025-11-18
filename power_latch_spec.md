# Firmware RP2040 – Power Latch Switch

This firmware has the objective of implementing a power cicle for a power latch switch. The wiring for this project is:

- Single WS2812B RGB led: gpio 28
- Power latch switch (pull up input): gpio 29
- Modifier button pulldown input button at: gpio 24

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
3. MODIFIER_COLOR: The color of the modifier key (ORANGE).

## General Flow:

- the firmware starts with the powert latch gpio as a pull up input CLOSE_LATCH (with that opening the power path to the other devices).
- If the firmware detects a FIRST_SWITCH_PRESS at the start of the program, then and only the latch should stay close CLOSE_LATCH. 
- If the firmware wont detect a FIRST_SWITCH_PRESS then OPEN_LATCH.
- IF the power switch is press the POWER_PRESS_COLOR should blink for one second periods (half on half off), till the switch is low again. 
- If a SWITCH_LONG_PRESS is detected then the power latch moves to OPEN_LATCH. 
- If the modifier button is pressed then the led should be solid MODIFIER_COLOR till the button is release again, then it goes back to idle.

## I2C Communication
* GPIO 0 (SDA) and GPIO 1 (SCL) are reserved for I2C communication.
* The RP2040 will act as an I2C slave device.
* The I2C interface will be used to report the current states of the power button and modifier button to an I2C master.
* The I2C slave address should be configurable (default value can be specified, e.g., 0x20).
* The I2C protocol should provide a way for the master to read a status byte or register, where each bit represents the state of a button:
* Bit 0: Power button state (1 = pressed, 0 = released)
* Bit 1: Modifier button state (1 = pressed, 0 = released)
* Remaining bits reserved for future use or additional buttons.
* The I2C logic must ensure reliable, debounced button state reporting.
* The I2C implementation must not interfere with the timing or reliability of the power latch and LED logic.


## Indications
- the tick for the timer must consider 1 ms.
- The implementation should follow good practices, a good abstraction and architecture.
- implement debounce times, focusing on reliability and low latency.
- It must consider that later the modifier inputs could drive other events and be expanded to more inputs.
- It must use optimum implementations for the manage of timers to permite a long term execution with low latency. 
- If the first press is more than 1000ms or 3000ms it should not be considered to be a SWITCH_LONG_PRESS. 
- The SWITCH_SHORT_PRESS should be indicated with the led but for now it wont trigger other changes of state (for now, later maybe not).
