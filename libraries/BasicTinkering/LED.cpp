#include <Arduino.h>
#include <LED.h>

// Bit numbers for state variable
#define STATE_STATUS  0
#define STATE_BLINK   1


LED::LED(uint8_t pin) : pin_(pin), state_(0x00), dim_value_(0x00)
{
  pinMode(pin_, OUTPUT);
}

bool LED::status() const
{
  return bitRead(state_, STATE_STATUS);
}

void LED::on()
{
  if (dim_value_)
  {
    setValue(dim_value_);
  }
  else
  {
    digitalWrite(pin_, HIGH);
    bitWrite(state_, STATE_STATUS, HIGH);
  }
}

void LED::off()
{
  digitalWrite(pin_, LOW);
  bitWrite(state_, STATE_STATUS, LOW);
}

void LED::toggle()
{
  bitRead(state_, STATE_STATUS) ? off() : on();
}

void LED::startBlinking(unsigned int time)
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
  {
    toggle();
    blink_start_ = now;
  }
}

void LED::stopBlinking()
{
  off();
  bitWrite(state_, STATE_BLINK, 0);
}

void LED::blockingBlink(unsigned int time, unsigned int n)
{
  if (n == 0)
    return;
  on();
  for (unsigned int i = 0; i < 2*n-1; ++i)
  {
    delay(time);
    toggle();
  }
  delay(time);
  off();
}

void LED::setValue(uint8_t value)
{
  analogWrite(pin_, value);
  bitWrite(state_, STATE_STATUS, value > 127);
}

void LED::setDimValue(uint8_t value)
{
  dim_value_ = value;
}

void LED::fadeIn(unsigned int time)
{
  const unsigned int dt = time / (255 / 5);
  for (uint8_t brightness = 0; brightness < 255; brightness+=5)
  {
    setValue(brightness);
    delay(dt);
  }
  on();
}

void LED::fadeOut(unsigned int time)
{
  const unsigned int dt = time / (255 / 5);
  for (uint8_t brightness = 255; brightness > 0; brightness-=5)
  {
    setValue(brightness);
    delay(dt);
  }
  off();
}
