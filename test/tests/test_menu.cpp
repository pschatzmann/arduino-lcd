// Tests of the declarative menu
#include "Rigs.h"

struct Event {
  int kind;  // 0: screen, 1: text, 2: element
  int screen;
  int text;
  std::string txt;
};
static std::vector<Event> events;

static void onScreen(uint16_t s, int t, const char *txt) {
  events.push_back({0, s, t, txt ? txt : "(null)"});
}
static void onText(uint16_t s, int t, const char *txt) {
  events.push_back({1, s, t, txt ? txt : "(null)"});
}
static void onElement(uint16_t s, int t, const char *txt) {
  events.push_back({2, s, t, txt ? txt : "(null)"});
}

struct MenuRig {
  Parallel4 rig;
  LCDMenuText title1{0, 0, "Settings"};
  LCDMenuText start{0, 1, "Start", true};
  LCDMenuText stop{8, 1, "Stop", true, onElement};
  const LCDMenuText *texts1[3] = {&title1, &start, &stop};
  LCDMenuScreen screen1{texts1, onScreen, onText};

  LCDMenuText title2{0, 0, "About"};
  LCDMenuText info{0, 1, "LCD"};
  const LCDMenuText *texts2[2] = {&title2, &info};
  LCDMenuScreen screen2{texts2};

  LCDMenuText a{0, 0, "A", true};
  LCDMenuText b{2, 0, "B", true};
  LCDMenuText c{4, 0, "C", true};
  const LCDMenuText *texts3[3] = {&a, &b, &c};
  LCDMenuScreen screen3{texts3};

  LCDMenuScreen *screens[3] = {&screen1, &screen2, &screen3};
  LCDMenu menu{rig.lcd, screens};

  MenuRig() {
    events.clear();
    rig.lcd.begin(16, 2);
  }

  void cursor(int &row, int &col) {
    CHECK(rig.hd.cursorCell(row, col));
  }
};

TEST(Menu, nothing_is_shown_before_begin) {
  MenuRig m;
  CHECK_EQ(m.rig.hd.line(0), pad("", 16));
}

TEST(Menu, shows_the_first_screen) {
  MenuRig m;
  m.menu.begin();
  CHECK_EQ(m.rig.hd.line(0), pad("Settings", 16));
  CHECK_EQ(m.rig.hd.line(1), pad("Start   Stop", 16));
  m.rig.hd.snapshot("screen1");
}

TEST(Menu, cursor_is_on_the_first_selectable_text) {
  MenuRig m;
  m.menu.begin();
  CHECK(m.rig.hd.cursorOn());
  CHECK(m.rig.hd.blinkOn());
  int row, col;
  m.cursor(row, col);
  CHECK_EQ(row, 1);
  CHECK_EQ(col, 0);
}

TEST(Menu, next_and_prior_text_wrap_around) {
  MenuRig m;
  m.menu.begin();
  int row, col;
  CHECK_EQ(m.menu.nextText(), 1);
  m.cursor(row, col);
  CHECK_EQ(col, 8);
  CHECK_EQ(m.menu.nextText(), 0);  // wraps
  m.cursor(row, col);
  CHECK_EQ(col, 0);
  CHECK_EQ(m.menu.priorText(), 1);  // wraps backwards
  m.cursor(row, col);
  CHECK_EQ(col, 8);
  CHECK_EQ(m.menu.priorText(), 0);
}

TEST(Menu, set_text_is_limited_to_the_selectable_texts) {
  MenuRig m;
  m.menu.begin();
  CHECK_EQ(m.menu.setText(1), 1);
  CHECK_EQ(m.menu.setText(2), 0);   // beyond the last: first
  CHECK_EQ(m.menu.setText(-1), 1);  // before the first: last
  CHECK_EQ(m.menu.setText(100), 0);
}

TEST(Menu, select_text_calls_the_callbacks) {
  MenuRig m;
  m.menu.begin();
  m.menu.nextText();  // "Stop"
  m.menu.selectText();
  // first the callback of the text, then the one of the screen
  CHECK_EQ(events.size(), (size_t)2);
  if (events.size() != 2) return;
  CHECK_EQ(events[0].kind, 2);
  CHECK_EQ(events[0].txt, "Stop");
  CHECK_EQ(events[0].screen, 0);
  CHECK_EQ(events[0].text, 1);
  CHECK_EQ(events[1].kind, 1);
  CHECK_EQ(events[1].txt, "Stop");
  CHECK_EQ(events[1].screen, 0);
  CHECK_EQ(events[1].text, 1);
}

TEST(Menu, select_text_without_text_callback) {
  MenuRig m;
  m.menu.begin();
  m.menu.selectText();  // "Start" has no own callback
  CHECK_EQ(events.size(), (size_t)1);
  if (events.size() != 1) return;
  CHECK_EQ(events[0].kind, 1);
  CHECK_EQ(events[0].txt, "Start");
  CHECK_EQ(events[0].text, 0);
}

