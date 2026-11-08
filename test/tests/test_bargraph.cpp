// Tests of LCDBarGraph
#include "Rigs.h"

static std::string chars(int full, int partial, int width) {
  std::string s(full, '\0');
  if (partial > 0) s += (char)partial;
  return pad(s, width);
}

TEST(BarGraph, defines_the_custom_characters) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 10);
  const uint8_t expected[5] = {0x1F, 0x10, 0x18, 0x1C, 0x1E};
  for (int slot = 0; slot < 5; slot++) {
    for (int row = 0; row < 8; row++) {
      CHECK_EQ(rig.hd.cgram(slot, row), expected[slot]);
    }
  }
}

TEST(BarGraph, constructor_clears_the_display) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("old text");
  LCDBarGraph bar(rig.lcd, 10);
  CHECK_EQ(rig.hd.line(0), pad("", 16));
  CHECK_EQ(rig.hd.address(), 0);  // back in DDRAM mode
}

TEST(BarGraph, zero_value_draws_nothing) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 10);
  bar.drawValue(0, 100);
  CHECK_EQ(rig.hd.line(0), pad("", 16));
}

TEST(BarGraph, half_and_full) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 10);
  bar.drawValue(50, 100);
  CHECK_EQ(rig.hd.line(0), chars(5, 0, 16));
  rig.hd.snapshot("half");
  bar.drawValue(100, 100);
  CHECK_EQ(rig.hd.line(0), chars(10, 0, 16));
  rig.hd.snapshot("full");
}

TEST(BarGraph, partial_characters) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 10);
  // one character is 10 of 100: 1/5 character is 2
  bar.drawValue(2, 100);
  CHECK_EQ(rig.hd.line(0), chars(0, 1, 16));
  bar.drawValue(4, 100);
  CHECK_EQ(rig.hd.line(0), chars(0, 2, 16));
  bar.drawValue(6, 100);
  CHECK_EQ(rig.hd.line(0), chars(0, 3, 16));
  bar.drawValue(8, 100);
  CHECK_EQ(rig.hd.line(0), chars(0, 4, 16));
  bar.drawValue(10, 100);
  CHECK_EQ(rig.hd.line(0), chars(1, 0, 16));
  bar.drawValue(34, 100);
  CHECK_EQ(rig.hd.line(0), chars(3, 2, 16));
  rig.hd.snapshot("partial");
}

TEST(BarGraph, shrinking_erases_the_old_bar) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 10);
  bar.drawValue(100, 100);
  bar.drawValue(30, 100);
  CHECK_EQ(rig.hd.line(0), chars(3, 0, 16));
  bar.drawValue(0, 100);
  CHECK_EQ(rig.hd.line(0), pad("", 16));
}

TEST(BarGraph, unchanged_value_is_not_redrawn) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 10);
  bar.drawValue(50, 100);
  unsigned long writes = mockHardware().pinWrites;
  bar.drawValue(50, 100);
  bar.drawValue(51, 100);  // same step (the bar has 50 steps)
  CHECK_EQ(mockHardware().pinWrites, writes);
}

TEST(BarGraph, out_of_range_values_are_limited) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 10);
  bar.drawValue(500, 100);
  CHECK_EQ(rig.hd.line(0), chars(10, 0, 16));
  bar.drawValue(-20, 100);
  CHECK_EQ(rig.hd.line(0), pad("", 16));
}

TEST(BarGraph, invalid_max_value_is_ignored) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 10);
  bar.drawValue(5, 0);
  bar.drawValue(5, -10);
  CHECK_EQ(rig.hd.line(0), pad("", 16));
}

TEST(BarGraph, position_and_row) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 4, 12, 1);  // 4 characters at the end of row 2
  bar.drawValue(100, 100);
  CHECK_EQ(rig.hd.line(0), pad("", 16));
  CHECK_EQ(rig.hd.line(1), pad("", 12) + std::string(4, '\0'));
  rig.hd.snapshot("end_of_second_row");
}

TEST(BarGraph, several_bars) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph a(rig.lcd, 4, 0, 0);
  LCDBarGraph b(rig.lcd, 4, 5, 0);
  LCDBarGraph c(rig.lcd, 4, 0, 1);
  a.drawValue(100, 100);
  b.drawValue(50, 100);
  c.drawValue(25, 100);
  CHECK_EQ(rig.hd.line(0), pad(std::string(4, '\0') + " " + std::string(2, '\0'), 16));
  CHECK_EQ(rig.hd.line(1), pad(std::string(1, '\0'), 16));
  rig.hd.snapshot("three_bars");
}

TEST(BarGraph, built_in_filled_character) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 10);
  CHECK_EQ(bar.getFilledChar(), -1);
  bar.setFilledChar(0xFF);
  CHECK_EQ(bar.getFilledChar(), 0xFF);
  bar.drawValue(30, 100);
  CHECK_EQ(rig.hd.line(0), pad(std::string(3, '\xFF'), 16));
  bar.setFilledChar(-1);
  bar.drawValue(30, 100);  // changing the character redraws the bar
  CHECK_EQ(rig.hd.line(0), chars(3, 0, 16));
}

TEST(BarGraph, works_with_i2c_display) {
  I2C rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 8);
  bar.drawValue(75, 100);
  CHECK_EQ(rig.hd.line(0), chars(6, 0, 16));
  rig.hd.snapshot("i2c");
}

TEST(BarGraph, animation) {
  // the recorded frames show the bar growing and shrinking
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  LCDBarGraph bar(rig.lcd, 16);
  for (int v = 0; v <= 100; v += 10) bar.drawValue(v, 100);
  for (int v = 100; v >= 0; v -= 20) bar.drawValue(v, 100);
  CHECK_EQ(rig.hd.line(0), pad("", 16));
  if (HD44780::defaultRecording) CHECK(rig.hd.frameCount() >= 15);
}
