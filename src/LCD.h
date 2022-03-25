#pragma once

#include "Arduino.h"
#include "Print.h"
#include <Wire.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Supported (remote) Commands which are sent over the wire
 *
 */
enum CmdEnum : uint8_t { UNDEFINED = 0, MODE, WRITE, DELAY, PULSE, BRIGHTNESS };

/**
 * @brief Command structure which is sent over the wire
 *
 */
struct __attribute__((__packed__)) Cmd {
  Cmd() = default;
  Cmd(CmdEnum id, uint16_t p1, uint16_t p2 = 0) {
    this->id = id;
    this->p1 = p1;
    this->p2 = p2;
  }
  CmdEnum id = UNDEFINED;
  uint16_t p1 = 0;
  uint16_t p2 = 0;
};

/**
 * @brief AbstractLCDDriver: commands which are serialized and sent over
 * the wire
 */
struct AbstractLCDDriver {
  virtual void pinModeLCD(uint16_t pin, uint16_t mode);
  virtual void digitalWriteLCD(uint16_t pin, uint16_t value);
  virtual void delayMicrosecondsLCD(uint16_t ms);
  virtual void pulseEnable(uint16_t pin);
  virtual void setBrightness(uint16_t pin, uint16_t percent);
};

/**
 * @brief LCDDriver with direct output to pins
 */
struct LCDDriver : public AbstractLCDDriver {
  void pinModeLCD(uint16_t pin, uint16_t mode) override { pinMode(pin, mode); }

  void digitalWriteLCD(uint16_t pin, uint16_t value) override {
    digitalWrite(pin, value);
  }

  void delayMicrosecondsLCD(uint16_t ms) override { delayMicroseconds(ms); }

  void pulseEnable(uint16_t pin) override {
    digitalWrite(pin, LOW);
    delayMicroseconds(1);
    digitalWriteLCD(pin, HIGH);
    delayMicroseconds(1); // enable pulse must be >450 ns
    digitalWrite(pin, LOW);
    delayMicroseconds(100); // commands need >37 us to settle
  }

  void setBrightness(uint16_t pin, uint16_t percent) override {
    int val = map(percent, 0, 100, 20, 225);
    analogWrite(pin, val);
  }

} defaultDriver;

/**
 * @brief Driver which sends the data over a Stream
 * (e.g. Serial Line)
 */
struct LCDWriteDriver : public AbstractLCDDriver {
  LCDWriteDriver(Print &out) { p_out = &out; }

  void pinModeLCD(uint16_t pin, uint16_t mode) override {
    Cmd cmd(MODE, pin, mode);
    p_out->write((uint8_t *)&cmd, sizeof(cmd));
  }

  void digitalWriteLCD(uint16_t pin, uint16_t value) override {
    Cmd cmd(WRITE, pin, value);
    p_out->write((uint8_t *)&cmd, sizeof(cmd));
  }

  void delayMicrosecondsLCD(uint16_t ms) override {
    Cmd cmd(DELAY, ms);
    p_out->write((uint8_t *)&cmd, sizeof(cmd));
  }

  void pulseEnable(uint16_t pin) override {
    Cmd cmd(PULSE, pin);
    p_out->write((uint8_t *)&cmd, sizeof(cmd));
  }

  void setBrightness(uint16_t pin, uint16_t percent) override {
    Cmd cmd(BRIGHTNESS, pin, percent);
    p_out->write((uint8_t *)&cmd, sizeof(cmd));
  }

protected:
  Print *p_out;
  static const int len = 80;
  char buffer[len];
};

/**
 * @brief LCDClient which processes the request provided by the indicated Stream
 * in the Arduino loop call process().
 */
class LCDClient {
  LCDClient(Stream &in) { p_in = &in; };