TEST(Menu, select_screen_calls_the_screen_callback) {
  MenuRig m;
  m.menu.begin();
  m.menu.selectScreen();
  CHECK_EQ(events.size(), (size_t)1);
  if (events.size() != 1) return;
  CHECK_EQ(events[0].kind, 0);
  CHECK_EQ(events[0].screen, 0);
  CHECK_EQ(events[0].txt, "Start");
}

TEST(Menu, screens_wrap_around) {
  MenuRig m;
  m.menu.begin();
  CHECK_EQ(m.menu.nextScreen(), 1);
  CHECK_EQ(m.rig.hd.line(0), pad("About", 16));
  CHECK_EQ(m.rig.hd.line(1), pad("LCD", 16));
  m.rig.hd.snapshot("screen2");
  CHECK_EQ(m.menu.nextScreen(), 2);
  CHECK_EQ(m.menu.nextScreen(), 0);  // wraps to the first screen
  CHECK_EQ(m.rig.hd.line(0), pad("Settings", 16));
  CHECK_EQ(m.menu.priorScreen(), 2);  // wraps to the last screen
  CHECK_EQ(m.rig.hd.line(0), pad("A B C", 16));
  CHECK_EQ(m.menu.priorScreen(), 1);
  CHECK_EQ(m.menu.priorScreen(), 0);
  m.rig.hd.snapshot("back_at_screen1");
}

TEST(Menu, first_screen_can_be_shown_again) {
  // regression: screen 0 used to be unreachable
  MenuRig m;
  m.menu.begin(1);
  CHECK_EQ(m.rig.hd.line(0), pad("About", 16));
  CHECK_EQ(m.menu.setScreen(0), 0);
  CHECK_EQ(m.rig.hd.line(0), pad("Settings", 16));
}

TEST(Menu, begin_can_start_at_any_screen) {
  MenuRig m;
  m.menu.begin(2);
  CHECK_EQ(m.rig.hd.line(0), pad("A B C", 16));
}

TEST(Menu, screen_without_selectable_text) {
  MenuRig m;
  m.menu.begin(1);
  CHECK(!m.rig.hd.cursorOn());
  CHECK(!m.rig.hd.blinkOn());
  CHECK_EQ(m.menu.nextText(), -1);
  CHECK_EQ(m.menu.priorText(), -1);
  CHECK_EQ(m.menu.setText(0), -1);
  m.menu.selectText();  // must not crash or call anything
  CHECK_EQ(events.size(), (size_t)0);
}

TEST(Menu, cursor_returns_when_the_screen_changes) {
  MenuRig m;
  m.menu.begin();
  m.menu.nextText();
  m.menu.nextScreen();
  m.menu.priorScreen();
  int row, col;
  m.cursor(row, col);
  CHECK_EQ(col, 0);  // starts at the first selectable text again
}

TEST(Menu, inactive_menu_ignores_calls) {
  MenuRig m;
  m.menu.nextScreen();
  m.menu.selectText();
  m.menu.selectScreen();
  CHECK_EQ(m.menu.nextText(), -1);
  CHECK_EQ(m.rig.hd.line(0), pad("", 16));
  CHECK_EQ(events.size(), (size_t)0);
  m.menu.begin();
  m.menu.end();
  m.menu.nextScreen();
  CHECK_EQ(m.rig.hd.line(0), pad("Settings", 16));  // unchanged
}

TEST(Menu, navigation_through_all_selectable_texts) {
  MenuRig m;
  m.menu.begin(2);
  int row, col;
  const int expected[] = {2, 4, 0, 2};
  for (int i = 0; i < 4; i++) {
    m.menu.nextText();
    m.cursor(row, col);
    CHECK_EQ(col, expected[i]);
  }
  m.rig.hd.snapshot("abc");
}

TEST(Menu, works_with_i2c_display) {
  I2C rig;
  rig.lcd.begin(16, 2);
  LCDMenuText t(0, 0, "I2C menu", true);
  LCDMenuText u(0, 1, "Second", true);
  const LCDMenuText *texts[] = {&t, &u};
  LCDMenuScreen screen(texts);
  LCDMenuScreen *screens[] = {&screen};
  LCDMenu menu(rig.lcd, screens);
  menu.begin();
  menu.nextText();
  CHECK_EQ(rig.hd.line(0), pad("I2C menu", 16));
  CHECK_EQ(rig.hd.line(1), pad("Second", 16));
  rig.hd.snapshot("i2c");
}

TEST(Menu, screens_can_be_copied) {
  // regression: copying a screen used to cause a double delete
  MenuRig m;
  LCDMenuScreen copy = m.screen1;
  (void)copy;
}
