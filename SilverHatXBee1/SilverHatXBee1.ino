//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Siler Hat! - 35 Lights. This version does not use a keypad
//
//  Hooked up an xBee to communicate color, timing, and show information
//
//  Current approach treats the Hedgehog as the master
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights. Reversed from usual!
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 35      // number of spikes on the hat

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Silver hat colors

int color1  = 85;   // Starting foreground color - Global!
int color2 = 170;  // Starting background color - Global!
#define MaxColor 255    // Colors go from 0 to 255

// Shows

int show=1;    // Show and number of initial show
int wait=6;    // index to keep track of delay time
#define maxWait 12     // Number of stored delay times 

// Timing variables for xBee communication

unsigned long OldTime;
unsigned long NewTime;

#define MAX_AWAY 20     // Slave goes into Alone mode if no signal in this #sec
#define SYNC_FREQ 500   // How often in msec that the master sends out data
#define NO_DATA 9999

//Depending on the unit, we need to run slightly different code
boolean I_AM_MASTER = true; //Set to true if this code is meant for a master unit
boolean Alone = true; // Flag for slave; turns false if master sends information

#define NO_CHANGE    0  // Slave has exact same status as Master
#define MINOR_CHANGE 1  // Colors have changed
#define MAJOR_CHANGE 2  // Show has changed
  
// Order of spines
byte SpineOrder[35] = {
        10,
   11,  20,   9,
     19,   21,
  12,18,23,22,8,
  13,17,24,25,7,
  14,16,27,26,6,
     15,28,5,
        29,
       30,4,
       31,3,
       32,2,
       33,1,
       34,0,
};

// Tail pattern
byte Tails[35] = {
         0,
    0,   0,   0,
      0,    0,
   0, 0, 0, 0,0,
   0, 0, 0, 0,0,
   0, 0, 0, 0,0,
      0, 0, 0,
        1,
       2,2,
       3,3,
       4,4,
       5,5,
       6,6,
};

// Hood Ring
byte Hood[35] = {
         1,
    2,   0,   2,
      0,    0,
   3, 0, 0, 0,3,
   4, 0, 0, 0,4,
   5, 0, 0, 0,5,
      6, 0, 6,
         0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
};

// Stripe Patten
byte Stripes[35] = {
         1,
    0,   2,   0,
      0,    0,
   0, 0, 3, 0,0,
   0, 0, 4, 0,0,
   0, 0, 5, 0,0,
      0, 6, 0,
         7,
        8,8,
        9,9,
        10,10,
        11,11,
        12,12,
};

void setup() {
  // xBee communication
  
  OldTime = 0;
  NewTime = 0;
  
  Serial.begin(9600);
  
  //not sure if this is necessary but try it anyway
  randomSeed(analogRead(0));
  
  for (int i=0; i < numLights; i++) current_frame[i] = 0; // clear display
  
  // Start up the LED counter
  strip.begin();

  // Update the strip, to start they are all 'off'
  clear();
  strip.show();
}

void loop() {
   
  // Random show if Master or Slave in Alone mode
  if (I_AM_MASTER || Alone) show = random(1,14);  // Show 0 is dark
  
  switch(show) {
    case 0:
      clearwithoutfade(random(1,4)*10000);
    case 1:
      allOn(random(20,100));
      break;
    case 2:
      randomfill(random(3,10)); 
      break;
    case 3:
      randomcolors(random(5,30)*10000);
      break;
    case 4:
      twocolor(random(1,30)*10000);
      break;
    case 5:
      rainbowshow(random(100,500));
      break;
    case 6:
      lightwave(random(2,10));
      break;
    case 7:
      lighttails(random(1,30)*10000);
      break;
    case 8:
      lighthood(random(1,30)*10000);
      break;
    case 9:
      centergradient(random(1,30)*10000);
      break;
    case 10:
      centerpulse(random(5,40));
      break;
    default:
      allOn(random(20,100));
      break;
  }
  // Then a break with all the lights off
  if (I_AM_MASTER || Alone)  {
    show = 0;
    clearwithoutfade(random(1,4)*10000);  // Hopefully 10-1200 seconds
  }
}

