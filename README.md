# StrangerSweaters
Stranger Things Light Wall Halloween Costume (In sweater form)

Controls 26 LEDs to light up letters in order

##Usage:

* Connect BLE device to the [Adafruit Bluefruit LE Connect](https://play.google.com/store/apps/details?id=com.adafruit.bluefruit.le.connect&hl=en).

* To send new phrase send "!phrase"

* To send a command send "/(n/f/g/m) [0-9]" where:

  * n controls o**n**-time (in multiples of 125ms)

  * f controls of**f**-time (in multiples of 200ms)

  * g controls **g**ap-time (in multiples of 1000ms)

  * m controls the **m**ode (1-4)




##Hardware:

* [Adafruit Feather 32u4 Bluefruit](https://www.adafruit.com/products/2829)

* [26x RGB Addressable, PTH LEDs](https://www.sparkfun.com/products/12877)

* [Logic Level Converter](https://www.sparkfun.com/products/12009)


##Libraries:

* [WS2812 Library for 8bit](https://github.com/cpldcpu/light_ws2812)

* [Adafruit BLE Library] (https://github.com/adafruit/Adafruit_BluefruitLE_nRF51)


##Pin Assignments:

* Buttons are connected to A0-A4 and short the signal to ground when pressed (uses internal pullup resistors otherwise)

* Power for LEDS is drawn directly from USB

* LED signal pin is 12 (connected through Logic Level Converter)

* LEDs are connected in reverse alphabetical order (where Z is connected first to the Arduino)
