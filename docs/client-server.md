# Client/server mode

The display can be driven by a second microcontroller, so the main board needs no pins for the display. The commands are sent over any `Print`/`Stream`, e.g. a serial line, Bluetooth or another wireless link.

```
main board                             display board
LCD + LCDWriteDriver  ---- Serial ---->  LCDServer ---> LCD pins
```

## Main board (sender)

```cpp
#include <LCD.h>

LCDWriteDriver driver(Serial);                 // where the commands are sent
LCD lcd(12, 11, 5, 4, 3, 2, driver);

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.print("hello, world!");
}

void loop() {}
```

The pin numbers are the pins of the display board. The sender does not write to any of its own pins. See `examples/Serial/SerialClient`.

## Display board

```cpp
#include <LCD.h>

LCDServer server(Serial);

void setup() { Serial.begin(115200); }

void loop() { server.process(); }
```

`process()` reads the commands and sets the pins. If no data is available, it waits for 100 ms (change it with `process(delay_ms)`). Do not use the stream for debug output. See `examples/Serial/SerialServer`.

## Protocol

Each command has 6 bytes: a sync byte (0xA5), the command (pin mode, write, delay, pulse, brightness) and two 16 bit parameters (little endian). After lost bytes the server skips data until the next sync byte. There is no checksum, so use a reliable link.
