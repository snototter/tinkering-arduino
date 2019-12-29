# Tinkering with Arduino
Tinkering projects using Arduino. This repository consists of custom libraries and some of my projects.

Overview:
* `libraries/BasicTinkering` - A collection of commonly used Arduino stuff I need for tinkering, in particular:
  * A `Button` wrapper for software-debounced push-buttons/triggers.
  * An `LED` wrapper which adds blinking/fading capabilities (if LED is wired to a PWM-enabled pin).
  * A `Potentiometer` wrapper which allows binning the potentiometer value.
  * A `SevenSegmentDisplay` wrapper currently supporting `TM1637` digital tubes (4-digit 7-segment display).
  * A `StopWatch` to measure time.
* `projects/SessionChairWatch` - A stop watch for session chairs to time presentations, notify speakers of their last X minutes and go crazy once they exceed their allocated slot.<br/>![Example image for Session Chair project](./projects/SessionChairWatch/session-chair.jpg "Session Chair's Stop Watch")
* `projects/Garduino` - A low-power tinkering project to water my tomatoes and chili plants. TODO (check disk backup for ino files)<br/>![Example image for Garduino](./projects/Garduino/garduino.jpg "The magic watering box")
