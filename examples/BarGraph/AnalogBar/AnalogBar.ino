/*
  LCDBarGraph: show an analog value as bar

  Reads the voltage at an analog pin (e.g. a potentiometer between 5V and
  GND with the wiper connected to A0) and shows it as bar over the whole
  first row of a 16x2 display. The second row shows the raw value.

  The circuit:
  * LCD RS pin to digital pin 12
  * LCD Enable pin to digital pin 11
  * LCD D4 pin to digital pin 5
  * LCD D5 pin to digital pin 4
  * LCD D6 pin to digital pin 3
  * LCD D7 pin to digital pin 2
  * LCD R/W pin to ground
  * potentiometer wiper to analog pin A0

  This example code is in the public domain.
*/

#include <LCD.h>

const int sensorPin = A0;
const int maxReading = 1023;  // 10 bit ADC
const int numCols = 16;

LCD lcd(12, 11, 5, 4, 3, 2);
LCDBarGraph bar(lcd, numCols);  // 16 characters, first row

void setup() {
  // the bar graph uses the custom characters 0-4, so start the display first
  lcd.begin(numCols, 2);
}

void loop() {
  int value = analogRead(sensorPin);
  bar.drawValue(value, maxReading);

  // show the number in the second row; the spaces erase old digits
  lcd.setCursor(0, 1);
  lcd.print(value);
  lcd.print("    ");

  delay(100);
}
