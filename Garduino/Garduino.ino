
// TODO/FIXME
// * Try to recover the correct Low-Power library version. Garduino cannot
//   be built using the latest versions (the SLEEP_MODE... definitions seem
//   to have changed).
//
// Description:
// An automated plant watering system:
// * Initially, the amount of water was calculated based on
//   the readings of a moisture sensor. Since this sensor broke after
//   a few months (all hail to oxidation), the moisture check is now optional
//   and must be enabled using a #define.
// * Water pump is protected from running dry using a water level sensor.
// * Uses a low-power library to save as much energy as possible.
// * Buttons are hardware-debounced (using a Schmitt trigger)
// Required libraries:
// * Low-Power by Rocket Scream, see https://github.com/rocketscream/Low-Power
//
// Note this was my first tinkering project (going beyond some simple hello-world-
// LED example). Additionally, I had to restore this source file from my disk backup.
// So be sure to read the source carefully and test the code before deploying it!
// Actually, it's better/less error-prone to built a new system from scratch :-p

// External library
#include <LowPower.h>

#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

// Based on all the stuff I've read, I'm pretty sure that we should use the 
// INTERNAL analog reference (i.e. band gap voltage of 1.1 volts). The DEFAULT 
// reference (5V) might fluctuate with decreasing battery levels (whereas the 
// band gap voltage should stay constant).
// If we use the 1.1 V reference for the ADC, we also need a voltage divider 
// for the moisture sensor (switching the reference takes a while, i.e. 
// multiple analogReads afterwards until we get a stable reading, so we only 
// set the reference voltage once).
// If you're sure you want to use the DEFAULT 5V reference, uncomment this flag:
//#define GARDUINO_REF_VOLTAGE5V

// Allow debug output?
//#define GARDUINO_DEBUG

// Quick and dirty: my moisture sensor broke, so only GARDUINO_NO_MOISTURE_CHECK is
// used now. The obsolete GARDUINO_MOISTURE_CHECK flag was used to debug the moisture
// sensor. This leads to pretty ugly #ifdef's below (may be refactored one day in the
// very distant future).
//#define GARDUINO_MOISTURE_CHECK // Debug the moisture sensor
#define GARDUINO_NO_MOISTURE_CHECK // Disable moisture check if sensor is worn out

// Interrupt pin to trigger watering (manual push button).
const int pin_water_trigger = 2;

// Output pin to control the pump
const int pin_pump = 3;

// Status LEDs
const int pin_led_battery_green  =  4;
const int pin_led_battery_yellow  = 5;
const int pin_led_battery_red     = 6;
const int pin_led_moisture_high   = 7;
const int pin_led_moisture_low    = 8;
const int pin_led_moisture_medium = 9;
const int pin_led_reservoir_green = 10;
const int pin_led_reservoir_red   = 11;
const int pin_led_watering        = 12;

// Moisture sensor
const int pin_moisture_gnd = A1;
const int pin_moisture_vcc = A0;
const int pin_moisture_sig = A2;

// Switch to indicate water reservoir level
const int pin_water_gauge = A3;

// Voltage divider for battery status
const int pin_battery_gauge = A7;

// Counter indicating the current loop() iteration
int current_loop_iteration;
// How many iterations should we sleep before quering the status? (<wait time>/8 sec(i.e. watchdog timeout))
const int sleep_iterations = 1800; // Check once every 4 hours

// Flag set by ISR
volatile bool flag_water_triggered = false;

// Flag to indicate we're still in the startup (test LEDs)
bool flag_led_test = true;

// Flag to indicate the water level
volatile bool flag_water_reservoir_full = false;

// Moisture level (also used as multiplier for watering period)
int moisture_level;

// Set voltage divider variables depending on the chosen reference voltage.
#if defined(GARDUINO_REF_VOLTAGE5V)
const float voltage_divider_factor = (5.0 / 1023.0);
const float voltage_divider_factor_moisture = voltage_divider_factor;
#else
const float voltage_divider_factor = (1.1 / 1023.0);
const float voltage_divider_factor_moisture = voltage_divider_factor * (1.0/0.176);
#endif
const float voltage_divider_factor_battery = voltage_divider_factor * (12.0 / 1.091); // 12 V at battery => 1.091 V at ADC


