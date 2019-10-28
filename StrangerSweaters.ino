// For Sweater
//#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>
#include <String.h>

// For Wifi
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "ConnectionInfo.h"

//#define CONFIG_MAIN_TASK_STACK_SIZE 10000
// For BLE
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

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
const int pagebufferlen = 4096;

//LED Settings
#define NUM_LEDS 26
#define PIN  12 
#define CHIPEN 27 //For enable pin on 5V buffer
CRGB yellow;
CRGB blue;
CRGB purple;
CRGB green;
CRGB orange;
CRGB pink;
CRGB red;
CRGB off;
CRGB pixels[NUM_LEDS];
CRGB colors[NUM_LEDS];

//Input Pins
#define UP A3
#define DOWN A4
#define CENTER A2
#define LEFT A0
#define RIGHT A1
#define VKNOB A5 // Volume Control
//#define LKNOB A10 //Ran out of analog pins

//Audio Settings
#define VS1053_RESET   -1
#define VS1053_CS      32     // VS1053 chip select pin (output)
#define VS1053_DCS     33     // VS1053 Data/command select pin (output)
#define CARDCS         14     // Card chip select pin
#define VS1053_DREQ    15     // VS1053 Data request, ideally an Interrupt pin

String bootup = "irajbsktcludmvenwofxpyghzq";

char grid[3][9]= {
  {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', ' '},
  {'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q'},
  {'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'}
};

void setBrightness(double bri);

WebServer server ( 80 );
BluetoothSerial ble;
Adafruit_VS1053_FilePlayer soundBoard =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);
TaskHandle_t sweaterTask_h;
TaskHandle_t serverTask_h;

void setup() {  
  Serial.begin( 115200 );
  WiFi.begin ( ssid, password );
  MDNS.begin ( "strangersweaters" );
  server.on ( "/", phraseInput );
  server.on ( "/advanced.cgi", advancedInput );
  server.begin();

  int ct = 0;
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
    ct += 1;
    if (ct > 6) break;
  }
  
  if (ct < 7){
    Serial.println ( "" );
    Serial.print ( "Connected to " );
    Serial.println ( ssid );
    Serial.print ( "IP address: " );
    Serial.println ( WiFi.localIP() );
  } else {
    Serial.println ("No Wifi");
  }
  ble.begin("Will Byer's iPhone");
  
  // Buttons
  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(UP, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(CENTER, INPUT_PULLUP);
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(VKNOB, INPUT);
  pinMode(13, OUTPUT);

  setBrightness(1);
  pinMode(CHIPEN, OUTPUT);
  digitalWrite(CHIPEN, LOW);
  FastLED.addLeds<WS2812B, PIN, RGB>(pixels, NUM_LEDS).setCorrection(TypicalLEDStrip);
  delay(1000);
  setColor(off);
  char bootstring[NUM_LEDS];
  bootup.toCharArray(bootstring, 27);
  for (int i=0; i<NUM_LEDS; i++){
    lightLetter(bootstring[i], 125);
  }
  Serial.println();
  delay(ontime);
  setColor(off);


  if (!soundBoard.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053"));
     soundenabled = false;
  } else {
    Serial.println(F("VS1053 found"));
    soundBoard.reset();
    soundBoard.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int
    soundBoard.setVolume(10,10);
  }
  if(true){    
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
  }
  
  
  Serial.println();
  digitalWrite(13, LOW);
  randomSeed(analogRead(VKNOB));
  
  xTaskCreatePinnedToCore(
                    sweaterTask,          /* Task function. */
                    "sweaterTask",        /* String with name of task. */
                    10000,            /* Stack size in words. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    &sweaterTask_h,
                    1);            /* Task handle. */
 
  xTaskCreatePinnedToCore(
                    serverTask,          /* Task function. */
                    "serverTask",        /* String with name of task. */
                    10000,            /* Stack size in words. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    &serverTask_h,
                    0);            /* Task handle. */
}


void loop() {
  vTaskDelay(10);
}

/* ---------------------------- Sweater Control Code ---------------------------- */ 

void sweaterTask(void* param){
  while(1){
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
            for (int i=0; i<80; i++){
              phrase[i] = 0;
            }
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
      tr[2] = digitalRead(DOWN); // ON/fOFF
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
      for (int i=0; i<80; i++){
        phrase[i] = 0;
      }
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
        Serial.println();
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
          Serial.println();
          break;
        }
      default:
  
        break;
    }
    lastcase = mode;
  }
  vTaskDelete( NULL );
}

bool setColor(CRGB color){
  // Sets entire grid to one color
  for (int i = 0; i<NUM_LEDS; i++) {
    pixels[i] = color;
  }
  FastLED.show();
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
    delay(duration/2);
    ramp(index, colors[index], off, duration/4);
    return true;
  } else {
    delay(duration);
    return false;
  }
}

bool ramp(int index, const CRGB s, const CRGB e, int dur){
  // Controls rate at which led turns on/off
  pixels[25-index] = s;
  FastLED.show();
  CRGB res;
  int t_init = millis();
  int t = millis();
  while ((t-t_init) < dur){
    uint8_t prog = 255*(t-t_init)/dur;
    pixels[25-index] = s.lerp8(e, prog);
    FastLED.show();
    delay(5);
    t = millis();
  }
  pixels[25-index] = e;
  FastLED.show();
}

