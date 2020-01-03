# Tinkering with Arduino
Tinkering projects using Arduino. This repository consists of custom libraries and some of my projects.

# BasicTinkering Library
A collection of wrappers for commonly used electronic components. Quite useful for tinkering and keeping embedded code maintainable. Includes wrappers for:
* `BtButton` - software-debounced push-buttons/triggers.
* `BtLED` - standard on/off plus blinking/fading capabilities (if LED is wired to a PWM-enabled pin).
* `BtPotentiometer` - allows binning the potentiometer value.
* `BtSevenSegmentDisplay` - 7-segment display wrappers currently supporting `TM1637` (4-digit) digital tubes.
* `BtStopWatch` - measures time.

# Tinkering Projects
Deployable tinkering projects.

## Automated Plant Watering
* A low-power tinkering project to water my tomatoes and chili plants.
* Source at `projects/Garduino`.
* Only useful to lookup stuff on interrupts, watch dog timers, low-power consumption, setting/using ADC reference voltage, measuring battery levels, etc. Due to external library updates, this project doesn't build currently.

![Example image for Garduino](./projects/Garduino/garduino.jpg "The magic watering box")

## Session Chair's Stop Watch
* A stop watch for session chairs to time presentations, notify speakers of their last X minutes and go crazy once they exceed their allocated slot.
* Source at `projects/SessionChairWatch`.

![Example image for Session Chair project](./projects/SessionChairWatch/session-chair.jpg "Session Chair's Stop Watch")

# TODO List
* add scheduling functionality to basictinkering
* test scheduling functionality
* use digitalwrite (or direct port access) instead of pinmode https://github.com/bremme/arduino-tm1637/tree/master/src vs https://github.com/avishorp/TM1637
* new repo for library, register with lib manager: https://github.com/arduino/Arduino/wiki/Library-Manager-FAQ
