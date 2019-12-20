#ifndef __BASIC_TINKERING_SEVEN_SEGMENT_DISPLAY__
#define __BASIC_TINKERING_SEVEN_SEGMENT_DISPLAY__

#define SEG_A     0b00000001
#define SEG_B     0b00000010
#define SEG_C     0b00000100
#define SEG_D     0b00001000
#define SEG_E     0b00010000
#define SEG_F     0b00100000
#define SEG_G     0b01000000
#define SEG_COLON 0b10000000

/**
 * Utility for 4-digit 7-segment displays using TM1637.
 */
class SevenSegmentDisplayTM1637
{
public:
  // * CLK and DIO must be connected to digital pins
  // * Choose initial brightness in [0, 7]
  // * Adjust delay (in microseconds) for sending bits to the I2C
  //   100 worked nicely for my tested displays.
  SevenSegmentDisplayTM1637(uint8_t pin_clk, uint8_t pin_dio,
    uint8_t brightness = 7, unsigned long bit_delay = 100);

  // Set the brightness in [0, 7], has an effect upon next display update.
  void setBrightness(uint8_t brightness);

  // Display the integer -999 <= x <= 9999, right-aligned.
  void displayInteger(int x) const;

  // Display a time, i.e. a:b (numbers will be zero-padded).
  // You are responsible of providing sane inputs (that is,
  // "99:99" would be displayed, too).
  void displayTime(uint8_t a, uint8_t b) const;

  // Clear the display
  void clear() const;

  // Set the segments directly
  void setSegments(const uint8_t segments[], uint8_t num_digits = 4, uint8_t first_pos = 0) const;

  // Get segment gfedcba encoding for digits 0-9, A-F
  uint8_t digitToSegment(uint8_t digit) const;

private:
  // Pin number for CLK
  uint8_t pin_clk_;

  // Pin number for DIO
  uint8_t pin_dio_;

  // Display brightness level in [0,7]
  uint8_t brightness_;

  // Delay in microseconds for sending bits to the I2C
  unsigned long bit_delay_;

  // Delay
  inline void bitDelay() const
  { delayMicroseconds(bit_delay_); }

  // Set up DIO to prepare data transmission
  void start() const;

  // Finalize data transmission
  void stop() const;

  // Send a single byte to I2C
  bool sendByte(uint8_t byte) const;
};
#endif
