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

#include "iotsa.h"
#include "iotsaWifi.h"
#include "iotsaOta.h"
#include "iotsaSimple.h"
#include "iotsaConfigFile.h"
#include "iotsaApi.h"
#ifdef ESP32
#include <HTTPClient.h>
#else
#include <ESP8266HTTPClient.h>
#endif
#define WITH_OTA // Define if you have an ESP12E or other board with enough Flash memory to allow OTA updates
#define WITH_LCD       // Enable support for LCD, undefine to disable
#define WITH_BUTTONS  // Enable support for buttons, undefine to disable
#define PIN_BUTTON_1 13
#define PIN_BUTTON_2 12
#undef WITH_CREDENTIALS

#define IFDEBUGX if(0)

IotsaWebServer server(80);
IotsaApplication application(server, "LCD Display Server");

// Configure modules we need
IotsaWifiMod wifiMod(application);  // wifi is always needed
#ifdef WITH_OTA
IotsaOtaMod otaMod(application);    // we want OTA for updating the software (will not work with esp-201)
#endif


//
// Configuration and implementation
//
#include "iotsaBuzzer.h"
#ifdef WITH_BUZZER
#define PIN_ALARM 14  // GPIO14 is pin  to which buzzer is connected (-1 for no buzzer)
IotsaBuzzerMod buzzerMod(application, PIN_ALARM);
IotsaBuzzerInterface *buzzer = &buzzerMod;
#else
IotsaBuzzerInterface *buzzer = NULL;
#endif // WITH_BUZZER

#ifdef WITH_LCD
// Includes and defines for liquid crystal server
#include "iotsaDisplay.h"
#define LCD_WIDTH 20  // Number of characters per line on LCD
#define LCD_HEIGHT 4  // Number of lines on LCD
#define PIN_SDA 5
#define PIN_SCL 4

IotsaDisplayMod displayMod(application, PIN_SDA, PIN_SCL, LCD_WIDTH, LCD_HEIGHT, buzzer);
#endif // WITH_LCD

//
// Button parameters and implementation
//
#ifdef WITH_BUTTONS
#ifdef ESP32
#define SSL_INFO_NAME "rootCA"
#else
#define SSL_INFO_NAME "fingerprint"
#endif

typedef struct _Button {
  int pin;
  String url;
  String sslInfo;
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

class IotsaButtonMod : IotsaApiMod {
public:
  IotsaButtonMod(IotsaApplication &_app) : IotsaApiMod(_app) {}
  void setup();
  void serverSetup();
  void loop();
  String info();
protected:
  bool getHandler(const char *path, JsonObject& reply);
  bool putHandler(const char *path, const JsonVariant& request, JsonObject& reply);
  void configLoad();
  void configSave();
  void handler();
};

//
// Decode percent-escaped string src.
// If dst is NULL the result is sent to the LCD.
// 
static void decodePercentEscape(const String &src, String &dst) {
    const char *arg = src.c_str();
    dst = String();
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
      dst += newch;
      arg++;
    }
}

void IotsaButtonMod::configLoad() {
  IotsaConfigFileLoad cf("/config/buttons.cfg");
  for (int i=0; i<nButton; i++) {
    String name = "button" + String(i+1) + "url";
    cf.get(name, buttons[i].url, "");
    name = "button" + String(i+1) + SSL_INFO_NAME;
    cf.get(name, buttons[i].sslInfo, "");
#ifdef WITH_CREDENTIALS
    name = "button" + String(i+1) + "credentials";
    cf.get(name, buttons[i].credentials, "");
#endif
    name = "button" + String(i+1) + "token";
    cf.get(name, buttons[i].token, "");
  }
}

void IotsaButtonMod::configSave() {
  IotsaConfigFileSave cf("/config/buttons.cfg");
  for (int i=0; i<nButton; i++) {
    String name = "button" + String(i+1) + "url";
    cf.put(name, buttons[i].url);
    name = "button" + String(i+1) + SSL_INFO_NAME;
    cf.put(name, buttons[i].sslInfo);
#ifdef WITH_CREDENTIALS
    name = "button" + String(i+1) + "credentials";
    cf.put(name, buttons[i].credentials);
#endif
    name = "button" + String(i+1) + "token";
    cf.put(name, buttons[i].token);
  }
}

void IotsaButtonMod::setup() {
  for (int i=0; i<nButton; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }
  configLoad();
}

