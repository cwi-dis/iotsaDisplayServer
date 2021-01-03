#ifndef _IOTSABUZZER_H_
#define _IOTSABUZZER_H_
#include "iotsa.h"
#include "iotsaApi.h"

class IotsaBuzzerInterface {
public:
  virtual void set(int duration) = 0;
};

class IotsaBuzzerMod : public IotsaApiMod, public IotsaBuzzerInterface {
public:
  IotsaBuzzerMod(IotsaApplication &_app, int _pin) : IotsaApiMod(_app), pin(_pin), alarmEndTime(0) {};
  virtual void setup() override;
  virtual void serverSetup() override {};
  virtual void loop() override;
  virtual String info() override { return ""; };
  virtual void set(int duration);
  virtual int get();
protected:
  int pin;
  unsigned long alarmEndTime;
};

#endif
