# iotsaDisplayServer - web server to drive an LCD display

iotsaDisplayServer is a web server that drives an LCD display, such as an i2c 4x20 character module. Support for a buzzer (to attract user attention) and buttons (programmable to trigger actions by accessing programmable URLs) is included.

Home page is <https://github.com/cwi-dis/iotsaDisplayServer>.

## Software requirements

* Arduino IDE, v1.6 or later.
* The iotsa framework, download from <https://github.com/cwi-dis/iotsa>.
* The new LiquidCrystal library, download from <https://bitbucket.org/fmalpartida/new-liquidcrystal>.

## Hardware requirements

* an esp8266 board, such as an ESP-12, ESP-201 or iotsa board.
* An i2c LCD module.
* Optionally some pushbuttons and a buzzer.

## Hardware construction

Instructions for constructing the hardware using an ESP-201 board are provided in the _extras_ subfolder:

* [DisplayServer-schematic.pdf](extras/DisplayServer-schematic.pdf) has the schematics.
* [DisplayServer-breadboard.pdf](extras/DisplayServer-breadboard.pdf) shows how to put the bits together on a breadboard. The [Fritzing](http://fritzing.org/home/) project is also available as [DisplayServer-bb.fzz](extras/DisplayServer-bb.fzz).
* [DisplayServer-stripboard.pdf](extras/DisplayServer-stripboard.pdf) shows how to put the bits together on a stripboard. The [Fritzing](http://fritzing.org/home/) project is also available as [DisplayServer-bb.fzz](extras/DisplayServer-print.fzz).

## Building the software

You may need to modify the defines `PIN_ALARM`, `WITH_LCD` and `WITH_BUTTONS` near the top, to reflect which optional hardware support you want.

A bit further down you specify the LCD parameters with `PIN_SDA`, `PIN_SCL`, `LCD_WIDTH` and `LCD_HEIGHT`. Depending on the specific LCD you use you may need to make changes in the following few lines.

About half way down the file you specify the GPIO pins to which buttons have been connected, in the initializer of the `buttons` variable.

Compile, and flash either using an FTDI or (if your esp board supports it) over-the-air.

## Operation

The first time the board boots it creates a Wifi network with a name similar to _config-iotsa1234_.  Connect a device to that network and visit <http://192.168.4.1>. Configure your device name (using the name _lcd_ is suggested), WiFi name and password, and after reboot the iotsa board should connect to your network and be visible as <http://lcd.local>.

Visit <http://lcd.local/display> to show a message on the display, and/or produce a sound with the beeper.

Visit <http://lcd.local/buttons> to configure the URLs for the buttons. Whenever a button is pressed an _http GET_ request is sent to the corresponding URL.

There is a command-line tool (for Linux or MacOSX) in _extras/lcdecho_ that allows you to show messages and control the other parameters programmatically, use

```
lcdecho --help
```

for help.