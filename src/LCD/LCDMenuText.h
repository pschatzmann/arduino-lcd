#pragma once

#include <stdint.h>

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
