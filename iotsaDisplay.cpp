#include "iotsa.h"
#include "iotsaDisplay.h"


#include <Wire.h>
// The LiquidCrystal library needed is from
// https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/LiquidCrystal_V1.2.1.zip
#include <LiquidCrystal_I2C.h>



LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
unsigned long clearTime;  // time at which to turn off backlight

// LCD handlers
void IotsaDisplayMod::setup() {
  IOTSA_LOG_DEBUG("display", "setup sda %d scl %d %dx%d", pin_sda, pin_scl, lcd_width, lcd_height);
  Wire.begin(pin_sda, pin_scl);
  lcd.begin(lcd_width, lcd_height);
  lcd.backlight();
  lcd.print(iotsaConfig.hostName);
  delay(200);
  lcd.noBacklight();
  lcd.clear();
}

// On the page an empty field means "not given": the cursor stays where it
// is, the backlight gets its default. The API leaves the field out instead.
#ifdef IOTSA_WITH_WEB
static bool _empty(IotsaWebContext& ctx, const char *name) {
  String value;
  return ctx.argPeek(name, value) && value == "";
}
#endif
static bool _empty(IotsaApiContext& ctx, const char *name) { return false; }

// msg, x, y, clear, backlight (seconds, default 5, 0 is off) and alarm
// (tenths of a second), the same on the page and in the API. Commands only:
// nothing is saved, and everyone on the network may use them.
template<class Ctx> void IotsaDisplayMod::_args(Ctx& ctx) {
  String msg;
  int newX = -1;
  int newY = -1;
  bool clear = false;
  float backlight = -1;
  int alarm = 0;
  bool any = ctx.arg("msg", msg, IotsaContext::Transient);
  bool pos = false;
  if (!_empty(ctx, "x")) pos |= ctx.arg("x", newX, IotsaContext::Transient);
  if (!_empty(ctx, "y")) pos |= ctx.arg("y", newY, IotsaContext::Transient);
  any |= ctx.arg("clear", clear, IotsaContext::Transient);
  bool light = !_empty(ctx, "backlight") && ctx.arg("backlight", backlight, IotsaContext::Transient);
  if (buzzer && !_empty(ctx, "alarm")) any |= ctx.arg("alarm", alarm, IotsaContext::Transient);
  if (pos && newX != -1 && (newX < 0 || newX >= lcd_width)) ctx.error(400, "x: 0 to " + String(lcd_width-1));
  if (pos && newY != -1 && (newY < 0 || newY >= lcd_height)) ctx.error(400, "y: 0 to " + String(lcd_height-1));
  if (light && backlight < 0) ctx.error(400, "backlight: seconds, 0 is off");
  if (alarm < 0) ctx.error(400, "alarm: tenths of a second");
  if (ctx.failed() || !(any || pos || light)) return;

  if (clear) {
    lcd.clear();
    if (!pos) x = y = 0;
  }
  if (newX != -1) x = newX;
  if (newY != -1) y = newY;
  lcd.setCursor(x, y);
  printString(msg);
  if (alarm) buzzer->set(alarm*100);
  if (!light) backlight = 5;
  if (backlight > 0) {
    clearTime = millis() + (unsigned long)(backlight*1000);
    IOTSA_LOG_DEBUG("display", "backlight on");
    lcd.backlight();
  } else {
    clearTime = 0;
    IOTSA_LOG_DEBUG("display", "backlight off");
    lcd.noBacklight();
  }
}

#ifdef IOTSA_WITH_WEB
void IotsaDisplayMod::formHandler_args(IotsaWebContext& ctx) {
  _args(ctx);
}

void IotsaDisplayMod::formHandler_fields(IotsaWebContext& ctx) {
  if (!ctx.includesEveryday()) return;
  IotsaWebContext::Form form(ctx, "Show");
  ctx.showField("Message", "msg", "", IotsaWebContext::Transient);
  String where = "empty: where the cursor is (" + String(x) + "," + String(y) + ")";
  ctx.showField("Position X", "x", "", IotsaWebContext::Transient, where.c_str());
  ctx.showField("Position Y", "y", "", IotsaWebContext::Transient);
  ctx.showCheckbox("Clear", "clear", false, IotsaWebContext::Transient, "also moves the cursor to 0,0, unless a position is given");
  ctx.showField("Backlight (seconds)", "backlight", "", IotsaWebContext::Transient, "empty: 5, 0: off");
  if (buzzer) ctx.showField("Alarm (tenths of a second)", "alarm", "", IotsaWebContext::Transient);
}
#endif // IOTSA_WITH_WEB

String IotsaDisplayMod::info() {
  return "<p>See <a href='/display'>/display</a> to display messages or <a href='/api/display'>/api/display</a> for REST interface.</p>";
}

void IotsaDisplayMod::getHandler(IotsaApiContext& ctx) {
  ctx.show("x", x);
  ctx.show("y", y);
  ctx.show("width", lcd_width);
  ctx.show("height", lcd_height);
  unsigned long now = millis();
  ctx.show("backlight", clearTime > now ? (int)((clearTime - now + 999) / 1000) : 0);
}

void IotsaDisplayMod::putHandler(IotsaApiContext& ctx) {
  _args(ctx);
}

void IotsaDisplayMod::loop() {
  if (clearTime && millis() > clearTime) {
    clearTime = 0;
    IOTSA_LOG_DEBUG("display", "backlight off");
    lcd.noBacklight();
  }
}

void IotsaDisplayMod::lateSetup() {
  // /display (the module's page) is registered by api.setup().
  api.setup("display", true, true, true);
  name = "display";
}


void IotsaDisplayMod::printString(String &src) {
  if (src != "") {
    for (size_t i=0; i<src.length(); i++) {
      char newch = src.charAt(i);
      lcd.print(newch);
      x++;
      if (x >= lcd_width) {
        x = 0;
        y++;
        if (y >= lcd_height) y = 0;
        lcd.setCursor(x, y);
      }
    }
  }

}