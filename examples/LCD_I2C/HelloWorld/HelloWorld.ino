/*
  LCD Library - Hello World

 Demonstrates the use of a 16x2 LCD display.  The LCD
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch prints "hello, world!" to the LCD
 and shows the time.

Using an I2C LCD Module
- SDA
- SCL
- VCC
- GND)

Using an I2C LCD Module

 This example code is in the public domain.

 https://www.arduino.cc/en/Tutorial/LibraryExamples/HelloWorld

*/

// include the library code:
#include <LCD.h>

// initialize the library by associating any needed LCD interface pin
// with the Arduino pin number it is connected to
// const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LCD_I2C lcd(0x27);

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(millis() / 1000);
}
