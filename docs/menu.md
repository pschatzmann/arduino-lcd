# Menu

A menu is a list of screens. A screen is a list of texts at fixed positions, some of which can be selected. The menu is defined with static arrays, so the library does not allocate memory. The arrays must stay valid as long as the menu is used.

```cpp
#include <LCD.h>

LCD lcd(12, 11, 5, 4, 3, 2);

void onText(uint16_t screen, int text, const char *txt) {
  Serial.println(txt);
}

LCDMenuText title(0, 0, "Settings");          // not selectable
LCDMenuText start(0, 1, "Start", true);       // selectable
LCDMenuText stop(8, 1, "Stop", true);
const LCDMenuText *texts[] = {&title, &start, &stop};
LCDMenuScreen screen(texts, nullptr, onText);

LCDMenuScreen *screens[] = {&screen};
LCDMenu menu(lcd, screens);

void setup() {
  lcd.begin(16, 2);
  menu.begin();
}

void loop() {
  // e.g. from buttons: menu.nextText(); menu.selectText(); menu.nextScreen();
}
```

A complete example is in `examples/Menu/SimpleMenu`.

## Classes

| Class | Constructor | Description |
|-------|-------------|-------------|
| `LCDMenuText` | `(x, y, text, [selectable], [callback])` | A text at column `x` and row `y`. The optional callback is called when the text is selected |
| `LCDMenuScreen` | `(texts, [screenCallback], [textCallback])` | The texts of one screen. `textCallback` is called when any text of the screen is selected |
| `LCDMenu` | `(lcd, screens)` | The screens of the menu |

The callbacks have the signature `void callback(uint16_t screen, int text, const char *txt)`: the index of the screen, the index of the selected text (counted among the selectable texts) and the text.

## LCDMenu functions

| Function | Description |
|----------|-------------|
| `begin([screen])` | Activates the menu and shows the screen (default: 0) |
| `end()` | Deactivates the menu. The functions below do nothing while it is inactive |
| `setScreen(n)`, `nextScreen()`, `priorScreen()` | Show a screen. The index wraps around |
| `setText(n)`, `nextText()`, `priorText()` | Move the cursor to a selectable text. The index wraps around. They return -1 if the screen has no selectable text |
| `selectText()` | Calls the callbacks of the text at the cursor |
| `selectScreen()` | Calls the callback of the screen |
| `clear()` | Clears the display |

The cursor is shown as blinking cursor at the selected text.
