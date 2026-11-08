# Why another LCD library?

LiquidCrystal works well for a display wired directly to a board. Real projects often need more than that. This library adds it without changing the API you already know.

- **One API for every wiring.** A parallel display (4/8-bit), an I2C backpack module and a remote display all use the same API (`LCD`, `LCD_I2C`, `LCDServer`). You can change the hardware without rewriting the application code.
- **Fewer pins.** A cheap I2C module needs only 2 pins. If you have no I2C module, the client/server mode can save pins as well.
- **Offload the display.** In client/server mode a second, inexpensive microcontroller runs the display and receives the output over a serial interface or a wireless link. The main board needs no display pins and can be placed away from the screen.
- **Header only.** There is no separately compiled code. The library needs only the Arduino core and `Wire.h`. `library.properties` declares `architectures=*`; see [Testing](testing.md) for what was checked.
- **More than text output.** Brightness control via PWM, `LCDBarGraph` for bars and level displays, and a declarative menu system (`LCDMenu`, `LCDMenuScreen`).
- **Print integration.** The display is a `Print`, so `print()` and `println()` work as they do with `Serial`.

## Differences from LiquidCrystal

| Topic | LiquidCrystal | This library |
|-------|---------------|--------------|
| Code | compiled library | header only |
| I2C modules | not supported | `LCD_I2C` |
| Remote display | no | client/server mode |
| Backlight brightness | no | `setBrightness()` with a PWM pin |
| Bars | no | `LCDBarGraph` |
| Menus | no | `LCDMenu` |
| `begin()` | called by the constructor with 16x1 | must be called by the sketch |
