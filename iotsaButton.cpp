#include "iotsaButton.h"
#include "iotsaConfigFile.h"

#define DEBOUNCE_DELAY 50 // 50 ms debouncing
#define BUTTON_BEEP_DUR 10  // 10ms beep for button press

void IotsaButtonMod::configLoad() {
  IotsaConfigFileLoad cf("/config/buttons.cfg");
  for (int i=0; i<nButton; i++) {
    String name = "button" + String(i+1);
    buttons[i].req.configLoad(cf, name);
  }
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
                if (buzzer) buzzer->set(BUTTON_BEEP_DUR);
            }
        }
      }
    }
  }
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
