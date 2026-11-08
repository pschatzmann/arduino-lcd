#include <stdlib.h>

#include <filesystem>
#include <string>

#include "Arduino.h"
#include "HD44780.h"
#include "TestFramework.h"
#include "Wire.h"

int main(int argc, char **argv) {
  std::string filter = argc > 1 ? argv[1] : "";
  const char *out = getenv("LCD_TEST_OUTPUT");
  std::string root = out ? out : "output";
  HD44780::defaultRecording = getenv("LCD_TEST_NO_IMAGES") == nullptr;

  std::error_code ec;
  if (HD44780::defaultRecording) std::filesystem::remove_all(root, ec);

  int run = 0, failed = 0;
  for (const TestCase &t : testRegistry()) {
    std::string name = std::string(t.suite) + "." + t.name;
    if (!filter.empty() && name.find(filter) == std::string::npos) continue;

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
