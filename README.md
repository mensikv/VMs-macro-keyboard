# VM's macro keyboard
This project is developed for Arduino Micro boards and it functions as a **macro keyboard with rotary encoder**. Eight mechanical cherry MX keys can be assigned various functions (writing text, combination and sequences of key codes, multimedia and mouse functions). Rotary encoder supports turning, click and double click.

## Features
* up to 4 keys codes sent at once
* sequence of maximum 16 length of separate key commands
* modifier key to change the default behavior of rotary encoder
* STL files for case
* all keys are directly connected to GPIOs, so no diodes are required
* can emulate keyboard, multimedia key, mouse and system commands

This project is inspired by and some code is taken from [Control volume knob by Prusa](https://blog.prusaprinters.org/3d-print-an-oversized-media-control-volume-knob-arduino-basics/).

## Prerequisites
### Hardware
* 8x [Cherry MX](https://www.aliexpress.com/item/32900957560.html) - I have used red ones
* 1x [25cm micro USB cable](https://www.aliexpress.com/item/32958208619.html) - you can of course use any micro USB cable
* 1x [Arduino Pro Micro 5V](https://www.aliexpress.com/item/32785518952.html) - as the board is powered from the USB, ie. with 5V, you can solder the J1 solder joint to bypass the LDO
* 1x [rotary encoder](https://www.aliexpress.com/item/1000001872933.html) - make sure it has thread to secure it on the case
* Optional - [sillicone wires](https://www.aliexpress.com/item/32872439317.html) - in comparison to my old wires, these are angel's stuff. I have used AWG26, but I suggest to use AWG 28, there are no big currents.

Connection between the components:
![Scheme](/images/VMs_macro_keyboard_scheme.png)

### Software
Project was tested with following version:
* Arduino IDE 1.8.12
* Arduino AVR boards 1.8.2

Libraries:
* [ClickEncoder](https://github.com/0xPIT/encoder)
* [TimerOne 1.1](https://github.com/PaulStoffregen/TimerOne)
* [HID-Project 2.6.1](https://github.com/NicoHood/HID)

## Setup
1. Assembly the board and the keys and the rotary encoder according to the [scheme](/images/VMs_macro_keyboard_scheme.png).
1. In Arduino IDE open the sktech `VMs_macro_keyboard.ino`.
1. In section `// Define actions for your keys` define what actions you want to assign to each individual key. There are examples, so inspire from them.
1. In default the rotary encoder functions as volume control, or scrolling (with the modifier key). Click is Pause, double click is Mute. If you want other behavior, tinker with the section `// ROTARY_ACTIONS`.

Bigger part of the arduino Micro memory is taken by array for key commands. If you want to free more memory, either decrease the `MAX_COMBINATION_KEYS` (key codes sent at once) or `MAX_SEQUENCE_KEYS` (length of sequence of commands).

## Supported commands
There are 5 types of key codes the board can send. Each key can be of one type:
### KEYBOARD
Can send common key strokes from any keyboard (letters, SHIFT, CTRL...). The full list of possible key codes is in the file `Arduino\libraries\HID-Project\src\KeyboardLayouts\ImprovedKeylayouts` in the section `enum KeyboardKeycode : uint8_t`. You can define up to 16 combinations each with up to 3 key codes. Each combination can have its own duration. The whole sequence can have up to 3 modifier key codes (for example SHIFT).

### CONSUMER
Can send multimedia and exotic commands. The full list of possible key codes is in the file `Arduino\libraries\HID-Project\src\HID-APIs\ConsumerAPI.h` in the section `enum ConsumerKeycode : uint16_t`. You can define up to 16 combinations each with up to 3 key codes. Each combination can have its own duration.

### SYSTEM
Can send command like *shutdown*, *wake up*. The full list of possible key codes is in the file `Arduino\libraries\HID-Project\src\HID-APIs\SystemAPI.h` in the section `enum SystemKeycode : uint8_t`. Only one command can be defined.

### MOUSE
Can move with the cursor or scroll and click left/right/middle buttons. For syntax see the source code.

### MODIFIER
Changes global variable `globalModifier` so you can use it to assign two functions to one key.

## STL for case
In folder `stl` you will find `*.stl` files for your 3D printer, so you can print nice case for this macro keyboard.
Aside from the provided files you need:
* 8x [KeyV2: Parametric Mechanical Keycap Library](https://www.thingiverse.com/thing:2783650) - I have used oem_row-5_length-1.stl
* 1x [volume knob](https://www.prusaprinters.org/prints/4739-media-control-volume-knob-smooth-knob/files) - I have used knob_smooth_playpause.3mf
More information about the case and photos can be found on [Thingiverse](https://www.thingiverse.com/thing:4322182).
