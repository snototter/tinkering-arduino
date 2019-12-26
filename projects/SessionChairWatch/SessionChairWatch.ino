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
// TODO Configure for VWA 2019/2020.

// Duration of presentation slot in seconds.
// Set to 0 to disable notification LEDs.
#define DURATION_PRESENTATION_SLOT    4

// Light up first notification LED TAU_REMAINDER seconds before
// the end of the allocated slot to indicate that "the end is near".
//
// You have to ensure that TAU_REMAINDER < DURATION_PRESENTATION_SLOT.
#define TAU_REMAINDER  2

// Start blinking LEDs if speaker exceeds more than
// TAU_EXCEED seconds of the allocated presentation slot.
//
// For example, 10 seconds after slot should've ended.
#define TAU_EXCEED     2

// How many milliseconds the LED should be on/off when blinking, i.e.
// a full blink cycle would be 2*DURATION_BLINKING milliseconds.
#define DURATION_BLINKING 500


//-----------------------------------------------------------------------------
// Wiring configuration.

// 7-segment display.
#define PIN_SSD_CLK           2
#define PIN_SSD_DIO           3

// LEDs (on PWM pins).
#define PIN_LED_STOP_WATCH    6
#define PIN_LED_REMAINDER     9
#define PIN_LED_TIMEOUT       11

// Buttons.
#define PIN_BTN_RESET         4
#define PIN_BTN_TOGGLE_WATCH  5


//-----------------------------------------------------------------------------
// Misc.

// Debounce time in milliseconds.
#define BTN_DEBOUNCE_MILLIS   50

// Force reset button to be held longer before resetting to prevent accidental
// resets ;-)
#define BTN_RESET_HOLD        500

// 7-segment display brightness [0,7].
#define SSD_BRIGHTNESS        3

// Delay in microseconds between bit transition of TM1637
#define SSD_BIT_DELAY 150

// Enable to log debug messages on the serial monitor.
//#define DEBUG_OUTPUT

// Enable if we wired up a third LED which lights up whenever the stop watch
// is running.
#define USE_STOP_WATCH_LED


//-----------------------------------------------------------------------------
// Include external utilities for slightly cleaner code:

#include <Button.h>
#include <LED.h>
#include <StopWatch.h>

// We want to query the previously shown data (see reset/display flashing
// implemented in scw_reset()).
#define SSD_STORE_SEGMENTS
#include <SevenSegmentDisplay.h>


//-----------------------------------------------------------------------------
// Program variables:

// Reset button to turn off all LEDs & stop watch.
Button btn_reset(PIN_BTN_RESET, BTN_DEBOUNCE_MILLIS, BTN_RESET_HOLD);

// Button to toggle stop watch (start/pause/stop).
Button btn_toggle(PIN_BTN_TOGGLE_WATCH, BTN_DEBOUNCE_MILLIS);

// Notification LED indicating that the slot will be over soon.
LED led_remainder(PIN_LED_REMAINDER);

// Notification LED indicating that the slot is over/has been exceeded.
LED led_timeout(PIN_LED_TIMEOUT);

// Notification LED indicating whether the stop watch is currently
// running (or paused).
#ifdef USE_STOP_WATCH_LED
LED led_stop_watch(PIN_LED_STOP_WATCH);
#endif // USE_STOP_WATCH_LED

// The 4-digit 7-segment display to show the elapsed time.
SevenSegmentDisplayTM1637 display(PIN_SSD_CLK, PIN_SSD_DIO, SSD_BRIGHTNESS, SSD_BIT_DELAY);

// Should be obvious...
StopWatch stop_watch;

// The previously displayed presentation time.
unsigned int prev_elapsed_sec;

// Raw 7-segment display data to show "--:--", i.e. we're in reset/init state.
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
  // Blue and red LEDs are pretty bright, so dim them down.
  led_remainder.setDimValue(128);
  led_timeout.setDimValue(64);
  
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

  // Turn on all lights for a short amount of time.
  led_remainder.on();
  led_timeout.on();
#ifdef USE_STOP_WATCH_LED
  led_stop_watch.on();
#endif // USE_STOP_WATCH_LED

