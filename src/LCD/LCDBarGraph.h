#pragma once

#include "Arduino.h"
#include "CommonLCD.h"

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
