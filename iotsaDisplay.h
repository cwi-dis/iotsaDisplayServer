#ifndef _IOTSADISPLAY_H_
#define _IOTSADISPLAY_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "iotsaBuzzer.h"

class IotsaDisplayMod : public IotsaModule {
public:
  IotsaDisplayMod(IotsaApplication &_app, int _pin_sda, int _pin_scl, int _lcd_width, int _lcd_height, IotsaBuzzerInterface *_buzzer=NULL)
  : IotsaModule(_app),
    pin_sda(_pin_sda),
    pin_scl(_pin_scl),
    lcd_width(_lcd_width),
    lcd_height(_lcd_height),
    buzzer(_buzzer),
    x(0),
    y(0)
  {}
  void setup() override;
  void lateSetup() override;
  void loop() override;
  String info() override;
protected:
  void getHandler(IotsaApiContext& ctx) override;
  void putHandler(IotsaApiContext& ctx) override;
  void postHandler(IotsaApiContext& ctx) override { putHandler(ctx); }
#ifdef IOTSA_WITH_WEB
  String webTitle() override { return "LCD Display"; }
  void formHandler_args(IotsaWebContext& ctx) override;
  void formHandler_fields(IotsaWebContext& ctx) override;
#endif
private:
  template<class Ctx> void _args(Ctx& ctx);
  void printString(String &src);
  int pin_sda;
  int pin_scl;
  int lcd_width;
  int lcd_height;
  IotsaBuzzerInterface *buzzer;
  int x;
  int y;
};
#endif // _IOTSADISPLAY_H_
