#pragma once
// Helpers which wire a library object to an emulated display
#include <deque>
#include <string>
#include <vector>

#include "Backends.h"
#include "HD44780.h"
#include "LCD.h"
#include "TestFramework.h"

/// returns text padded with spaces to the given width
inline std::string pad(const std::string &s, size_t width) {
  std::string r = s;
  r.resize(width, ' ');
  return r;
}

/// 4 bit parallel display: rs=12, en=11, d4..d7 = 5,4,3,2
struct Parallel4 {
  HD44780 hd;
  ParallelBus bus;
  LCD lcd;
  Parallel4(int cols = 16, int rows = 2, int led = 0)
      : hd(cols, rows),
        bus(hd, 12, 11, {5, 4, 3, 2}),
        lcd(12, 11, 5, 4, 3, 2, led) {}
};

/// 8 bit parallel display with RW: rs=12, rw=10, en=11, d0..d7 = 2..9
struct Parallel8 {
  HD44780 hd;
  ParallelBus bus;
  LCD lcd;
  Parallel8(int cols = 16, int rows = 2)
      : hd(cols, rows),
        bus(hd, 12, 11, {2, 3, 4, 5, 6, 7, 8, 9}, 10),
        lcd(12, 10, 11, 2, 3, 4, 5, 6, 7, 8, 9) {}
};

/// I2C display (PCF8574 backpack at 0x27)
struct I2C {
  HD44780 hd;
  PCF8574Backpack backpack;
  LCD_I2C lcd;
  I2C(int cols = 16, int rows = 2)
      : hd(cols, rows), backpack(hd, Wire, 0x27), lcd(0x27) {}
};

/// Byte queue which is both input and output (e.g. a serial loopback)
class LoopbackStream : public Stream {
 public:
  std::deque<uint8_t> data;
  size_t write(uint8_t b) override {
    data.push_back(b);
    return 1;
  }
  using Print::write;
  int available() override { return (int)data.size(); }
  int read() override {
    if (data.empty()) return -1;
    int v = data.front();
    data.pop_front();
    return v;
  }
  int peek() override { return data.empty() ? -1 : data.front(); }
};
