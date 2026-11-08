# Bar graph

`LCDBarGraph` shows a value as horizontal bar. Each character has 5 steps, so a bar with 16 characters has 80 steps.

```cpp
#include <LCD.h>

LCD lcd(12, 11, 5, 4, 3, 2);
LCDBarGraph bar(lcd, 16);        // 16 characters in the first row

void setup() {
  lcd.begin(16, 2);
  lcd.clear();
}

void loop() {
  bar.drawValue(analogRead(A0), 1024);
  delay(100);
}
```

## Functions

| Function | Description |
|----------|-------------|
| `LCDBarGraph(lcd, numCols, [startX], [startY])` | Bar with `numCols` characters at column `startX` and row `startY` (0 based) |
| `drawValue(value, maxValue)` | Draws a value from 0 to `maxValue`. Values outside of the range are limited. Nothing is written if the bar did not change |
| `setFilledChar(code)` | Uses the built-in character `code` for completely filled positions. The default `-1` uses a custom character |
| `getFilledChar()` | Provides the code which was set with `setFilledChar()` |

## Notes

- The bar uses the custom characters 0 to 4 of the display and overwrites your own characters in these slots.
- The constructor clears the display, so create the bar after `lcd.begin()`.
- Several bars can be shown at the same time, see `examples/BarGraph/MultipleBars`.
- Do not call `drawValue()` more often than needed: frequent writes can make the display flicker.
