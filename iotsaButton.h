#ifndef _IOTSABUTTON_H_
#define _IOTSABUTTON_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "iotsaBuzzer.h"
#include "iotsaConfigFile.h"

class IotsaRequest {
public:
  IotsaRequest() : url(""), sslInfo(""), credentials(""), token("") {}
  bool send();
  void configLoad(IotsaConfigFileLoad& cf, String& name);
  void configSave(IotsaConfigFileSave& cf, String& name);
  void formHandler(String& message, String& text, String& name);
  bool formArgHandler(IotsaWebServer &server, String name);
  void getHandler(JsonObject& reply);
  bool putHandler(const JsonVariant& request);
  String url;
  String sslInfo;
  String credentials;
  String token;
};

class Button {
public:
  Button(int _pin) : pin(_pin) {}
  int pin;
  int debounceState;
  int debounceTime;
  bool buttonState;
  IotsaRequest req;
};


class IotsaButtonMod : IotsaApiMod {
public:
  IotsaButtonMod(IotsaApplication &_app, Button* _buttons, int _nButton, IotsaBuzzerInterface *_buzzer=NULL)
  : IotsaApiMod(_app),
    buttons(_buttons),
    nButton(_nButton),
    buzzer(_buzzer)
  {}
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
  Button* buttons;
  int nButton;
  IotsaBuzzerInterface *buzzer;
};

#endif // _IOTSABUTTON_H_