void setup() 
{
#ifdef GARDUINO_DEBUG
  Serial.begin(9600);
#endif

  // Setup interrupt for watering
  pinMode(pin_water_trigger, INPUT_PULLUP);
  attachInterrupt(0, isr_water, RISING);

  // Setup pump
  pinMode(pin_pump, OUTPUT);

  // Setup status LEDs
  pinMode(pin_led_battery_green, OUTPUT);
  pinMode(pin_led_battery_yellow, OUTPUT);
  pinMode(pin_led_battery_red, OUTPUT);
  pinMode(pin_led_moisture_high, OUTPUT);
  pinMode(pin_led_moisture_medium, OUTPUT);
  pinMode(pin_led_moisture_low, OUTPUT);
  pinMode(pin_led_reservoir_green, OUTPUT);
  pinMode(pin_led_reservoir_red, OUTPUT);
  pinMode(pin_led_watering, OUTPUT);

  // Upon startup, light up all LEDs as function test
#ifdef GARDUINO_DEBUG
      Serial.println("Status LED test");
#endif 
  digitalWrite(pin_led_battery_green, HIGH);
  digitalWrite(pin_led_battery_yellow, HIGH);
  digitalWrite(pin_led_battery_red, HIGH);
  digitalWrite(pin_led_moisture_high, HIGH);
  digitalWrite(pin_led_moisture_medium, HIGH);
  digitalWrite(pin_led_moisture_low, HIGH);
  digitalWrite(pin_led_reservoir_green, HIGH);
  digitalWrite(pin_led_reservoir_red, HIGH);
  digitalWrite(pin_led_watering, HIGH);
  flag_led_test = true;

  // Set up moisture sensor
  pinMode(pin_moisture_gnd, OUTPUT);
  pinMode(pin_moisture_vcc, OUTPUT);
  pinMode(pin_moisture_sig, INPUT);
  
  // Shut down moisture sensor
  digitalWrite(pin_moisture_gnd, LOW);
  digitalWrite(pin_moisture_vcc, LOW);
  moisture_level = 0;
  
  // Water gauge
  pinMode(pin_water_gauge, INPUT_PULLUP);

  // Set up battery voltage meter
  pinMode(pin_battery_gauge, INPUT);
#if defined(GARDUINO_REF_VOLTAGE5V)
  analogReference(DEFAULT);
#else
  analogReference(INTERNAL);
#endif

  current_loop_iteration = 0; 
}

