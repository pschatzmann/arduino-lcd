#pragma once

#include "Print.h"
#include "AbstractLCDDriver.h"
#include "Cmd.h"

/**
 * @brief Driver which sends the data over a Print (e.g. Serial Line)
 */
struct LCDWriteDriver : public AbstractLCDDriver {
  LCDWriteDriver(Print &out) { p_out = &out; }

  void pinModeLCD(uint16_t pin, uint16_t mode) override {
    sendCmd(Cmd(CmdEnum::MODE, pin, mode));
  }

  void digitalWriteLCD(uint16_t pin, uint16_t value) override {
    sendCmd(Cmd(CmdEnum::WRITE, pin, value));
  }

  void delayMicrosecondsLCD(uint16_t us) override {
    sendCmd(Cmd(CmdEnum::DELAY, us));
  }

  void pulseEnable(uint16_t pin) override {
    sendCmd(Cmd(CmdEnum::PULSE, pin));
  }

  void setBrightness(uint16_t pin, uint16_t percent) override {
    sendCmd(Cmd(CmdEnum::BRIGHTNESS, pin, percent));
  }

 protected:
  Print *p_out;

  void sendCmd(const Cmd &cmd) {
    p_out->write((const uint8_t *)&cmd, sizeof(cmd));
  }
};
