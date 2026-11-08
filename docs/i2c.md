# I2C displays

`LCD_I2C` controls a display with a PCF8574 I2C backpack. It has the same functions as `LCD` (see the [API](api.md)) plus the backlight control of the backpack.

```cpp
#include <LCD.h>

LCD_I2C lcd(0x27);  // I2C address of the module

void setup() {
  Wire.begin();       // the sketch starts the I2C bus
  lcd.begin(16, 2);
  lcd.print("hello, world!");
}

void loop() {}
```

The sketch must start the I2C bus. This lets you select the pins (e.g. `Wire.begin(SDA, SCL)` on the ESP32) or use another bus.

## Functions

| Function | Description |
|----------|-------------|
| `LCD_I2C(address, [pwmPin])` | Defines the I2C address and optionally the PWM pin for `setBrightness()` |
| `begin(cols, rows)` | Starts the display |
| `begin(cols, rows, charsize, wire)` | Starts the display on another `TwoWire` object |
| `setWire(wire)` | Selects another `TwoWire` object (call before `begin()`) |
| `backlight()` / `noBacklight()` | Switches the backlight of the module on or off |
| `setBacklight(bool)` | Switches the backlight on or off |
| `getBacklight()` | Provides the state of the backlight |

The common address of a PCF8574 module is `0x27`, a PCF8574A module uses `0x3F`.
