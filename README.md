# LCD Library

This library is using the same API like the [LiquidCristal](https://github.com/arduino-libraries/LiquidCrystal) library with the following differences
- The library is __header only__
- We supports a __client server__ mode, so that we can use a separate cheap microcontroller as LCD server - The communication can be wirelessly or via a serial interface. This is an alternative to a separate I2C LCD module.
- Management of the __Brightness using PWM__
- __LCDBarGraph__ class to display bars
- Support for __I2C Module__

