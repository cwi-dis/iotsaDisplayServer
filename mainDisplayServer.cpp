//
// iotsaDisplayServer: drive an I2C character LCD (e.g. a 20x4 module) over the
// network. Messages, cursor position, backlight and an optional attention buzzer
// are controlled through a small REST-like interface (/api/display) or the
// /display web form. Optional pushbuttons can each fire an HTTP GET to a
// preprogrammed URL on press.
//
// Hardware schematics and breadboard/stripboard layouts are in the "extras"
// folder (Fritzing): the DisplayServer-* files are the original ESP-201 build,
// DisplayServerIotsa.* is the later iotsa-board (ESP-12 / iotsa_v4) version.
//
// (c) 2016 Jack Jansen, Centrum Wiskunde & Informatica. MIT license, see LICENSE.txt.
//

#include <Esp.h>
#include "iotsa.h"
#include "iotsaWifi.h"
#include "iotsaOta.h"

IotsaApplication application("LCD Display Server");

// Configure modules we need
IotsaWifiMod wifiMod(application);  // wifi is always needed
IotsaOtaMod otaMod(application);    // OTA firmware updates

//
// Buzzer section. Set `buzzer` to NULL if there is no buzzer.
//
#include "iotsaBuzzer.h"
#define PIN_ALARM 14  // GPIO14 -- buzzer (-1 for no buzzer)
IotsaBuzzerMod buzzerMod(application, PIN_ALARM);
IotsaBuzzerInterface *buzzer = &buzzerMod;
#define BUTTON_BEEP_DUR 10  // 10ms beep for button press

//
// LCD section.
//
#include "iotsaDisplay.h"
#define LCD_WIDTH 20  // characters per line
#define LCD_HEIGHT 4  // number of lines
#define PIN_SDA 5
#define PIN_SCL 4

IotsaDisplayMod displayMod(application, PIN_SDA, PIN_SCL, LCD_WIDTH, LCD_HEIGHT, buzzer);

//
// Button section. Remove if you don't want buttons.
//
#include "iotsaButton.h"

#define PIN_BUTTON_1 13
#define PIN_BUTTON_2 12

Button buttons[] = {
  Button(PIN_BUTTON_1, true, false),
  Button(PIN_BUTTON_2, true, false)
};
const int nButton = sizeof(buttons) / sizeof(buttons[0]);
callback buttonOk = std::bind(&IotsaBuzzerInterface::set, buzzer, BUTTON_BEEP_DUR);

IotsaButtonMod buttonMod(application, buttons, nButton, NULL, buttonOk);

void setup(void) {
  application.setup();
  application.lateSetup();
#ifndef ESP32
  ESP.wdtEnable(WDTO_120MS);
#endif
}

void loop(void) {
  application.loop();
}
