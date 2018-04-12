#ifndef _IOTSABUTTON_H_
#define _IOTSABUTTON_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "iotsaBuzzer.h"

typedef struct _Request {
  _Request() : url(""), sslInfo(""), credentials(""), token("") {}
  String url;
  String sslInfo;
  String credentials;
  String token;
};

typedef struct _Button {
  int pin;
  int debounceState;
  int debounceTime;
  bool buttonState;
  _Request req;
} Button;


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