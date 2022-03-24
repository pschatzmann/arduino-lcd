# LCD Library

This library is using the same API like the LCD library with the following differences
- The library is header only
- We supports a client server mode, so that we can use a separate cheap processor as LCD server - The communication can be wirelessly or via a serial interface. This is an alternative to a separate I2C LCD module.
- Management of the Brightness using PWM
- LCDBarGraph class to display bar charts