/*
 * set all cells to black but don't call show yet
 * ignores buffering
 */
void clear() {
  for (int i=0; i<numLights; i++) {
    strip.setPixelColor(i, Color(0,0,0));
    setPixelColor(i, Color(0,0,0));
  }
}

/*
 * set all cells to black and calls the show
 * no morphing
 */
void clearwithoutfade(int darktime) {
  clear();
  strip.show();
  talkdelay(darktime);
}

/*
 * Fades lights to black over fade time
 */
void clearWithFade(int fadeTime) {
  for (int i=0; i<numLights; i++) {
    setPixelColor(i, Color(0,0,0));
  }
  morph_frame(fadeTime);
}

/*
 * All On
 *
 * Simply turns all the pixes on to a random color and then cycles
 *
 */
 
void allOn(int cycles) {
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      setPixelColor(i, Wheel(color1));
    }
    if(morph_frame(GetDelayTime(wait))==MAJOR_CHANGE) return;  // Slave must recalibrate
  
    if (!random(100)) { if (wait++ > maxWait) wait = 0; }
    if (color1+=5 > MaxColor) color1 = color1 - MaxColor;
  }
  clearWithFade(1000);
}

/*
 * randomcolors: turns each pixel on to a random color
 */

void randomcolors(long time) {
  for (int i=0; i < numLights; i++) setPixelColor(i, Wheel(random(MaxColor)));
  if(morph_frame(GetDelayTime(wait))==MAJOR_CHANGE) return;  // Slave must recalibrate
  talkdelay(time);
  clearWithFade(1000);
}

/*
 * twocolor: turns each pixel on to either of two random colors
 */

void twocolor(long time) {
  
  for (int i=0; i < numLights; i++) {
    if (random(2)) { setPixelColor(i, Wheel(color1)); }
    else { setPixelColor(i, Wheel(color2)); }
  }
  if(morph_frame(1000)==MAJOR_CHANGE) return;  // Slave must recalibrate
  talkdelay(time);
  clearWithFade(1000);
}

/*
 * lighttails: turn on the tail lights to a gradient
 */
 
void lighttails(long time) {
  int spacing = random(2,20);   // Spacing between colors
  
  for (long t=time; t=t-500; t>0) {
    for (int i=0; i < numLights; i++) {
     if (!Tails[i]) { strip.setPixelColor(SpineOrder[i], Color(0,0,0)); }
     else { strip.setPixelColor(SpineOrder[i],
       Wheel((color1 + (spacing*Tails[i]) % MaxColor))); }
    }
    strip.show();
    if(talkdelay(500) == MAJOR_CHANGE) return;
    if (!random(5)) color1 = (color1 + random(5,20)) % MaxColor;
  }
}

/*
 * lighthood: ring the hood with one color; fill everything else with another color
 */
 
void lighthood(long time) {
  int spacing = random(2,20);   // Spacing between colors
  
  for (long t=time; t=t-500; t>0) {
    for (int i=0; i < numLights; i++) {
     if (!Hood[i]) { strip.setPixelColor(SpineOrder[i], Wheel(color1)); }
     else { strip.setPixelColor(SpineOrder[i],
       Wheel((color2 + (spacing*Hood[i]) % MaxColor))); }
    }
    strip.show();
    if(talkdelay(500) == MAJOR_CHANGE) return;
    if (!random(5)) color1 = (color1 + random(5,20)) % MaxColor;
  }
}

/*
 * centergradient: turn on the central spine with a light gradient
 */
 
void centergradient(long time) {
  int spacing = random(2,6);   // Spacing between colors
  
  for (long t=time; t=t-500; t>0) {
    for (int i=0; i < numLights; i++) {
     if (!Stripes[i]) { strip.setPixelColor(SpineOrder[i], Color(0,0,0)); }
     else { strip.setPixelColor(SpineOrder[i],
       Wheel((color1 + (spacing*Stripes[i]) % MaxColor))); }
    }
    strip.show();
    if(talkdelay(500) == MAJOR_CHANGE) return;
    if (!random(5)) color1 = (color1 + random(5,20)) % MaxColor;
  }
}

