#ifndef _IOTSADISPLAY_H_
#define _IOTSADISPLAY_H_
#include "iotsa.h"
#include "iotsaApi.h"

class IotsaDisplayMod : IotsaApiMod {
public:
  IotsaDisplayMod(IotsaApplication &_app, int _pin_sda, int _pin_scl, int _lcd_width, int _lcd_height, int _pin_alarm=-1)
  : IotsaApiMod(_app),
    pin_sda(_pin_sda),
    pin_scl(_pin_scl),
    lcd_width(_lcd_width),
    lcd_height(_lcd_height),
    pin_alarm(_pin_alarm),
    x(0),
    y(0),
    alarmEndTime(0)
  {}
  void setup();
  void serverSetup();
  void loop();
  String info();
  void alarm(int ms);
protected:
  bool postHandler(const char *path, const JsonVariant& request, JsonObject& reply);
private:
  void handler();
  void printPercentEscape(String &src);
  void printString(String &src);
  int pin_sda;
  int pin_scl;
  int lcd_width;
  int lcd_height;
  int pin_alarm;
  int x;
  int y;
  unsigned long alarmEndTime;
};
#endif // _IOTSADISPLAY_H_