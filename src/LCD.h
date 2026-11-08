#pragma once

#include <Wire.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Arduino.h"
#include "Print.h"


/**
 * @brief Supported (remote) Commands which are sent over the wire
 *
 */
enum class CmdEnum : uint8_t {
  UNDEFINED = 0,
  MODE,
  WRITE,
  DELAY,
  PULSE,
  BRIGHTNESS
};

/**
 * @brief Command structure which is sent over the wire. Each command starts
 * with a sync byte so that the receiver can resynchronize after lost data.
 */
struct __attribute__((__packed__)) Cmd {
  static const uint8_t SYNC = 0xA5;

  Cmd() = default;
  Cmd(CmdEnum id, uint16_t p1, uint16_t p2 = 0) {
    this->id = id;
    this->p1 = p1;
    this->p2 = p2;
  }
  uint8_t sync = SYNC;
  CmdEnum id = CmdEnum::UNDEFINED;
  uint16_t p1 = 0;
  uint16_t p2 = 0;
};

/**
 * @brief AbstractLCDDriver: commands which are serialized and sent over
 * the wire
 */
struct AbstractLCDDriver {
  virtual ~AbstractLCDDriver() = default;
  virtual void pinModeLCD(uint16_t pin, uint16_t mode) = 0;
  virtual void digitalWriteLCD(uint16_t pin, uint16_t value) = 0;
  virtual void delayMicrosecondsLCD(uint16_t us) = 0;
  virtual void pulseEnable(uint16_t pin) = 0;
  virtual void setBrightness(uint16_t pin, uint16_t percent) = 0;
};

/**
 * @brief LCDDriver with direct output to pins
 */
struct LCDDriver : public AbstractLCDDriver {
  void pinModeLCD(uint16_t pin, uint16_t mode) override { pinMode(pin, mode); }

  void digitalWriteLCD(uint16_t pin, uint16_t value) override {
    digitalWrite(pin, value);
  }

  void delayMicrosecondsLCD(uint16_t us) override { delayMicroseconds(us); }

  void pulseEnable(uint16_t pin) override {
    digitalWriteLCD(pin, LOW);
    delayMicroseconds(1);
    digitalWriteLCD(pin, HIGH);
    delayMicroseconds(1);  // enable pulse must be >450 ns
    digitalWriteLCD(pin, LOW);
    delayMicroseconds(100);  // commands need >37 us to settle
  }

  /// Sets the brightness of a PWM pin: 0 (off) to 100 (full)
  void setBrightness(uint16_t pin, uint16_t percent) override {
    if (percent > 100) percent = 100;
    analogWrite(pin, (int)map(percent, 0, 100, 0, 255));
  }
};

/// Provides the default driver (created on first use, so that the header can
/// be included from multiple source files).
inline LCDDriver &defaultLCDDriver() {
  static LCDDriver driver;
  return driver;
}

/**
 * @brief Driver which sends the data over a Print (e.g. Serial Line)
 */
struct LCDWriteDriver : public AbstractLCDDriver {
  LCDWriteDriver(Print &out) { p_out = &out; }

  void pinModeLCD(uint16_t pin, uint16_t mode) override {
    sendCmd(Cmd(CmdEnum::MODE, pin, mode));
  }

  void digitalWriteLCD(uint16_t pin, uint16_t value) override {
    sendCmd(Cmd(CmdEnum::WRITE, pin, value));
  }

  void delayMicrosecondsLCD(uint16_t us) override {
    sendCmd(Cmd(CmdEnum::DELAY, us));
  }

  void pulseEnable(uint16_t pin) override {
    sendCmd(Cmd(CmdEnum::PULSE, pin));
  }

  void setBrightness(uint16_t pin, uint16_t percent) override {
    sendCmd(Cmd(CmdEnum::BRIGHTNESS, pin, percent));
  }

 protected:
  Print *p_out;

  void sendCmd(const Cmd &cmd) {
    p_out->write((const uint8_t *)&cmd, sizeof(cmd));
  }
};

/**
 * @brief LCDServer receives the commands sent by the LCDWriteDriver from the
 * indicated Stream and executes them on the local pins. Call process() in the
 * Arduino loop.
 */
class LCDServer {
 public:
  LCDServer(Stream &in) { p_in = &in; }

