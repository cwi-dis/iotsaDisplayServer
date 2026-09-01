# iotsaDisplayServer - web server to drive an LCD display

![build-platformio](https://github.com/cwi-dis/iotsaDisplayServer/workflows/build-platformio/badge.svg)
![build-arduino](https://github.com/cwi-dis/iotsaDisplayServer/workflows/build-arduino/badge.svg)

iotsaDisplayServer is a web server that drives an LCD display, such as an i2c 4x20 character module. Support for a buzzer (to attract user attention) and buttons (programmable to trigger actions by accessing programmable URLs) is included.

Home page is <https://github.com/cwi-dis/iotsaDisplayServer>.
This software is licensed under the [MIT license](LICENSE.txt) by the   CWI DIS group, <http://www.dis.cwi.nl>.

## Software requirements

* PlatformIO (recommended). `pio run` builds every environment in `platformio.ini`; the LiquidCrystal library is pulled in automatically.
* Or the Arduino IDE, with the iotsa framework from <https://github.com/cwi-dis/iotsa> and the fmalpartida "New LiquidCrystal" library (<https://github.com/fmalpartida/New-LiquidCrystal>).

## Hardware requirements

* an ESP8266 board (ESP-12 / iotsa_v4; the original build used an ESP-201).
* An i2c LCD module.
* Optionally some pushbuttons and a buzzer.

## Hardware construction

The _extras_ subfolder has the design files. `DisplayServer-schematic.pdf` / `-breadboard.pdf` / `-stripboard.pdf` (and the matching `.fzz` [Fritzing](http://fritzing.org/home/) projects) are the original ESP-201 build; `DisplayServerIotsa.fzz` / `.pdf` is the later iotsa-board version.

## Building the software

The pin assignments, LCD size and button GPIOs are `#define`s near the top of `mainDisplayServer.cpp` -- adjust them to your hardware. Compile and flash with `pio run -e nodemcuv2 -t upload`, or over-the-air once the device is on the network.

It is also possible to build variants of this service by specifying _iotsa_ build flags:

- `-DIOTSA_WITHOUT_HTTP -DIOTSA_WITH_HTTPS` builds a version that uses https (for securing your display)
- `-DIOTSA_WITHOUT_HTTP -DIOTSA_WITHOUT_REST -DIOTSA_WITH_COAP` builds a version with no web interface and no REST interface. In stead, the API is exposed over UDP using the COAP protocol. This could be used for an low-power version if you have a suitable low power display.

## Operation

The first time the board boots it creates a Wifi network with a name similar to _config-iotsa1234_.  Connect a device to that network and visit <http://192.168.4.1>. Configure your device name (using the name _lcd_ is suggested), WiFi name and password, and after reboot the iotsa board should connect to your network and be visible as <http://lcd.local>.

Visit <http://lcd.local/display> to show a message on the display, and/or produce a sound with the beeper.

Visit <http://lcd.local/buttons> to configure the URLs for the buttons. Whenever a button is pressed an _http GET_ request is sent to the corresponding URL.

There is a command-line tool (for Linux or MacOSX) in _extras/lcdecho_ that allows you to show messages and control the other parameters programmatically, use

```
lcdecho --help
```

for help.