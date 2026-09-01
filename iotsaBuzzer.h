#ifndef _IOTSABUZZER_H_
#define _IOTSABUZZER_H_
#include "iotsa.h"

class IotsaBuzzerInterface {
public:
  virtual void set(int duration) = 0;
};

// No REST/web surface of its own -- driven only through IotsaBuzzerInterface by
// other modules (display, buttons). Hence IotsaBaseModule, not IotsaModule.
class IotsaBuzzerMod : public IotsaBaseModule, public IotsaBuzzerInterface {
public:
  IotsaBuzzerMod(IotsaApplication &_app, int _pin) : IotsaBaseModule(_app), pin(_pin), alarmEndTime(0) {};
  void setup() override;
  void loop() override;
  String info() override { return ""; };
  void set(int duration) override;
  int get();
protected:
  int pin;
  unsigned long alarmEndTime;
};

#endif
