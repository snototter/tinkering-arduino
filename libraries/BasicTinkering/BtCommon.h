//#ifndef __BASIC_TINKERING_COMMON__
//#define __BASIC_TINKERING_COMMON__

//#include <Arduino.h>

//// AVR support direct port access which speeds up things a little.
//// Thus, we use these macros which fall back to standard pin manipulation
//// commands if not available.
//// Based on Arduino-TM1637 library.
//#if defined(__AVR_ATmega8__) || defined(__AVR_ATmega8A__)\
//    || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168A__)\
//    || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega168PA__)\
//    || defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
//  #define portOfPin(P) (((P) >= 0 && (P) < 8) ? &PORTD : (((P) > 7 && (P) < 14) ? &PORTB : &PORTC))
//  #define ddrOfPin(P)  (((P) >= 0 && (P) < 8) ? &DDRD  : (((P) > 7 && (P) < 14) ? &DDRB  : &DDRC))
//  #define pinOfPin(P)  (((P) >= 0 && (P) < 8) ? &PIND  : (((P) > 7 && (P) < 14) ? &PINB  : &PINC))

//  #define pinIndex(P)         ((uint8_t)(P > 13 ? P-14 : P&7))
//  #define pinMask(P)          ((uint8_t)(1 << pinIndex(P)))

//  #define pinAsInput(P)       *(ddrOfPin(P)) &= ~pinMask(P)
//  #define pinAsInputPullUp(P) *(ddrOfPin(P)) &= ~pinMask(P); digitalHigh(P)
//  #define pinAsOutput(P)      *(ddrOfPin(P)) |= pinMask(P)

//  #define digitalLow(P)       *(portOfPin(P)) &= ~pinMask(P)
//  #define digitalHigh(P)      *(portOfPin(P)) |= pinMask(P)

//  #define isHigh(P) ((*(pinOfPin(P)) & pinMask(P)) > 0)
//  #define isLow(P)  ((*(pinOfPin(P)) & pinMask(P)) == 0)

//  #define digitalState(P) ((uint8_t)isHigh(P))
//#else // AVR checks
//  #define pinAsInput(P)       pinMode(P, INPUT)
//  #define pinAsInputPullUp(P) pinMode(P, INPUT_PULLUP)
//  #define pinAsOutput(P)      pinMode(P, OUTPUT)

//  #define digitalLow(P)       digitalWrite(P, LOW)
//  #define digitalHigh(P)      digitalWrite(P, HIGH)

//  #define isHigh(P)           (digitalRead(P) == 1)
//  #define isLow(P)            (digitalRead(P) == 0)

//  #define digitalState(P)     digitalRead(P)
//#endif // AVR checks
//#endif // __BASIC_TINKERING_COMMON__