/*
 * centerpulse - One pixel travelling down the center spine
 */
 
void centerpulse(int cycles) {
  int randcolor;
  
  if(!random(5)) { randcolor = 1; }  // White!
  else { randcolor = random(0, MaxColor); } // pick a random starting color
  
  for (int count = 0; count < cycles; count++) {
    for (int i=1; i < 13; i++) {  // Where we are on the center spine
      for (int j=0; j < strip.numPixels(); j++) {
        if (Stripes[j] == i) { setPixelColor(SpineOrder[j], Wheel(randcolor)); }
        else { setPixelColor(SpineOrder[j], Color(0,0,0)); }
      }
      if (morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
      if (!random(100)) { if (wait++ > maxWait) wait = 0; }
      if (!random(10)) {
        if (randcolor+=5 > MaxColor) randcolor = randcolor - MaxColor;
      }
    }
  }
}

/*
 * randomfill: randomly fills in pixels from blank to all on
 * then takes them away random until blank
 */
 
void randomfill(int cycles) {
  int i, j, save;
  byte shuffle[numLights];  // Will contain a shuffle of 0 to totPixels
  
  for (int count = 0; count < cycles; count++) {
    
    // Shuffle sort to determine order to turn on lights
    for (i=0; i < numLights; i++) shuffle[i] = i; // before shuffle
    for (i=0; i < numLights; i++) {  // here's position
      j = random(numLights);       // there's position
      save = shuffle[i];
      shuffle[i] = shuffle[j];     // first swap
      shuffle[j] = save;           // second swap
    }
    // Blank the screen
    clear();
    strip.show();
    if(talkdelay(GetDelayTime(wait)) == MAJOR_CHANGE) return;
    
    // Turn on lights one at a time
    for (i=0; i < numLights; i++) {
      setPixelColor(shuffle[i], Wheel(color1));
      if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
    }
    // Turn off lights one at a time
    for (i=0; i < numLights; i++) {
      setPixelColor(shuffle[numLights-i-1], Color(0,0,0));
      if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
    }
    // Possibly new delay
    if (!random(2)) { wait = random(0,maxWait); }
  }
}  

/*
 * rainbowshow: fills the spines with a gradient rainbow
 * over time, the starting color changes
 */
 
void rainbowshow(int cycles) {
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
      strip.setPixelColor(SpineOrder[i], Wheel(((i*256 / strip.numPixels()) + color1) % 256) );
    }  
    strip.show();   // write all the pixels out
    if( talkdelay(50) == MAJOR_CHANGE) return;
    if (color1+=15 > MaxColor) color1 = color1 - MaxColor;
  }
}

/*
 * visit each light in order and illuminate its neighbors
 * ignores buffering
 */
void unit_test() {
  
  for (int i=0; i<numLights; i++) {
    clear();
    strip.setPixelColor(SpineOrder[i], Color(255,0,0));
    strip.show();
    delay(500);    
  }
}

/*
 * lightwave - Just one traveling pixel
 */
 
void lightwave(int cycles) {
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      for (int j=0; j < strip.numPixels(); j++) {
        if (i == j) { setPixelColor(j, Wheel(color1)); }
        else { setPixelColor(SpineOrder[j], Color(0,0,0)); }
      }
      if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
      if (!random(100)) { if (wait++ > maxWait) wait = 0; }
      if (!random(10)) {
        if (color1+=5 > MaxColor) color1 = color1 - MaxColor;
      }
    }
  }
}

//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code from Greg and Robie
//

void draw_frame(int delay_ms) {
  for (int i = 0; i < numLights; i++) {
    strip.setPixelColor(i, next_frame[i] & 0xffffff);
    current_frame[i] = next_frame[i];
  }
  strip.show();
  delay(delay_ms);
}

void draw_frame() {
  draw_frame(250);
}

#define FRAME_MSEC 100
#define MORPH_STEPS 25
#define STEP_LENGTH 10

