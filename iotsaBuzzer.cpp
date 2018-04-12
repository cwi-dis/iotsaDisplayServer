#include "iotsaBuzzer.h"

void IotsaBuzzerMod::setup() {
  pinMode(pin, INPUT); // Trick: we configure to input so we make the pin go Hi-Z.
}

void IotsaBuzzerMod::set(int dur) {
  if (dur) {
    alarmEndTime = millis() + dur*100;
    IotsaSerial.println("alarm on");
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  } else {
    alarmEndTime = 0;
  }
}

int IotsaBuzzerMod::get() {
    if (alarmEndTime == 0) return 0;
    return alarmEndTime - millis();
}

void IotsaBuzzerMod::loop() {
  if (alarmEndTime && millis() > alarmEndTime) {
    alarmEndTime = 0;
    IotsaSerial.println("alarm off");
    pinMode(pin, INPUT);
  }
}