bool pulsePhrase(char letters[]){
  // Has shirt display a phrase one letter at a time
  int i = 0;
  while (letters[i] !=0) {
    pulseLetter(letters[i], ontime);
    delay(offtime);
    i++;
  }
  Serial.println();
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
    delay(duration/2);
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
  yellow =  CRGB((int)255*br, (int)255*br,  (int)32*br);
  blue =    CRGB((int)0*br,   (int)0*br,    (int)255*br);
  purple =  CRGB((int)170*br, (int)0*br,    (int)255*br);
  green =   CRGB((int)0*br,   (int)255*br,  (int)0*br);
  orange =  CRGB((int)255*br, (int)170*br,  (int)0*br);
  pink =    CRGB((int)255*br, (int)128*br,  (int)128*br);
  red =     CRGB((int)255*br, (int)0*br,    (int)0*br);
  off =     CRGB(0,           0,            0);
  CRGB colors2[NUM_LEDS] = {yellow, blue, purple, green, blue, orange, pink, blue, green, pink, blue, green, orange, pink, purple, green, red, green, yellow, orange, blue, pink, blue, orange, pink, red};
  memcpy(colors, colors2, NUM_LEDS*3);
}

/* ---------------------------- Webpage Input Code ---------------------------- */

void serverTask(void* param){
  while(1){
    server.handleClient();
  }
  vTaskDelete( NULL );
}

void advancedInput(){
  String message;
  String drop1;
  String drop2;
  String drop3;
  char tmp[pagebufferlen];
  char s[32];
  char selected[16] = " selected>";
  char deselected[2] = ">";
  if (server.method() == HTTP_POST){
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if(server.argName(i) == "ontime"){
        ontime = server.arg(i).toInt();
      } else if (server.argName(i) == "offtime"){
        offtime = server.arg(i).toInt();
      } else if (server.argName(i) == "gaptime"){
        gaptime = server.arg(i).toInt();
      } else if (server.argName(i) == "mode"){
        mode = server.arg(i).toInt();
      }
    }  
  }

  sprintf(tmp,  
"<html>\n\
  <body>\n\
    <form name=\"myform\" action=\"advanced.cgi\" method=\"POST\">\n\
      <select name=\"mode\">\n"
  );

  message = String(tmp);

  for (int i=0; i<5; i++){
    char* sel = (mode == i)? selected : deselected;
    switch (i){
      case 0:
        sprintf(s, "Off");
        break;
      case 1:
        sprintf(s, "Play Once");
        break;
      case 2:
        sprintf(s, "Play Repeatedly");
        break;
      case 3:
        sprintf(s, "Random Letters");
        break;
      case 4:
        sprintf(s, "Random Rows");
        break;
    }
    sprintf(tmp, "        <option value=%i%s%s</option>\n", i, sel, s);
    message += String(tmp);
  }
  message += "      </select>\n";
  
  drop1 = "      <select name=\"ontime\">\n";
  drop2 = "      <select name=\"offtime\">\n";
  drop3 = "      <select name=\"gaptime\">\n";
  char option[400] = "        <option value=%d%s%d</option>\n";
  
  for (int i=0; i<10; i++){
    int val1 = (i+1)*125;
    char tmp1[400]; 
    char* sel1 = (val1 == ontime)? selected : deselected;
    sprintf(tmp1, option, val1, sel1, val1);
    drop1 += String(tmp1);
  
    int val2 = (i+1)*200;
    char tmp2[400];
    char* sel2 = (val2 == offtime)? selected : deselected;
    sprintf(tmp2, option, val2, sel2, val2);
    drop2 += String(tmp2);
     
    int val3 = (i+1)*1000;
    char tmp3[400];
    char* sel3 = (val3 == gaptime)? selected : deselected;
    sprintf(tmp3, option, val3, sel3, val3);
    drop3 += String(tmp3);
  }
  
  drop1 += "      </select>\n";
  drop2 += "      </select>\n";
  drop3 += "      </select>\n";
  message += drop1 + drop2 + drop3;
  
  message += "      <br><input type=\"submit\" value=\"Select Options\"><br>\n\
    </form>\n";
  message += "  </body>\n</html>";

  Serial.println(message.length());
  
  server.send ( 200, "text/html", message ); 
  Serial.println("I did it");
}
void handleRoot(){
  char temp[pagebufferlen];
  int l = snprintf ( temp, pagebufferlen,
"<html>\
  <body>\
    sup\
  </body>\
</html>"
  );
  server.send ( 200, "text/html", temp ); 
}

void phraseInput() {
  char temp[pagebufferlen];
  int sec = millis() / 1000;  
  int min = sec / 60;
  int hr = min / 60;
  if (server.method() == HTTP_POST){
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if(server.argName(i) == "phrase"){
        for (int i=0; i<80; i++){
          phrase[i] = 0;
        }
        server.arg(i).toCharArray(phrase, 80);
        Serial.println(server.arg(i));
        first = true;
        if (!(mode == 1) && !(mode == 2)){
          mode = 1;
        }
      }
    }  
  }
  
  int l = snprintf ( temp, pagebufferlen,
"<html>\
  <body>\
    <form name=\"myform\" action=\"/\" method=\"POST\">\
      <div align=\"left\">\
      <br><br>\
      <input name =\"phrase\" type=\"text\" size=\"25\" value=\"%s\">\
      <br><input type=\"submit\" value=\"Submit phrase\"><br>\
      </div>\
    </form>\
  </body>\
</html>",
  phrase);
  Serial.println(phrase);
  server.send ( 200, "text/html", temp );  
}
