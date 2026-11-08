#pragma once

#include "CommonLCD.h"
#include "LCDMenuScreen.h"

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
