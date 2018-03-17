#include <WS2812.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include <String.h>

//Control Seetings
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
bool soundenabled = false;
int totalsounds = 10; //0-9

//LED Settings
#define NUM_LEDS 26
#define PIN 3
#define CHIPEN 11 //For enable pin on 5V buffer
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

//Input Pins
#define UP A3
#define DOWN A4
#define CENTER A2
#define LEFT A0
#define RIGHT A1
#define VKNOB A5 // Volume Control
//#define LKNOB 2 //Ran out of analog pins

//Bluefruit Settings
#define MODE_LED_BEHAVIOUR          "MODE"
#define BUFSIZE                        128   // Size of the read buffer for incoming data
#define VERBOSE_MODE                   false  // If set to 'true' enables debug output
#define BLUEFRUIT_SPI_CS               8
#define BLUEFRUIT_SPI_IRQ              7
#define BLUEFRUIT_SPI_RST              4

//Audio Settings
#define VS1053_RESET   -1
#define VS1053_CS       6     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          5     // Card chip select pin
#define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

String bootup = "irajbsktcludmvenwofxpyghzq";

char grid[3][9]= {
  {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', ' '},
  {'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q'},
  {'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'}
};

void setBrightness(double bri);

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_VS1053_FilePlayer soundBoard =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

void setup() {
  Serial.begin( 9600 );
  ble.begin(VERBOSE_MODE);

  // Buttons
  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(UP, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(CENTER, INPUT_PULLUP);
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(VKNOB, INPUT);
  pinMode(13, OUTPUT);

  Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
  ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);

  ble.setMode(BLUEFRUIT_MODE_DATA);
  setBrightness(0.25);
  pinMode(CHIPEN, OUTPUT);
  digitalWrite(CHIPEN, LOW);
  delay(1000);
  pixels.setOutput(PIN);
  pixels.setColorOrderRGB();
  setColor(off);
  char bootstring[NUM_LEDS];
  bootup.toCharArray(bootstring, 27);
  for (int i=0; i<NUM_LEDS; i++){
    lightLetter(bootstring[i], 125);
  }
  delay(ontime);
  setColor(off);
  //Serial.println("test");
  if (!soundBoard.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053"));
     while (1);
  }


  Serial.println(F("VS1053 found"));
  soundBoard.reset();
  soundBoard.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int
  soundBoard.setVolume(10,10);

  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    soundenabled = false;
    pulseLetter('n', 250);
  } else {
    digitalWrite(13,LOW);
    Serial.println("SD OK!");
    if(SD.exists("Light000.mp3")){
      digitalWrite(13, HIGH);
      soundenabled = true;
      pulseLetter('y', 250);
    } else {
      digitalWrite(13, HIGH);
      soundenabled = false;
      pulseLetter('m', 250);
    }

  }
  digitalWrite(13, LOW);
  randomSeed(analogRead(VKNOB));

}

