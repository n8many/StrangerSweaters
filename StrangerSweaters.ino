#include <SPI.h>
#include <WS2812.h>
//#include <FastLED.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "BluefruitConfig.h"
#include <String.h>

#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"
#define NUM_LEDS 26
#define PIN 12

int ontime = 250;
int offtime = 400;
int gaptime = 1000;
int mode = 1;
int lm = 1;
int lastcase = 0;
int phrasen = 0;
bool first = false;
int phrasesize = 0;
char phrase[80];
//Indexed by letter*/


//Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_RGB + NEO_KHZ800);

WS2812 pixels(NUM_LEDS);
cRGB yellow;
cRGB blue;
cRGB purple;
cRGB green;
cRGB orange;
cRGB pink;
cRGB red;
cRGB off;
cRGB colors[NUM_LEDS];




String bootup = "irajbsktcludmvenwofxpyghzq";
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

void setup() {
  // put your setup code here, to run once
  Serial.begin( 9600 );
  ble.begin(VERBOSE_MODE);
  ble.verbose(false);
  // buttons

  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  pinMode(A4, INPUT_PULLUP);


  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }
  ble.setMode(BLUEFRUIT_MODE_DATA);
  setBrightness(0.25);
  //Serial1.attachRts(3);
  pinMode(13, OUTPUT);
  setColor(off);
  delay(1000);
  digitalWrite(13, HIGH);
  pixels.setOutput(12);
  pixels.setColorOrderRGB();
  digitalWrite(13, LOW);
  Serial.println("Three");
  digitalWrite(13, HIGH);
  char bootstring[NUM_LEDS];
  bootup.toCharArray(bootstring, 27);
  for (int i=0; i<NUM_LEDS; i++){
    lightLetter(bootstring[i], 125);
  }
  delay(ontime);
  setColor(off);
  digitalWrite(13, LOW);

  //Serial.println("test");

}

void loop() {
  /*char text[80];
  for (int i=0; i <80; i++){

  }*/
  if (! ble.isConnected()) {
      //delay(500);
  } else {
    if (ble.available()) {
      first = true;
      int phrasesizet = 0;
      char cmd[80];
      int cmdsize = 0;
      bool md = false;
      bool say = false;
      bool o = true;
      while ( ble.available() )
      {
        char c = ble.read();
        Serial.print(c);
        if (o) {
          if (c == '/'){
            md = true;
          } else if (c=='!') {
            say  = true;
            if ((mode == 0) || (mode == 1)){
              mode = 1;
            }
          } else {
            ble.println("nope");
          }
          o = false;
        } else {
          if (c == '\n') {
            if (say) {
              ble.println("Got it");
              phrasesize = phrasesizet;
              phrasen = 0;
            }
            break;
          }
          if (md) {
            cmd[cmdsize] = c;
            cmdsize ++;
          } else if (say) {
            if  (phrasesizet < 80) {
              phrase[phrasesizet] = c;
              phrasesizet++;
            }
          }
        }
      }
      if (md) {
        char printline[80] = "error";
        int ctl = cmd[1] - '0' + 1;
        if ((ctl < 11) && (ctl > 0)) {
          switch (cmd[0]){
          case 'n':
          ontime = ctl*125;
          sprintf(printline, "ontime: %i", ontime);
          break;

          case 'f':
          offtime = ctl*200;
          sprintf(printline, "offtime: %i", offtime);
          break;

          case 'g':
          gaptime = ctl*1000;
          sprintf(printline, "gaptime: %i", gaptime);
          break;

          case 'm':
          ctl=ctl-1;
          mode = ctl%4
          sprintf(printline, "mode: %i", ctl);
          break;
          default:
          sprintf(printline, "ERROR");
          break;
          }
        }

        ble.println(printline);
        //interpret cmd
      }
  }

  }

  bool lr[5];
  bool tr[5];

  lr[0] = digitalRead(A0);
  lr[1] = digitalRead(A1);
  lr[2] = digitalRead(A2);
  lr[3] = digitalRead(A3);
  lr[4] = digitalRead(A4);

  long unsigned int starttime = millis();
  String pt = "";

  while ((millis()-starttime) < gaptime){
    // Poor man's interrupts (not interrupt capable pins)
    tr[0] = digitalRead(A0);
    tr[1] = digitalRead(A1);
    tr[2] = digitalRead(A2);
    tr[3] = digitalRead(A3);
    tr[4] = digitalRead(A4);

    if(!tr[0] && lr[0]){
      // Rotate mode
      mode = (mode + 1) % 4;
      pulseLetter('a'+ mode, 250);
      delay(100);
      starttime = millis();
    }
    if(!tr[1] && lr[1]){
      // Say again
      first = true;
      mode = 1;
      delay(100);
      starttime = millis();
    }
    if(!tr[2] && lr[2]){
      // Turn off
      if (mode == 0) {
        mode = lm;
      } else {
        lm = mode;
        mode = 0;
      }
      pulseLetter('a'+ mode, 250);
      delay(100);
      starttime = millis();
    }
    if(!tr[3] && lr[3]){
      // Set phrase to... light corresponding letter to show current mode
      phrasen++;
      phrasen = phrasen % 9;
      switch(phrasen) {
        case 0:
        //i
          pt = "";
          break;
        case 1:
        //j
          pt = "hey";

          break;
        case 2:
        //k
          pt = "goodbye";
          break;
        case 3:
        //l
          pt = "im here";
          break;
        case 4:
        //m
          pt = "r u n";

          break;
        case 5:
        //n
          pt = "thanks";

          break;
        case 6:
        //o
          pt = "i see you";

          break;
        case 7:
        //p
          pt = "nice costume";
          break;
        case 8:
        //q
          pt = "happy halloween";
          break;
        default:
          pt = "";
          break;
      }

      pulseLetter('i' + phrasen, 250);
      delay(100);
      starttime = millis();
    }
    if(!tr[4] && lr[4]){
      // Reset other settings
      mode = 1;
      first == true;
      pulseLetter('z', 250);

    }

    for (int i=0; i<5; i++){
      // Poor man's shift register
      lr[i] = tr[i];
    }
  }

  if (mode != lastcase) {
    // Reset lights (just in case something goes weird)
    setColor(off);
  }

  delay(100);
  // Input new phrase if phrase is selected by button menu
  if (pt.length()>0){
    phrasesize = pt.length();
    for (int i = 0; i < phrasesize; i++) {
      phrase[i] = pt[i];
    }
    first = true;
  }

  switch (mode) {
    case 0:
      //Off
      break;
    case 1:
      //Say once
      if (first) {
        for (int i=0; i < phrasesize; i++){
          pulseLetter(phrase[i], ontime);
          delay(offtime);
        }
        Serial.println('.');
      }
      first = false;
      break;
    case 2:
      //Repeat with period
      for (int i=0; i < phrasesize; i++){
        pulseLetter(phrase[i], ontime);
        delay(offtime);
      }
      Serial.println('=');
      break;
    case 3:
      //Random pulsing
      pulseLetter(((((25*millis())%12+21))*14)%26 +'a', 500);
      Serial.println('?');
      break;
    default:

      break;
  }
  lastcase = mode;

//  delay(2000);
//  pulseLetter(719*millis()%26 + 'a', 500);
  //setColor(blue);

}

