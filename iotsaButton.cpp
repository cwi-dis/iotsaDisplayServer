#include "iotsaButton.h"
#include "iotsaConfigFile.h"
#ifdef ESP32
#include <HTTPClient.h>
#else
#include <ESP8266HTTPClient.h>
#endif

#ifdef ESP32
#define SSL_INFO_NAME "rootCA"
#else
#define SSL_INFO_NAME "fingerprint"
#endif


#define DEBOUNCE_DELAY 50 // 50 ms debouncing
#define BUTTON_BEEP_DUR 10  // 10ms beep for button press

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

void IotsaRequest::configLoad(IotsaConfigFileLoad& cf, String& name) {
    cf.get(name+"url", url, "");
    cf.get(name + SSL_INFO_NAME, sslInfo, "");
    cf.get(name + "credentials", credentials, "");
    cf.get(name + "token", token, "");
}

void IotsaButtonMod::configLoad() {
  IotsaConfigFileLoad cf("/config/buttons.cfg");
  for (int i=0; i<nButton; i++) {
    String name = "button" + String(i+1);
    buttons[i].req.configLoad(cf, name);
  }
}

void IotsaRequest::configSave(IotsaConfigFileSave& cf, String& name) {
    cf.put(name + "url", url);
    cf.put(name + SSL_INFO_NAME, sslInfo);
    cf.put(name + "credentials", credentials);
    cf.put(name + token, token);
}

void IotsaButtonMod::configSave() {
  IotsaConfigFileSave cf("/config/buttons.cfg");
  for (int i=0; i<nButton; i++) {
    String name = "button" + String(i+1);
    buttons[i].req.configSave(cf, name);
  }
}

void IotsaButtonMod::setup() {
  for (int i=0; i<nButton; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }
  configLoad();
}

void IotsaRequest::formHandler(String& message, String& text, String& name) {  
    message += "<em>" + text +  "</em><br>\n";
    message += "Activation URL: <input name='" + name +  "url' value='";
    message += url;
    message += "'><br>\n";
#ifdef ESP32
    message += "Root CA cert <i>(https only)</i>: <input name='" + name + "rootCA' value='";
    message += sslInfo;
#else
    message += "Fingerprint <i>(https only)</i>: <input name='" + name +  "fingerprint' value='";
    message += sslInfo;
#endif
    message += "'><br>\n";

    message += "Bearer token <i>(optional)</i>: <input name='" + name + "token' value='";
    message += token;
    message += "'><br>\n";

    message += "Credentials <i>(optional, user:pass)</i>: <input name='" + name + "credentials' value='";
    message += credentials;
    message += "'><br>\n";
}

bool IotsaRequest::formArgHandler(IotsaWebServer &server, String name) {
  bool any = false;
  String wtdName = name + "url";
  if (server.hasArg(wtdName)) {
    decodePercentEscape(server.arg(wtdName), url);
    IFDEBUG IotsaSerial.print(wtdName);
    IFDEBUG IotsaSerial.print("=");
    IFDEBUG IotsaSerial.println(url);
    any = true;
  }
  wtdName = name + "SSL_INFO_NAME";
  if (server.hasArg(wtdName)) {
    decodePercentEscape(server.arg(wtdName), sslInfo);
    IFDEBUG IotsaSerial.print(wtdName);
    IFDEBUG IotsaSerial.print("=");
    IFDEBUG IotsaSerial.println(sslInfo);
    any = true;
  }
  wtdName = name + "credentials";
  if (server.hasArg(wtdName)) {
    decodePercentEscape(server.arg(wtdName), credentials);
    IFDEBUG IotsaSerial.print(wtdName);
    IFDEBUG IotsaSerial.print("=");
    IFDEBUG IotsaSerial.println(credentials);
    any = true;
    }
  wtdName = name + "token";
  if (server.hasArg(wtdName)) {
    decodePercentEscape(server.arg(wtdName), token);
    IFDEBUG IotsaSerial.print(wtdName);
    IFDEBUG IotsaSerial.print("=");
    IFDEBUG IotsaSerial.println(token);
    any = true;
  }
  return any;
}