void loop() {
  noInterrupts(); //Prevent soundboard from interfering
  if (! ble.isConnected()) {
      //delay(500);
  } else {
    while (ble.available()) {
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
            //Command issued
            md = true;
          } else if (c=='!') {
            say  = true;
            if (!(mode == 1) && !(mode == 2)){
              mode = 1;

            }
          } else {
            ble.println("nope");
          }
          o = false;
        } else {
          if (c == '\n') {
            //Load phrase
            if (say) {
              ble.println("Got it");
              phrasesize = phrasesizet;
              phrasen = 0;
            }
            break;
          }
          if (md) {
            //Load command
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
          // Time LED is on in ms
          ontime = ctl*125;
          sprintf(printline, "ontime: %i", ontime);
          break;

          case 'f':
          // Time LED is off in ms
          offtime = ctl*200;
          sprintf(printline, "offtime: %i", offtime);
          break;

          case 'g':
          // Time between cycle repeats
          gaptime = ctl*1000;
          sprintf(printline, "gaptime: %i", gaptime);
          break;

          case 'm':
          // Mode Selection
          ctl=ctl-1;
          mode = ctl%5;
          sprintf(printline, "mode: %i", mode);
          break;

          case 's':
          // Sound Control
          ctl=ctl-1;
          if (ctl==1) {
            soundenabled = true;
          } else {
            soundenabled = false;
          }
          sprintf(printline, "sound: %i", soundenabled);
          break;

          default:
          sprintf(printline, "ERROR");
          break;
          }
        }

        ble.println(printline);
      }
    }
  }
  interrupts();


  bool lr[5];
  bool tr[5];

  lr[0] = true;
  lr[1] = true;
  lr[2] = true;
  lr[3] = true;
  lr[4] = true;

  long unsigned int starttime = millis();
  String pt = "";
  digitalWrite(13, HIGH);
  bool firstbutton = false;
  while ((millis()-starttime) < gaptime){
    // Poor man's interrupts (not interrupt capable pins)
    tr[0] = digitalRead(CENTER); // Mode Swicth
    tr[1] = digitalRead(UP); // Repeat/Startnow
    tr[2] = digitalRead(DOWN); // ON/OFF
    tr[3] = digitalRead(RIGHT); // Phrase Select
    tr[4] = digitalRead(LEFT); // Sound Control

    if(!tr[0] && lr[0]){
      // Rotate mode
      mode = (mode + 1) % 5;
      pulseLetter('a'+ mode, 250);
      delay(100);
      starttime = millis();
    }
    if(!tr[1] && lr[1]){
      //Repeat and Start now
      if (mode == 0) {
        mode = lm;
      }
      // Start Now
      first = true;
      break;
    }
    if(!tr[2] && lr[2]){
      // Turn off
      if (mode == 0) {
        mode = lm;
        first = true;
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
          pt = "hello";
          break;
        case 2:
        //k
          pt = "boo";
          break;
        case 3:
        //l
          pt = "r u n";
          break;
        case 4:
        //m
          pt = "right here ";
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
          pt = "barb";
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
      // Toggle sound
      soundenabled = !soundenabled;
      pulseLetter('y'+ soundenabled, 250);
      delay(100);
      starttime = millis();

    }
    if (firstbutton && !(tr[1] && tr[2] && tr[3] && tr[4] && tr[5])){
      firstbutton = false;
      randomSeed(millis());
    }
    for (int i=0; i<5; i++){
      // Poor man's shift register
      lr[i] = tr[i];
    }
  }
  digitalWrite(13, LOW);
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
        pulsePhrase(phrase);
      }
      first = false;
      break;
    case 2:
      //Repeat with period
      pulsePhrase(phrase);
      break;
    case 3:
      //Random pulsing
      pulseLetter(random(26) +'a', 500);
      break;
    case 4:
      //Trace lines across sweater in random directions
      {
        int startr = random(3);
        int endr = random(3);
        int dir = random(2);
        int startc = dir*8;
        int endc = (1-dir)*8;

        if (startr == 0 && dir == 1){
          startc = 7;
        } else if (endr == 0 && dir == 0){
          endc = 7;
        }

        int curc = startc;
        int curr = startr;

        while (!((curc==endc) && (curr==endr))){
          lightLetter(grid[curr][curc], ontime+offtime);
          if (curc > endc) {
            curc = curc-1;
          } else if (curc < endc) {
            curc = curc+1;
          }
          if (curr != endr) {
            if (abs((endr-curr))>random(abs(endc-curc))){
              if (curr > endr){
                curr = curr - 1;
              } else {
                curr = curr + 1;
              }
            }
          }
        }

        lightLetter(grid[curr][curc], ontime);
        delay(offtime);
        setColor(off);
        break;
      }
    default:

      break;
  }
  lastcase = mode;
}

bool setColor(cRGB color){
  // Sets entire grid to one color
  for (int i = 0; i<NUM_LEDS; i++) {
    pixels.set_crgb_at(i, color);
  }
  pixels.sync();
  return true;
}

bool pulseLetter(char letter, int duration) {
  //Turns LED on for set duration and then turns it back off
  int index = letterToIndex(letter);

  if (index != -1) {
    if(soundenabled){
      lightSound(index);
    }
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
  // Controls rate at which led turns on/off
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
    delay(5);
    t = millis();
  }
  pixels.set_crgb_at(25-index, e);
  pixels.sync();
}

bool pulsePhrase(char letters[]){
  // Has shirt display a phrase one letter at a time
  int i = 0;
  while (letters[i] !=0) {
    pulseLetter(letters[i], ontime);
    delay(offtime);
    i++;
  }
}

bool lightLetter(char letter, int duration) {
  // Lights up letter for set duration, but does not turn it back off
  // Must use setColor(off) to undo.
  int index = letterToIndex(letter);
  if (index != -1) {
    if(soundenabled){
      lightSound(index);
    }
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
  // Calculate light index from letter
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

void lightSound(int index){
  // Set volume and sound file to use
  char filename [14];
  int fileno = random(10);
  sprintf(filename, "Light%03d.mp3", fileno);

  //Set total volume
  int rsetting = analogRead(VKNOB)/12; //Maxread is 1023
  int lsetting = rsetting + 4; // Sound balance

  // Make sound appear to be coming from the LED via stereo speakers
  int maxbalance = 10;
  int balance = ((25-index) % 9)*maxbalance/8;
  uint8_t lvol = lsetting + balance;
  uint8_t rvol = maxbalance - balance + rsetting;
  soundBoard.setVolume(lvol, rvol);
  soundBoard.startPlayingFile(filename);
}

void setBrightness(double br){
  // Sets max brightness of all LEDs
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
