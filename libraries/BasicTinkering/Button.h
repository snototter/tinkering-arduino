#ifndef __BASIC_TINKERING_BUTTON__
#define __BASIC_TINKERING_BUTTON__

/**
 * Utility for software-debounced buttons.
 */
class Button
{
public:
  //TODO comment
  // notify_hold_once: isHeld() will return true only once (unless button is released and pressed again)
  Button(uint8_t pin, unsigned int debounce_delay = 50, unsigned int hold_delay = 500, bool notify_hold_once = true);

  // Must be invoked in every loop iteration to scan the button's pin.
  // Returns true if the button is currently pressed.
  bool read();

  // Returns the current state of the button.
  // Invoke after read().
  bool isPressed() const;

  // Returns true if the button is currently active and has
  // been pressed for the configured minimum amount of time.
  bool isHeld() const;

  // Returns true if the button state changed.
  // Invoke after read().
  bool changed() const;

private:
  // Pin number (digital in)
  uint8_t pin_;

  // Internal state variable
  uint8_t state_;

  unsigned int debounce_delay_;
  unsigned long debounce_start_;

  unsigned int hold_delay_;
  unsigned long hold_start_;
};
#endif
