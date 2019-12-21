#ifndef __BASIC_TINKERING_LED__
#define __BASIC_TINKERING_LED__

/**
 * Wrapper for LEDs (supports blocking/non-blocking blinking).
 */
class LED
{
public:
  // Set the pin number upon construction.
  LED(uint8_t pin);

  // Returns current pin state.
  bool status() const;

  // Turn LED on.
  void on();

  // Turn LED off.
  void off();

  // Toggle on/off.
  void toggle();

  // Start non-blocking blinking (~"time" ms on,
  // ~"time" ms off). You must call blink() which
  // takes care of toggling the LED.
  void start_blinking(unsigned int time);

  // Loop iteration to be called repeatedly after
  // you set up blinking via start_blinking().
  void blink();

  //TODO
  void stop_blinking();

private:
  // Pin number (digital in).
  uint8_t pin_;

  // Internal state variable.
  uint8_t state_;

  // Number of milliseconds the LED should be
  // on or off while blinking.
  unsigned int blink_time_;

  // Last LED toggle while blinking.
  unsigned long blink_start_;
};
#endif