byte morph_frame(int msec) {
  byte flag = NO_CHANGE;
  int steps = msec / STEP_LENGTH;
  int deltas[numLights][3];
  for (int i = 0; i < numLights; i++) {
    for (int j = 0; j < 3; j++) {
      uint32_t old_color = (current_frame[i] >> (j * 8)) & 0xff; 
      uint32_t new_color = (next_frame[i] >> (j * 8)) & 0xff;
      deltas[i][j] = (int)new_color - (int)old_color;
    }
  }

  for (int t = 1; t < steps; t++) {
    for (int i = 0; i < numLights; i++) {
      uint32_t color = 0;
      for (int j = 0; j < 3; j++) {
        uint32_t old_color = (current_frame[i] >> (j * 8)) & 0xff;
        color |= ((old_color + (deltas[i][j] * t / steps)) & 0xff) << (8 * j);
      }
      strip.setPixelColor(i, color);
    }
    flag = ExchangeInfo();
    if (flag != NO_CHANGE) break;
    strip.show();
    delay(STEP_LENGTH);
  }
  for (int i = 0; i < numLights; i++) {
    current_frame[i] = next_frame[i];
    strip.setPixelColor(i, current_frame[i]);
  }
  strip.show();
  delay(STEP_LENGTH);
  flag = max(ExchangeInfo(), flag);
  return(flag);
}

void setPixelColor(int pos, uint32_t color) {
  next_frame[pos] = color;
}

void fill(uint32_t color) {
  for (int i = 0; i < numLights; i++) {
    setPixelColor(i, color);
  }
}

//
// talk delay
//
// Runs a normal timed delay
//
// Meanwhile, exchanges information every POLL_TIMEms
//
// MASTER: Send out data ever SYNC_FREQ
// SLAVE: See every POLL_TIMEms whether there is data to get

#define POLL_TIME 10

byte talkdelay(int delaytime) {
  int i;
  byte flag;
  for(int i=delaytime; i>POLL_TIME; i-=POLL_TIME) {
    flag = ExchangeInfo();
    if (flag != NO_CHANGE) return(flag);  // Quit immediately!
    delay(POLL_TIME);
  }
  delay(i); // The remainder
}

//
// Exchange Info
//
// If Master, routine sends out "#", then show number, delay number, color1, and color2
//
// If Slave, routine looks for a "#", then receives show number, delay number, color1, and color2
//

byte ExchangeInfo() {
  int heardNum;
  byte flag = NO_CHANGE;
  
  NewTime = millis();  // Take a time point
  
  if (I_AM_MASTER) {
    if (NewTime > (OldTime + SYNC_FREQ)) { // Time for Master to send out signals
      OldTime = NewTime;     // Reset the clock 
      Serial.print('#');     // Symbol to start communication
      Serial.print(show);    // Send show information
      Serial.print('$');     // Space character
      Serial.print(wait);    // Send delay information
      Serial.print('$');     // Space character
      Serial.print(color1);  // Send color1
      Serial.print('$');     // Space character
      Serial.print(color2);  // Send color2
      Serial.println('$');   // Space character and new line
    }
    return(NO_CHANGE);  // no system update required
  
  } else {  // Slave
    if (Serial.available()) {
        char incoming = Serial.read();
        if (incoming == '#') {  // Heard from the master
          OldTime = NewTime;    // Reset the clock
          Alone = false;        // We're in contact! 
          //Serial.print('#');
          heardNum = HearNum();  // Show
          //Serial.print("Show = ");
          //Serial.println(heardNum);
          if (heardNum == NO_DATA) return(flag); // Not getting a legit number
          else {
            if (show != heardNum) {  // Slave has different show than master
              show = heardNum;
              flag = MAJOR_CHANGE;           // Force update
            }
          }
          
          heardNum = HearNum();    // wait
          //Serial.print("wait = ");
          //Serial.println(heardNum);
          if (heardNum == NO_DATA) return(flag); // Not getting a legit number
          else {
            if (wait != heardNum) {
              wait = heardNum;
              flag = max(flag, MINOR_CHANGE);
            }
          }
          
          heardNum = HearNum();    // color1
          //Serial.print("color1 = ");
          //Serial.println(heardNum);
          if (heardNum == NO_DATA) return(flag); // Not getting a legit number
          else {
            if (color1 != heardNum) {
              color1 = heardNum;
              flag = max(flag, MINOR_CHANGE);
            }
          }
          
          heardNum = HearNum();    // color2
          //Serial.print("color2 = ");
          //Serial.println(heardNum);
          if (heardNum == NO_DATA) return(flag); // Not getting a legit number
          else {
            if (color2 != heardNum) {
              color2 = heardNum;
              flag = max(flag, MINOR_CHANGE);
            }
          }
        }
        return(flag);
    }
    // No received Serial.available or No '#'
    //
    // Check how long since last hearing from the Master

    if((NewTime - OldTime)/1000 > MAX_AWAY) {
      OldTime = NewTime;    // Reset the clock (for some reason?)
      Alone = true;  // Lost contact with the Master
      //Serial.println("Feeling very alone");
    }
    return(NO_CHANGE);  // no system update required
  }
}