#ifdef SSD_STORE_SEGMENTS
  // Flash the previously shown data on the display twice before resetting.
  uint8_t segments[4];
  display.getSegments(segments);
  for (uint8_t i = 0; i < 2; ++i)
  {
    display.setSegments(ssd_seg_reset);
    delay(300);
    display.setSegments(segments);
    delay(500);
  }
#else // SSD_STORE_SEGMENTS
  // If we cannot query the currently displayed data, leave the
  // display as is.
  delay(1000);
#endif // SSD_STORE_SEGMENTS
  
  // Turn off all LEDs.
  // "stopBlinking" implicitly calls off(), so it even works for
  // non-blinking LEDs.
  led_remainder.stopBlinking();
  led_timeout.stopBlinking();

#ifdef USE_STOP_WATCH_LED
  led_stop_watch.off();
#endif // USE_STOP_WATCH_LED

  // Show reset on display.
  display.setSegments(ssd_seg_reset);

  // Reset stop watch.
  stop_watch.reset();

  // Set to invalid number (which doesn't fit on the
  // 7-segment display).
  prev_elapsed_sec = 10000;
}


// Query stop watch time and display it as "MM:SS".
unsigned int updateDisplayTime()
{
  const unsigned int elapsed_sec = stop_watch.elapsed() / 1000;
  // Update display only if time has changed.
  if (elapsed_sec != prev_elapsed_sec)
  {
    prev_elapsed_sec = elapsed_sec;
    const unsigned int sec = elapsed_sec % 60;
    const unsigned int min = elapsed_sec / 60;
    display.displayTime(min, sec);
  }
  return elapsed_sec;
}


// Ensures that the notification LEDs are (non-blocking) blinking.
void warnSlotExceeded()
{
  // Only blink as long as the stop watch is active, i.e. 
  // speaker is still talking.
  if (stop_watch.isRunning())
  {
    if (!led_blinking_started)
    {
      // Configure LEDs such that they alternate.
      led_blinking_started = true;
      led_remainder.on();
      led_remainder.startBlinking(DURATION_BLINKING);
      led_timeout.off();
      led_timeout.startBlinking(DURATION_BLINKING);
    }
    led_remainder.blink();
    led_timeout.blink();
  }
  else
  {
    // Stop watch has been paused, turn off lights...
    led_remainder.off();
    led_timeout.off();
    // ... however, if the talk continues, we want to
    // alternate the LEDs again.
    led_blinking_started = false;
  }
}

// Set LEDs according to talk progress.
void updateLEDs(unsigned int elapsed_sec)
{
  if (talk_in_progress)
  {
#ifdef USE_STOP_WATCH_LED
    // LED "led_stop_watch" indicates whether the stop
    // watch is currently running.
    if (stop_watch.isRunning())
      led_stop_watch.on();
    else
      led_stop_watch.off();
#endif // USE_STOP_WATCH_LED

    if (!DURATION_PRESENTATION_SLOT)
    {
      // Notification LEDs are disabled.
      led_remainder.off();
      led_timeout.off();
    }
    else
    {
      // Check if the elapsed time is close to the end of the slot or
      // if the speaker already exceeded his allocated slot.
      if (elapsed_sec < DURATION_PRESENTATION_SLOT)
      {
        // Light up notification LED ("hurry up").
        if (elapsed_sec >= (DURATION_PRESENTATION_SLOT - TAU_REMAINDER))
          led_remainder.on();
      }
      else
      {
        // Light up second notification LED ("Time is over") or start
        // flashing once grace period is over.
        if (elapsed_sec < (DURATION_PRESENTATION_SLOT + TAU_EXCEED))
        {
          led_timeout.on();
          led_remainder.off();
        }
        else
          warnSlotExceeded();
      }
    }
  }
  /* TODO test (reset uses scw_reset() which already turns off all LEDs, so
   *  no need to do it here again).
  else
  {
    // We're in reset/init state, so turn off all the LEDs.
    led_remainder.off();
    led_timeout.off();
#ifdef USE_STOP_WATCH_LED
    led_stop_watch.off();
#endif // USE_STOP_WATCH_LED
  }*/
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
  }

  // Check if we should reset the session chair's watch.
  if (btn_reset.isHeld())
  {
    scw_reset();
  }

  // Display talk duration & light up LEDs if speaker is presenting.
  if (talk_in_progress)
  {
    const unsigned int elapsed_sec = updateDisplayTime();
    updateLEDs(elapsed_sec);
  }
}

