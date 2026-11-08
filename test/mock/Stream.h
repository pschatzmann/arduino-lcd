#pragma once
#include "Print.h"

/// Desktop version of the Arduino Stream class (the subset which is used)
class Stream : public Print {
 public:
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;

  size_t readBytes(uint8_t *buffer, size_t length) {
    size_t n = 0;
    while (n < length && available() > 0) buffer[n++] = (uint8_t)read();
    return n;
  }
  size_t readBytes(char *buffer, size_t length) {
    return readBytes((uint8_t *)buffer, length);
  }
};
