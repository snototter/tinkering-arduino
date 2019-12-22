// A stop watch to support chairing presentation sessions.
// 
// * 7-segment display shows the talk's duration.
// * Toggle button (starts/stops) the stop watch
// * Reset button reset the stop watch ;-)
// * Orange/Blue LED lights up before slot is over.
// * Red LED lights up when slot is over.
// * LEDs flash if speaker exceeds more than some defined
//   threshold.
//
// Buttons are software-debounced, continuously polled. No
// interrupt logic for this simple project.

//-----------------------------------------------------------------------------
// Configuration
// 2019/12 - Configuration values according to VWA lecture requirements.

// Duration of slot (milliseconds)
// Set to 0 to disable LEDs
#define PRESENTATION_SLOT    (7*60*1000)

// Light up first LED TAU_REMAINDER milliseconds before
// the end of the allocated slot to indicate that "the end is near".
//
// For example, 60000 (1 minute before end)
#define TAU_REMAINDER (60000)

// Start blinking LEDs if speaker exceeds more than
// TAU_EXCEED milliseconds of the allocated presentation slot.
//
// For example, 10000 (10 seconds after slot should've ended)
#define TAU_EXCEED   (10000)


//-----------------------------------------------------------------------------
// Wiring configuration.

#define PIN_SSD_CLK           2
#define PIN_SSD_DIO           3
#define PIN_LED_REMAINDER     9
#define PIN_LED_TIMEOUT      10
#define PIN_BTN_RESET         6
#define PIN_BTN_TOGGLE_WATCH  7


//-----------------------------------------------------------------------------
// Misc.

// Debounce time in milliseconds.
#define BTN_DEBOUNCE_MILLIS   50

// Force reset button to be held longer before resetting to prevent accidental
// resets ;-)
#define BTN_RESET_HOLD        500

// 7-segment display brightness [0,7].
#define SSD_BRIGHTNESS        7

#define DEBUG_OUTPUT

//-----------------------------------------------------------------------------
// Utilities for slightly cleaner code:
#include <Button.h>
#include <LED.h>
#include <SevenSegmentDisplay.h>
#include <StopWatch.h>

//-----------------------------------------------------------------------------
// Program variables:
Button btn_reset( PIN_BTN_RESET,        BTN_DEBOUNCE_MILLIS, 500);
Button btn_toggle(PIN_BTN_TOGGLE_WATCH, BTN_DEBOUNCE_MILLIS);

LED led_remainder(PIN_LED_REMAINDER);
LED led_timeout(PIN_LED_TIMEOUT);

SevenSegmentDisplayTM1637 display(PIN_SSD_CLK, PIN_SSD_DIO, SSD_BRIGHTNESS);

StopWatch stop_watch;


// 7-segment display data show "--:--"
const uint8_t ssd_seg_reset[] = 
{
  SEG_G,
  SEG_G | SEG_COLON,
  SEG_G,
  SEG_G
};

// This program has two states: init/reset (before a talk starts) and 
// counting presentation time. So a boolean flag suffices to indicate the
// current state.
bool talk_in_progress = false;


void setup()
{
  // Initialization (called after initializing the globals,
  // before first loop iteration).

  scw_reset();
  
#ifdef DEBUG_OUTPUT
  Serial.begin(9600);
#endif // DEBUG_OUTPUT
}


void scw_reset()
{
  talk_in_progress = false;
  
  // Turn off all LEDs.
  led_remainder.off();
  led_timeout.off();

  // Show reset on display.
  display.setSegments(ssd_seg_reset);

  // Reset stop watch.
  stop_watch.reset();
}

void loop()
{
  btn_toggle.read();
  btn_reset.read();
  if (talk_in_progress)
  {
    if (btn_toggle.changedToPressed())
    {
      stop_watch.toggle();
      led_remainder.toggle();
    }

    if (btn_reset.isHeld())
    {
      scw_reset();
    }
    else
    {
      const unsigned long elapsed_ms = stop_watch.elapsed();
      const unsigned int sec = (elapsed_ms % 60000)/1000;
      const unsigned int min = elapsed_ms / 60000;
      display.displayTime(min, sec);
    }
  }
  else
  {
    if (btn_toggle.changedToPressed())
    {
      talk_in_progress = true;
      led_remainder.on();
      stop_watch.start();
    }
    // Timer has been reset, show "--:--".
  }
}

