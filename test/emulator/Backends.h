#pragma once
/**
 * Front ends which connect the HD44780 emulator to the (mocked) hardware:
 * - ParallelBus: observes the pins which are written with digitalWrite()
 * - PCF8574Backpack: an I2C backpack (as used by the LCD_I2C class)
 */
#include <vector>

#include "Arduino.h"
#include "HD44780.h"
#include "Wire.h"

/// LCD connected directly to the pins of the microcontroller. The controller
/// reads the bus when the enable pin changes from HIGH to LOW.
class ParallelBus {
 public:
  /**
   * @param dataPins 4 pins (DB4..DB7) or 8 pins (DB0..DB7)
   * @param rw pin of the RW line or -1 if it is connected to ground
   */
  ParallelBus(HD44780 &lcd, int rs, int enable, std::vector<int> dataPins,
              int rw = -1)
      : lcd_(lcd), rs_(rs), rw_(rw), enable_(enable), data_(dataPins) {
    id_ = mockHardware().addObserver(
        [this](uint8_t pin, uint8_t value) { onPin(pin, value); });
  }

  ~ParallelBus() { mockHardware().removeObserver(id_); }

 private:
  HD44780 &lcd_;
  int rs_, rw_, enable_;
  std::vector<int> data_;
  int id_ = 0;
  bool enableWasHigh_ = false;

  void onPin(uint8_t pin, uint8_t value) {
    if (pin != enable_) return;
    bool high = value != 0;
    if (enableWasHigh_ && !high) latch();
    enableWasHigh_ = high;
  }

  void latch() {
    MockHardware &hw = mockHardware();
    bool rs = hw.value[rs_] != 0;
    bool rw = rw_ >= 0 && hw.value[rw_] != 0;
    uint8_t data = 0;
    bool highOnly = data_.size() == 4;
    for (size_t i = 0; i < data_.size(); i++) {
      if (hw.value[data_[i]]) data |= 1 << (highOnly ? i + 4 : i);
    }
    lcd_.latch(rs, rw, data, highOnly);
  }
};

/// PCF8574 I2C backpack: P0=RS, P1=RW, P2=E, P3=backlight, P4..P7=DB4..DB7
class PCF8574Backpack : public I2CDevice {
 public:
  PCF8574Backpack(HD44780 &lcd, TwoWire &wire, uint8_t address)
      : lcd_(lcd), wire_(wire), address_(address) {
    wire_.attach(address_, this);
  }
  ~PCF8574Backpack() { wire_.detach(address_); }

  void i2cWrite(uint8_t v) override {
    bool enable = v & 0x04;
    lcd_.setBacklight(v & 0x08);
    if (enableWasHigh_ && !enable) {
      lcd_.latch(v & 0x01, v & 0x02, v & 0xF0, true);
    }
    enableWasHigh_ = enable;
  }

 private:
  HD44780 &lcd_;
  TwoWire &wire_;
  uint8_t address_;
  bool enableWasHigh_ = false;
};
