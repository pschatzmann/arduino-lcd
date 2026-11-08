#pragma once

#include "Arduino.h"
#include "AbstractLCDDriver.h"

/**
 * @brief LCDDriver with direct output to pins
 */
struct LCDDriver : public AbstractLCDDriver {
  void pinModeLCD(uint16_t pin, uint16_t mode) override { pinMode(pin, mode); }

  void digitalWriteLCD(uint16_t pin, uint16_t value) override {
    digitalWrite(pin, value);
  }

  void delayMicrosecondsLCD(uint16_t us) override { delayMicroseconds(us); }

  void pulseEnable(uint16_t pin) override {
    digitalWriteLCD(pin, LOW);
    delayMicroseconds(1);
    digitalWriteLCD(pin, HIGH);
    delayMicroseconds(1);  // enable pulse must be >450 ns
    digitalWriteLCD(pin, LOW);
    delayMicroseconds(100);  // commands need >37 us to settle
  }

  /// Sets the brightness of a PWM pin: 0 (off) to 100 (full)
  void setBrightness(uint16_t pin, uint16_t percent) override {
    if (percent > 100) percent = 100;
    analogWrite(pin, (int)map(percent, 0, 100, 0, 255));
  }
};

/// Provides the default driver (created on first use, so that the header can
/// be included from multiple source files).
inline LCDDriver &defaultLCDDriver() {
  static LCDDriver driver;
  return driver;
}
