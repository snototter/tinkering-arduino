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
#define DURATION_PRESENTATION_SLOT    (7*60000)

// Light up first LED TAU_REMAINDER milliseconds before
// the end of the allocated slot to indicate that "the end is near".
//
// You have to ensure that TAU_REMAINDER < DURATION_PRESENTATION_SLOT.
#define TAU_REMAINDER (60000)

// Start blinking LEDs if speaker exceeds more than
// TAU_EXCEED milliseconds of the allocated presentation slot.
//
// For example, 10000 (10 seconds after slot should've ended)
#define TAU_EXCEED   (10000)

// How many milliseconds the LED should be on/off when blinking, i.e.
// a full blink cycle would be 2xDURATION_BLINKING ms.
#define DURATION_BLINKING 500


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

// We use the non-blocking LED blink calls, thus we need to keep track
// of whether we already started blinking or not...
bool led_blinking_started = false;


// Initialization.
void setup()
{
  scw_reset();
  
#ifdef DEBUG_OUTPUT
  Serial.begin(9600);
#endif // DEBUG_OUTPUT
}


// Reset the session chair's watch.
void scw_reset()
{
  talk_in_progress = false;
  led_blinking_started = false;
  
  // Turn off all LEDs.
  led_remainder.stopBlinking();
  led_remainder.off();

  led_timeout.stopBlinking();
  led_timeout.off();
  //TODO Add third LED if we want to implement it

  // Show reset on display.
  display.setSegments(ssd_seg_reset);

  // Reset stop watch.
  stop_watch.reset();
}


// Query stop watch time and display it as "MM:SS".
unsigned int updateDisplayTime()
{
  const unsigned int elapsed_sec = stop_watch.elapsed() / 1000;
  const unsigned int sec = elapsed_sec % 60;
  const unsigned int min = elapsed_sec / 60;
  display.displayTime(min, sec);
  return elapsed_sec;
}

// Ensures that the LEDs are flashing.
void warnSlotExceeded()
{
  if (!led_blinking_started)
  {
    //TODO Add third LED if we want to implement it
    led_blinking_started = true;
    led_remainder.startBlinking(DURATION_BLINKING);
    led_timeout.startBlinking(DURATION_BLINKING);
  }

  // Only blink as long as the stop watch is active/speaker
  // is still talking.
  if (stop_watch.isRunning())
  {
    led_remainder.blink();
    led_timeout.blink();
  }
  else
  {
    led_remainder.off();
    led_timeout.off();
  }
}

// Set LEDs according to talk progress.
void updateLEDs(unsigned int elapsed_sec)
{
  if (talk_in_progress)
  {
    if (!DURATION_PRESENTATION_SLOT)
    {
      // LEDs are disabled.
      //TODO Add third LED if we want to implement it
      led_remainder.off();
      led_timeout.off();
    }
    else
    {
      if (elapsed_sec < DURATION_PRESENTATION_SLOT)
      {
        //TODO Add third LED if we want to implement it (always on if stop watch.isRunning())
        if (elapsed_sec >= (DURATION_PRESENTATION_SLOT - TAU_REMAINDER))
          led_remainder.on();
      }
      else
      {
        if (elapsed_sec < (DURATION_PRESENTATION_SLOT + TAU_EXCEED))
          led_timeout.on();
        else
          warnSlotExceeded();
      }
    }
  }
  else
  {
    // Turn the lights off
    //TODO Add third LED if we want to implement it
    led_remainder.off();
    led_timeout.off();
  }
}

// Main loop.
void loop()
{
  // Update button states.
  btn_toggle.read();
  btn_reset.read();

  // Handle start/pause/stop.
  if (btn_toggle.changedToPressed())
  {
    talk_in_progress = true;
    stop_watch.toggle();
    //TODO remove
    led_remainder.toggle();
    //TODO add progress LED if we want to implement it
  }

  
  if (btn_reset.isHeld())
  {
    scw_reset();
  }

  if (talk_in_progress)
  {
    // Display talk duration & light up LEDs accordingly.
    const unsigned int elapsed_sec = updateDisplayTime();
    updateLEDs(elapsed_sec);
  }
}

