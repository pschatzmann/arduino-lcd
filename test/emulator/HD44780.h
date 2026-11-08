#pragma once
/**
 * Desktop emulator of an HD44780 compatible character LCD.
 *
 * The emulator models the controller (4/8 bit interface negotiation, DDRAM,
 * CGRAM, entry mode, cursor, display shift, busy time) and renders the screen
 * into images. If recording is enabled, every change of the visible screen is
 * saved as a PNG file.
 *
 * It is connected to the library via a front end (see Backends.h).
 */
#include <stdint.h>

#include <filesystem>
#include <string>
#include <vector>

#include "Arduino.h"
#include "Font5x7.h"
#include "Png.h"

class HD44780 {
 public:
  /// directory into which the frames are written (set by the test runner)
  static inline std::string defaultOutputDir = "output";
  /// records frames automatically when an emulator is created
  static inline bool defaultRecording = true;

  /// number of screen pixels per LCD pixel in the images
  int scale = 5;

  HD44780(int cols, int rows, const std::string &label = "lcd")
      : cols_(cols), rows_(rows), label_(label) {
    for (int i = 0; i < 128; i++) ddram_[i] = 0x20;
    for (int i = 0; i < 64; i++) cgram_[i] = 0;
    recording_ = defaultRecording;
    outputDir_ = defaultOutputDir;
  }

  // ------------------------------------------------------------------
  // bus interface (used by the front ends)
  // ------------------------------------------------------------------

  /**
   * Is called when the enable line goes low: the controller reads the bus.
   * @param rs register select: false: command, true: data
   * @param rw read/write: true: read (not supported, only counted)
   * @param data the value of the lines DB7..DB0
   * @param highOnly true if only DB4..DB7 are connected
   */
  void latch(bool rs, bool rw, uint8_t data, bool highOnly) {
    if (firstLatchMicros_ == (unsigned long)-1) {
      firstLatchMicros_ = micros();
      if (firstLatchMicros_ < 40000) timingViolations_++;  // power on time
    }
    if (rw) {
      readAttempts_++;
      return;
    }
    if (bits8_) {
      if (highOnly) {
        // only 4 lines connected: the controller sees the low lines as 0
        data &= 0xF0;
        execute(rs, data, true);
      } else {
        execute(rs, data, false);
      }
    } else {
      // 4 bit interface: only DB4..DB7 are used, high nibble first
      if (!havePending_) {
        pending_ = data & 0xF0;
        havePending_ = true;
      } else {
        havePending_ = false;
        execute(rs, pending_ | ((data >> 4) & 0x0F), false);
      }
    }
  }

  void setBacklight(bool on) {
    if (backlight_ != on) {
      backlight_ = on;
      record();
    }
  }

  // ------------------------------------------------------------------
  // inspection
  // ------------------------------------------------------------------
  int cols() const { return cols_; }
  int rows() const { return rows_; }
  bool is4BitInterface() const { return !bits8_; }
  bool displayOn() const { return displayOn_; }
  bool cursorOn() const { return cursorOn_; }
  bool blinkOn() const { return blinkOn_; }
  bool backlight() const { return backlight_; }
  bool twoLine() const { return twoLine_; }
  bool entryIncrement() const { return increment_; }
  bool entryShift() const { return shiftOn_; }
  int displayShift() const { return shift_; }
  int address() const { return addr_; }
  uint8_t ddram(int address) const { return ddram_[address & 0x7F]; }
  /// row (0-7) of the custom character in the slot (0-7)
  uint8_t cgram(int slot, int row) const { return cgram_[(slot & 7) * 8 + (row & 7)]; }
  /// Number of commands which were sent before the controller was ready
  int timingViolations() const { return timingViolations_; }
  int readAttempts() const { return readAttempts_; }
  int commandCount() const { return commands_; }

  /// The visible characters of a row (raw character codes)
  std::string line(int row) const {
    std::string s;
    for (int c = 0; c < cols_; c++) s += (char)charAt(cellAddress(row, c));
    return s;
  }

  /// Finds the cell at which the cursor is displayed
  bool cursorCell(int &row, int &col) const {
    for (int r = 0; r < rows_; r++) {
      for (int c = 0; c < cols_; c++) {
        if (cellAddress(r, c) == addr_) {
          row = r;
          col = c;
          return true;
        }
      }
    }
    return false;
  }

  // ------------------------------------------------------------------
  // images
  // ------------------------------------------------------------------
  void enableRecording(bool on) { recording_ = on; }
  void setOutputDir(const std::string &dir) { outputDir_ = dir; }
  int frameCount() const { return frames_; }
  const std::vector<std::string> &frameFiles() const { return files_; }

