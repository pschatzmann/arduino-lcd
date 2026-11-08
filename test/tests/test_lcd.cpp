// Tests of the parallel LCD class against the HD44780 emulator
#include "Rigs.h"

TEST(LCD, constructor_does_not_touch_pins) {
  Parallel4 rig;
  CHECK_EQ(mockHardware().pinWrites, 0UL);
}

TEST(LCD, init_negotiates_4bit_interface) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  CHECK(rig.hd.is4BitInterface());
  CHECK(rig.hd.twoLine());
  CHECK(rig.hd.displayOn());
  CHECK(!rig.hd.cursorOn());
  CHECK(!rig.hd.blinkOn());
  CHECK(rig.hd.entryIncrement());
  CHECK(!rig.hd.entryShift());
  CHECK_EQ(rig.hd.timingViolations(), 0);
}

TEST(LCD, pins_are_configured_as_output) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  const int pins[] = {12, 11, 5, 4, 3, 2};
  for (int p : pins) CHECK_EQ((int)mockHardware().mode[p], (int)OUTPUT);
}

TEST(LCD, hello_world) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("Hello, World!");
  CHECK_EQ(rig.hd.line(0), pad("Hello, World!", 16));
  CHECK_EQ(rig.hd.line(1), pad("", 16));
  CHECK_EQ(rig.hd.timingViolations(), 0);
  rig.hd.snapshot("hello_world");
}

TEST(LCD, print_numbers) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print(42);
  rig.lcd.print(' ');
  rig.lcd.print(-7);
  rig.lcd.print(' ');
  rig.lcd.print(3.14159, 2);
  CHECK_EQ(rig.hd.line(0), pad("42 -7 3.14", 16));
}

TEST(LCD, set_cursor_second_line) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.setCursor(3, 1);
  rig.lcd.print("abc");
  CHECK_EQ(rig.hd.line(0), pad("", 16));
  CHECK_EQ(rig.hd.line(1), pad("   abc", 16));
  CHECK_EQ(rig.hd.address(), 0x40 + 6);
}

TEST(LCD, set_cursor_row_is_limited) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.setCursor(0, 7);  // only 2 rows
  rig.lcd.print("x");
  CHECK_EQ(rig.hd.line(1), pad("x", 16));
}

TEST(LCD, clear_and_home) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("Hello");
  rig.lcd.clear();
  CHECK_EQ(rig.hd.line(0), pad("", 16));
  CHECK_EQ(rig.hd.address(), 0);
  rig.lcd.setCursor(5, 1);
  rig.lcd.home();
  CHECK_EQ(rig.hd.address(), 0);
  CHECK_EQ(rig.hd.timingViolations(), 0);
}

TEST(LCD, display_on_off_keeps_content) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("Hello");
  rig.hd.snapshot("on");
  rig.lcd.noDisplay();
  CHECK(!rig.hd.displayOn());
  CHECK_EQ(rig.hd.line(0), pad("Hello", 16));
  rig.hd.snapshot("off");
  rig.lcd.display();
  CHECK(rig.hd.displayOn());
}

TEST(LCD, cursor_and_blink) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("Hi");
  rig.lcd.cursor();
  CHECK(rig.hd.cursorOn());
  CHECK(rig.hd.displayOn());
  rig.hd.snapshot("cursor");
  rig.lcd.blink();
  CHECK(rig.hd.blinkOn());
  CHECK(rig.hd.cursorOn());
  rig.hd.snapshot("blink");
  rig.lcd.noCursor();
  CHECK(!rig.hd.cursorOn());
  CHECK(rig.hd.blinkOn());
  rig.lcd.noBlink();
  CHECK(!rig.hd.blinkOn());
  CHECK(rig.hd.displayOn());
  int row, col;
  CHECK(rig.hd.cursorCell(row, col));
  CHECK_EQ(row, 0);
  CHECK_EQ(col, 2);
}

TEST(LCD, scroll_display_left_and_right) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("0123456789ABCDEFGH");  // 2 characters more than visible
  rig.lcd.scrollDisplayLeft();
  rig.lcd.scrollDisplayLeft();
  CHECK_EQ(rig.hd.displayShift(), 2);
  CHECK_EQ(rig.hd.line(0), "23456789ABCDEFGH");
  rig.hd.snapshot("scrolled_left");
  rig.lcd.scrollDisplayRight();
  CHECK_EQ(rig.hd.line(0), "123456789ABCDEFG");
}

TEST(LCD, autoscroll) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.setCursor(16, 0);
  rig.lcd.autoscroll();
  CHECK(rig.hd.entryShift());
  rig.lcd.print("ABC");
  CHECK_EQ(rig.hd.displayShift(), 3);
  rig.lcd.noAutoscroll();
  CHECK(!rig.hd.entryShift());
}

TEST(LCD, text_direction) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.setCursor(5, 0);
  rig.lcd.rightToLeft();
  CHECK(!rig.hd.entryIncrement());
  rig.lcd.print("abc");
  // written backwards: c is at column 3 (5-2)
  CHECK_EQ(rig.hd.line(0), pad("   cba", 16));
  rig.lcd.leftToRight();
  CHECK(rig.hd.entryIncrement());
}

