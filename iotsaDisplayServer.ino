//
// Server for a 4-line display with a buzzer and 2 buttons.
// Messages to the display (and buzzes) can be controlled with a somewhat REST-like interface.
// The buttons can be polled, or can be setup to send a GET request to a preprogrammed URL.
// 
// Support for buttons, LCD and buzzer can be disabled selectively.
//
// Hardware schematics and PCB stripboard layout can be found in the "extras" folder, in Fritzing format,
// for an ESP201-based device.
//
// (c) 2016, Jack Jansen, Centrum Wiskunde & Informatica.
// License TBD.
//

#include <ESP.h>
#include <FS.h>
#include <ESP8266HTTPClient.h>
#include "iotsa.h"
#include "iotsaWifi.h"
#include "iotsaOta.h"
#include "iotsaFilesBackup.h"
#include "iotsaSimple.h"
#include "iotsaConfigFile.h"

#define WITH_OTA // Define if you have an ESP12E or other board with enough Flash memory to allow OTA updates

#define PIN_ALARM 14  // GPIO14 is pin  to which buzzer is connected (undefine for no buzzer)
#define WITH_LCD       // Enable support for LCD, undefine to disable
#define LCD_WIDTH 20  // Number of characters per line on LCD
#define LCD_HEIGHT 4  // Number of lines on LCD
#define PIN_SDA 5
#define PIN_SCL 4
#define WITH_BUTTONS  // Enable support for buttons, undefine to disable
#define PIN_BUTTON_1 13
#define PIN_BUTTON_2 12
#undef WITH_CREDENTIALS

#define IFDEBUGX if(0)

ESP8266WebServer server(80);
IotsaApplication application(server, "LCD Display Server");

// Configure modules we need
IotsaWifiMod wifiMod(application);  // wifi is always needed
#ifdef WITH_OTA
IotsaOtaMod otaMod(application);    // we want OTA for updating the software (will not work with esp-201)
#endif
IotsaFilesBackupMod filesBackupMod(application);  // we want backup to clone the display server

static void decodePercentEscape(String &src, String *dst); // Forward declaration

//
// Buzzer configuration and implementation
//
#ifdef PIN_ALARM
unsigned long alarmEndTime;
#endif

//
// LCD configuration and implementation
//
#ifdef WITH_LCD
// Includes and defines for liquid crystal server

#include <Wire.h>
// The LiquidCrystal library needed is from
// https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/LiquidCrystal_V1.2.1.zip
#include <LiquidCrystal_I2C.h>



LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
unsigned long clearTime;  // time at which to turn off backlight

// LCD handlers
void lcdSetup() {
  IFDEBUG IotsaSerial.print("lcdSetup");
  Wire.begin(PIN_SDA, PIN_SCL);
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  lcd.backlight();
  lcd.print(hostName);
  delay(200);
  lcd.noBacklight();
  lcd.clear();
#ifdef PIN_ALARM
  pinMode(PIN_ALARM, INPUT); // Trick: we configure to input so we make the pin go Hi-Z.
#endif
  IFDEBUG IotsaSerial.println(" done");
}

int x=0;
int y=0;