//
// HearNum
//
// Converts xBee characters into a number
// '$' character terminates the number string
//
// If no number, returns NO_DATA signifier

int HearNum() {
  #define BUFSIZE 10
  #define TIMEOUT_TRIES 100
  
  int waitTime = 0;  // Start counter
  int charplace = 0; // Start of number
  char buf[BUFSIZE];
  
  while (waitTime < TIMEOUT_TRIES) {
    delay(1);
    if (Serial.available()) {
      char tmp = Serial.read();
      Serial.print(tmp);
      if (tmp != '$') { // Hopefully 0-9 and '-'
        waitTime = 0;  // Reset wait counter
        buf[charplace] = tmp;
        if (charplace++ >= BUFSIZE) {
          //Serial.println("Number too long");
          return(NO_DATA);
         }
       } else {      // Got a '$' to end the word
         if (charplace>0) return atoi(buf); else {  // successfully reached the end of a number 
           //Serial.println("i=0 number read");
           return(NO_DATA);  // Returning an empty word
         }
       }
      }
      waitTime++;
    }
  //Serial.println("Out_of_Time");
  return(NO_DATA);  // ran out of time for next character: bail and return NO_DATA
}

/*
 * Get Delay Time - Returns a delay time from an array.
 */
 
int GetDelayTime(int wait) {
  int DelayValues[maxWait] = { 10, 20, 40, 60, 80, 100, 150, 200, 300, 500, 750, 1000 };
  if (wait < maxWait) { return (DelayValues[wait]); } else { return (DelayValues[maxWait]); }
}
  
void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 96 * 3; j++) {     // 3 cycles of all 96 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 96));
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t frame, uint8_t wait) {
  int i;
  for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
     strip.setPixelColor(i, Wheel(((i*24 / strip.numPixels()) + frame) % 96) );
  }  
  strip.show();   // write all the pixels out
  delay(wait);
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint16_t c, uint8_t wait) {
  int i;
  
  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
  }  
  strip.show();
  delay(wait);
}


/* Helper functions */

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
  //if (WheelPos == 0) return Color(0,0,0);       // Black
  //if (WheelPos == 1) return Color(255,255,255); // White
  
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170; 
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
 
//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
//Furthermore, the colors are attenuated:
//3=full strength, 2=2/3rd strength, 1=1/3rd strength, 0=barely on
uint32_t GradientWheel(byte WheelPos, int strength)
{
  int r,g,b;
  int s[4] = { 2, 10, 50, 255 };  // This is the attenuation percent
  
  if (WheelPos < 85) {
   r = WheelPos * 3;
   g = 255 - WheelPos *3;
   b = 0;
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   r = 255 - WheelPos *3;
   g = 0;
   b = WheelPos * 3;
  } else {
   WheelPos -= 170; 
   r = 0;
   g = WheelPos * 3;
   b = 255 - WheelPos *3;
  }
  
  // Attenuate r,g,b
  r = r * s[strength] / 100;
  g = g * s[strength] / 100;
  b = b * s[strength] / 100;
  return Color(r,g,b);
}     