  /// Call this method in the loop
  void process(int delay_no_data = 100) {
    // resynchronize: skip everything up to the next sync byte
    while (p_in->available() > 0 && p_in->peek() != Cmd::SYNC) {
      p_in->read();
    }

    if (p_in->available() == 0) {
      delay(delay_no_data);
      return;
    }

    // wait until the full command has arrived
    if (p_in->available() < (int)sizeof(Cmd)) return;

    if (p_in->readBytes((uint8_t *)&cmd, sizeof(Cmd)) != sizeof(Cmd)) return;

    switch (cmd.id) {
      case CmdEnum::MODE:
        driver.pinModeLCD(cmd.p1, cmd.p2);
        break;
      case CmdEnum::WRITE:
        driver.digitalWriteLCD(cmd.p1, cmd.p2);
        break;
      case CmdEnum::DELAY:
        driver.delayMicrosecondsLCD(cmd.p1);
        break;
      case CmdEnum::PULSE:
        driver.pulseEnable(cmd.p1);
        break;
      case CmdEnum::BRIGHTNESS:
        driver.setBrightness(cmd.p1, cmd.p2);
        break;
      default:
        break;  // unknown command: ignore
    }
  }

 protected:
  Stream *p_in = nullptr;
  Cmd cmd;
  LCDDriver driver;
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
      uint8_t leda = 0, AbstractLCDDriver &driver = defaultLCDDriver()) {
    init(0, rs, rw, enable, d0, d1, d2, d3, d4, d5, d6, d7, leda, driver);
  }

  LCD(uint8_t rs, uint8_t enable, uint8_t d0, uint8_t d1, uint8_t d2,
      uint8_t d3, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
      uint8_t leda = 0, AbstractLCDDriver &driver = defaultLCDDriver()) {
    init(0, rs, 255, enable, d0, d1, d2, d3, d4, d5, d6, d7, leda, driver);
  }

  LCD(uint8_t rs, uint8_t rw, uint8_t enable, uint8_t d0, uint8_t d1,
      uint8_t d2, uint8_t d3, uint8_t leda = 0,
      AbstractLCDDriver &driver = defaultLCDDriver()) {
    init(1, rs, rw, enable, d0, d1, d2, d3, 0, 0, 0, 0, leda, driver);
  }

  LCD(uint8_t rs, uint8_t enable, uint8_t d0, uint8_t d1, uint8_t d2,
      uint8_t d3, uint8_t leda = 0, AbstractLCDDriver &driver = defaultLCDDriver()) {
    init(1, rs, 255, enable, d0, d1, d2, d3, 0, 0, 0, 0, leda, driver);
  }

  /// Initializes the display. This must be called before using the display
  /// (typically in setup()).
  void begin(uint8_t cols, uint8_t lines,
             uint8_t dotsize = LCD_5x8DOTS) override {
    _displayfunction &= ~(LCD_2LINE | LCD_5x10DOTS);
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
      delayMicrosecondsLCD(4500);  // wait min 4.1ms

      // second try
      write4bits(0x03);
      delayMicrosecondsLCD(4500);  // wait min 4.1ms

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
      delayMicrosecondsLCD(4500);  // wait more than 4.1 ms

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
  void setBrightness(uint16_t percent) override {
    if (_led_a != 0) {
      p_driver->setBrightness(_led_a, percent);
    }
  }

 protected:
  void init(uint8_t fourbitmode, uint8_t rs, uint8_t rw, uint8_t enable,
            uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4,
            uint8_t d5, uint8_t d6, uint8_t d7, uint8_t led_a,
            AbstractLCDDriver &driver = defaultLCDDriver()) {
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
    // Note: begin() is not called here, so that the constructor never touches
    // the pins (or the serial line) before setup() runs.
  }
  // write either command or data, with automatic 4/8-bit selection
  void send(uint8_t value, uint8_t mode) override {
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

  void delayMicrosecondsLCD(uint16_t us) override {
    p_driver->delayMicrosecondsLCD(us);
  }

  // variables
  uint8_t _rs_pin;      // LOW: command.  HIGH: character.
  uint8_t _rw_pin;      // LOW: write to LCD.  HIGH: read from LCD.
  uint8_t _enable_pin;  // activated by a HIGH pulse.
  uint8_t _data_pins[8];

  uint8_t _displayfunction = 0;

  AbstractLCDDriver *p_driver = nullptr;
};

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

/**
 * @brief Horizontal bar graph for an LCD. The bar is drawn into one row,
 * starting at column startX and spanning numCols characters. Each character
 * shows 5 steps, so the bar has a resolution of numCols * 5 steps.
 *
 * The bar uses the custom characters 0-4 of the display (0: completely
 * filled, 1-4: 1 to 4 filled pixel columns). Any custom characters you defined
 * in these slots are overwritten. Create the bar after lcd.begin(). If the
 * display has a built-in solid block character, you can select it with
 * setFilledChar().
 */
class LCDBarGraph {
 public:
  /**
   * @param lcd the (already started) display
   * @param numCols width of the bar in characters
   * @param startX column of the first bar character (0 based)
   * @param startY row of the bar (0 based)
   */
  LCDBarGraph(CommonLCD &lcd, byte numCols, byte startX = 0, byte startY = 0) {
    p_lcd = &lcd;
    cols = numCols;
    x = startX;
    y = startY;
    for (byte slot = 0; slot <= 4; slot++) {
      defineGlyph(slot);
    }
    p_lcd->clear();  // leave CGRAM mode and start with an empty display
  }

  /**
   * Defines the character code that is used for completely filled positions.
   * Use this if the display has a built-in solid block character. The default
   * is -1: the solid block is defined as custom character 0.
   */
  void setFilledChar(int code) {
    filledChar = code;
    drawnSteps = -1;  // force a redraw with the new character
  }

  /// Provides the character code for filled positions (-1: custom char 0)
  int getFilledChar() const { return filledChar; }

  /**
   * Draws the bar for a value between 0 and maxValue. Out of range values are
   * limited to this range. Nothing is written if the bar does not change.
   */
  void drawValue(int value, int maxValue) {
    if (maxValue <= 0) return;
    if (value < 0) value = 0;
    if (value > maxValue) value = maxValue;

    // bar length in steps of 1/5 character
    long steps = (long)value * cols * 5 / maxValue;
    if (steps == drawnSteps) return;

    byte whole = steps / 5;
    byte rest = steps % 5;
    byte used = whole + (rest > 0 ? 1 : 0);

    p_lcd->setCursor(x, y);
    byte solid = filledChar >= 0 ? (byte)filledChar : 0;
    for (byte i = 0; i < whole; i++) p_lcd->write(solid);
    if (rest > 0) p_lcd->write(rest);
    // erase what the previous (longer) bar left behind
    for (byte i = used; i < drawnChars; i++) p_lcd->write(' ');

    drawnSteps = steps;
    drawnChars = used;
  }

 private:
  CommonLCD *p_lcd;
  byte cols;
  byte x;
  byte y;
  long drawnSteps = 0;
  byte drawnChars = 0;
  int filledChar = -1;

  /// Stores a glyph in the given custom character slot: slot 1-4 fill that
  /// many pixel columns from the left, slot 0 is the solid block (5 columns).
  void defineGlyph(byte slot) {
    byte width = (slot == 0) ? 5 : slot;
    byte row = (0x1F << (5 - width)) & 0x1F;
    uint8_t glyph[8];
    for (byte i = 0; i < 8; i++) glyph[i] = row;
    p_lcd->createChar(slot, glyph);
  }
};

// forward declarations
class LCDMenuScreen;
class LCDMenu;

/// Callback which is called when a screen or a text is selected
typedef void (*LCDMenuCallback)(uint16_t screen_pos, int text_pos,
                                const char *txt);

/**
 * @brief Defines a text at the indicated position. Only selectable texts can
 * be reached with nextText()/priorText().
 */
class LCDMenuText {
 public:
  LCDMenuText(uint16_t x, uint16_t y, const char *txt, bool selectable = false,
              LCDMenuCallback selectText = nullptr) {
    this->x = x;
    this->y = y;
    this->txt = txt;
    this->selectable = selectable;
    this->p_select_text = selectText;
  }

 protected:
  friend class LCDMenuScreen;
  uint16_t x;
  uint16_t y;
  const char *txt = nullptr;
  bool selectable;
  LCDMenuCallback p_select_text = nullptr;
};

/**
 * @brief Defines texts for one screen. The array of texts must stay valid for
 * the lifetime of the screen.
 */
class LCDMenuScreen {
 public:
  template <int N>
  LCDMenuScreen(const LCDMenuText *(&texts)[N],
                LCDMenuCallback selectScreen = nullptr,
                LCDMenuCallback selectText = nullptr) {
    this->txt = texts;
    this->len = N;
    this->p_select_screen = selectScreen;
    this->p_select_text = selectText;
  }

  /// Shows all texts and puts the cursor on the first selectable one
  void display() {
    if (p_lcd == nullptr) return;
    clear();
    for (int j = 0; j < len; j++) {
      p_lcd->setCursor(txt[j]->x, txt[j]->y);
      p_lcd->print(txt[j]->txt);
    }
    setText(0);
  }

  /// calls the callback of the screen
  void selectScreen(uint16_t pos) {
    if (p_select_screen != nullptr) {
      p_select_screen(pos, text_pos, selectedText());
    }
  }

  /// moves to the next selectable text (wraps around)
  int nextText() { return setText(text_pos + 1); }

  /// moves to the prior selectable text (wraps around)
  int priorText() { return setText(text_pos - 1); }

  /// selects the n-th selectable text; returns -1 if there is none
  int setText(int pos) {
    int count = selectableCount();
    if (count == 0 || p_lcd == nullptr) {
      text_pos = -1;
      if (p_lcd != nullptr) {
        p_lcd->noCursor();
        p_lcd->noBlink();
      }
      return text_pos;
    }
    if (pos >= count) pos = 0;
    if (pos < 0) pos = count - 1;
    text_pos = pos;
    const LCDMenuText *t = selectableAt(text_pos);
    p_lcd->setCursor(t->x, t->y);
    p_lcd->cursor();
    p_lcd->blink();
    return text_pos;
  }

  void clear() {
    if (p_lcd != nullptr) p_lcd->clear();
  }

 protected:
  friend class LCDMenu;
  CommonLCD *p_lcd = nullptr;
  int len = 0;
  const LCDMenuText *const *txt = nullptr;
  LCDMenuCallback p_select_screen = nullptr;
  LCDMenuCallback p_select_text = nullptr;
  int text_pos = -1;  // index in the selectable texts; -1: none

  void setLCD(CommonLCD *lcd) { p_lcd = lcd; }

  int selectableCount() const {
    int count = 0;
    for (int j = 0; j < len; j++) {
      if (txt[j]->selectable) count++;
    }
    return count;
  }

  const LCDMenuText *selectableAt(int n) const {
    for (int j = 0; j < len; j++) {
      if (txt[j]->selectable && n-- == 0) return txt[j];
    }
    return nullptr;
  }

  const char *selectedText() const {
    const LCDMenuText *t = text_pos >= 0 ? selectableAt(text_pos) : nullptr;
    return t != nullptr ? t->txt : nullptr;
  }

  /// calls the callbacks on the actually selected text
  void selectText(uint16_t screen) {
    if (text_pos < 0) return;
    const LCDMenuText *t = selectableAt(text_pos);
    if (t == nullptr) return;
    // callback defined on the text element
    if (t->p_select_text != nullptr) {
      t->p_select_text(screen, text_pos, t->txt);
    }
    // callback defined on the screen
    if (p_select_text != nullptr) {
      p_select_text(screen, text_pos, t->txt);
    }
  }
};

/**
 * @brief Defines a Menu as an array of Screens. The array must stay valid for
 * the lifetime of the menu.
 */
class LCDMenu {
 public:
  template <int N>
  LCDMenu(CommonLCD &lcd, LCDMenuScreen *(&screens)[N]) {
    this->p_lcd = &lcd;
    this->screens = screens;
    this->len = N;
    current = 0;
    // setup lcd for screens
    for (int j = 0; j < len; j++) {
      screens[j]->setLCD(&lcd);
    }
  }

  /// Activates the menu and shows the indicated screen
  void begin(int pos = 0) {
    active = true;
    setScreen(pos);
  }

  /// Deactivates the menu
  void end() { active = false; }

  /// Clears the screen
  void clear() { p_lcd->clear(); }

  /// Moves to the next screen (wraps around)
  int nextScreen() { return setScreen(current + 1); }

  /// Moves to the prior screen (wraps around)
  int priorScreen() { return setScreen(current - 1); }

  /// Moves to the indicated screen index
  int setScreen(int pos) {
    if (active && len > 0) {
      if (pos >= len) pos = 0;
      if (pos < 0) pos = len - 1;
      current = pos;
      screens[current]->display();
    }
    return current;
  }

  /// Executes the callback of the currently selected screen
  void selectScreen() {
    if (active && len > 0) screens[current]->selectScreen(current);
  }

  /// Executes the callback of the currently selected text
  void selectText() {
    if (active && len > 0) screens[current]->selectText(current);
  }

  /// moves to the next selectable text
  int nextText() { return active && len > 0 ? screens[current]->nextText() : -1; }
  /// moves to the prior selectable text
  int priorText() { return active && len > 0 ? screens[current]->priorText() : -1; }
  /// Selects the text at the indicated index
  int setText(int pos) {
    return active && len > 0 ? screens[current]->setText(pos) : -1;
  }

 protected:
  int len = 0;
  CommonLCD *p_lcd = nullptr;
  LCDMenuScreen **screens = nullptr;
  int current = 0;
  bool active = false;
};
