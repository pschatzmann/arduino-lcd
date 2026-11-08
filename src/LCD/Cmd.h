#pragma once

#include <stdint.h>

/**
 * @brief Supported (remote) Commands which are sent over the wire
 *
 */
enum class CmdEnum : uint8_t {
  UNDEFINED = 0,
  MODE,
  WRITE,
  DELAY,
  PULSE,
  BRIGHTNESS
};

/**
 * @brief Command structure which is sent over the wire. Each command starts
 * with a sync byte so that the receiver can resynchronize after lost data.
 */
struct __attribute__((__packed__)) Cmd {
  static const uint8_t SYNC = 0xA5;

  Cmd() = default;
  Cmd(CmdEnum id, uint16_t p1, uint16_t p2 = 0) {
    this->id = id;
    this->p1 = p1;
    this->p2 = p2;
  }
  uint8_t sync = SYNC;
  CmdEnum id = CmdEnum::UNDEFINED;
  uint16_t p1 = 0;
  uint16_t p2 = 0;
};
