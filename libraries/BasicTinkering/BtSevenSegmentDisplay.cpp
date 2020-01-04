#include <Arduino.h>
#include <BtSevenSegmentDisplay.h>

// Lookup segments for digits 0-9, A-F.
// We need gfedcba encoding, see https://en.wikipedia.org/wiki/Seven-segment_display
const uint8_t lookup_digit_segments[] =
{
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6f,
  0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71
};

BtSevenSegmentDisplayTM1637::BtSevenSegmentDisplayTM1637(uint8_t pin_clk,
  uint8_t pin_dio, uint8_t brightness, unsigned long bit_delay) :
  pin_clk_(pin_clk), pin_dio_(pin_dio), bit_delay_(bit_delay)
{
#ifdef SSD_STORE_SEGMENTS
  for (uint8_t i = 0; i < 4; ++i)
    segments_[i] = 0;
#endif // SSD_STORE_SEGMENTS

  setBrightness(brightness);

  // Set up pins (use pull-up to pull CLK and DIO up)
  pinMode(pin_clk_, INPUT);
  digitalWrite(pin_clk_, LOW);
  pinMode(pin_dio_, INPUT);
  digitalWrite(pin_dio_, LOW);
}

void BtSevenSegmentDisplayTM1637::setBrightness(uint8_t brightness)
{
  // 0x08 enables the digit's LED (... | 0x00 would turn the digit off).
  brightness_ = (brightness & 0x07) | 0x08;
}

void BtSevenSegmentDisplayTM1637::clear()
{
  const uint8_t data[] = { 0, 0, 0, 0 };
  setSegments(data);
}


void BtSevenSegmentDisplayTM1637::displayInteger(int x)
{
  // Sanity check
  if (x > 9999 || x < -999)
    return;

  // Make absolute
  const bool negative = x < 0;
  if (negative)
    x = -x;

  uint8_t segments[4];
  uint8_t nlz = 0; // Count number of leading zeroes
  bool skip_zeros = true;
  int div = 1000;
  for (uint8_t i = 0; i < 3; ++i)
  {
    uint8_t digit = x / div;
    if (digit == 0 && skip_zeros)
    {
      ++nlz;
      segments[i] = 0;
    }
    else
    {
      skip_zeros = false;
      segments[i] = digitToSegment(digit);
    }
    if (i < 2)
    {
      x %= div;
      div /= 10;
    }
  }
  segments[3] = digitToSegment(x % 10);
  if (negative)
    segments[nlz-1] = BT_SSD_SEG_G;
  setSegments(segments);
}

void BtSevenSegmentDisplayTM1637::displayTime(uint8_t a, uint8_t b)
{
  if (a > 99 || b > 99)
    return;

  uint8_t segments[4];
  segments[0] = digitToSegment(a / 10);
  segments[1] = digitToSegment(a % 10) | BT_SSD_SEG_COLON;
  segments[2] = digitToSegment(b / 10);
  segments[3] = digitToSegment(b % 10);
  setSegments(segments);
}

void BtSevenSegmentDisplayTM1637::setSegments(const uint8_t segments[], uint8_t num_digits, uint8_t first_pos)
{
  // Send COMM1
  start();
  sendByte(0x40);
  stop();

  // Send COMM2 & address of first digit
  start();
  sendByte(0xC0 + (first_pos & 0x03));
  // Send data
  for (uint8_t i = 0; i < num_digits; ++i)
    sendByte(segments[i]);
  stop();

  // Send COMM3 & LED brightness
  start();
  sendByte(0x80 + (brightness_ & 0x0f));
  stop();

#ifdef SSD_STORE_SEGMENTS
  for (uint8_t i = 0; i < 4; ++i)
    segments_[i] = 0;
  for (uint8_t i = 0; i < num_digits; ++i)
    segments_[i+first_pos] = segments[i];
#endif // SSD_STORE_SEGMENTS
}

uint8_t BtSevenSegmentDisplayTM1637::digitToSegment(uint8_t digit) const
{
  return lookup_digit_segments[digit & 0x0f];
}

#ifdef SSD_STORE_SEGMENTS
void BtSevenSegmentDisplayTM1637::getSegments(uint8_t segments[]) const
{
  for (uint8_t i = 0; i < 4; ++i)
    segments[i] = segments_[i];
}
#endif // SSD_STORE_SEGMENTS

void BtSevenSegmentDisplayTM1637::start() const
{
  pinMode(pin_dio_, OUTPUT);
  bitDelay();
}

void BtSevenSegmentDisplayTM1637::stop() const
{
  pinMode(pin_dio_, OUTPUT);
  bitDelay();
  pinMode(pin_clk_, INPUT);
  bitDelay();
  pinMode(pin_dio_, INPUT);
  bitDelay();
}

