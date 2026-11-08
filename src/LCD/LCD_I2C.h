#pragma once

#include <Wire.h>
#include "Arduino.h"
#include "CommonLCD.h"

/**
 * @brief Control LCD Display using I2C module. The Wire object needs to be set up
 * separately.
 */
class LCD_I2C : public CommonLCD {
 public:
  LCD_I2C(uint8_t lcd_addr, uint8_t led_a = 0) {
    _addr = lcd_addr;
    _backlightval = LCD_BACKLIGHT;
    _led_a = led_a;
    _p_wire = &Wire;
  }

  void setWire(TwoWire &wire) { _p_wire = &wire; }

  void begin(uint8_t lcd_cols, uint8_t lcd_rows, uint8_t charsize,
             TwoWire &wire) {
    setWire(wire);
    begin(lcd_cols, lcd_rows, charsize);
  }

  void begin(uint8_t lcd_cols, uint8_t lcd_rows,
             uint8_t charsize = LCD_5x8DOTS) override {
    _cols = lcd_cols;
    _rows = lcd_rows;
    _charsize = charsize;
    _numlines = lcd_rows;
    setRowOffsets(0x00, 0x40, 0x00 + lcd_cols, 0x40 + lcd_cols);
    if (_p_wire == nullptr) {
      _p_wire = &Wire;
    }

    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

    if (_rows > 1) {
      _displayfunction |= LCD_2LINE;
    }

    // for some 1 line displays you can select a 10 pixel high font
    if ((_charsize != 0) && (_rows == 1)) {
      _displayfunction |= LCD_5x10DOTS;
    }

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises
    // above 2.7V before sending commands. Arduino can turn on way befer 4.5V so
    // we'll wait 50
    delay(50);

    // Now we pull both RS and R/W low to begin commands
    expanderWrite(_backlightval);  // reset expander (backlight bit as set)
    delay(50);

    // put the LCD into 4 bit mode
    //  this is according to the hitachi HD44780 datasheet
    //  figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
    write4bits(0x03 << 4);
    delayMicrosecondsLCD(4500);  // wait min 4.1ms

    // second try
    write4bits(0x03 << 4);
    delayMicrosecondsLCD(4500);  // wait min 4.1ms

    // third go!
    write4bits(0x03 << 4);
    delayMicrosecondsLCD(150);

    // finally, set to 4-bit interface
    write4bits(0x02 << 4);

    // set # lines, font size, etc.
    command(LCD_FUNCTIONSET | _displayfunction);

    // turn the display on with no cursor or blinking default
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();

    // clear it off
    clear();

    // Initialize to default text direction (for roman languages)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

    // set the entry mode
    command(LCD_ENTRYMODESET | _displaymode);

    home();
  }

  // Turn the (optional) backlight off/on
  void noBacklight(void) {
    _backlightval = LCD_NOBACKLIGHT;
    expanderWrite(0);
  }

  void backlight(void) {
    _backlightval = LCD_BACKLIGHT;
    expanderWrite(0);
  }
  bool getBacklight() { return _backlightval == LCD_BACKLIGHT; }

  void setBacklight(bool new_val) {
    if (new_val) {
      backlight();  // turn backlight on
    } else {
      noBacklight();  // turn backlight off
    }
  }

 protected:
  uint8_t _addr;
  uint8_t _displayfunction = 0;
  uint8_t _cols = 0;
  uint8_t _rows = 0;
  uint8_t _charsize = 0;
  uint8_t _backlightval;
  TwoWire *_p_wire = nullptr;

  static const uint8_t LCD_BACKLIGHT = 0x08;
  static const uint8_t LCD_NOBACKLIGHT = 0x00;
  static const uint8_t En = 0x04;  // Enable bit
  static const uint8_t Rw = 0x02;  // Read/Write bit
  static const uint8_t Rs = 0x01;  // Register select bit

  void send(uint8_t value, uint8_t mode) override {
    uint8_t highnib = value & 0xf0;
    uint8_t lownib = (value << 4) & 0xf0;
    write4bits((highnib) | mode);
    write4bits((lownib) | mode);
  }

  void write4bits(uint8_t value) {
    expanderWrite(value);
    pulseEnable(value);
  }

  void expanderWrite(uint8_t _data) {
    _p_wire->beginTransmission(_addr);
    _p_wire->write((int)(_data) | _backlightval);
    _p_wire->endTransmission();
  }

  void pulseEnable(uint8_t _data) {
    expanderWrite(_data | En);  // En high
    delayMicrosecondsLCD(1);    // enable pulse must be >450ns

    expanderWrite(_data & ~En);  // En low
    delayMicrosecondsLCD(50);    // commands need > 37us to settle
  }

  void load_custom_character(uint8_t char_num, uint8_t *rows) {
    createChar(char_num, rows);
  }

  void delayMicrosecondsLCD(uint16_t us) override { delayMicroseconds(us); }
};