void IotsaButtonMod::handler() {
  bool any = false;

  for (uint8_t i=0; i<server.args(); i++){
    for (int j=0; j<nButton; j++) {
      if (buttons[j].req.formArgHandler(server, "button" + String(j+1))) {
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
    buttons[i].req.formHandler(message, "Button " + String(i+1), "button" + String(i+1));

  }
  message += "<input type='submit'></form></body></html>";
  server.send(200, "text/html", message);
}

String IotsaButtonMod::info() {
  return "<p>See <a href='/buttons'>/buttons</a> to program URLs for button presses.</a>";
}

bool IotsaRequest::send() {
  bool rv = true;
  HTTPClient http;

  if (url.startsWith("https:")) {
#ifdef ESP32
    rv = http.begin(url, sslInfo.c_str());
#else
    rv = http.begin(url, sslInfo);
#endif
  } else {
    rv = http.begin(url);  
  }
  if (!rv) return false;
  if (token != "") {
    http.addHeader("Authorization", "Bearer " + token);
  }

  if (credentials != "") {
#if 1
    IotsaSerial.print("Credentials not yet implemented");
#else
  	String cred64 = b64encode(credentials);
    http.addHeader("Authorization", "Basic " + cred64);
#endif
  }
  int code = http.GET();
  if (code >= 200 && code <= 299) {
    IFDEBUG IotsaSerial.print(code);
    IFDEBUG IotsaSerial.print(" OK GET ");
    IFDEBUG IotsaSerial.println(url);
  } else {
    IFDEBUG IotsaSerial.print(code);
    IFDEBUG IotsaSerial.print(" FAIL GET ");
    IFDEBUG IotsaSerial.print(url);
#ifdef ESP32
    IFDEBUG IotsaSerial.print(", RootCA ");
#else
    IFDEBUG IotsaSerial.print(", fingerprint ");
#endif
    IFDEBUG IotsaSerial.println(sslInfo);
    rv = false;
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
        if (buttons[i].buttonState && buttons[i].req.url != "") {
            if (buttons[i].req.send()) {
                if (buzzer) buzzer->set(10);
            }
        }
      }
    }
  }
}

void IotsaRequest::getHandler(JsonObject& reply) {
  reply["url"] = url;
  reply[SSL_INFO_NAME] = sslInfo;
  reply["hasCredentials"] = credentials != "";
  reply["hasToken"] = token != "";
}

bool IotsaButtonMod::getHandler(const char *path, JsonObject& reply) {
  JsonArray& rv = reply.createNestedArray("buttons");
  for (Button *b=buttons; b<buttons+nButton; b++) {
    JsonObject& bRv = rv.createNestedObject();
    b->req.getHandler(bRv);
    bRv["state"] = b->buttonState;
  }
  return true;
}

bool IotsaRequest::putHandler(const JsonVariant& request) {
  if (!request.is<JsonObject>()) return false;
  bool any = false;
  const JsonObject& reqObj = request.as<JsonObject>();
  if (reqObj.containsKey("url")) {
    any = true;
    url = reqObj.get<String>("url");
  }
  if (reqObj.containsKey(SSL_INFO_NAME)) {
    any = true;
    sslInfo = reqObj.get<String>(SSL_INFO_NAME);
  }
  if (reqObj.containsKey("credentials")) {
    any = true;
    credentials = reqObj.get<String>("credentials");
  }
  if (reqObj.containsKey("token")) {
    any = true;
    token = reqObj.get<String>("token");
  }
}

bool IotsaButtonMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  bool any = false;
  if (strcmp(path, "/api/buttons") == 0) {
      if (!request.is<JsonArray>()) return false;
      const JsonArray& all = request.as<JsonArray>();
      for (int i=0; i<nButton; i++) {
          const JsonVariant& r = all[i];
          if (buttons[i].req.putHandler(r)) {
              any = true;
          }
      }
  } else {
      String num(path);
      num.remove(0, 12);
      int idx = num.toInt();
      Button *b = buttons + idx;
      if (b->req.putHandler(request)) {
        any = true;
      }
  }
  if (any) configSave();
  return any;
}

void IotsaButtonMod::serverSetup() {
  server.on("/buttons", std::bind(&IotsaButtonMod::handler, this));
  api.setup("/api/buttons", true, false);
  for(int i=0; i<nButton; i++) {
      String *p = new String("/api/buttons" + String(i));
      api.setup(p->c_str(), true, true);
  }
  name = "buttons";
}