// The main loop runs over and over again forever
void loop() 
{
#ifdef GARDUINO_MOISTURE_CHECK
  //***************************************************
  // Check plant's soil moisture
  // Turn the sensor on, read, print
  digitalWrite(pin_moisture_vcc, HIGH);
  delay_ms(20); // Let everything settle
  float val = 0.0;
  for (int i = 0; i < 10; ++i)
  {
    delay_ms(100);
    val = analogRead(pin_moisture_sig) * voltage_divider_factor_moisture;
    Serial.print("Moisture: ");
    Serial.println(val);
  }
  delay_ms(400);
#else // GARDUINO_MOISTURE_CHECK  
  if (flag_led_test)
  {
    // Turn off LEDs after 8 seconds (1 watchdog timeout)
    if (current_loop_iteration == 1)
    {
      flag_led_test = false;
      current_loop_iteration = sleep_iterations; // Trick loop to refresh sensor values
      digitalWrite(pin_led_battery_green, LOW);
      digitalWrite(pin_led_battery_yellow, LOW);
      digitalWrite(pin_led_battery_red, LOW);
      digitalWrite(pin_led_moisture_high, LOW);
      digitalWrite(pin_led_moisture_medium, LOW);
      digitalWrite(pin_led_moisture_low, LOW);
      digitalWrite(pin_led_reservoir_green, LOW);
      digitalWrite(pin_led_reservoir_red, LOW);
      digitalWrite(pin_led_watering, LOW);
      // Dummy reads to let ADC settle
      for (int i = 0; i < 10; ++i)
        analogRead(pin_battery_gauge);
    }
  }
  if (!flag_led_test)
  {
    if (current_loop_iteration == sleep_iterations || flag_water_triggered)
    {
#ifdef GARDUINO_DEBUG
      if (flag_water_triggered)
        Serial.println("Woke on trigger");
      else
        Serial.println("Woke on WDT");
#endif // GARDUINO_DEBUG     
      
      //***************************************************
      // Check battery voltage
      float val = 0.0;
      for (int i = 0; i < 30; ++i)
      {
        delay_ms(20); // Maybe this solves our query problem? (battery gauge often shows empty, despite full battery voltage)
        val += analogRead(pin_battery_gauge);
      }
      val /= 30.0;
      float vcc_voltage = val * voltage_divider_factor_battery;
      if (vcc_voltage >= 11.5)
      {
        digitalWrite(pin_led_battery_red, LOW);
        digitalWrite(pin_led_battery_yellow, LOW);
        digitalWrite(pin_led_battery_green, HIGH);
      }
      else
      {
        if (vcc_voltage >= 11.0)
        {
          digitalWrite(pin_led_battery_red, LOW);
          digitalWrite(pin_led_battery_yellow, HIGH);
          digitalWrite(pin_led_battery_green, LOW);
        }
        else
        {
          digitalWrite(pin_led_battery_red, HIGH);
          digitalWrite(pin_led_battery_yellow, LOW);
          digitalWrite(pin_led_battery_green, LOW);
        }
      }
#ifdef GARDUINO_DEBUG
      Serial.print("Voltage: ");
      Serial.println(vcc_voltage);
#endif
    
    
      //***************************************************
      // Check water reservoir
      flag_water_reservoir_full = digitalRead(pin_water_gauge);
      if (flag_water_reservoir_full)
      {
        digitalWrite(pin_led_reservoir_green, HIGH);
        digitalWrite(pin_led_reservoir_red, LOW);
      }
      else
      {
        digitalWrite(pin_led_reservoir_green, LOW);
        digitalWrite(pin_led_reservoir_red, HIGH);
      }
#ifdef GARDUINO_DEBUG
      Serial.print("Water reservoir: ");
      Serial.println(flag_water_reservoir_full);
#endif
    
    
      //***************************************************
      // Check plant's soil moisture
#ifdef GARDUINO_NO_MOISTURE_CHECK
      // Assume dry soil if no moisture sensor is available:
      moisture_level = 0;
#else // GARDUINO_NO_MOISTURE_CHECK
      // Turn the sensor on, read, turn off
      digitalWrite(pin_moisture_vcc, HIGH);
      // Let everything settle for short period, then average a few readings
      // to get a less noisy measurement:
      delay_ms(20);
      val = 0.0;
      for (int i = 0; i < 10; ++i)
      {
        delay_ms(20);
        val += analogRead(pin_moisture_sig);
      }
      val /= 10.0;
      digitalWrite(pin_moisture_vcc, LOW);
      float moisture_voltage = val * voltage_divider_factor_moisture;

      // Threshold moisture voltage (empirically set using GARDUINO_MOISTURE_CHECK,
      // some examples of dry/wet soil, a glass of water and sweaty fingers,...)
      if (moisture_voltage >= 2.6)
      {
        digitalWrite(pin_led_moisture_high, HIGH);
        digitalWrite(pin_led_moisture_medium, HIGH);
        digitalWrite(pin_led_moisture_low, HIGH);
        moisture_level = 3;
      }
      else
      {
        if (moisture_voltage >= 1.5)
        {
          digitalWrite(pin_led_moisture_high, LOW);
          digitalWrite(pin_led_moisture_medium, HIGH);
          digitalWrite(pin_led_moisture_low, HIGH);
          moisture_level = 2;
        }
        else
        {
          if (moisture_voltage >= 0.5)
          {
            digitalWrite(pin_led_moisture_high, LOW);
            digitalWrite(pin_led_moisture_medium, LOW);
            digitalWrite(pin_led_moisture_low, HIGH);
            moisture_level = 1;
          }
          else
          {
            digitalWrite(pin_led_moisture_high, LOW);
            digitalWrite(pin_led_moisture_medium, LOW);
            digitalWrite(pin_led_moisture_low, LOW);
            moisture_level = 0;
          }
        }
      }
  #ifdef GARDUINO_DEBUG  
      Serial.print("Moisture: ");
      Serial.println(moisture_voltage);
  #endif
#endif // GARDUINO_NO_MOISTURE_CHECK
    
      // Water the plants if the button was triggered or the soil is dry.
      if (flag_water_triggered || moisture_level < 3)
        water();  
      current_loop_iteration = 0; // Restart loop counter
    }
  }
  ++current_loop_iteration;
  flag_water_triggered = false; // Reset flag (if set)
  goToSleep(); 
#endif // GARDUINO_MOISTURE_CHECK
}

