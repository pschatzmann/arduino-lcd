/*
  LCDBarGraph on an I2C display: progress bar

  Shows a progress bar in the second row of a 16x2 display which is connected
  with an I2C backpack, and the percentage in the first row. When the bar is
  full, the progress starts again.

  The circuit:
  * I2C backpack SDA and SCL to the I2C pins of the board
  * backpack address 0x27 (0x3F for some modules)

  This example code is in the public domain.
*/

#include <LCD.h>

LCD_I2C lcd(0x27);
LCDBarGraph bar(lcd, 16, 0, 1);  // 16 characters in the second row
int percent = 0;

void setup() {
  Wire.begin();  // the sketch starts the I2C bus
  lcd.begin(16, 2);
  lcd.print("Progress:");
}

void loop() {
  bar.drawValue(percent, 100);

  lcd.setCursor(10, 0);
  lcd.print(percent);
  lcd.print("%  ");

  percent = (percent + 1) % 101;
  delay(80);
}
