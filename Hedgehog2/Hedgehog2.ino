//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Hedgehog! - 50 Lights. This version does not use a keypad
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights. Reversed from usual!
// 
#define dataPin 3       // 'yellow' wire
#define clockPin 2      // 'green' wire

#define numLights 50      // number of spikes

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Hedgehog colors

int currColor  = 85;   // Starting foreground color
int currColor2 = 170;  // Starting background color
#define MaxColor 255    // Colors go from 0 to 255

// Shows

int f=0;       // indices to keep track of position within a show
int x=0;       // another counter
int temp;      // temporary place holder
int wait=6;    // index to keep track of delay time
#define maxWait 12     // Number of stored delay times 

// Order of spines
byte SpineOrder[50] = {
      8,
    9,13,7,
   10,12,14,6,
    11,15,5,
   17,16,3,4,
    18,2,0,
     19,1,
    20,22,24,
     21,23,
    40,25,27,
   41,39,26,28,
    42,38,29,
   43,37,31,30,
    44,36,32,
   45,47,35,33,
    46,48,34,
      49,
};

// Centering: creates a center bright ring
byte centering[50] = {
      0,
    0,1,0,
   0,1,1,0,
    0,2,0,
   0,2,2,0,
    1,2,1,
     2,2,
    2,3,2,
     3,3,
    2,3,2,
   2,3,3,2,
    2,3,2,
   1,2,2,1,
    0,2,0,
   0,1,1,0,
    0,1,0,
      0,
};

// chevpattern: pattern of chevrons
byte chevpattern[50] = {
      9,
    9,8,9,
   9,8,8,9,
    8,7,8,
   8,7,7,8,
    7,6,7,
     6,6,
    6,5,6,
     5,5,
    5,4,5,
   5,4,4,5,
    4,3,4,
   4,3,3,4,
    3,2,3,
   3,2,2,3,
    2,1,2,
      0,
};  
      
void setup() {
  //Serial.begin(9600);
  //Serial.println("Start");
  // not sure if this is necessary but try it anyway
  randomSeed(analogRead(0));
     
  // Start up the LED counter
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
}


void loop() {
   
  // Start with a fun random show
  switch(random(11)) {
    
    case 0:    
      allOn(random(20,100));
      break;
    case 1:
      randomfill(random(3,10)); 
      break;
    case 2:
      chevrons(random(50,200));
      break;
    case 3:
      randomcolors(random(5,30)*1000);
      break;
    case 4:
      twocolor(random(1,30)*10000);
      break;
    case 5:
      rainbowshow(random(20,100));
      break;
    case 6:
      hogshell(random(10,30));
      break;
    case 7:
      onlyedge(random(1,30)*10000);
      break;
    case 8:
      lightwave(random(2,10));
      break;
    case 9:
      bullseye(random(1,30)*10000);
      break;
    case 10:
      chevronrainbow(random(1,30)*10000);
      break;
  }
  // Then a break with all the lights off
  clear();
  strip.show();
  delay(random(5,30)*1000);  // Hopefully 5-30 seconds
}

/*
 * set all cells to black but don't call show yet
 * ignores buffering
 */
void clear() {
  for (int i=0; i<numLights; i++) {
    strip.setPixelColor(i, Color(0,0,0));
  }
}

/*
 * All On
 *
 * Simply turns all the pixes on to a random color and then cycles
 *
 */
 
void allOn (int cycles) {
  int randcolor = random(0, MaxColor); // pick a random starting color
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(randcolor));
    }
  strip.show();
  delay(GetDelayTime(wait));
  
  if (!random(100)) { if (wait++ > maxWait) wait = 0; }
  randcolor++;
  if (randcolor > MaxColor) randcolor = randcolor - MaxColor;  
  }
}

/*
 * randomcolors: turns each pixel on to a random color
 */

void randomcolors(long time) {
  for (int i=0; i < numLights; i++) strip.setPixelColor(i, Wheel(random(MaxColor)));
  strip.show();
  delay(time);
}

/*
 * twocolor: turns each pixel on to either of two random colors
 */

void twocolor(long time) {
  int color1, color2;
  color1 = random(MaxColor);  // Set up color1
  color2 = color1 + random(25,MaxColor/2);  // Set up color2
  if (color2 > MaxColor) color2 = color2 - MaxColor;
  
  for (int i=0; i < numLights; i++) {
    if (random(0,1)) { strip.setPixelColor(i, Wheel(color1)); }
    else { strip.setPixelColor(i, Wheel(color2)); }
  }
  strip.show();
  delay(time);
}

/*
 * bullseye: lights up the centering image with 4 colors
 */

