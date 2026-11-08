/**
 * Declarative menu: two screens with selectable texts.
 * Open the Serial Monitor and send: n = next text, p = prior text,
 * s = select text, > = next screen, < = prior screen
 */
#include <LCD.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LCD lcd(rs, en, d4, d5, d6, d7);

void onText(uint16_t screen, int text, const char *txt) {
  Serial.print("selected: ");
  Serial.println(txt);
}

// screen 1
LCDMenuText title1(0, 0, "Settings");
LCDMenuText start(0, 1, "Start", true);
LCDMenuText stop(8, 1, "Stop", true);
const LCDMenuText *texts1[] = {&title1, &start, &stop};
LCDMenuScreen screen1(texts1, nullptr, onText);

// screen 2
LCDMenuText title2(0, 0, "About");
LCDMenuText info(0, 1, "LCD library");
const LCDMenuText *texts2[] = {&title2, &info};
LCDMenuScreen screen2(texts2);

LCDMenuScreen *screens[] = {&screen1, &screen2};
LCDMenu menu(lcd, screens);

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  menu.begin();
}

void loop() {
  if (Serial.available()) {
    switch (Serial.read()) {
      case 'n': menu.nextText(); break;
      case 'p': menu.priorText(); break;
      case 's': menu.selectText(); break;
      case '>': menu.nextScreen(); break;
      case '<': menu.priorScreen(); break;
    }
  }
}
