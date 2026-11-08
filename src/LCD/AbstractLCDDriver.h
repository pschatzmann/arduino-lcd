#pragma once

#include <stdint.h>

/**
 * @brief AbstractLCDDriver: commands which are serialized and sent over
 * the wire
 */
struct AbstractLCDDriver {
  virtual ~AbstractLCDDriver() = default;
  virtual void pinModeLCD(uint16_t pin, uint16_t mode) = 0;
  virtual void digitalWriteLCD(uint16_t pin, uint16_t value) = 0;
  virtual void delayMicrosecondsLCD(uint16_t us) = 0;
  virtual void pulseEnable(uint16_t pin) = 0;
  virtual void setBrightness(uint16_t pin, uint16_t percent) = 0;
};