void bullseye(long time) {
  int colors[4];
  int randcolor;
  int colorspacing;
  
  randcolor = random(MaxColor); // Initial color is random
  colorspacing = random(5, MaxColor/5);
  
  for (int i=0; i<4; i++) {
    colors[i] = randcolor;
    randcolor = randcolor + colorspacing;
    if (randcolor > MaxColor) randcolor = randcolor - MaxColor;
  }
  
  for (int j=0; j < strip.numPixels(); j++) {
      strip.setPixelColor(SpineOrder[j], Wheel(colors[centering[j]]));
  }
  strip.show();
  delay(time);
}

/*
 * chevronrainbow: lights up the chevron with a rainbow pattern with 10 colors
 */

void chevronrainbow(long time) {
  int colors[10];
  int randcolor;
  int colorspacing;
  
  randcolor = random(MaxColor); // Initial color is random
  colorspacing = random(5, MaxColor/11);
  
  for (int i=0; i<10; i++) {
    colors[i] = randcolor;
    randcolor = randcolor + colorspacing;
    if (randcolor > MaxColor) randcolor = randcolor - MaxColor;
  }
  
  for (int j=0; j < strip.numPixels(); j++) {
      strip.setPixelColor(SpineOrder[j], Wheel(colors[chevpattern[j]]));
  }
  strip.show();
  delay(time);
}
/*
 * randomfill: randomly fills in pixels from blank to all on
 * then takes them away random until blank
 */
 
void randomfill(int cycles) {
  int i, j, save;
  byte shuffle[numLights];  // Will contain a shuffle of 0 to totPixels
  byte randcolor;
  
  for (int count = 0; count < cycles; count++) {
    randcolor = random(0, MaxColor); // pick a random starting color
    
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
    delay(GetDelayTime(wait));
    
    // Turn on lights one at a time
    for (i=0; i < numLights; i++) {
      strip.setPixelColor(shuffle[i], Wheel(randcolor));
      strip.show();
      delay(GetDelayTime(wait));
    }
    // Turn off lights one at a time
    for (i=0; i < numLights; i++) {
      strip.setPixelColor(shuffle[numLights-i-1], Color(0,0,0));
      strip.show();
      delay(GetDelayTime(wait));
    }
    // Possibly new delay
    if (!random(2)) { wait = random(0,maxWait); }
  }
}  

/*
 * rainbow: fills the spines with a gradient rainbow
 * over time, the starting color changes
 */
 
void rainbowshow(int cycles) {
  byte frame = random(MaxColor);  // Starting position for pixel 0
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel(((i*24 / strip.numPixels()) + frame) % 96) );
    }  
    strip.show();   // write all the pixels out
    if (frame++ >= MaxColor) frame = 0;
    delay(GetDelayTime(wait));
    // Possibly new delay
    if (!random(10)) { if (wait++ > maxWait) wait = 0; }
  }
}

/*
 * chevrons: upward chevrons
 */
 
void chevrons(int cycles) {
  int chev, i, randcolor;
  randcolor = random(0, MaxColor); // pick a random starting color
    
  for (int count = 0; count < cycles; count++) {
    for (chev=0; chev < 10; chev++) {
      for (i=0; i < numLights; i++) {
        if (chevpattern[i] == chev) {
          strip.setPixelColor(SpineOrder[i], Wheel(randcolor));
        } else { strip.setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      strip.show();
      delay(GetDelayTime(wait));
    }
    if (!random(3)) { if (wait++ > maxWait) wait = 0; }
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

void hogshell(int cycles) {
  int randcolor = random(MaxColor); // pick a random starting color
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(SpineOrder[i], GradientWheel(randcolor,centering[i]));
    }
    strip.show();
    delay(GetDelayTime(wait));
    if (!random(100)) { if (wait++ > maxWait) wait = 0; }
    randcolor++;
    if (randcolor > MaxColor) randcolor = randcolor - MaxColor; 
  }
}

/*
 * onlyedge - Turns only the edge pixels on to a random color. Others are dark
 */
 
void onlyedge(long time) {
  int randcolor = random(MaxColor); // pick a random  color
  
  for (int i=0; i < strip.numPixels(); i++) {
    if (centering[i] == 0) { strip.setPixelColor(SpineOrder[i], Wheel(randcolor)); }
    else { strip.setPixelColor(SpineOrder[i], Color(0,0,0)); }
  }
  strip.show();   // write all the pixels out
  delay(time);
}

/*
 * lightwave - Just one pixel traveling from 0 to 49
 */
 
void lightwave(int cycles) {
  int randcolor = random(MaxColor); // pick a random starting color
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      for (int j=0; i < strip.numPixels(); i++) {
        if (i == j) { strip.setPixelColor(SpineOrder[i], Wheel(randcolor)); }
        else { strip.setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      strip.show();   // write all the pixels out
      delay(GetDelayTime(wait));
      if (!random(100)) { if (wait++ > maxWait) wait = 0; }
      if (!random(10)) {
        if (randcolor+=5 > MaxColor) randcolor = randcolor - MaxColor;
      }
    }
  }
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
