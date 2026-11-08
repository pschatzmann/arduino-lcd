#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/// Desktop version of the Arduino Print class (the subset which is used)
class Print {
 public:
  virtual ~Print() {}
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t *buffer, size_t size) {
    size_t n = 0;
    while (size--) n += write(*buffer++);
    return n;
  }
  size_t write(const char *str) {
    return str == nullptr ? 0 : write((const uint8_t *)str, strlen(str));
  }

  size_t print(const char *s) { return write(s); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(int v, int base = 10) { return print((long)v, base); }
  size_t print(unsigned int v, int base = 10) { return print((unsigned long)v, base); }
  size_t print(long v, int base = 10) {
    char buf[40];
    snprintf(buf, sizeof(buf), base == 16 ? "%lx" : "%ld", v);
    return write(buf);
  }
  size_t print(unsigned long v, int base = 10) {
    char buf[40];
    snprintf(buf, sizeof(buf), base == 16 ? "%lx" : "%lu", v);
    return write(buf);
  }
  size_t print(double v, int digits = 2) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.*f", digits, v);
    return write(buf);
  }
  size_t println() { return write("\r\n"); }
  template <class T>
  size_t println(T v) {
    size_t n = print(v);
    return n + println();
  }
};
