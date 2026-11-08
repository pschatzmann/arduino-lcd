# Usage notes

- **Call `begin()`.** Unlike LiquidCrystal, the `LCD` constructor does not call `begin(16, 1)`, so the constructor never touches the pins (or a serial line) before `setup()` runs. Call `lcd.begin(cols, rows)` in `setup()`.
- **Client/server mode.** A second microcontroller can drive the display, see [Client/server mode](client-server.md).
- **Menu.** See [Menu](menu.md).
- **Bar graph.** See [Bar graph](bargraph.md). It overwrites the custom characters 0-4.
- **Brightness.** `setBrightness(0..100)` maps linearly to PWM 0-255. It needs a PWM-capable backlight pin: call `setBrightnessPin(pin)` before `begin()` (`LCD_I2C` also accepts the pin in its constructor). There is no backlight pin constructor parameter, because it would make the constructors with and without the RW pin ambiguous.
- **Source layout.** `LCD.h` only includes the headers in `src/LCD/`, one per class.
- **Licensing.** The whole library follows the LGPL (see `License.txt`). `LCDBarGraph` is an independent implementation of the same API.
