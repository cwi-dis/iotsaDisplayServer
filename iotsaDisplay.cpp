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
  IFDEBUG IotsaSerial.print("lcdSetup");
  Wire.begin(pin_sda, pin_scl);
  lcd.begin(lcd_width, lcd_height);
  lcd.backlight();
  lcd.print(iotsaConfig.hostName);
  delay(200);
  lcd.noBacklight();
  lcd.clear();
  IFDEBUG IotsaSerial.println(" done");
}

void IotsaDisplayMod::handler() {
  String msg;
  bool any = false;
  bool didBacklight = false;
  bool didPos = false;

  for (uint8_t i=0; i<server->args(); i++){
    if( server->argName(i) == "msg") {
      msg = server->arg(i);
      any = true;
    }
    if( server->argName(i) == "clear") {
      if (atoi(server->arg(i).c_str()) > 0) {
        lcd.clear();
        if (!didPos) x = y = 0;
      }
      any = true;
    }
    if( server->argName(i) == "x") {
      const char *arg = server->arg(i).c_str();
      if (arg && *arg) {
        didPos = true;
        x = atoi(server->arg(i).c_str());
      }
    }
    if( server->argName(i) == "y") {
      const char *arg = server->arg(i).c_str();
      if (arg && *arg) {
        didPos = true;
        y = atoi(server->arg(i).c_str());
      }
    }
    if (server->argName(i) == "backlight") {
      const char *arg = server->arg(i).c_str();
      if (arg && *arg) {
        int dur = atoi(server->arg(i).c_str());
//        IFDEBUG IotsaSerial.print("arg backlight=");
//        IFDEBUG IotsaSerial.println(dur);
        any = true;
        didBacklight = true;
        if (dur) {
          clearTime = millis() + dur*1000;
        } else {
          clearTime = 0;
        }
        any = true;
      }
    }
    if (buzzer) {
      if (server->argName(i) == "alarm") {
        const char *arg = server->arg(i).c_str();
        if (arg && *arg) {
          int dur = atoi(server->arg(i).c_str());
          buzzer->set(dur*100);
        }
      }
    }
  }
  if (any) {
      lcd.setCursor(x, y);
//      IFDEBUG IotsaSerial.print("Show message ");
//      IFDEBUG IotsaSerial.println(msg);
      printPercentEscape(msg);
      if (!didBacklight) 
         clearTime = millis() + 5000; // 5 seconds is the default display time
      if (clearTime) {
        IFDEBUG IotsaSerial.println("backlight");
        lcd.backlight();
      } else {
        IFDEBUG IotsaSerial.println("nobacklight");
        lcd.noBacklight();
      }
  }
  String message = "<html><head><title>LCD Server</title></head><body><h1>LCD Server</h1>";
  message += "<form method='get'>Message: <input name='msg' value=''><br>\n";
  message += "Position X: <input name='x' value=''> Y: <input name='y' value=''><br>\n";
  message += "<input name='clear' type='checkbox' value='1'>Clear<br>\n";
  message += "Backlight: <input name='backlight' value=''> seconds<br>\n";
  if (buzzer) {
    message += "Alarm: <input name='alarm' value=''> (times 0.1 second)<br>\n";
  }
  message += "<input type='submit'></form></body></html>";
  server->send(200, "text/html", message);
  
}

bool IotsaDisplayMod::postHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  bool any = false;
  if (!request.is<JsonObject>()) return false;
  JsonObject& reqObj = request.as<JsonObject>();
  if (reqObj.get<bool>("clear")) {
    any = true;
    lcd.clear();
  }
  if (reqObj.containsKey("x") || reqObj.containsKey("y")) {
    x = reqObj.get<int>("x");
    y = reqObj.get<int>("y");
    lcd.setCursor(x, y);
    any = true;
  }
  if (buzzer) {
    int alarm = reqObj.get<int>("alarm");
    if (alarm) {
      any = true;
      buzzer->set(alarm*100);      
    }
  }
  int backlight = 5000;
  if (reqObj.containsKey("backlight")) {
    backlight = int(reqObj.get<float>("backlight") * 1000);
    any = true;
  }
  if (backlight) {
    clearTime = millis() + backlight;
    IFDEBUG IotsaSerial.println("backlight");
    lcd.backlight();
  } else {
    IFDEBUG IotsaSerial.println("nobacklight");
    lcd.noBacklight();
  }
  String msg = reqObj.get<String>("msg");
  if (msg != "") {
      printString(msg);
      any = true;
  }
  return any;
}

String IotsaDisplayMod::info() {
  return "<p>See <a href='/display'>/display</a> to display messages or <a href='/api/display'>/api/display</a> for REST interface.</p>";
}

void IotsaDisplayMod::loop() {
  if (clearTime && millis() > clearTime) {
    clearTime = 0;
    IFDEBUG IotsaSerial.println("nobacklight");
    lcd.noBacklight();
  }
}

void IotsaDisplayMod::serverSetup() {
  server->on("/display", std::bind(&IotsaDisplayMod::handler, this));
  api.setup("/api/display", false, false, true);
  name = "display";
}


void IotsaDisplayMod::printPercentEscape(String &src) {
    const char *arg = src.c_str();
    while (*arg) {
      char newch;
      if (*arg == '+') newch = ' ';
      else if (*arg == '%') {
        arg++;
        if (*arg >= '0' && *arg <= '9') newch = (*arg-'0') << 4;
        if (*arg >= 'a' && *arg <= 'f') newch = (*arg-'a'+10) << 4;
        if (*arg >= 'A' && *arg <= 'F') newch = (*arg-'A'+10) << 4;
        arg++;
        if (*arg == 0) break;
        if (*arg >= '0' && *arg <= '9') newch |= (*arg-'0');
        if (*arg >= 'a' && *arg <= 'f') newch |= (*arg-'a'+10);
        if (*arg >= 'A' && *arg <= 'F') newch |= (*arg-'A'+10);
      } else {
        newch = *arg;
      }
      lcd.print(newch);
      x++;
      if (x >= lcd_width) {
        x = 0;
        y++;
        if (y >= lcd_height) y = 0;
        lcd.setCursor(x, y);
      }
      arg++;
    }
}

void IotsaDisplayMod::printString(String &src) {
  if (src != "") {
    for (int i=0; i<src.length(); i++) {
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