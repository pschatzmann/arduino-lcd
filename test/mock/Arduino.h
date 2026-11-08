#pragma once
// Minimal desktop replacement for the Arduino core: just enough to compile
// LCD.h and to observe what the library does to the pins.
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <functional>
#include <utility>
#include <vector>

typedef uint8_t byte;
typedef bool boolean;

#define HIGH 0x1
#define LOW 0x0
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

/// State of the simulated microcontroller (pins, PWM and a virtual clock)
struct MockHardware {
  unsigned long micros_now = 0;  ///< virtual clock: advanced by delay*()
  uint8_t mode[256];
  uint8_t value[256];
  int pwm[256];  ///< last analogWrite value per pin, -1: never written
  unsigned long pinWrites = 0;
  unsigned long pwmWrites = 0;
  std::vector<std::pair<int, std::function<void(uint8_t, uint8_t)>>> observers;
  int nextObserverId = 1;

  MockHardware() { reset(); }

  void reset() {
    micros_now = 0;
    memset(mode, 0, sizeof(mode));
    memset(value, 0, sizeof(value));
    for (int i = 0; i < 256; i++) pwm[i] = -1;
    pinWrites = 0;
    pwmWrites = 0;
    observers.clear();
  }

  /// Registers a function which is called for each digitalWrite()
  int addObserver(std::function<void(uint8_t, uint8_t)> f) {
    observers.push_back(std::make_pair(nextObserverId, f));
    return nextObserverId++;
  }

  void removeObserver(int id) {
    for (size_t i = 0; i < observers.size(); i++) {
      if (observers[i].first == id) {
        observers.erase(observers.begin() + i);
        return;
      }
    }
  }
};

inline MockHardware &mockHardware() {
  static MockHardware hw;
  return hw;
}

inline void pinMode(uint8_t pin, uint8_t mode) { mockHardware().mode[pin] = mode; }

inline void digitalWrite(uint8_t pin, uint8_t val) {
  MockHardware &hw = mockHardware();
  hw.value[pin] = val;
  hw.pinWrites++;
  // copy: observers may be removed while we iterate
  auto observers = hw.observers;
  for (auto &o : observers) o.second(pin, val);
}

inline int digitalRead(uint8_t pin) { return mockHardware().value[pin]; }

inline void analogWrite(uint8_t pin, int val) {
  mockHardware().pwm[pin] = val;
  mockHardware().pwmWrites++;
}

inline void delayMicroseconds(unsigned int us) { mockHardware().micros_now += us; }
inline void delay(unsigned long ms) { mockHardware().micros_now += ms * 1000UL; }
inline unsigned long micros() { return mockHardware().micros_now; }
inline unsigned long millis() { return mockHardware().micros_now / 1000UL; }

inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#include "Print.h"
#include "Stream.h"
