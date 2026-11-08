// Tests of the LCD_I2C class against the HD44780 emulator (PCF8574 backpack)
#include "Rigs.h"

TEST(I2C, init_negotiates_4bit_interface) {
  I2C rig;
  rig.lcd.begin(16, 2);
  CHECK(rig.hd.is4BitInterface());
  CHECK(rig.hd.twoLine());
  CHECK(rig.hd.displayOn());
  CHECK_EQ(rig.hd.timingViolations(), 0);
}

TEST(I2C, hello_world) {
  I2C rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("Hello, World!");
  CHECK_EQ(rig.hd.line(0), pad("Hello, World!", 16));
  CHECK_EQ(rig.hd.timingViolations(), 0);
  rig.hd.snapshot("hello_world");
}

TEST(I2C, set_cursor_uses_row_offsets) {
  I2C rig;
  rig.lcd.begin(16, 2);
  rig.lcd.setCursor(4, 1);
  rig.lcd.print("row 2");
  CHECK_EQ(rig.hd.line(1), pad("    row 2", 16));
  CHECK_EQ(rig.hd.address(), 0x40 + 9);
}

TEST(I2C, set_cursor_row_is_limited) {
  I2C rig;
  rig.lcd.begin(16, 2);
  rig.lcd.setCursor(0, 5);
  rig.lcd.print("x");
  CHECK_EQ(rig.hd.line(1), pad("x", 16));
}

TEST(I2C, display_20x4) {
  I2C rig(20, 4);
  rig.lcd.begin(20, 4);
  for (int r = 0; r < 4; r++) {
    rig.lcd.setCursor(0, r);
    rig.lcd.print("Row ");
    rig.lcd.print(r + 1);
  }
  for (int r = 0; r < 4; r++) {
    CHECK_EQ(rig.hd.line(r), pad(std::string("Row ") + char('1' + r), 20));
  }
  rig.hd.snapshot("lines");
}

TEST(I2C, display_flags_are_tracked_by_the_base_class) {
  // regression: LCD_I2C used to shadow _displaycontrol / _displaymode
  I2C rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("flags");
  rig.lcd.noDisplay();
  CHECK(!rig.hd.displayOn());
  rig.lcd.display();
  CHECK(rig.hd.displayOn());
  rig.lcd.cursor();
  CHECK(rig.hd.cursorOn());
  CHECK(rig.hd.displayOn());
  rig.lcd.blink();
  CHECK(rig.hd.blinkOn());
  rig.lcd.noCursor();
  rig.lcd.noBlink();
  CHECK(!rig.hd.cursorOn());
  CHECK(!rig.hd.blinkOn());
  CHECK(rig.hd.displayOn());
  CHECK_EQ(rig.hd.line(0), pad("flags", 16));
}

TEST(I2C, text_direction_and_autoscroll) {
  I2C rig;
  rig.lcd.begin(16, 2);
  rig.lcd.rightToLeft();
  CHECK(!rig.hd.entryIncrement());
  rig.lcd.leftToRight();
  CHECK(rig.hd.entryIncrement());
  rig.lcd.autoscroll();
  CHECK(rig.hd.entryShift());
  rig.lcd.noAutoscroll();
  CHECK(!rig.hd.entryShift());
}

TEST(I2C, scroll_display) {
  I2C rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("0123456789ABCDEFGH");
  rig.lcd.scrollDisplayLeft();
  CHECK_EQ(rig.hd.line(0), "123456789ABCDEFG");
  rig.lcd.scrollDisplayRight();
  rig.lcd.scrollDisplayRight();
  CHECK_EQ(rig.hd.displayShift(), -1);
}

TEST(I2C, backlight) {
  I2C rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("light");
  CHECK(rig.hd.backlight());
  CHECK(rig.lcd.getBacklight());
  rig.hd.snapshot("backlight_on");
  rig.lcd.noBacklight();
  CHECK(!rig.hd.backlight());
  CHECK(!rig.lcd.getBacklight());
  rig.hd.snapshot("backlight_off");
  rig.lcd.print("!");  // the backlight stays off while writing
  CHECK(!rig.hd.backlight());
  CHECK_EQ(rig.hd.line(0), pad("light!", 16));
  rig.lcd.backlight();
  CHECK(rig.hd.backlight());
  rig.lcd.setBacklight(false);
  CHECK(!rig.hd.backlight());
}

TEST(I2C, custom_characters) {
  I2C rig;
  rig.lcd.begin(16, 2);
  const uint8_t bell[8] = {0x04, 0x0E, 0x0E, 0x0E, 0x1F, 0x00, 0x04, 0x00};
  rig.lcd.createChar(3, bell);
  for (int i = 0; i < 8; i++) CHECK_EQ(rig.hd.cgram(3, i), bell[i]);
  rig.lcd.setCursor(0, 0);
  rig.lcd.write((uint8_t)3);
  CHECK_EQ(rig.hd.line(0)[0], '\x03');
  rig.hd.snapshot("custom_character");
}

TEST(I2C, other_wire_object) {
  TwoWire wire2;
  HD44780 hd(16, 2);
  PCF8574Backpack backpack(hd, wire2, 0x3F);
  LCD_I2C lcd(0x3F);
  lcd.begin(16, 2, 0, wire2);
  lcd.print("wire 2");
  CHECK_EQ(hd.line(0), pad("wire 2", 16));
}

TEST(I2C, set_wire_before_begin) {
  TwoWire wire2;
  HD44780 hd(16, 2);
  PCF8574Backpack backpack(hd, wire2, 0x3F);
  LCD_I2C lcd(0x3F);
  lcd.setWire(wire2);
  lcd.begin(16, 2);
  lcd.print("set wire");
  CHECK_EQ(hd.line(0), pad("set wire", 16));
}

TEST(I2C, uses_default_wire_object) {
  I2C rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("Wire");
  CHECK_EQ(rig.hd.line(0), pad("Wire", 16));
}

TEST(I2C, brightness_pin) {
  I2C rig;
  rig.lcd.setBrightnessPin(9);
  rig.lcd.begin(16, 2);
  rig.lcd.setBrightness(100);
  CHECK_EQ(mockHardware().pwm[9], 255);
  rig.lcd.setBrightness(20);
  CHECK_EQ(mockHardware().pwm[9], 51);
}

TEST(I2C, brightness_without_pin_is_ignored) {
  I2C rig;
  rig.lcd.begin(16, 2);
  rig.lcd.setBrightness(80);
  CHECK_EQ(mockHardware().pwmWrites, 0UL);
}

TEST(I2C, brightness_is_limited_to_100_percent) {
  I2C rig;
  rig.lcd.setBrightnessPin(9);
  rig.lcd.begin(16, 2);
  rig.lcd.setBrightness(250);
  CHECK_EQ(mockHardware().pwm[9], 255);
  rig.lcd.setBrightness(0);
  CHECK_EQ(mockHardware().pwm[9], 0);
}