TEST(LCD, custom_characters) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  const uint8_t heart[8] = {0x00, 0x0A, 0x1F, 0x1F, 0x1F, 0x0E, 0x04, 0x00};
  const uint8_t smiley[8] = {0x00, 0x0A, 0x0A, 0x00, 0x11, 0x0E, 0x00, 0x00};
  rig.lcd.createChar(0, heart);
  rig.lcd.createChar(1, smiley);
  for (int i = 0; i < 8; i++) {
    CHECK_EQ(rig.hd.cgram(0, i), heart[i]);
    CHECK_EQ(rig.hd.cgram(1, i), smiley[i]);
  }
  rig.lcd.setCursor(0, 0);  // back to DDRAM
  rig.lcd.write((uint8_t)0);
  rig.lcd.write((uint8_t)1);
  rig.lcd.print(" I love it");
  CHECK_EQ(rig.hd.line(0), pad(std::string("\x00\x01", 2) + " I love it", 16));
  rig.hd.snapshot("custom_characters");
}

TEST(LCD, custom_character_slot_is_limited_to_8) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  const uint8_t ch[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  rig.lcd.createChar(9, ch);  // 9 & 7 = slot 1
  CHECK_EQ(rig.hd.cgram(1, 0), 1);
  CHECK_EQ(rig.hd.cgram(1, 7), 8);
}

TEST(LCD, display_20x4_row_offsets) {
  Parallel4 rig(20, 4);
  rig.lcd.begin(20, 4);
  const char *text[] = {"Line 1", "Line 2", "Line 3", "Line 4"};
  for (int r = 0; r < 4; r++) {
    rig.lcd.setCursor(0, r);
    rig.lcd.print(text[r]);
  }
  for (int r = 0; r < 4; r++) CHECK_EQ(rig.hd.line(r), pad(text[r], 20));
  rig.lcd.setCursor(0, 2);
  CHECK_EQ(rig.hd.address(), 0x14);
  rig.lcd.setCursor(0, 3);
  CHECK_EQ(rig.hd.address(), 0x54);
  rig.hd.snapshot("lines");
}

TEST(LCD, single_line_display) {
  Parallel4 rig(16, 1);
  rig.lcd.begin(16, 1);
  CHECK(!rig.hd.twoLine());
  rig.lcd.print("one line");
  CHECK_EQ(rig.hd.line(0), pad("one line", 16));
}

TEST(LCD, begin_can_be_repeated_with_other_size) {
  Parallel4 rig(16, 2);
  rig.lcd.begin(16, 2);
  CHECK(rig.hd.twoLine());
  rig.lcd.begin(16, 1);
  CHECK(!rig.hd.twoLine());
}

TEST(LCD, eight_bit_interface_with_rw_pin) {
  Parallel8 rig;
  rig.lcd.begin(16, 2);
  CHECK(!rig.hd.is4BitInterface());
  CHECK(rig.hd.twoLine());
  CHECK(rig.hd.displayOn());
  rig.lcd.print("8 bit mode");
  rig.lcd.setCursor(0, 1);
  rig.lcd.print("with RW");
  CHECK_EQ(rig.hd.line(0), pad("8 bit mode", 16));
  CHECK_EQ(rig.hd.line(1), pad("with RW", 16));
  CHECK_EQ(rig.hd.readAttempts(), 0);
  CHECK_EQ(rig.hd.timingViolations(), 0);
  CHECK_EQ((int)mockHardware().value[10], 0);  // RW stays low (write)
  rig.hd.snapshot("8bit");
}

TEST(LCD, brightness_with_pwm_pin) {
  Parallel4 rig(16, 2, 6);
  rig.lcd.begin(16, 2);
  CHECK_EQ((int)mockHardware().mode[6], (int)OUTPUT);
  rig.lcd.setBrightness(100);
  CHECK_EQ(mockHardware().pwm[6], 255);
  rig.lcd.setBrightness(0);
  CHECK_EQ(mockHardware().pwm[6], 0);
  rig.lcd.setBrightness(50);
  CHECK_EQ(mockHardware().pwm[6], 127);
  rig.lcd.setBrightness(250);  // limited to 100%
  CHECK_EQ(mockHardware().pwm[6], 255);
}

TEST(LCD, brightness_without_pwm_pin_is_ignored) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.setBrightness(50);
  CHECK_EQ(mockHardware().pwmWrites, 0UL);
}

TEST(LCD, two_displays_share_the_driver) {
  // two displays with their own enable pin
  HD44780 hd1(16, 2, "a"), hd2(16, 2, "b");
  ParallelBus bus1(hd1, 12, 11, {5, 4, 3, 2});
  ParallelBus bus2(hd2, 12, 10, {5, 4, 3, 2});
  LCD lcd1(12, 11, 5, 4, 3, 2);
  LCD lcd2(12, 10, 5, 4, 3, 2);
  lcd1.begin(16, 2);
  lcd2.begin(16, 2);
  lcd1.print("first");
  lcd2.print("second");
  CHECK_EQ(hd1.line(0), pad("first", 16));
  CHECK_EQ(hd2.line(0), pad("second", 16));
}

TEST(LCD, images_are_recorded) {
  Parallel4 rig;
  rig.lcd.begin(16, 2);
  rig.lcd.print("Hi");
  if (HD44780::defaultRecording) {
    // blank screen after init, then one frame per character
    CHECK(rig.hd.frameCount() >= 3);
    for (const std::string &f : rig.hd.frameFiles()) {
      CHECK(std::filesystem::exists(f));
    }
  }
}
