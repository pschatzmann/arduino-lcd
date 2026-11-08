/*
  LCDBarGraph: several bars on one display

  Shows six small bars (4 characters each) which move at different speeds:
  three in the first row and three in the second row. Each bar needs its own
  LCDBarGraph object with the width, the start column and the row.

  The circuit:
  * LCD RS pin to digital pin 12
  * LCD Enable pin to digital pin 11
  * LCD D4 pin to digital pin 5
  * LCD D5 pin to digital pin 4
  * LCD D6 pin to digital pin 3
  * LCD D7 pin to digital pin 2
  * LCD R/W pin to ground

  This example code is in the public domain.
*/

#include <LCD.h>

LCD lcd(12, 11, 5, 4, 3, 2);

//                  display, width, column, row
LCDBarGraph bar0(lcd, 4, 0, 0);
LCDBarGraph bar1(lcd, 4, 6, 0);
LCDBarGraph bar2(lcd, 4, 12, 0);
LCDBarGraph bar3(lcd, 4, 0, 1);
LCDBarGraph bar4(lcd, 4, 6, 1);
LCDBarGraph bar5(lcd, 4, 12, 1);

// a value which rises from 0 to 100 and falls back again;
// the speed defines how many steps a full cycle takes
int triangle(unsigned long step, int speed) {
  int pos = (step * speed) % 200;
  return pos < 100 ? pos : 200 - pos;
}

void setup() {
  lcd.begin(16, 2);
}

void loop() {
  unsigned long step = millis() / 50;
  bar0.drawValue(triangle(step, 1), 100);
  bar1.drawValue(triangle(step, 2), 100);
  bar2.drawValue(triangle(step, 3), 100);
  bar3.drawValue(triangle(step, 4), 100);
  bar4.drawValue(triangle(step, 5), 100);
  bar5.drawValue(triangle(step, 7), 100);
  delay(50);
}