  /// Call this method in the loop
  void process(int delay_no_data = 100) {
    if (p_in->available() > 0) {
      if (p_in->readBytes((uint8_t *)&cmd, sizeof(Cmd)) > 0) {
        switch (cmd.id) {
        case MODE:
          pinMode(cmd.p1, cmd.p1);
          break;
        case WRITE:
          digitalWrite(cmd.p1, cmd.p1);
          break;
        case DELAY:
          delayMicroseconds(cmd.p1);
          break;
        case PULSE:
          defaultDriver.pulseEnable(cmd.p1);
          break;
        case BRIGHTNESS:
          defaultDriver.setBrightness(cmd.p1, cmd.p2);
          break;
        default:
          Serial.print("Error - undefined id");
          break;
        }
      }
    } else {
      delay(delay_no_data);
    }
  }

protected:
  Stream *p_in = nullptr;
  static const int len = 80;
  Cmd cmd;
  LCDDriver defaultDriver;
};

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
    command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
    delayMicrosecondsLCD(2000); // this command takes a long time!
  }

  void home() {
    command(LCD_RETURNHOME);    // set cursor position to zero
    delayMicrosecondsLCD(2000); // this command takes a long time!
  }

  void setCursor(uint8_t col, uint8_t row) {
    const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
    if (row >= max_lines) {
      row = max_lines - 1; // we count rows starting w/ 0
    }
    if (row >= _numlines) {
      row = _numlines - 1; // we count rows starting w/ 0
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

  void createChar(uint8_t location, const uint8_t charmap[]) {
    createChar(location, (uint8_t *)charmap);
  }

  // Allows us to fill the first 8 CGRAM locations
  // with custom characters
  void createChar(uint8_t location, uint8_t charmap[]) {
    location &= 0x7; // we only have 8 locations 0-7
    command(LCD_SETCGRAMADDR | (location << 3));
    for (int i = 0; i < 8; i++) {
      write(charmap[i]);
    }
  }

  /*********** mid level commands, for sending data/cmds */

  inline void command(uint8_t value) { send(value, LOW); }

  inline size_t write(uint8_t value) {
    send(value, HIGH);
    return 1; // assume success
  }

  using Print::write;

  virtual void delayMicrosecondsLCD(uint16_t ms) = 0;
  virtual void send(uint8_t value, uint8_t mode) = 0;

protected:
  // commands
  const uint8_t LCD_CLEARDISPLAY = 0x01;
  const uint8_t LCD_RETURNHOME = 0x02;
  const uint8_t LCD_ENTRYMODESET = 0x04;
  const uint8_t LCD_DISPLAYCONTROL = 0x08;
  const uint8_t LCD_CURSORSHIFT = 0x10;
  const uint8_t LCD_FUNCTIONSET = 0x20;
  const uint8_t LCD_SETCGRAMADDR = 0x40;
  const uint8_t LCD_SETDDRAMADDR = 0x80;

  // flags for display entry mode
  const uint8_t LCD_ENTRYRIGHT = 0x00;
  const uint8_t LCD_ENTRYLEFT = 0x02;
  const uint8_t LCD_ENTRYSHIFTINCREMENT = 0x01;
  const uint8_t LCD_ENTRYSHIFTDECREMENT = 0x00;

  // flags for display on/off control
  const uint8_t LCD_DISPLAYON = 0x04;
  const uint8_t LCD_DISPLAYOFF = 0x00;
  const uint8_t LCD_CURSORON = 0x02;
  const uint8_t LCD_CURSOROFF = 0x00;
  const uint8_t LCD_BLINKON = 0x01;
  const uint8_t LCD_BLINKOFF = 0x00;

  // flags for display/cursor shift
  const uint8_t LCD_DISPLAYMOVE = 0x08;
  const uint8_t LCD_CURSORMOVE = 0x00;
  const uint8_t LCD_MOVERIGHT = 0x04;
  const uint8_t LCD_MOVELEFT = 0x00;

  // flags for function set
  static const uint8_t LCD_8BITMODE = 0x10;
  static const uint8_t LCD_4BITMODE = 0x00;
  static const uint8_t LCD_2LINE = 0x08;
  static const uint8_t LCD_1LINE = 0x00;
  static const uint8_t LCD_5x10DOTS = 0x04;
  static const uint8_t LCD_5x8DOTS = 0x00;

  // variables
  uint8_t _row_offsets[4];
  uint8_t _displaymode;
  uint8_t _displaycontrol;
  uint8_t _numlines;
};

/**
 * @brief Output to LCD
 * 
 */
class LCD : public CommonLCD {
public:
  // When the display powers up, it is configured as follows:
  //
  // 1. Display clear
  // 2. Function set:
  //    DL = 1; 8-bit interface data
  //    N = 0; 1-line display
  //    F = 0; 5x8 dot character font
  // 3. Display on/off control:
  //    D = 0; Display off
  //    C = 0; Cursor off
  //    B = 0; Blinking off
  // 4. Entry mode set:
  //    I/D = 1; Increment by 1
  //    S = 0; No shift
  //
  // Note, however, that resetting the Arduino doesn't reset the LCD, so we
  // can't assume that it's in that state when a sketch starts (and the
  // LCD constructor is called).

  LCD(uint8_t rs, uint8_t rw, uint8_t enable, uint8_t d0, uint8_t d1,
      uint8_t d2, uint8_t d3, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
      uint8_t leda = 0, AbstractLCDDriver &driver = defaultDriver) {
    init(0, rs, rw, enable, d0, d1, d2, d3, d4, d5, d6, d7, leda, driver);
  }

  LCD(uint8_t rs, uint8_t enable, uint8_t d0, uint8_t d1, uint8_t d2,
      uint8_t d3, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
      uint8_t leda = 0, AbstractLCDDriver &driver = defaultDriver) {
    init(0, rs, 255, enable, d0, d1, d2, d3, d4, d5, d6, d7, leda, driver);
  }

  LCD(uint8_t rs, uint8_t rw, uint8_t enable, uint8_t d0, uint8_t d1,
      uint8_t d2, uint8_t d3, uint8_t leda = 0,
      AbstractLCDDriver &driver = defaultDriver) {
    init(1, rs, rw, enable, d0, d1, d2, d3, 0, 0, 0, 0, leda, driver);
  }

  LCD(uint8_t rs, uint8_t enable, uint8_t d0, uint8_t d1, uint8_t d2,
      uint8_t d3, uint8_t leda = 0, AbstractLCDDriver &driver = defaultDriver) {
    init(1, rs, 255, enable, d0, d1, d2, d3, 0, 0, 0, 0, leda, driver);
  }

  void init(uint8_t fourbitmode, uint8_t rs, uint8_t rw, uint8_t enable,
            uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4,
            uint8_t d5, uint8_t d6, uint8_t d7, uint8_t led_a,
            AbstractLCDDriver &driver = defaultDriver) {

    p_driver = &driver;
    _rs_pin = rs;
    _rw_pin = rw;
    _enable_pin = enable;

    _data_pins[0] = d0;
    _data_pins[1] = d1;
    _data_pins[2] = d2;
    _data_pins[3] = d3;
    _data_pins[4] = d4;
    _data_pins[5] = d5;
    _data_pins[6] = d6;
    _data_pins[7] = d7;

    _led_a = led_a;

    if (fourbitmode)
      _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
    else
      _displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;

    begin(16, 1);
  }

  void begin(uint8_t cols, uint8_t lines, uint8_t dotsize = LCD_5x8DOTS) {
    if (lines > 1) {
      _displayfunction |= LCD_2LINE;
    }
    _numlines = lines;

    if (_led_a != 0) {
      pinModeLCD(_led_a, OUTPUT);
    }

    setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);

    // for some 1 line displays you can select a 10 pixel high font
    if ((dotsize != LCD_5x8DOTS) && (lines == 1)) {
      _displayfunction |= LCD_5x10DOTS;
    }

    pinModeLCD(_rs_pin, OUTPUT);
    // we can save 1 pin by not using RW. Indicate by passing 255 instead of
    // pin#
    if (_rw_pin != 255) {
      pinModeLCD(_rw_pin, OUTPUT);
    }
    pinModeLCD(_enable_pin, OUTPUT);

    // Do these once, instead of every time a character is drawn for speed
    // reasons.
    for (int i = 0; i < ((_displayfunction & LCD_8BITMODE) ? 8 : 4); ++i) {
      pinModeLCD(_data_pins[i], OUTPUT);
    }

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40 ms after power rises
    // above 2.7 V before sending commands. Arduino can turn on way before 4.5 V
    // so we'll wait 50
    delayMicrosecondsLCD(50000);
    // Now we pull both RS and R/W low to begin commands
    digitalWriteLCD(_rs_pin, LOW);
    digitalWriteLCD(_enable_pin, LOW);
    if (_rw_pin != 255) {
      digitalWriteLCD(_rw_pin, LOW);
    }

    // put the LCD into 4 bit or 8 bit mode
    if (!(_displayfunction & LCD_8BITMODE)) {
      // this is according to the Hitachi HD44780 datasheet
      // figure 24, pg 46

      // we start in 8bit mode, try to set 4 bit mode
      write4bits(0x03);
      delayMicrosecondsLCD(4500); // wait min 4.1ms

      // second try
      write4bits(0x03);
      delayMicrosecondsLCD(4500); // wait min 4.1ms

      // third go!
      write4bits(0x03);
      delayMicrosecondsLCD(150);

      // finally, set to 4-bit interface
      write4bits(0x02);
    } else {
      // this is according to the Hitachi HD44780 datasheet
      // page 45 figure 23

      // Send function set command sequence
      command(LCD_FUNCTIONSET | _displayfunction);
      delayMicrosecondsLCD(4500); // wait more than 4.1 ms

      // second try
      command(LCD_FUNCTIONSET | _displayfunction);
      delayMicrosecondsLCD(150);

      // third go
      command(LCD_FUNCTIONSET | _displayfunction);
    }

    // finally, set # lines, font size, etc.
    command(LCD_FUNCTIONSET | _displayfunction);

    // turn the display on with no cursor or blinking default
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();

    // clear it off
    clear();

    // Initialize to default text direction (for romance languages)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // set the entry mode
    command(LCD_ENTRYMODESET | _displaymode);
  }

  /// Defines the brightness (0-100)
  void setBrightness(int percent) {
    if (_led_a != 0) {
      p_driver->setBrightness(_led_a, percent);
    }
  }

protected:
  /************ low level data pushing commands **********/

  // write either command or data, with automatic 4/8-bit selection
  void send(uint8_t value, uint8_t mode) {
    digitalWriteLCD(_rs_pin, mode);

    // if there is a RW pin indicated, set it low to Write
    if (_rw_pin != 255) {
      digitalWriteLCD(_rw_pin, LOW);
    }

    if (_displayfunction & LCD_8BITMODE) {
      write8bits(value);
    } else {
      write4bits(value >> 4);
      write4bits(value);
    }
  }

  void pulseEnable(void) { p_driver->pulseEnable(_enable_pin); }

  void write4bits(uint8_t value) {
    for (int i = 0; i < 4; i++) {
      digitalWriteLCD(_data_pins[i], (value >> i) & 0x01);
    }

    pulseEnable();
  }

  void write8bits(uint8_t value) {
    for (int i = 0; i < 8; i++) {
      digitalWriteLCD(_data_pins[i], (value >> i) & 0x01);
    }
    pulseEnable();
  }

  void pinModeLCD(uint16_t pin, uint16_t mode) {
    p_driver->pinModeLCD(pin, mode);
  }

  void digitalWriteLCD(uint16_t pin, uint16_t value) {
    p_driver->digitalWriteLCD(pin, value);
  }

  void delayMicrosecondsLCD(uint16_t ms) { p_driver->delayMicrosecondsLCD(ms); }

  // variables
  uint8_t _rs_pin;     // LOW: command.  HIGH: character.
  uint8_t _rw_pin;     // LOW: write to LCD.  HIGH: read from LCD.
  uint8_t _enable_pin; // activated by a HIGH pulse.
  uint8_t _data_pins[8];
  uint8_t _led_a;

  uint8_t _displayfunction;
  uint8_t _initialized;

  AbstractLCDDriver *p_driver = nullptr;
};



