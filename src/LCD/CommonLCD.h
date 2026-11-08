#pragma once

#include <stdint.h>
#include "Arduino.h"
#include "Print.h"

/**
 * @brief Output to LCD - Common Functionality
 *
 */
class CommonLCD : public Print {
 public:
  void setRowOffsets(int row0, int row1, int row2, int row3) {
    _row_offsets[0] = row0;
    _row_offsets[1] = row1;
    _row_offsets[2] = row2;
    _row_offsets[3] = row3;
  }

  /********** high level commands, for the user! */
  void clear() {
    command(LCD_CLEARDISPLAY);   // clear display, set cursor position to zero
    delayMicrosecondsLCD(2000);  // this command takes a long time!
  }

  void home() {
    command(LCD_RETURNHOME);     // set cursor position to zero
    delayMicrosecondsLCD(2000);  // this command takes a long time!
  }

  void setCursor(uint8_t col, uint8_t row) {
    const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
    if (row >= max_lines) {
      row = max_lines - 1;  // we count rows starting w/ 0
    }
    if (row >= _numlines) {
      row = _numlines - 1;  // we count rows starting w/ 0
    }

    command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));
  }

  // Turn the display on/off (quickly)
  void noDisplay() {
    _displaycontrol &= ~LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
  }
  void display() {
    _displaycontrol |= LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
  }

  // Turns the underline cursor on/off
  void noCursor() {
    _displaycontrol &= ~LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
  }

  void cursor() {
    _displaycontrol |= LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
  }

  // Turn on and off the blinking cursor
  void noBlink() {
    _displaycontrol &= ~LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
  }
  void blink() {
    _displaycontrol |= LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
  }

  // These commands scroll the display without changing the RAM
  void scrollDisplayLeft(void) {
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
  }
  void scrollDisplayRight(void) {
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
  }

  // This is for text that flows Left to Right
  void leftToRight(void) {
    _displaymode |= LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
  }

  // This is for text that flows Right to Left
  void rightToLeft(void) {
    _displaymode &= ~LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
  }

  // This will 'right justify' text from the cursor
  void autoscroll(void) {
    _displaymode |= LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
  }

  // This will 'left justify' text from the cursor
  void noAutoscroll(void) {
    _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
  }

  // Allows us to fill the first 8 CGRAM locations
  // with custom characters
  void createChar(uint8_t location, const uint8_t charmap[]) {
    location &= 0x7;  // we only have 8 locations 0-7
    command(LCD_SETCGRAMADDR | (location << 3));
    for (int i = 0; i < 8; i++) {
      write(charmap[i]);
    }
  }

  /// Starts the processing by defining the number of columns and rows
  virtual void begin(uint8_t lcd_cols, uint8_t lcd_rows,
                     uint8_t charsize = LCD_5x8DOTS) = 0;

  /// Output of a single char
  inline size_t write(uint8_t value) {
    send(value, HIGH);
    return 1;  // assume success
  }

  using Print::write;

  /// Defines the PWM pin of the backlight which is used by setBrightness().
  /// For LCD this must be called before begin().
  void setBrightnessPin(uint8_t pin) { _led_a = pin; }

  /// Defines the brightness (0-100) of the (PWM) backlight pin, if one was
  /// defined
  virtual void setBrightness(uint16_t percent) {
    if (_led_a == 0) return;
    if (percent > 100) percent = 100;
    analogWrite(_led_a, (int)map(percent, 0, 100, 0, 255));
  }

 protected:
  // commands
  static const uint8_t LCD_CLEARDISPLAY = 0x01;
  static const uint8_t LCD_RETURNHOME = 0x02;
  static const uint8_t LCD_ENTRYMODESET = 0x04;
  static const uint8_t LCD_DISPLAYCONTROL = 0x08;
  static const uint8_t LCD_CURSORSHIFT = 0x10;
  static const uint8_t LCD_FUNCTIONSET = 0x20;
  static const uint8_t LCD_SETCGRAMADDR = 0x40;
  static const uint8_t LCD_SETDDRAMADDR = 0x80;

  // flags for display entry mode
  static const uint8_t LCD_ENTRYRIGHT = 0x00;
  static const uint8_t LCD_ENTRYLEFT = 0x02;
  static const uint8_t LCD_ENTRYSHIFTINCREMENT = 0x01;
  static const uint8_t LCD_ENTRYSHIFTDECREMENT = 0x00;

  // flags for display on/off control
  static const uint8_t LCD_DISPLAYON = 0x04;
  static const uint8_t LCD_DISPLAYOFF = 0x00;
  static const uint8_t LCD_CURSORON = 0x02;
  static const uint8_t LCD_CURSOROFF = 0x00;
  static const uint8_t LCD_BLINKON = 0x01;
  static const uint8_t LCD_BLINKOFF = 0x00;

  // flags for display/cursor shift
  static const uint8_t LCD_DISPLAYMOVE = 0x08;
  static const uint8_t LCD_CURSORMOVE = 0x00;
  static const uint8_t LCD_MOVERIGHT = 0x04;
  static const uint8_t LCD_MOVELEFT = 0x00;

  // flags for function set
  static const uint8_t LCD_8BITMODE = 0x10;
  static const uint8_t LCD_4BITMODE = 0x00;
  static const uint8_t LCD_2LINE = 0x08;
  static const uint8_t LCD_1LINE = 0x00;
  static const uint8_t LCD_5x10DOTS = 0x04;
  static const uint8_t LCD_5x8DOTS = 0x00;

  // variables
  uint8_t _row_offsets[4] = {0x00, 0x40, 0x14, 0x54};
  uint8_t _displaymode = 0;
  uint8_t _displaycontrol = 0;
  uint8_t _numlines = 1;
  uint8_t _led_a = 0;  // LED brightness pin (0: not used)

  inline void command(uint8_t value) { send(value, LOW); }
  virtual void delayMicrosecondsLCD(uint16_t us) { delayMicroseconds(us); }
  virtual void send(uint8_t value, uint8_t mode) = 0;
};
