#ifndef _IOTSABUTTON_H_
#define _IOTSABUTTON_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "iotsaBuzzer.h"
#include "iotsaRequest.h"

class Button {
public:
  Button(int _pin, bool _sendOnPress, bool _sendOnRelease) : pin(_pin), sendOnPress(_sendOnPress), sendOnRelease(_sendOnRelease) {}
  int pin;
  bool sendOnPress;
  bool sendOnRelease;
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