/**
 * @brief Control LCD Display using I2C module
 * 
 */
class LCD_I2C : public CommonLCD {
public:
  LCD_I2C(uint8_t lcd_addr, uint8_t lcd_cols, uint8_t lcd_rows,
          uint8_t charsize) {
    _addr = lcd_addr;
    _cols = lcd_cols;
    _rows = lcd_rows;
    _charsize = charsize;
    _backlightval = LCD_BACKLIGHT;
  }

  void begin() {
    Wire.begin();
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
    expanderWrite(
        _backlightval); // reset expanderand turn backlight off (Bit 8 =1)
    delay(1000);

    // put the LCD into 4 bit mode
    //  this is according to the hitachi HD44780 datasheet
    //  figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
    write4bits(0x03 << 4);
    delayMicroseconds(4500); // wait min 4.1ms

    // second try
    write4bits(0x03 << 4);
    delayMicroseconds(4500); // wait min 4.1ms

    // third go!
    write4bits(0x03 << 4);
    delayMicroseconds(150);

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

protected:
  uint8_t _addr;
  uint8_t _displaycontrol;
  uint8_t _displayfunction;
  uint8_t _displaymode;
  uint8_t _cols;
  uint8_t _rows;
  uint8_t _charsize;
  uint8_t _backlightval;

  const uint8_t LCD_BACKLIGHT = 0x08;
  const uint8_t LCD_NOBACKLIGHT = 0x00;
  const uint8_t En = B00000100; // Enable bit
  const uint8_t Rw = B00000010; // Read/Write bit
  const uint8_t Rs = B00000001; // Register select bit

  void send(uint8_t value, uint8_t mode) {
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
    Wire.beginTransmission(_addr);
    Wire.write((int)(_data) | _backlightval);
    Wire.endTransmission();
  }

  void pulseEnable(uint8_t _data) {
    expanderWrite(_data | En); // En high
    delayMicroseconds(1);      // enable pulse must be >450ns

    expanderWrite(_data & ~En); // En low
    delayMicroseconds(50);      // commands need > 37us to settle
  }

  void load_custom_character(uint8_t char_num, uint8_t *rows) {
    createChar(char_num, rows);
  }

  void setBacklight(uint8_t new_val) {
    if (new_val) {
      backlight(); // turn backlight on
    } else {
      noBacklight(); // turn backlight off
    }
  }

  void printstr(const char c[]) {
    // This function is not identical to the function used for "real" I2C
    // displays it's here so the user sketch doesn't have to be changed
    print(c);
  }
};


/**
 * @brief LCDBarGraph is class for displaying analog values in LCD display,
 * which is previously initialized. This library uses LiquedCrystal library
 * for displaying.
 * @author Balazs Kelemen
 * @copyright 2010 Balazs Kelemen
 * contact: prampec+arduino@gmail.com
 * credits: Hans van Neck
 * copying: permission statement:
    This file is part of LCDBarGraph.
    LCDBarGraph is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

class LCDBarGraph {
public:
  /**
   * Create an instance of the class. The bar will be drawn in the startY row
   * of the LCD, from the startX column positon (inclusive) to to the
   * startX+numCols column position (inclusive). lcd - A LiquidCristal instance
   * should be passed. numCols - Width of the bar. startX - Horzontal starting
   * position (column) of the bar. Zero based value. startY - Vertical starting
   * position (row) of the bar. Zero based value.
   */
  LCDBarGraph(CommonLCD &lcd, byte numCols, byte startX = 0, byte startY = 0) {
    // -- setting fields
    _lcd = &lcd;
    _numCols = numCols;
    _startX = startX;
    _startY = startY;
    // -- creating characters
#ifndef USE_BUILDIN_FILLED_CHAR
    _lcd->createChar(0, this->_level0);
#endif
    _lcd->createChar(1, this->_level1);
    _lcd->createChar(2, this->_level2);
    _lcd->createChar(3, this->_level3);
    _lcd->createChar(4, this->_level4);
    _lcd->clear(); // put lcd back into DDRAM mode
    // -- setting initial values
    this->_prevValue = 0;     // -- cached value
    this->_lastFullChars = 0; // -- cached value
  }