bool BtSevenSegmentDisplayTM1637::sendByte(uint8_t byte) const
{
  // Send each bit
  uint8_t data = byte;
  for(uint8_t i = 0; i < 8; ++i)
  {
    // CLK low
    pinMode(pin_clk_, OUTPUT);
    bitDelay();
    // Send bit
    if (data & 0x01)
      pinMode(pin_dio_, INPUT);
    else
      pinMode(pin_dio_, OUTPUT);
    bitDelay();
    // CLK high
    pinMode(pin_clk_, INPUT);
    bitDelay();
    // Next bit
    data = data >> 1;
  }
  // CLK low
  pinMode(pin_clk_, OUTPUT);
  pinMode(pin_dio_, INPUT);
  bitDelay();
  // CLK high
  pinMode(pin_clk_, INPUT);
  bitDelay();
  // Query ACK
  const uint8_t ack = digitalRead(pin_dio_);
  if (ack == 0)
    pinMode(pin_dio_, OUTPUT);
  // CLK low
  bitDelay();
  pinMode(pin_clk_, OUTPUT);
  bitDelay();

  return ack != 0;
}

//#include <Arduino.h>
//#include <BtSevenSegmentDisplay.h>

//// Lookup segments for digits 0-9, A-F.
//// We need gfedcba encoding, see https://en.wikipedia.org/wiki/Seven-segment_display
//const uint8_t lookup_digit_segments[] =
//{
//  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6f,
//  0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71
//};

//BtSevenSegmentDisplayTM1637::BtSevenSegmentDisplayTM1637(uint8_t pin_clk,
//  uint8_t pin_dio, uint8_t brightness, unsigned long bit_delay) :
//  pin_clk_(pin_clk), pin_dio_(pin_dio), bit_delay_(bit_delay)
//{
//#ifdef SSD_STORE_SEGMENTS
//  for (uint8_t i = 0; i < 4; ++i)
//    segments_[i] = 0;
//#endif // SSD_STORE_SEGMENTS

//  // Set up pin modes
////  pinAsOutput(pin_clk_);
////  pinAsOutput(pin_dio_);
////  digitalHigh(pin_clk_);
////  digitalHigh(pin_clk_);
//  pinAsInput(pin_clk_);
//  digitalLow(pin_clk_);
//  pinAsInput(pin_dio_);
//  digitalLow(pin_dio_);

//  // Set display brightness
//  //setBrightness(brightness);

//  // Write SET_DATA defaults
//  start();
//  sendByte(0x40);
//  ack();
//  stop();

////  // Set up pins (use pull-up to pull CLK and DIO up)
////  pinMode(pin_clk_, INPUT);
////  digitalWrite(pin_clk_, LOW);
////  pinMode(pin_dio_, INPUT);
////  digitalWrite(pin_dio_, LOW);
//}

//bool BtSevenSegmentDisplayTM1637::setBrightness(uint8_t brightness)
//{
//  // 0x08 enables the digit's LED (... | 0x00 would turn the digit off).
//  //brightness_ = (brightness & 0x07) | (brightness > 0 ? 0x08 : 0x00);
//  uint8_t b = (brightness & 0x07) | (brightness > 0 ? 0x08 : 0x00);
//  start();
//  sendByte(0x80 + (b & 0x0f));
//  const bool acknowledged = ack();
//  stop();
//  return acknowledged;
//}

//void BtSevenSegmentDisplayTM1637::clear()
//{
//  const uint8_t data[] = { 0, 0, 0, 0 };
//  setSegments(data);
//}


//void BtSevenSegmentDisplayTM1637::displayInteger(int x)
//{
//  // Sanity check
//  if (x > 9999 || x < -999)
//    return;

//  // Make absolute
//  const bool negative = x < 0;
//  if (negative)
//    x = -x;

//  uint8_t segments[4];
//  uint8_t nlz = 0; // Count number of leading zeroes
//  bool skip_zeros = true;
//  int div = 1000;
//  for (uint8_t i = 0; i < 3; ++i)
//  {
//    uint8_t digit = x / div;
//    if (digit == 0 && skip_zeros)
//    {
//      ++nlz;
//      segments[i] = 0;
//    }
//    else
//    {
//      skip_zeros = false;
//      segments[i] = digitToSegment(digit);
//    }
//    if (i < 2)
//    {
//      x %= div;
//      div /= 10;
//    }
//  }
//  segments[3] = digitToSegment(x % 10);
//  if (negative)
//    segments[nlz-1] = BT_SSD_SEG_G;
//  setSegments(segments);
//}

//void BtSevenSegmentDisplayTM1637::displayTime(uint8_t a, uint8_t b)
//{
//  if (a > 99 || b > 99)
//    return;