  /// Saves the current screen as PNG (independent of the recording)
  bool saveImage(const std::string &path) const {
    int w, h;
    std::vector<uint8_t> rgb = render(w, h);
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
      std::error_code ec;
      std::filesystem::create_directories(p.parent_path(), ec);
    }
    return png::write(path, w, h, rgb);
  }

  /// Saves the current screen as <outputDir>/<label>_<name>.png
  std::string snapshot(const std::string &name) const {
    if (!recording_) return "";
    std::string path = outputDir_ + "/" + label_ + "_" + name + ".png";
    return saveImage(path) ? path : "";
  }

  /// Renders the screen (RGB, 3 bytes per pixel)
  std::vector<uint8_t> render(int &width, int &height) const {
    const int pitchX = 6, pitchY = 9, pad = 4;
    int lw = cols_ * pitchX - 1 + 2 * pad;
    int lh = rows_ * pitchY - 1 + 2 * pad;
    width = lw * scale;
    height = lh * scale;

    uint8_t bg[3], on[3], ghost[3];
    if (backlight_) {
      set(bg, 150, 190, 60);
      set(on, 20, 40, 15);
      set(ghost, 139, 178, 55);
    } else {
      set(bg, 62, 76, 52);
      set(on, 12, 22, 10);
      set(ghost, 58, 71, 49);
    }
    std::vector<uint8_t> img((size_t)width * height * 3);
    for (size_t i = 0; i < img.size(); i += 3) {
      img[i] = bg[0];
      img[i + 1] = bg[1];
      img[i + 2] = bg[2];
    }

    int crow = -1, ccol = -1;
    bool hasCursor = cursorCell(crow, ccol);

    for (int r = 0; r < rows_; r++) {
      for (int c = 0; c < cols_; c++) {
        if (!displayOn_) continue;
        uint8_t g[8];
        glyph(charAt(cellAddress(r, c)), g);
        if (hasCursor && r == crow && c == ccol) {
          if (cursorOn_) g[7] |= 0x1F;
          if (blinkOn_) {
            for (int i = 0; i < 8; i++) g[i] = 0x1F;
          }
        }
        for (int py = 0; py < 8; py++) {
          for (int px = 0; px < 5; px++) {
            bool lit = (g[py] >> (4 - px)) & 1;
            int x = pad + c * pitchX + px;
            int y = pad + r * pitchY + py;
            fillPixel(img, x, y, lit ? on : ghost);
          }
        }
      }
    }
    return img;
  }

 private:
  int cols_, rows_;
  std::string label_;

  // controller state
  bool bits8_ = true;
  bool havePending_ = false;
  uint8_t pending_ = 0;
  bool twoLine_ = false;
  bool font10_ = false;
  bool displayOn_ = false;
  bool cursorOn_ = false;
  bool blinkOn_ = false;
  bool increment_ = true;
  bool shiftOn_ = false;
  bool inCgram_ = false;
  int addr_ = 0;       // DDRAM address
  int cgAddr_ = 0;     // CGRAM address
  int shift_ = 0;      // display shift
  uint8_t ddram_[128];
  uint8_t cgram_[64];
  bool backlight_ = true;

  // timing
  unsigned long busyUntil_ = 0;
  unsigned long firstLatchMicros_ = (unsigned long)-1;
  int timingViolations_ = 0;
  int readAttempts_ = 0;
  int commands_ = 0;

  // recording
  bool recording_ = false;
  std::string outputDir_;
  int frames_ = 0;
  std::vector<uint8_t> lastFrame_;
  std::vector<std::string> files_;

  static void set(uint8_t *c, int r, int g, int b) {
    c[0] = r;
    c[1] = g;
    c[2] = b;
  }

  void fillPixel(std::vector<uint8_t> &img, int x, int y, const uint8_t *color) const {
    int width = (cols_ * 6 - 1 + 8) * scale;
    for (int dy = 0; dy < scale - 1; dy++) {
      for (int dx = 0; dx < scale - 1; dx++) {
        size_t i = ((size_t)(y * scale + dy) * width + (x * scale + dx)) * 3;
        img[i] = color[0];
        img[i + 1] = color[1];
        img[i + 2] = color[2];
      }
    }
  }

  bool validAddress(int a) const {
    if (twoLine_) return a <= 0x27 || (a >= 0x40 && a <= 0x67);
    return a <= 0x4F;
  }

  uint8_t charAt(int a) const { return validAddress(a) ? ddram_[a] : 0x20; }

  /// The DDRAM address which is displayed in the cell
  int cellAddress(int row, int col) const {
    if (twoLine_) {
      int pos = ((row / 2) * cols_ + col + shift_) % 40;
      if (pos < 0) pos += 40;
      return (row % 2 ? 0x40 : 0x00) + pos;
    }
    int pos = (row * cols_ + col + shift_) % 80;
    if (pos < 0) pos += 80;
    return pos;
  }

  /// The 5x8 pixels of a character (one byte per row, bit 4 is the left pixel)
  void glyph(uint8_t code, uint8_t *rows) const {
    for (int i = 0; i < 8; i++) rows[i] = 0;
    if (code < 16) {
      for (int i = 0; i < 8; i++) rows[i] = cgram_[(code & 7) * 8 + i] & 0x1F;
    } else if (code >= 0x20 && code <= 0x7E) {
      const uint8_t *f = font5x7[code - 0x20];
      for (int r = 0; r < 7; r++) {
        for (int c = 0; c < 5; c++) {
          if ((f[c] >> r) & 1) rows[r] |= 1 << (4 - c);
        }
      }
    } else if (code >= 0x80) {
      // not part of the emulated character set: show a box
      for (int i = 0; i < 8; i++) rows[i] = 0x11;
      rows[0] = rows[7] = 0x1F;
    }
  }

  void busy(unsigned long us) { busyUntil_ = micros() + us; }

  void execute(bool rs, uint8_t value, bool nibbleOnly) {
    if (micros() < busyUntil_) timingViolations_++;
    commands_++;

    if (rs) {
      writeData(value);
      busy(41);
    } else {
      command(value, nibbleOnly);
    }
    record();
  }

  void writeData(uint8_t value) {
    if (inCgram_) {
      cgram_[cgAddr_ & 0x3F] = value & 0x1F;
      cgAddr_ = (cgAddr_ + (increment_ ? 1 : -1)) & 0x3F;
      return;
    }
    if (validAddress(addr_)) ddram_[addr_] = value;
    addr_ = (addr_ + (increment_ ? 1 : -1)) & 0x7F;
    if (shiftOn_) shift_ += increment_ ? 1 : -1;
  }

  void command(uint8_t v, bool nibbleOnly) {
    busy(37);
    if (v & 0x80) {  // set DDRAM address
      inCgram_ = false;
      addr_ = v & 0x7F;
    } else if (v & 0x40) {  // set CGRAM address
      inCgram_ = true;
      cgAddr_ = v & 0x3F;
    } else if (v & 0x20) {  // function set
      bool dl8 = v & 0x10;
      bits8_ = dl8;
      havePending_ = false;
      if (!dl8 && nibbleOnly) {
        // switched to 4 bit mode with this single nibble: N and F are
        // sent with the next command
      } else {
        twoLine_ = v & 0x08;
        font10_ = v & 0x04;
      }
    } else if (v & 0x10) {  // cursor or display shift
      bool display = v & 0x08;
      bool right = v & 0x04;
      if (display) {
        shift_ += right ? -1 : 1;
      } else {
        addr_ = (addr_ + (right ? 1 : -1)) & 0x7F;
      }
    } else if (v & 0x08) {  // display control
      displayOn_ = v & 0x04;
      cursorOn_ = v & 0x02;
      blinkOn_ = v & 0x01;
    } else if (v & 0x04) {  // entry mode
      increment_ = v & 0x02;
      shiftOn_ = v & 0x01;
    } else if (v & 0x02) {  // return home
      inCgram_ = false;
      addr_ = 0;
      shift_ = 0;
      busy(1520);
    } else if (v & 0x01) {  // clear display
      for (int i = 0; i < 128; i++) ddram_[i] = 0x20;
      inCgram_ = false;
      addr_ = 0;
      shift_ = 0;
      increment_ = true;
      busy(1520);
    }
  }

  /// Saves a frame if the screen has changed
  void record() {
    if (!recording_) return;
    int w, h;
    std::vector<uint8_t> frame = render(w, h);
    if (frame == lastFrame_) return;
    lastFrame_ = frame;
    char name[32];
    snprintf(name, sizeof(name), "_%04d.png", ++frames_);
    std::string path = outputDir_ + "/" + label_ + name;
    std::error_code ec;
    std::filesystem::create_directories(outputDir_, ec);
    if (png::write(path, w, h, frame)) files_.push_back(path);
  }
};
