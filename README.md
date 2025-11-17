# Bit-0 Keyboard Firmware

This repo implements the main functionality for the keyboard of our tiny computer, the firmware have more roles rather than just the keyboard. 

1. It manage the state of the power latch and the power button functionality.
2. Reads the 6x7 key matrix and some independent individual keys for the mouse and simultaneous action switches. 
3. Controls and displays the different states with a programable rgb LED.
4. Communicate with a separate main SOC via i2C.
5. Read the battery % and displays it with the rgb LED.
