#pragma once

#include "Arduino.h"
#include "Cmd.h"
#include "LCDDriver.h"

/**
 * @brief LCDServer receives the commands sent by the LCDWriteDriver from the
 * indicated Stream and executes them on the local pins. Call process() in the
 * Arduino loop.
 */
class LCDServer {
 public:
  LCDServer(Stream &in) { p_in = &in; }

  /// Call this method in the loop
  void process(int delay_no_data = 100) {
    // resynchronize: skip everything up to the next sync byte
    while (p_in->available() > 0 && p_in->peek() != Cmd::SYNC) {
      p_in->read();
    }

    if (p_in->available() == 0) {
      delay(delay_no_data);
      return;
    }

    // wait until the full command has arrived
    if (p_in->available() < (int)sizeof(Cmd)) return;

    if (p_in->readBytes((uint8_t *)&cmd, sizeof(Cmd)) != sizeof(Cmd)) return;

    switch (cmd.id) {
      case CmdEnum::MODE:
        driver.pinModeLCD(cmd.p1, cmd.p2);
        break;
      case CmdEnum::WRITE:
        driver.digitalWriteLCD(cmd.p1, cmd.p2);
        break;
      case CmdEnum::DELAY:
        driver.delayMicrosecondsLCD(cmd.p1);
        break;
      case CmdEnum::PULSE:
        driver.pulseEnable(cmd.p1);
        break;
      case CmdEnum::BRIGHTNESS:
        driver.setBrightness(cmd.p1, cmd.p2);
        break;
      default:
        break;  // unknown command: ignore
    }
  }

 protected:
  Stream *p_in = nullptr;
  Cmd cmd;
  LCDDriver driver;
};
