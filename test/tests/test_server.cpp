// Tests of the client/server mode: an LCD which sends its commands over a
// stream (LCDWriteDriver) and the LCDServer which executes them on the pins.
#include "Rigs.h"

static void drain(LCDServer &server, LoopbackStream &stream) {
  int guard = 100000;
  while (stream.available() > 0 && guard-- > 0) server.process(0);
}

static void send(LoopbackStream &stream, const Cmd &cmd) {
  const uint8_t *p = (const uint8_t *)&cmd;
  for (size_t i = 0; i < sizeof(Cmd); i++) stream.write(p[i]);
}

TEST(Server, command_format) {
  CHECK_EQ(sizeof(Cmd), (size_t)6);
  Cmd cmd(CmdEnum::WRITE, 0x1234, 0x5678);
  const uint8_t *p = (const uint8_t *)&cmd;
  CHECK_EQ(p[0], (uint8_t)Cmd::SYNC);
  CHECK_EQ(p[1], (uint8_t)CmdEnum::WRITE);
  CHECK_EQ(p[2], 0x34);  // little endian
  CHECK_EQ(p[3], 0x12);
}

TEST(Server, driver_sends_commands) {
  LoopbackStream stream;
  LCDWriteDriver driver(stream);
  driver.pinModeLCD(7, OUTPUT);
  driver.digitalWriteLCD(7, HIGH);
  driver.delayMicrosecondsLCD(50);
  driver.pulseEnable(11);
  driver.setBrightness(9, 50);
  CHECK_EQ(stream.data.size(), 5 * sizeof(Cmd));
  // nothing happens on the local pins
  CHECK_EQ(mockHardware().pinWrites, 0UL);
}

TEST(Server, server_executes_commands_on_the_pins) {
  LoopbackStream stream;
  LCDServer server(stream);
  send(stream, Cmd(CmdEnum::MODE, 7, OUTPUT));
  send(stream, Cmd(CmdEnum::WRITE, 7, HIGH));
  send(stream, Cmd(CmdEnum::BRIGHTNESS, 9, 100));
  drain(server, stream);
  CHECK_EQ((int)mockHardware().mode[7], (int)OUTPUT);
  CHECK_EQ((int)mockHardware().value[7], (int)HIGH);
  CHECK_EQ(mockHardware().pwm[9], 255);
  send(stream, Cmd(CmdEnum::WRITE, 7, LOW));
  drain(server, stream);
  CHECK_EQ((int)mockHardware().value[7], (int)LOW);
}

TEST(Server, delay_command_waits) {
  LoopbackStream stream;
  LCDServer server(stream);
  unsigned long t = micros();
  send(stream, Cmd(CmdEnum::DELAY, 4500));
  drain(server, stream);
  CHECK_EQ(micros() - t, 4500UL);
}

TEST(Server, display_over_the_wire) {
  LoopbackStream stream;
  LCDWriteDriver driver(stream);
  LCD lcd(12, 11, 5, 4, 3, 2, driver);

  HD44780 hd(16, 2);
  ParallelBus bus(hd, 12, 11, {5, 4, 3, 2});
  LCDServer server(stream);

  lcd.begin(16, 2);
  lcd.print("Over the wire");
  lcd.setCursor(0, 1);
  lcd.print("serial LCD");
  CHECK_EQ(hd.line(0), pad("", 16));  // nothing arrived yet

  drain(server, stream);
  CHECK(hd.is4BitInterface());
  CHECK_EQ(hd.line(0), pad("Over the wire", 16));
  CHECK_EQ(hd.line(1), pad("serial LCD", 16));
  CHECK_EQ(hd.timingViolations(), 0);
  hd.snapshot("over_the_wire");
}

TEST(Server, brightness_over_the_wire) {
  LoopbackStream stream;
  LCDWriteDriver driver(stream);
  LCD lcd(12, 11, 5, 4, 3, 2, driver);
  lcd.setBrightnessPin(6);
  LCDServer server(stream);
  lcd.begin(16, 2);
  lcd.setBrightness(40);
  drain(server, stream);
  CHECK_EQ(mockHardware().pwm[6], 102);
}

TEST(Server, resynchronizes_after_garbage) {
  LoopbackStream stream;
  LCDServer server(stream);
  for (uint8_t b : {0x00, 0x13, 0xFF, 0x42}) stream.write(b);  // lost data
  send(stream, Cmd(CmdEnum::WRITE, 20, HIGH));
  drain(server, stream);
  CHECK_EQ((int)mockHardware().value[20], (int)HIGH);
}

TEST(Server, waits_for_incomplete_commands) {
  LoopbackStream stream;
  LCDServer server(stream);
  Cmd cmd(CmdEnum::WRITE, 21, HIGH);
  const uint8_t *p = (const uint8_t *)&cmd;
  for (size_t i = 0; i < 3; i++) stream.write(p[i]);
  server.process(0);
  CHECK_EQ((int)mockHardware().value[21], 0);
  CHECK_EQ(stream.data.size(), (size_t)3);  // nothing was consumed
  for (size_t i = 3; i < sizeof(Cmd); i++) stream.write(p[i]);
  server.process(0);
  CHECK_EQ((int)mockHardware().value[21], (int)HIGH);
}

TEST(Server, unknown_commands_are_ignored) {
  LoopbackStream stream;
  LCDServer server(stream);
  Cmd bad(CmdEnum::UNDEFINED, 22, HIGH);
  send(stream, bad);
  Cmd unknown;
  uint8_t *raw = (uint8_t *)&unknown;
  raw[1] = 99;
  send(stream, unknown);
  send(stream, Cmd(CmdEnum::WRITE, 23, HIGH));
  drain(server, stream);
  CHECK_EQ((int)mockHardware().value[22], 0);
  CHECK_EQ((int)mockHardware().value[23], (int)HIGH);
}

TEST(Server, waits_when_there_is_no_data) {
  LoopbackStream stream;
  LCDServer server(stream);
  unsigned long t = micros();
  server.process(100);
  CHECK_EQ(micros() - t, 100000UL);
}