//  uint8_t segments[4];
//  segments[0] = digitToSegment(a / 10);
//  segments[1] = digitToSegment(a % 10) | BT_SSD_SEG_COLON;
//  segments[2] = digitToSegment(b / 10);
//  segments[3] = digitToSegment(b % 10);
//  setSegments(segments);
//}

//bool BtSevenSegmentDisplayTM1637::setSegments(const uint8_t segments[], uint8_t num_digits, uint8_t first_pos)
//{
//  bool acknowledged = true;
//    //TODO adjust to TM1637::command
////  // Send data set
//  start();
//  sendByte(0x40);
//  stop();
//  acknowledged &= ack();

//  // Send address command
//  start();
//  sendByte(0xC0 + (first_pos & 0x03));
//  // Send data
//  for (uint8_t i = 0; i < num_digits; ++i)
//  {
//    sendByte(segments[i]);
//    acknowledged &= ack();
//  }
//  stop();

//  // Send display control command
////  start();
////  sendByte(0x80 + (brightness_ & 0x0f));
////  stop();
//  acknowledged &= setBrightness(7);

//#ifdef SSD_STORE_SEGMENTS
//  for (uint8_t i = 0; i < 4; ++i)
//    segments_[i] = 0;
//  for (uint8_t i = 0; i < num_digits; ++i)
//    segments_[i+first_pos] = segments[i];
//#endif // SSD_STORE_SEGMENTS
//  return acknowledged;
//}

//uint8_t BtSevenSegmentDisplayTM1637::digitToSegment(uint8_t digit) const
//{
//  return lookup_digit_segments[digit & 0x0f];
//}

//#ifdef SSD_STORE_SEGMENTS
//void BtSevenSegmentDisplayTM1637::getSegments(uint8_t segments[]) const
//{
//  for (uint8_t i = 0; i < 4; ++i)
//    segments[i] = segments_[i];
//}
//#endif // SSD_STORE_SEGMENTS

//void BtSevenSegmentDisplayTM1637::start() const
//{
//  //pinMode(pin_dio_, OUTPUT);
//  //bitDelay();
//  // DIO high, CLK high, delay, DIO low
//  digitalHigh(pin_dio_);
//  digitalHigh(pin_clk_);
//  bitDelay();
//  digitalLow(pin_dio_);
//}

//void BtSevenSegmentDisplayTM1637::stop() const
//{
//  //pinMode(pin_dio_, OUTPUT);
////  bitDelay();
////  pinMode(pin_clk_, INPUT);
////  bitDelay();
////  pinMode(pin_dio_, INPUT);
////  bitDelay();
//  digitalLow(pin_clk_);   // CLK low
//  bitDelay();
//  digitalLow(pin_dio_);   // DIO low
//  bitDelay();
//  digitalHigh(pin_clk_);  // CLK high
//  bitDelay();
//  digitalHigh(pin_dio_);  // DIO high
//}

//bool BtSevenSegmentDisplayTM1637::ack() const
//{
//  bool res = false;
//  // CLK low, set DIO mode to input pullup
//  digitalLow(pin_clk_);
//  pinAsInputPullUp(pin_dio_);
//  bitDelay();

//  // ACK should pull the pin down again
//  res = isLow(pin_dio_);

//  digitalHigh(pin_clk_);
//  bitDelay();

//  digitalLow(pin_clk_);
//  pinAsOutput(pin_dio_);

//  return res;
//}

//bool BtSevenSegmentDisplayTM1637::sendByte(uint8_t data) const
//{
//  // Send each bit
//  for(uint8_t i = 0; i < 8; ++i)
//  {
//    // CLK low
//    digitalLow(pin_clk_);
//    //pinMode(pin_clk_, OUTPUT);
//    //bitDelay();
//    // Send bit

//    // DIO high or low, according to data bit
//    if (data & 0x01)
//      digitalHigh(pin_dio_);
//      //pinMode(pin_dio_, INPUT);
//    else
//      digitalLow(pin_dio_);
//      //pinMode(pin_dio_, OUTPUT);
//    bitDelay();

//    // CLK high
//    digitalHigh(pin_clk_);
//    //pinMode(pin_clk_, INPUT);
//    bitDelay();
//    // Next bit
//    data = data >> 1;
//  }
//  // CLK low
//  pinMode(pin_clk_, OUTPUT);
//  pinMode(pin_dio_, INPUT);
//  bitDelay();
//  // CLK high
//  pinMode(pin_clk_, INPUT);
//  bitDelay();
//  // Query ACK
//  const uint8_t ack = digitalRead(pin_dio_);
//  if (ack == 0)
//    pinMode(pin_dio_, OUTPUT);
//  // CLK low
//  bitDelay();
//  pinMode(pin_clk_, OUTPUT);
//  bitDelay();

//  return ack != 0;
//}