void IotsaButtonMod::handler() {
  bool any = false;

  for (uint8_t i=0; i<server.args(); i++){
    for (int j=0; j<nButton; j++) {
      String wtdName = "button" + String(j+1) + "url";
      if (server.argName(i) == wtdName) {
        String arg = server.arg(i);
        decodePercentEscape(arg, buttons[j].url);
        IFDEBUG IotsaSerial.print(wtdName);
        IFDEBUG IotsaSerial.print("=");
        IFDEBUG IotsaSerial.println(buttons[j].url);
        any = true;
      }
      wtdName = "button" + String(j+1) + SSL_INFO_NAME;
      if (server.argName(i) == wtdName) {
        String arg = server.arg(i);
        decodePercentEscape(arg, buttons[j].sslInfo);
        IFDEBUG IotsaSerial.print(wtdName);
        IFDEBUG IotsaSerial.print("=");
        IFDEBUG IotsaSerial.println(buttons[j].sslInfo);
        any = true;
      }
#ifdef WITH_CREDENTIALS
      wtdName = "button" + String(j+1) + "credentials";
      if (server.argName(i) == wtdName) {
        String arg = server.arg(i);
        decodePercentEscape(arg, buttons[j].credentials);
        IFDEBUG IotsaSerial.print(wtdName);
        IFDEBUG IotsaSerial.print("=");
        IFDEBUG IotsaSerial.println(buttons[j].credentials);
        any = true;
      }
#endif
      wtdName = "button" + String(j+1) + "token";
      if (server.argName(i) == wtdName) {
        String arg = server.arg(i);
        decodePercentEscape(arg, buttons[j].token);
        IFDEBUG IotsaSerial.print(wtdName);
        IFDEBUG IotsaSerial.print("=");
        IFDEBUG IotsaSerial.println(buttons[j].token);
        any = true;
      }
    }
  }
  if (any) configSave();

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
#ifdef ESP32
    message += "Root CA cert <i>(https only)</i>: <input name='button" + String(i+1) + "rootCA' value='";
    message += buttons[i].sslInfo;
#else
    message += "Fingerprint <i>(https only)</i>: <input name='button" + String(i+1) + "fingerprint' value='";
    message += buttons[i].sslInfo;
#endif
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

String IotsaButtonMod::info() {
  return "<p>See <a href='/buttons'>/buttons</a> to program URLs for button presses.</a>";
}

bool sendRequest(String urlStr, String token, String credentials, String sslInfo) {
  bool rv = true;
  HTTPClient http;

  if (urlStr.startsWith("https:")) {
#ifdef ESP32
    http.begin(urlStr, sslInfo.c_str());
#else
    http.begin(urlStr, sslInfo);
#endif
  } else {
    http.begin(urlStr);  
  }
  if (token != "") {
    http.addHeader("Authorization", "Bearer " + token);
  }

#ifdef WITH_CREDENTIALS
  if (credentials != "") {
  	String cred64 = b64encode(credentials);
    http.addHeader("Authorization", "Basic " + cred64);
  }
#endif
  int code = http.GET();
  if (code >= 200 && code <= 299) {
    IFDEBUG IotsaSerial.print(code);
    IFDEBUG IotsaSerial.print(" OK GET ");
    IFDEBUG IotsaSerial.println(urlStr);
    // xxxjack should call beep
  } else {
    IFDEBUG IotsaSerial.print(code);
    IFDEBUG IotsaSerial.print(" FAIL GET ");
    IFDEBUG IotsaSerial.print(urlStr);
#ifdef ESP32
    IFDEBUG IotsaSerial.print(", RootCA ");
#else
    IFDEBUG IotsaSerial.print(", fingerprint ");
#endif
    IFDEBUG IotsaSerial.println(sslInfo);
  }
  http.end();
  return rv;
}

void IotsaButtonMod::loop() {
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
        	sendRequest(buttons[i].url, buttons[i].token, buttons[i].credentials, buttons[i].sslInfo);
        }
      }
    }
  }
}

bool IotsaButtonMod::getHandler(const char *path, JsonObject& reply) {
  JsonArray& rv = reply.createNestedArray("buttons");
  for (Button *b=buttons; b<buttons+nButton; b++) {
    JsonObject& bRv = rv.createNestedObject();
    bRv["url"] = b->url;
    bRv[SSL_INFO_NAME] = b->sslInfo;
    bRv["state"] = b->buttonState;
    bRv["hasCredentials"] = b->credentials != "";
    bRv["hasToken"] = b->token != "";
  }
  return true;
}

bool IotsaButtonMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  Button *b;
  if (strcmp(path, "/api/buttons/0") == 0) {
    b = &buttons[0];
  } else if (strcmp(path, "/api/buttons/1") == 0) {
    b = &buttons[1];
  } else {
    return false;
  }
  if (!request.is<JsonObject>()) return false;
  JsonObject& reqObj = request.as<JsonObject>();
  bool any = false;
  if (reqObj.containsKey("url")) {
    any = true;
    b->url = reqObj.get<String>("url");
  }
  if (reqObj.containsKey(SSL_INFO_NAME)) {
    any = true;
    b->sslInfo = reqObj.get<String>(SSL_INFO_NAME);
  }
  if (reqObj.containsKey("credentials")) {
    any = true;
    b->credentials = reqObj.get<String>("credentials");
  }
  if (reqObj.containsKey("token")) {
    any = true;
    b->token = reqObj.get<String>("token");
  }
  if (any) configSave();
  return any;
}

void IotsaButtonMod::serverSetup() {
  server.on("/buttons", std::bind(&IotsaButtonMod::handler, this));
  api.setup("/api/buttons", true, false);
  api.setup("/api/buttons/0", false, true);
  api.setup("/api/buttons/1", false, true);
  name = "buttons";
}

IotsaButtonMod buttonMod(application);
#endif // WITH_BUTTON


//
// Boilerplate for iotsa server, with hooks to our code added.
//
void setup(void) {
  application.setup();
  application.serverSetup();
#ifndef ESP32
  ESP.wdtEnable(WDTO_120MS);
#endif
}
 
void loop(void) {
  application.loop();
} 
