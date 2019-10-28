# StrangerSweaters
Stranger Things Light Wall Halloween Costume (In sweater form)

Controls 26 LEDs to light up letters in order, sound effects are also played to match the lighting of the letters.

Has now been mounted on the wall, and been turned into an IoT device.

See branch 32u4-ble for a bluetooth only version.

##Usage:

* Connect to ESP32 via WiFi

  * To change the phrase go to "/phrase.cgi"

  * To change advanced options (timing, etc) go to "/advanced.cgi"

* Connect BLE device to your phone (Not sure what app to use now).

  * To send new phrase send "!phrase"

  * To send a command send "/(n/f/g/m) [0-9]" where:

    * n controls o**n**-time (in multiples of 125ms)

    * f controls of**f**-time (in multiples of 200ms)

    * g controls **g**ap-time (in multiples of 1000ms)

    * m controls the **m**ode (1-4)

    * s contols the **s**ound (0 is off, 1 is on)

* Use buttons to cycle through modes, pre-select phrases, reset settings, and turn off the device




##Hardware:

* [Adafruit Feather ESP32](https://www.adafruit.com/product/3405)

* [26x RGB Addressable, PTH LEDs](https://www.sparkfun.com/products/12877)

* [Logic Level Converter](https://www.sparkfun.com/products/12009)
    * Alternatively use [MC74HCT245](https://www.arrow.com/en/products/mc74hct245ang/on-semiconductor)

* [Music Maker Featherwing](https://www.adafruit.com/product/3436)

* [2x Small Speakers](https://www.arrow.com/en/products/cds-25148/cui-inc)

* [10k Potentiometer](https://www.arrow.com/en/products/rk09k1130ap5/alps-electric)
##Libraries:

* [FastLED (samguyer's fork for now)](https://github.com/samguyer/FastLED)

* [WebServer_tng](https://github.com/bbx10/WebServer_tng)

* [Adafruit VS1053](https://github.com/adafruit/Adafruit_VS1053_Library)

* Be sure to include ConnectionInfo.h which looks something like:
```
    const char *ssid = "wifi_name";
    const char *password = "hunter2";
```

##Pin Assignments:

* Buttons are connected to A0-A4 and short the signal to ground when pressed (ESP32 needs external pullup resistors for some buttons)

* Power for LEDS is drawn directly from USB

* LED signal pin is 12 (connected through Logic Level Converter or Buffer)

* LEDs are connected in reverse alphabetical order (where Z is connected first to the Arduino)

##Current Notes:

* Sound does not work currently, something to do with the SD card.

* The sketch fills 98% of the memory on the ESP32, so some optimization is being worked on.

* mDNS does not work due to lack of space.

* Bluetooth Serial is untested but should work.
