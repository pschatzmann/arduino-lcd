/*
  LCD Library - Cursor

 Demonstrates the use of a 16x2 LCD display.  The LCD
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch prints "hello, world!" to the LCD and
 uses the cursor()  and noCursor() methods to turn
 on and off the cursor.

Using an I2C LCD Module
- SDA
- SCL
- VCC
- GND

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 modified 7 Nov 2016
 by Arturo Guadalupi

 This example code is in the public domain.

 https://www.arduino.cc/en/Tutorial/LibraryExamples/LCDCursor

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
  // Turn off the cursor:
  lcd.noCursor();
  delay(500);
  // Turn on the cursor:
  lcd.cursor();
  delay(500);
}