void water()
{
  //TODO input param: moisture level (low: 2-3xwatering time)
  if (flag_water_reservoir_full)
  {
#ifdef GARDUINO_NO_MOISTURE_CHECK
    int duration = 6000; // Check, 4 seconds is too short, 8 too long (overflows the smaller pots) - summer'17
#else // GARDUINO_NO_MOISTURE_CHECK
    //int duration = 3000 * (3-moisture_level); // driest: 9sec, dry: 6sec, moist: 3sec, wet: 0sec; 
    //int duration = 6000 * (4-moisture_level); // wet: 6s, moist: 12s, dry: 18s, driest: 24s;
    int duration = 4000 * (3-moisture_level); // wet: 0s, moist: 4s, dry: 8s, driest: 12s;
#endif // GARDUINO_NO_MOISTURE_CHECK
#ifdef GARDUINO_DEBUG
    Serial.print("[INFO] Watering the plants ");
    Serial.print(duration/1000);
    Serial.println(" sec");
    Serial.flush();
#endif
    bool keep_watering = true;
    // We don't want to be interrupted while pumping
    noInterrupts();
    // Turn on pump & indicator LED
    digitalWrite(pin_led_watering, HIGH);
    digitalWrite(pin_pump, HIGH);
    // Re-check water reservoir every 250ms
    for (int time_it = 0; keep_watering && time_it < duration; time_it+=250)
    {
      delay_ms(250);
      flag_water_reservoir_full = digitalRead(pin_water_gauge);
      if (!flag_water_reservoir_full)
      {
        keep_watering = false;
        digitalWrite(pin_led_reservoir_green, LOW);
        digitalWrite(pin_led_reservoir_red, HIGH);
      }
      //delay_ms(pump_base_duration*(4-moisture_level));
    }
    // Turn off pump and LED
    digitalWrite(pin_pump, LOW);
    digitalWrite(pin_led_watering, LOW);
    // Clear flag for interrupt 0 (in case it was enqueued in the meantime)
    EIFR = bit(INTF0); 
    // Reenable interrupts
    interrupts();
#ifdef GARDUINO_DEBUG
    if (!keep_watering)
      Serial.println("[ERROR] Ran out of water supply during watering");
#endif
  }
  else
  {
#ifdef GARDUINO_DEBUG
    Serial.println("[ERROR] Water reservoir empty - won't start watering");
#endif
  }
}

void isr_water()
{
  // ISR will be executed, sleep mode terminates and main loop continues: thus, indicate interrupt via flag
  flag_water_triggered = true;
}

void goToSleep()
{ 
#ifdef GARDUINO_DEBUG
  // Required to prevent weird ASCII symbols popping up in our serial monitor...
  Serial.flush();
  Serial.end();
#endif
  /*power_all_disable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // Turn off Brown-out detection
  MCUCR = bit(BODS) | bit(BODSE);
  MCUCR = bit(BODS);
  
  // Go to sleep
  sleep_cpu();
  
  // Program will continue from here after the WDT timeout/interrupt
  sleep_disable(); // First thing to do is disable sleep
  
  // Re-enable the peripherals
  power_all_enable();*/

  // Above stuff can be replaced by:
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
#ifdef GARDUINO_DEBUG
  Serial.begin(9600);
#endif
}

void delay_ms(unsigned int time) 
{ 
  while (time--) 
    _delay_ms(1); 
} 

