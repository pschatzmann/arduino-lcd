#pragma once
#include <stdint.h>
#include <stddef.h>
#include <map>
#include <vector>

#include "Arduino.h"

/// A device on the (simulated) I2C bus
struct I2CDevice {
  virtual ~I2CDevice() {}
  /// Called for each byte which is sent to the device
  virtual void i2cWrite(uint8_t value) = 0;
};

/// Desktop version of TwoWire: forwards the data to the attached I2CDevice
class TwoWire {
 public:
  /// Time which a transmission needs on the bus (400 kHz, 2 bytes)
  unsigned int microsPerTransmission = 50;

  void begin() {}
  void attach(uint8_t address, I2CDevice *device) { devices[address] = device; }
  void detach(uint8_t address) { devices.erase(address); }

  void beginTransmission(uint8_t address) {
    current = address;
    pending.clear();
  }
  size_t write(uint8_t value) {
    pending.push_back(value);
    return 1;
  }
  size_t write(int value) { return write((uint8_t)value); }

  /// 0: ok, 2: address not acknowledged
  uint8_t endTransmission() {
    delayMicroseconds(microsPerTransmission);
    auto it = devices.find(current);
    if (it == devices.end()) return 2;
    for (uint8_t v : pending) it->second->i2cWrite(v);
    pending.clear();
    return 0;
  }

 private:
  std::map<uint8_t, I2CDevice *> devices;
  std::vector<uint8_t> pending;
  uint8_t current = 0;
};

inline TwoWire Wire;
