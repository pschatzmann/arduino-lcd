# LCD Library

Arduino library for HD44780 compatible character LCDs. It has the same API as [LiquidCrystal](https://github.com/arduino-libraries/LiquidCrystal), so existing sketches port with little or no change, and adds:

- header only code
- __I2C modules__ (`LCD_I2C`)
- a __client/server__ mode: a second cheap microcontroller drives the display, connected via serial or wireless
- backlight brightness with __PWM__
- `LCDBarGraph` to display bars
- a declarative __menu__ system

```cpp
#include <LCD.h>

LCD lcd(12, 11, 5, 4, 3, 2);  // rs, enable, d4, d5, d6, d7

void setup() {
  lcd.begin(16, 2);
  lcd.print("hello, world!");
}

void loop() {}
```

For an I2C module use `LCD_I2C lcd(0x27);` instead. The rest of the sketch stays the same.

## Documentation

- [Class Documentation](https://pschatzmann.github.io/arduino-lcd/annotated.html)
- [Why another LCD library?](docs/why.md)
- [Usage notes](docs/usage-notes.md)
- [API reference](docs/api.md)
- [Testing](docs/testing.md): desktop tests with an HD44780 emulator that records the screen as images
- [Examples](examples): `LCD`, `LCD_I2C`, `BarGraph`, `Menu` and `Serial` (client/server)
