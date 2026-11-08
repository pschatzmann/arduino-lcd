#pragma once

#include "CommonLCD.h"
#include "LCDMenuText.h"

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