bool setColor(cRGB color){
  for (int i = 0; i<NUM_LEDS; i++) {
    pixels.set_crgb_at(i, color);
  }
  pixels.sync();
  return true;
}

bool pulseLetter(char letter, int duration) {
  int index = letterToIndex(letter);
  if (index < 26) {
    Serial.print(letter);
    ramp(index, off, colors[index], duration/4);
    pixels.set_crgb_at(25-index, colors[index]);
    pixels.sync();
    delay(duration/2);
    ramp(index, colors[index], off, duration/4);
    pixels.set_crgb_at(25-index, off);
    pixels.sync();
    return true;
  } else {
    delay(duration);
    return false;
  }
}

bool ramp(int index, cRGB s, cRGB e, int dur){
  pixels.set_crgb_at(25-index, s);
  pixels.sync();
  cRGB res;
  int t_init = millis();
  int t = millis();
  while ((t-t_init) < dur){
    res.r = (byte)(s.r*(1-(t-t_init)*1.0/dur) + e.r*(t-t_init)*1.0/dur);
    res.g = (byte)(s.g*(1-(t-t_init)*1.0/dur) + e.g*(t-t_init)*1.0/dur);
    res.b = (byte)(s.b*(1-(t-t_init)*1.0/dur) + e.b*(t-t_init)*1.0/dur);
    pixels.set_crgb_at(25-index, res);
    pixels.sync();
    t = millis();
  }
  pixels.set_crgb_at(25-index, e);
  pixels.sync();

}

bool pulsePhrase(char letters[]){
  int i = 0;
  while (letters[i] !=0) {
    pulseLetter(letters[i], ontime);
    delay(offtime);
    i++;
  }
}

bool lightLetter(char letter, int duration) {
  int index = letterToIndex(letter);
  if (index != -1) {
    Serial.print(letter);
    ramp(index, off, colors[index], duration/2);
    pixels.set_crgb_at(25-index, colors[index]);
    pixels.sync();
    delay(duration*3/4);
    return true;
  } else {
    delay(duration);
    return false;
  }
}

int letterToIndex(char letter){
  unsigned int index = letter-'A';
  if (index > 31){
    index = index -32;
  }
  if (index < 26) {
    return (int)index;
  } else {
    return -1;
  }
}

cRGB dim(cRGB color, double ratio){
  cRGB result;
  result.r = (byte)color.r*ratio;
  result.g = (byte)color.g*ratio;
  result.b = (byte)color.b*ratio;

  return result;
}

void setBrightness(double br){
  yellow.r=(int)255*br; yellow.g=(int)255*br; yellow.b=(int)32*br;
  blue.r=(int)0*br;     blue.g=(int)0*br;     blue.b=(int)255*br;
  purple.r=(int)170*br; purple.g=(int)0*br;   purple.b=(int)255*br;
  green.r=(int)0*br;    green.g=(int)255*br;  green.b=(int)0*br;
  orange.r=(int)255*br; orange.g=(int)170*br; orange.b=(int)0*br;
  pink.r=(int)255*br;   pink.g=(int)128*br;   pink.b=(int)128*br;
  red.r=(int)255*br;    red.g=(int)0*br;      red.b=(int)0*br;
  off.r=0;              off.g=0;              off.b=0;
  cRGB colors2[NUM_LEDS] = {yellow, blue, purple, green, blue, orange, pink ,blue, green, pink, blue, green, orange, pink, purple, green, red,green, yellow, orange, blue, pink, blue, orange, pink, red};
  memcpy(colors, colors2, NUM_LEDS*3);
}

bool setMode(char mode){

}
