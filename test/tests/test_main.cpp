#include <stdlib.h>

#include <filesystem>
#include <string>

#include "Arduino.h"
#include "HD44780.h"
#include "TestFramework.h"
#include "Wire.h"

int main(int argc, char **argv) {
  // usage: run_tests [--list | --exact <Suite.name> | <part of the name>]
  std::string filter;
  bool exact = false, list = false;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--list") {
      list = true;
    } else if (a == "--exact") {
      exact = true;
    } else {
      filter = a;
    }
  }
  if (list) {
    for (const TestCase &t : testRegistry()) printf("%s.%s\n", t.suite, t.name);
    return 0;
  }
  const char *out = getenv("LCD_TEST_OUTPUT");
  std::string root = out ? out : "output";
  HD44780::defaultRecording = getenv("LCD_TEST_NO_IMAGES") == nullptr;

  std::error_code ec;
  // a run of a single test (e.g. from ctest) keeps the images of the others
  if (HD44780::defaultRecording && !exact) std::filesystem::remove_all(root, ec);

  int run = 0, failed = 0;
  for (const TestCase &t : testRegistry()) {
    std::string name = std::string(t.suite) + "." + t.name;
    if (!filter.empty()) {
      if (exact ? name != filter : name.find(filter) == std::string::npos) continue;
    }

    mockHardware().reset();
    HD44780::defaultOutputDir = root + "/" + t.suite + "_" + t.name;
    currentFailures() = 0;
    run++;
    t.fn();
    if (currentFailures() == 0) {
      printf("[ PASS ] %s\n", name.c_str());
    } else {
      printf("[ FAIL ] %s\n", name.c_str());
      failed++;
    }
    mockHardware().reset();
  }
  printf("\n%d tests, %d failed\n", run, failed);
  if (HD44780::defaultRecording) printf("images: %s/\n", root.c_str());
  return failed == 0 ? 0 : 1;
}
