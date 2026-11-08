# Usage notes

- **Call `begin()`.** Unlike LiquidCrystal, the `LCD` constructor does not call `begin(16, 1)`, so the constructor never touches the pins (or a serial line) before `setup()` runs. Call `lcd.begin(cols, rows)` in `setup()`.
- **Client/server mode.** The sending side uses an `LCDWriteDriver` (see `examples/Serial/SerialClient`). The side that is connected to the display runs an `LCDServer` and calls `process()` in `loop()` (see `examples/Serial/SerialServer`). Each command starts with a sync byte, so the receiver recovers from lost bytes. There is no checksum.
- **Menu.** See `examples/Menu/SimpleMenu`. The arrays of texts and screens must stay valid for as long as the menu is used. The library does not allocate memory.
- **Bar graph.** `LCDBarGraph` uses the custom characters 0-4, so it overwrites custom characters you defined in these slots. Create the bar after `lcd.begin()`. If your display has a built-in solid block character, select it with `setFilledChar()`.
- **Brightness.** `setBrightness(0..100)` maps linearly to PWM 0-255. It needs a PWM-capable backlight pin: call `setBrightnessPin(pin)` before `begin()` (`LCD_I2C` also accepts the pin in its constructor). There is no backlight pin constructor parameter, because it would make the constructors with and without the RW pin ambiguous.
- **Source layout.** `LCD.h` only includes the headers in `src/LCD/`, one per class.
- **Licensing.** The whole library follows the LGPL (see `License.txt`). `LCDBarGraph` is an independent implementation of the same API.