void lcdHandler() {
  String msg;
  bool any = false;
  bool didBacklight = false;
  bool didPos = false;

  for (uint8_t i=0; i<server.args(); i++){
    if( server.argName(i) == "msg") {
      msg = server.arg(i);
      any = true;
    }
    if( server.argName(i) == "clear") {
      if (atoi(server.arg(i).c_str()) > 0) {
        lcd.clear();
        if (!didPos) x = y = 0;
      }
      any = true;
    }
    if( server.argName(i) == "x") {
      const char *arg = server.arg(i).c_str();
      if (arg && *arg) {
        didPos = true;
        x = atoi(server.arg(i).c_str());
      }
    }
    if( server.argName(i) == "y") {
      const char *arg = server.arg(i).c_str();
      if (arg && *arg) {
        didPos = true;
        y = atoi(server.arg(i).c_str());
      }
    }
    if (server.argName(i) == "backlight") {
      const char *arg = server.arg(i).c_str();
      if (arg && *arg) {
        int dur = atoi(server.arg(i).c_str());
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
#ifdef PIN_ALARM
    if (server.argName(i) == "alarm") {
      const char *arg = server.arg(i).c_str();
      if (arg && *arg) {
        int dur = atoi(server.arg(i).c_str());
//        IFDEBUG IotsaSerial.print("arg alarm=");
//        IFDEBUG IotsaSerial.println(dur);
        if (dur) {
          alarmEndTime = millis() + dur*100;
          IotsaSerial.println("alarm on");
          pinMode(PIN_ALARM, OUTPUT);
          digitalWrite(PIN_ALARM, LOW);
        } else {
          alarmEndTime = 0;
        }
      }
    }
#endif
  }
  if (any) {
      lcd.setCursor(x, y);
//      IFDEBUG IotsaSerial.print("Show message ");
//      IFDEBUG IotsaSerial.println(msg);
      decodePercentEscape(msg, NULL);
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
#ifdef PIN_ALARM
  message += "Alarm: <input name='alarm' value=''> (times 0.1 second)<br>\n";
#endif
  message += "<input type='submit'></form></body></html>";
  server.send(200, "text/html", message);
  
}

String lcdInfo() {
  return "<p>See <a href='/display'>/display</a> to display messages.";
}

void lcdLoop() {
  if (clearTime && millis() > clearTime) {
    clearTime = 0;
    lcd.noBacklight();
  }
#ifdef PIN_ALARM
  if (alarmEndTime && millis() > alarmEndTime) {
    alarmEndTime = 0;
    IotsaSerial.println("alarm off");
    pinMode(PIN_ALARM, INPUT);
  }
#endif
}

IotsaSimpleMod displayMod(application, "/display", lcdHandler, lcdInfo);

#endif // WITH_LCD

//
// Button parameters and implementation
//
#ifdef WITH_BUTTONS

typedef struct _Button {
  int pin;
  String url;
  String fingerprint;
  String credentials;
  String token;
  int debounceState;
  int debounceTime;
  bool buttonState;
} Button;

Button buttons[] = {
  { PIN_BUTTON_1, "", "", "", "", 0, 0, false},
  { PIN_BUTTON_2, "", "", "", "", 0, 0, false}
};

const int nButton = sizeof(buttons) / sizeof(buttons[0]);

#define DEBOUNCE_DELAY 50 // 50 ms debouncing
#define BUTTON_BEEP_DUR 10  // 10ms beep for button press

void buttonConfigLoad() {
  IotsaConfigFileLoad cf("/config/buttons.cfg");
  for (int i=0; i<nButton; i++) {
    String name = "button" + String(i+1) + "url";
    cf.get(name, buttons[i].url, "");
    name = "button" + String(i+1) + "fingerprint";
    cf.get(name, buttons[i].fingerprint, "");
#ifdef WITH_CREDENTIALS
    name = "button" + String(i+1) + "credentials";
    cf.get(name, buttons[i].credentials, "");
#endif
    name = "button" + String(i+1) + "token";
    cf.get(name, buttons[i].token, "");
  }
}

void buttonConfigSave() {
  IotsaConfigFileSave cf("/config/buttons.cfg");
  for (int i=0; i<nButton; i++) {
    String name = "button" + String(i+1) + "url";
    cf.put(name, buttons[i].url);
    name = "button" + String(i+1) + "fingerprint";
    cf.put(name, buttons[i].fingerprint);
#ifdef WITH_CREDENTIALS
    name = "button" + String(i+1) + "credentials";
    cf.put(name, buttons[i].credentials);
#endif
    name = "button" + String(i+1) + "token";
    cf.put(name, buttons[i].token);
  }
}

void buttonSetup() {
  for (int i=0; i<nButton; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }
  buttonConfigLoad();
}

void buttonHandler() {
  bool any = false;
  bool isJSON = false;

  for (uint8_t i=0; i<server.args(); i++){
    for (int j=0; j<nButton; j++) {
      String wtdName = "button" + String(j+1) + "url";
      if (server.argName(i) == wtdName) {
        String arg = server.arg(i);
        decodePercentEscape(arg, &buttons[j].url);
        IFDEBUG IotsaSerial.print(wtdName);
        IFDEBUG IotsaSerial.print("=");
        IFDEBUG IotsaSerial.println(buttons[j].url);
        any = true;
      }
      wtdName = "button" + String(j+1) + "fingerprint";
      if (server.argName(i) == wtdName) {
        String arg = server.arg(i);
        decodePercentEscape(arg, &buttons[j].fingerprint);
        IFDEBUG IotsaSerial.print(wtdName);
        IFDEBUG IotsaSerial.print("=");
        IFDEBUG IotsaSerial.println(buttons[j].fingerprint);
        any = true;
      }
#ifdef WITH_CREDENTIALS
      wtdName = "button" + String(j+1) + "credentials";
      if (server.argName(i) == wtdName) {
        String arg = server.arg(i);
        decodePercentEscape(arg, &buttons[j].credentials);
        IFDEBUG IotsaSerial.print(wtdName);
        IFDEBUG IotsaSerial.print("=");
        IFDEBUG IotsaSerial.println(buttons[j].credentials);
        any = true;
      }
#endif
      wtdName = "button" + String(j+1) + "token";
      if (server.argName(i) == wtdName) {
        String arg = server.arg(i);
        decodePercentEscape(arg, &buttons[j].token);
        IFDEBUG IotsaSerial.print(wtdName);
        IFDEBUG IotsaSerial.print("=");
        IFDEBUG IotsaSerial.println(buttons[j].token);
        any = true;
      }
    }
    if (server.argName(i) == "format" && server.arg(i) == "json") {
      isJSON = true;
    }
  }
  if (any) buttonConfigSave();
  if (isJSON) {
    String message = "{\"buttons\" : [";
    for (int i=0; i<nButton; i++) {
      if (i > 0) message += ", ";
      if (buttons[i].buttonState) {
        message += "true";
      } else {
        message += "false";
      }
    }
    message += "]";
    for (int i=0; i<nButton; i++) {
      message += ", \"button";
      message += String(i+1);
      message += "url\" : \"";
      message += buttons[i].url;
      message += "\"";

      message += ", \"button";
      message += String(i+1);
      message += "fingerprint\" : \"";
      message += buttons[i].fingerprint;
      message += "\"";

#ifdef WITH_CREDENTIALS
      message += ", \"button";
      message += String(i+1);
      message += "credentials\" : \"";
      message += buttons[i].credentials;
      message += "\"";
#endif

      message += ", \"button";
      message += String(i+1);
      message += "token\" : \"";
      message += buttons[i].token;
      message += "\"";

    }
    message += "\"}\n";
    server.send(200, "application/json", message);
  } else {
    String message = "<html><head><title>LCD Server Buttons</title></head><body><h1>LCD Server Buttons</h1>";
    for (int i=0; i<nButton; i++) {
      message += "<p>Button " + String(i+1) + ": ";
      if (buttons[i].buttonState) message += "on"; else message += "off";
      message += "</p>";
    }
    message += "<form method='get'>";
    for (int i=0; i<nButton; i++) {
      message += "<em>Button " + String(i+1) + "</em><br>\n";
      message += "Activation URL: <input name='button" + String(i+1) + "url' value='";
      message += buttons[i].url;
      message += "'><br>\n";

      message += "Fingerprint <i>(https only)</i>: <input name='button" + String(i+1) + "fingerprint' value='";
      message += buttons[i].fingerprint;
      message += "'><br>\n";

      message += "Bearer token <i>(optional)</i>: <input name='button" + String(i+1) + "token' value='";
      message += buttons[i].token;
      message += "'><br>\n";

#ifdef WITH_CREDENTIALS
      message += "Credentials <i>(optional, user:pass)</i>: <input name='button" + String(i+1) + "credentials' value='";
      message += buttons[i].credentials;
      message += "'><br>\n";
#endif

    }
    message += "<input type='submit'></form></body></html>";
    server.send(200, "text/html", message);
  }
}

String buttonInfo() {
  return "<p>See <a href='/buttons'>/buttons</a> to program URLs for button presses.</a>";
}

bool sendRequest(String urlStr, String token, String credentials, String fingerprint) {
  bool rv = true;
  HTTPClient http;

  if (urlStr.startsWith("https:")) {
    http.begin(urlStr, fingerprint);
  } else {
    http.begin(urlStr);  
  }
  if (token != "") {
    http.addHeader("Authorization", "Bearer " + token);
  }

#ifdef WITH_CREDENTIALS
  if (credentials != "") {
  	String cred64 = b64encode(credentials);
    http.addHeader("Authorization", "Bearer " + cred64);
  }
#endif
  int code = http.GET();
  if (code >= 200 && code <= 299) {
    IFDEBUG IotsaSerial.print(code);
    IFDEBUG IotsaSerial.print(" OK GET ");
    IFDEBUG IotsaSerial.println(urlStr);
 #ifdef PIN_ALARM
    alarmEndTime = millis() + BUTTON_BEEP_DUR;
    pinMode(PIN_ALARM, OUTPUT);
    digitalWrite(PIN_ALARM, LOW);
#endif // PIN_ALARM
  } else {
    IFDEBUG IotsaSerial.print(code);
    IFDEBUG IotsaSerial.print(" FAIL GET ");
    IFDEBUG IotsaSerial.println(urlStr);
  }
  http.end();
  return rv;
}

void buttonLoop() {
  for (int i=0; i<nButton; i++) {
    int state = digitalRead(buttons[i].pin);
    if (state != buttons[i].debounceState) {
      buttons[i].debounceTime = millis();
    }
    buttons[i].debounceState = state;
    if (millis() > buttons[i].debounceTime + DEBOUNCE_DELAY) {
      int newButtonState = (state == LOW);
      if (newButtonState != buttons[i].buttonState) {
        buttons[i].buttonState = newButtonState;
        if (buttons[i].buttonState && buttons[i].url != "") {
        	sendRequest(buttons[i].url, buttons[i].token, buttons[i].credentials, buttons[i].fingerprint);
        }
      }
    }
  }
}

IotsaSimpleMod buttonMod(application, "/buttons", buttonHandler, buttonInfo);

#endif // WITH_BUTTON

//
// Decode percent-escaped string src.
// If dst is NULL the result is sent to the LCD.
// 
static void decodePercentEscape(String &src, String *dst) {
    const char *arg = src.c_str();
    if (dst) *dst = String();
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
      if (dst) {
        *dst += newch;
      } else {
#ifdef WITH_LCD
        lcd.print(newch);
        x++;
        if (x >= LCD_WIDTH) {
          x = 0;
          y++;
          if (y >= LCD_HEIGHT) y = 0;
          lcd.setCursor(x, y);
        }
#endif
      }
      arg++;
    }
}

//
// Boilerplate for iotsa server, with hooks to our code added.
//
void setup(void) {
  application.setup();
  application.serverSetup();
#ifdef WITH_LCD
  lcdSetup();
#endif
#ifdef WITH_BUTTONS
  buttonSetup();
#endif
  ESP.wdtEnable(WDTO_120MS);
}
 
void loop(void) {
  application.loop();
#ifdef WITH_LCD
  lcdLoop();
#endif
#ifdef WITH_BUTTONS
  buttonLoop();
#endif
} 
