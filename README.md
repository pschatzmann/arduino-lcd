# LCD Library

This library drives HD44780-compatible character LCDs and uses the same API as the [LiquidCrystal](https://github.com/arduino-libraries/LiquidCrystal) library, so existing sketches port with little or no change.

## Why another LCD library?

LiquidCrystal works well for a display wired directly to a board. Real projects often need more than that, and this library adds it without changing the API you already know:

- **One library for every wiring.** A parallel display (4/8-bit), an I2C backpack module and a remote display all use the same API (`LCD`, `LCD_I2C`, `LCDServer`). You can change the hardware without rewriting the application code.
- **Fewer pins.** A cheap I2C module needs only 2 pins. If you have no I2C module, the client/server mode below can save pins as well.
- **Offload the display.** In client/server mode a second, inexpensive microcontroller runs the display and receives the output over a serial interface or a wireless link. The main board then needs no display pins and can be placed away from the screen. This is an alternative to buying an I2C LCD module.
- **Header only.** There is a single `LCD.h` with no separate compiled sources. It needs only the Arduino core and `Wire.h`. The `library.properties` declares `architectures=*`; see [Testing](#testing) for what was checked.
- **Features beyond text output.** Brightness control via PWM, `LCDBarGraph` for bars and level displays, and a declarative menu system (`LCDMenu`, `LCDMenuScreen`) save you from rewriting this code in every project.
- **Print integration.** The display is a `Print`, so `print()` and `println()` work as they do with `Serial`.

## Summary of differences from LiquidCrystal

- The library is __header only__
- Support for __I2C Modules__
- A __client server__ mode (see Notes), so that a separate cheap microcontroller can act as the LCD server. The communication can be wireless or via a serial interface. This is an alternative to a separate I2C LCD module.
- Management of the __brightness using PWM__
- __LCDBarGraph__ class to display bars
- A declarative __menu__ system (see Notes)

## Notes

- **Call `begin()`.** Unlike LiquidCrystal, the `LCD` constructor no longer calls `begin(16, 1)`, so the constructor never touches the pins (or a serial line) before `setup()` runs. Call `lcd.begin(cols, rows)` in `setup()`.
- **Client/server mode.** The sending side uses an `LCDWriteDriver` (see `examples/Serial/SerialClient`). The side that is connected to the display runs an `LCDServer` and calls `process()` in `loop()` (see `examples/Serial/SerialServer`). Each command starts with a sync byte, so the receiver recovers from lost bytes. There is no checksum.
- **Menu.** See `examples/Menu/SimpleMenu`. The arrays of texts and screens must stay valid for as long as the menu is used. The library does not allocate memory.
- **Bar graph.** `LCDBarGraph` uses the custom characters 0-4, so it overwrites custom characters you defined in these slots. Create the bar after `lcd.begin()`.
- **Brightness.** `setBrightness(0..100)` maps linearly to PWM 0-255 and needs a PWM-capable backlight pin passed to the constructor.
- **Licensing.** The whole library follows the LGPL (see `License.txt`). `LCDBarGraph` is an independent implementation of the same API.

## Testing

The library and all examples were compile-checked with `arduino-cli` for AVR (Uno). The library header itself was also compiled for ESP32 and RP2040. Nothing was run on real hardware, so please test your display, especially `LCD_I2C` (initialization delay shortened from 1000 ms to 50 ms), the client/server mode and the menu.

## Documentation

- [Class Documentation](https://pschatzmann.github.io/arduino-lcd/annotated.html)
- Examples are in the [examples](examples) folder: `LCD`, `LCD_I2C`, `BarGraph`, `Menu` and `Serial` (client/server).
