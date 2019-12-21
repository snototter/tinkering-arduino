#include <Arduino.h>
#include <LED.h>

// Bit numbers for state variable
#define STATE_STATUS  0
#define STATE_BLINK   1


LED::LED(uint8_t pin) : pin_(pin), state_(0x00)
{
  pinMode(pin_, OUTPUT);
}

bool LED::status() const
{
  return bitRead(state_, STATE_STATUS);
}

void LED::on()
{
  digitalWrite(pin_, HIGH);
  bitWrite(state_, STATE_STATUS, HIGH);
}

void LED::off()
{
  digitalWrite(pin_, LOW);
  bitWrite(state_, STATE_STATUS, LOW);
}

void toggle()
{
  bitRead(state_, STATE_STATUS) ? off() : on();
}

void LED::start_blinking(unsigned int time)
{
  bitWrite(state_, STATE_BLINK, 1);
  blink_time_ = time;
  blink_start_ = millis();
  toggle();
}

void LED::blink()
{
  if (!bitRead(state_, STATE_BLINK))
    return;
  const unsigned long now = millis();
  if ((now - blink_start_) >= blink_time_)
    toggle();
}

void LED::stop_blinking()
{
  off();
  bitWrite(state_, STATE_BLINK, 0);
}
