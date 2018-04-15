//
// Server for a 4-line display with a buzzer and 2 buttons.
// Messages to the display (and buzzes) can be controlled with a somewhat REST-like interface.
// The buttons can be polled, or can be setup to send a GET request to a preprogrammed URL.
// 
// Support for buttons, LCD and buzzer can be disabled selectively.
//
// Hardware schematics and PCB stripboard layout can be found in the "extras" folder, in Fritzing format,
// for an ESP201-based device.
//
// (c) 2016, Jack Jansen, Centrum Wiskunde & Informatica.
// License TBD.
//

#include "iotsa.h"
#include "iotsaWifi.h"
#include "iotsaOta.h"
#include "iotsaSimple.h"
#include "iotsaApi.h"

IotsaWebServer server(80);
IotsaApplication application(server, "LCD Display Server");

// Configure modules we need
IotsaWifiMod wifiMod(application);  // wifi is always needed
IotsaOtaMod otaMod(application);    // we want OTA for updating the software (will not work with esp-201)

//
// Buzzer section. Remove if you don't want the buzzer.
// In that case set variable buzzer to NULL.
//
#include "iotsaBuzzer.h"
#define PIN_ALARM 14  // GPIO14 is pin  to which buzzer is connected (-1 for no buzzer)
IotsaBuzzerMod buzzerMod(application, PIN_ALARM);
#if 1
IotsaBuzzerInterface *buzzer = &buzzerMod;
#else
#define buzzer ((IotsaBuzzerInterface *)0)
#endif
#define BUTTON_BEEP_DUR 10  // 10ms beep for button press

//
// LCD section. Removve if you don't want LCD support.
//
#include "iotsaDisplay.h"
#define LCD_WIDTH 20  // Number of characters per line on LCD
#define LCD_HEIGHT 4  // Number of lines on LCD
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

IotsaButtonMod buttonMod(application, buttons, nButton, buttonOk);

//
// Boilerplate for iotsa server, with hooks to our code added.
//
void setup(void) {
  application.setup();
  application.serverSetup();
#ifndef ESP32
  ESP.wdtEnable(WDTO_120MS);
#endif
}
 
void loop(void) {
  application.loop();
} 
