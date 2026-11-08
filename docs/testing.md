# Testing

The library is tested on the desktop, without any hardware. The tests run the library against an __HD44780 emulator__ which models the controller (4/8 bit interface negotiation, DDRAM/CGRAM, cursor, display shift, busy times) and renders every change of the screen to a PNG image.

```bash
cmake -S test -B build/test
cmake --build build/test
ctest --test-dir build/test --output-on-failure
```

- The images are written to `build/test/output/<test>/`, one folder per test. Each file is one screen update; `<label>_<name>.png` are named snapshots.
- Every test case is a separate CTest test, e.g. `ctest --test-dir build/test -R Menu` or `-j4` to run them in parallel. `build/test/run_tests --list` lists them, `build/test/run_tests <part of the name>` runs the matching ones directly. Set `LCD_TEST_NO_IMAGES=1` to skip the images.
- The emulator reports commands which are sent too early (timing violations), so the tests also check the initialization sequence and the delays.

## Not covered

- The examples are only compile-checked with `arduino-cli` (Uno and ESP32).
- Nothing was run on real hardware. Please test your display, especially `LCD_I2C` (the initialization waits only 50 ms).
