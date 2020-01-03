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
// 2019-12-29 Parameters set for final VWA presentations 2019/2020.

// Duration of presentation slot in seconds.
// Set to 0 to disable notification LEDs.
#define DURATION_PRESENTATION_SLOT    420

// Light up first notification LED TAU_REMAINDER seconds before
// the end of the allocated slot to indicate that "the end is near".
//
// You have to ensure that TAU_REMAINDER < DURATION_PRESENTATION_SLOT.
#define TAU_REMAINDER  60

// Start blinking LEDs if speaker exceeds more than
// TAU_EXCEED seconds of the allocated presentation slot.
//
// For example, 10 seconds after slot should've ended.
#define TAU_EXCEED     10

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
#define SSD_BRIGHTNESS        7

// Delay in microseconds between bit transition of TM1637
#define SSD_BIT_DELAY 150

// Enable to log debug messages on the serial monitor.
//#define DEBUG_OUTPUT

// Enable if we wired up a third LED which lights up whenever the stop watch
// is running.
#define USE_STOP_WATCH_LED

// We want to query the previously shown data (see reset/display flashing
// implemented for PROGSTATE_RESETTING).
#define SSD_STORE_SEGMENTS

// How often should the display flash the previously timed duration
// upon resetting?
#ifdef SSD_STORE_SEGMENTS
#define FLASH_PREV_DISPLAY_N_TIMES 3
#endif // SSD_STORE_SEGMENTS


//-----------------------------------------------------------------------------
// Include external utilities for slightly cleaner code:

#include <BtButton.h>
#include <BtLED.h>
#include <BtStopWatch.h>
// You must ensure that SSD_STORE_SEGMENTS is also defined in the following
// header (just defining it in the .ino file leads to linking errors because
// arduino doesn't seem to update the previously built library (*.o?) files).
#include <BtSevenSegmentDisplay.h>


//-----------------------------------------------------------------------------
// Program variables:

// Reset button to turn off all LEDs & stop watch.
BtButton btn_reset(PIN_BTN_RESET, BTN_DEBOUNCE_MILLIS, BTN_RESET_HOLD);

// Button to toggle stop watch (start/pause/stop).
BtButton btn_toggle(PIN_BTN_TOGGLE_WATCH, BTN_DEBOUNCE_MILLIS);

// Notification LED indicating that the slot will be over soon.
BtLED led_remainder(PIN_LED_REMAINDER);

// Notification LED indicating that the slot is over/has been exceeded.
BtLED led_timeout(PIN_LED_TIMEOUT);

// Notification LED indicating whether the stop watch is currently
// running (or paused).
#ifdef USE_STOP_WATCH_LED
BtLED led_stop_watch(PIN_LED_STOP_WATCH);
#endif // USE_STOP_WATCH_LED

// The 4-digit 7-segment display to show the elapsed time.
BtSevenSegmentDisplayTM1637 display(PIN_SSD_CLK, PIN_SSD_DIO, SSD_BRIGHTNESS, SSD_BIT_DELAY);

// Should be obvious...
BtStopWatch stop_watch;

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


// We have three states:
// 1) Resetting (previously displayed time flashes if SSD_STORE_SEGMENTS is #define'd)
#define PROGSTATE_RESETTING 0x00
// 2) Idle (we're reset, showing the "--:--" display)
#define PROGSTATE_IDLE 0x01
// 3) Talk in progress (speaker is presenting or has finished)
#define PROGSTATE_TALK 0x02
// Variable to store the program's current state:
uint8_t program_state = PROGSTATE_IDLE;

// We use the non-blocking LED blink calls, thus we need to keep track
// of whether we already started blinking or not...
bool led_blinking_started = false;

// If we want the display to flash upon resetting, we need to store the
// previously shown segments:
#ifdef SSD_STORE_SEGMENTS
uint8_t ssd_previous_segments[4];

// We need to remember what we display during flashing (otherwise, the digital tube
// may start flickering because we "displayX()" too often).
#define SSD_RESET
#define SSD_RESET_BLANK 0x00
#define SSD_RESET_PREVDISP  0x01
uint8_t ssd_reset_currently_shown = SSD_RESET_BLANK;
#endif // SSD_STORE_SEGMENTS



// Initialization.
void setup()
{
  // Blue and red LEDs are pretty bright, so dim them down.
  led_remainder.setDimValue(128);
  led_timeout.setDimValue(128);

  // Start by showing "--:--"
  display.setSegments(ssd_seg_reset);
  delay(100);
  
  startResetting();
  
#ifdef DEBUG_OUTPUT
  Serial.begin(9600);
#endif // DEBUG_OUTPUT
}


