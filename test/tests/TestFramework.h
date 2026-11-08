#pragma once
// Minimal unit test framework (no external dependencies)
#include <stdio.h>

#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

struct TestCase {
  const char *suite;
  const char *name;
  void (*fn)();
};

inline std::vector<TestCase> &testRegistry() {
  static std::vector<TestCase> tests;
  return tests;
}

struct TestRegistrar {
  TestRegistrar(const char *suite, const char *name, void (*fn)()) {
    testRegistry().push_back({suite, name, fn});
  }
};

inline int &currentFailures() {
  static int failures = 0;
  return failures;
}

inline void reportFailure(const char *file, int line, const std::string &what,
                          const std::string &detail = "") {
  currentFailures()++;
  printf("    FAILED %s:%d: %s%s%s\n", file, line, what.c_str(),
         detail.empty() ? "" : "  -> ", detail.c_str());
}

template <class T>
std::string toStr(const T &v) {
  std::ostringstream o;
  if constexpr (std::is_same<T, uint8_t>::value || std::is_same<T, char>::value) {
    o << (int)v;
  } else if constexpr (std::is_same<T, std::string>::value) {
    o << "\"";
    for (char c : v) {
      if (c >= 32 && c < 127) {
        o << c;
      } else {
        char buf[8];
        snprintf(buf, sizeof(buf), "\\x%02x", (unsigned char)c);
        o << buf;
      }
    }
    o << "\"";
  } else {
    o << v;
  }
  return o.str();
}

#define TEST(suite, name)                                                \
  static void test_##suite##_##name();                                   \
  static TestRegistrar registrar_##suite##_##name(#suite, #name,         \
                                                  test_##suite##_##name); \
  static void test_##suite##_##name()

#define CHECK(cond)                                    \
  do {                                                 \
    if (!(cond)) reportFailure(__FILE__, __LINE__, #cond); \
  } while (0)

#define CHECK_EQ(a, b)                                                       \
  do {                                                                       \
    auto va_ = (a);                                                          \
    auto vb_ = (b);                                                          \
    if (!(va_ == vb_))                                                       \
      reportFailure(__FILE__, __LINE__, #a " == " #b,                        \
                    toStr(va_) + " != " + toStr(vb_));                       \
  } while (0)
