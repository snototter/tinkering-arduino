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
// Utilities for slightly cleaner code:
#include <Button.h>
#include <LED.h>
#include <StopWatch.h>

StopWatch stop_watch;

void setup()
{
  // Initialization (called after initializing the globals,
  // before first loop iteration).
}

void loop()
{
}