// Switch state to resetting.
void startResetting()
{
  // Reset variables
  program_state = PROGSTATE_RESETTING;
  led_blinking_started = false;
  stop_watch.reset();
  stop_watch.start();

  // Set to invalid number (which doesn't fit on the
  // 7-segment display).
  prev_elapsed_sec = 10000;

    // Turn on all lights during resetting.
  led_remainder.on();
  led_timeout.on();
#ifdef USE_STOP_WATCH_LED
  led_stop_watch.on();
#endif // USE_STOP_WATCH_LED

#ifdef SSD_STORE_SEGMENTS
  // Store the currently shown segments.
  display.getSegments(ssd_previous_segments);
  ssd_reset_currently_shown = SSD_RESET_BLANK;
#endif // SSD_STORE_SEGMENTS
  display.setSegments(ssd_seg_reset);
}

void loopResetting()
{
  const unsigned long elapsed_ms = stop_watch.elapsed();
  bool visual_reset_finished = false;
  
#ifdef SSD_STORE_SEGMENTS
  // How many times did we already flash the display?
  const unsigned int num_flashed = elapsed_ms / 1000;
  if (num_flashed < FLASH_PREV_DISPLAY_N_TIMES)
  {
    // We want to show "--:--" for ~500 ms and the previously
    // displayed time for ~500ms each second:
    const unsigned int mod = elapsed_ms % 1000;
    if (mod < 500)
    {
      // Only update the display if we need to change it actually:
      if (ssd_reset_currently_shown != SSD_RESET_BLANK)
      {
        display.setSegments(ssd_seg_reset);
        ssd_reset_currently_shown = SSD_RESET_BLANK;
      }
    }
    else
    {
      // Again, prevent updating the display too often/fast:
      if (ssd_reset_currently_shown != SSD_RESET_PREVDISP)
      {
        display.setSegments(ssd_previous_segments);
        ssd_reset_currently_shown = SSD_RESET_PREVDISP;
      }
    }
  }
  else
  {
    visual_reset_finished = true;
  }
#else
  visual_reset_finished = elapsed_ms >= 1000;
#endif

  if (visual_reset_finished)
    finishResetting(true);
}


// Turn off lights, stop flashing, prepare variables after
// the reset procedure has finished
void finishResetting(bool timed_out)
{
  // Turn off all LEDs.
  // "stopBlinking" implicitly calls off(), so it even works for
  // non-blinking LEDs.
  led_remainder.stopBlinking();
  led_timeout.stopBlinking();
#ifdef USE_STOP_WATCH_LED
  led_stop_watch.off();
#endif // USE_STOP_WATCH_LED

  if (timed_out)
  {
    // Only show reset "--:--" on display if the resetting procedure timed out.
    // Otherwise, the user requested to start the stop watch immediately.
    display.setSegments(ssd_seg_reset);
  }

  // Reset stop watch.
  stop_watch.reset();

  // Set proper state.
  program_state = PROGSTATE_IDLE;
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
      led_remainder.off();
      led_remainder.startBlinking(DURATION_BLINKING);
      led_timeout.on();
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
  if (program_state == PROGSTATE_TALK)
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
}

// Main loop.
void loop()
{  
  // Update button states.
  btn_reset.read();
  btn_toggle.read();
  
  // Check if we should reset the session chair's watch.
  if (btn_reset.isHeld())
  {
    // We only need to reset if we're in the TALK state (i.e.
    // speaker is currently presenting or has finished).
    // Otherwise, the display will already/still show "--:--" and
    // all relevant variables will be reset/initialized.
    if (program_state == PROGSTATE_TALK)
      startResetting();
  }

  // Handle start/pause/stop.
  if (btn_toggle.changedToPressed())
  {
    // If we're currently in the resetting procedure (e.g. still flashing the 
    // digital tube), stop doing so:
    if (program_state == PROGSTATE_RESETTING)
      finishResetting(false);

    program_state = PROGSTATE_TALK;
    stop_watch.toggle();
  }

  if (program_state == PROGSTATE_RESETTING)
  {
    loopResetting();
  }
  else if (program_state == PROGSTATE_TALK)
  {
    // Update the displayed presentation time.
    const unsigned int elapsed_sec = updateDisplayTime();
    updateLEDs(elapsed_sec);
  }  
}