  /**
   * Draw a bargraph with a value between 0 and maxValue.
   */
  void drawValue(int value, int maxValue) {
    // -- calculate full (filled) character count
    byte fullChars = (long)value * _numCols / maxValue;
    // -- calculate partial character bar count
    byte mod = ((long)value * _numCols * 5 / maxValue) % 5;

    // -- if value does not change, do not draw anything
    int normalizedValue = (int)fullChars * 5 + mod;
    if (this->_prevValue != normalizedValue) {
      // -- do not clear the display to eliminate flickers
      _lcd->setCursor(_startX, _startY);

      // -- write filled characters
      for (byte i = 0; i < fullChars; i++) {
#ifdef USE_BUILDIN_FILLED_CHAR
        _lcd->write(
            (byte)USE_BUILDIN_FILLED_CHAR); // -- use build in filled char
#else
        _lcd->write((byte)0);
#endif
      }

      // -- write the partial character
      if (mod > 0) {
        _lcd->write(mod); // -- index the right partial character
        ++fullChars;
      }

      // -- clear characters left over the previous draw
      for (byte i = fullChars; i < this->_lastFullChars; i++) {
        _lcd->write(' ');
      }

      // -- save cache
      this->_lastFullChars = fullChars;
      this->_prevValue = normalizedValue;
    }
  }

private:
  CommonLCD *_lcd;
  byte _numCols;
  byte _startX;
  byte _startY;
  int _prevValue;
  byte _lastFullChars;

// -- initializing bar segment characters
#ifndef USE_BUILDIN_FILLED_CHAR
  // -- filled character
  const uint8_t _level0[8] = {B11111, B11111, B11111, B11111,
                              B11111, B11111, B11111, B11111};
#endif

  // -- character with one bar
  const uint8_t _level1[8] = {B10000, B10000, B10000, B10000,
                              B10000, B10000, B10000, B10000};
  // -- character with two bars
  const uint8_t _level2[8] = {B11000, B11000, B11000, B11000,
                              B11000, B11000, B11000, B11000};
  // -- character with three bars
  const uint8_t _level3[8] = {B11100, B11100, B11100, B11100,
                              B11100, B11100, B11100, B11100};
  // -- character with four bars
  const uint8_t _level4[8] = {B11110, B11110, B11110, B11110,
                              B11110, B11110, B11110, B11110};
};
