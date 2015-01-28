//
// Silver Hat
//
// 12 August 2014
//
// Full color wheel
//
// No xBee

//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Siler Hat! - 35 Lights. This version does not use a keypad
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

// Hat colors

int currColor  = 0;   // Starting foreground color
int currColor2 = 500;  // Starting background color
#define MaxColor 1530    // Colors are 6 * 255

// Shows

int f=0;       // indices to keep track of position within a show
int x=0;       // another counter
int temp;      // temporary place holder
int wait=6;    // index to keep track of delay time
#define maxWait 12     // Number of stored delay times 
  
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
  //Serial.begin(9600);
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
   
  // Start with a fun random show
  
  switch(random(10)) {
    case 0:    
      allOn(random(20,100));
      break;
    case 1:
      randomfill(random(3,10)); 
      break;
    case 2:
      randomcolors(random(5,30)*10000);
      break;
    case 3:
      twocolor(random(1,30)*10000);
      break;
    case 4:
      rainbowshow(random(100,500));
      break;
    case 5:
      lightwave(random(2,10));
      break;
    case 6:
      lighttails(random(1,30)*10000);
      break;
    case 7:
      lighthood(random(1,30)*10000);
      break;
    case 8:
      centergradient(random(1,30)*10000);
      break;
    case 9:
      centerpulse(random(5,40));
      break;
  }
  // Then a break with all the lights off
  clear();
  strip.show();
  delay(random(1,4)*10000);  // Hopefully 10-1200 seconds
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
 
void allOn (int cycles) {
  int randcolor = random(MaxColor); // pick a random starting color
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      setPixelColor(i, Wheel(randcolor));
    }
    morph_frame(GetDelayTime(wait));
  
    if (!random(100)) { if (wait++ > maxWait) wait = 0; }
    if (randcolor+=5 > MaxColor) randcolor = randcolor - MaxColor;  
  }
  clearWithFade(1000);
}

/*
 * Stripe
 *
 * Put a stripe down the center Slowly color change
 *
 */
/* 
void stripe(int cycles) {
  int randcolor = random(MaxColor); // pick a random starting color
  
  for (int count = 0; count < cycles; count++) {
   for (int i=0; i < numLights; i++) {
     if (centerstripe[i]) { setPixelColor(SpineOrder[i], Wheel(randcolor));
     } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
   }
   morph_frame(GetDelayTime(wait));
  
   if (!random(100)) { if (wait++ > maxWait) wait = 0; }
   if (randcolor+=10 > MaxColor) randcolor = randcolor - MaxColor;  
  }
  clearWithFade(1000);
}*/

/*
 * randomcolors: turns each pixel on to a random color
 */

void randomcolors(long time) {
  for (int i=0; i < numLights; i++) setPixelColor(i, Wheel(random(MaxColor)));
  morph_frame(1000);
  delay(time);
  clearWithFade(1000);
}

/*
 * twocolor: turns each pixel on to either of two random colors
 */

void twocolor(long time) {
  int color1, color2, maxspacing;
  color1 = random(MaxColor);  // Set up color1
  color2 = color1 + random(25,MaxColor/2);  // Set up color2
  if (color2 > MaxColor) color2 = color2 - MaxColor;
  
  for (int i=0; i < numLights; i++) {
    if (random(2)) { setPixelColor(i, Wheel(color1)); }
    else { setPixelColor(i, Wheel(color2)); }
  }
  morph_frame(1000);
  delay(time);
  clearWithFade(1000);
}

/*
 * lighttails: turn on the tail lights to a gradient
 */
 
void lighttails(long time) {
  int color1 = random(MaxColor);  // Set up color1
  int spacing = random(2,20);   // Spacing between colors
  
  for (long t=time; t=t-500; t>0) {
    for (int i=0; i < numLights; i++) {
     if (!Tails[i]) { strip.setPixelColor(SpineOrder[i], Color(0,0,0)); }
     else { strip.setPixelColor(SpineOrder[i],
       Wheel((color1 + (spacing*Tails[i]) % MaxColor))); }
    }
    strip.show();
    delay(500);
    if (!random(5)) color1 = (color1 + random(5,20)) % MaxColor;
  }
}

/*
 * lighthood: ring the hood with one color; fill everything else with another color
 */
 
void lighthood(long time) {
  int color1 = random(MaxColor);  // Set up color1
  int color2;
  int spacing = random(2,20);   // Spacing between colors
  
  if (random(3)) color2 = 0;
  else color2 = random(MaxColor); // Set up color2
  
  for (long t=time; t=t-500; t>0) {
    for (int i=0; i < numLights; i++) {
     if (!Hood[i]) { strip.setPixelColor(SpineOrder[i], Wheel(color2)); }
     else { strip.setPixelColor(SpineOrder[i],
       Wheel((color1 + (spacing*Hood[i]) % MaxColor))); }
    }
    strip.show();
    delay(500);
    if (!random(5)) color1 = (color1 + random(5,20)) % MaxColor;
  }
}

/*
 * centergradient: turn on the central spine with a light gradient
 */
 
void centergradient(long time) {
  int color1 = random(MaxColor);  // Set up color1
  int spacing = random(2,6);   // Spacing between colors
  
  for (long t=time; t=t-500; t>0) {
    for (int i=0; i < numLights; i++) {
     if (!Stripes[i]) { strip.setPixelColor(SpineOrder[i], Color(0,0,0)); }
     else { strip.setPixelColor(SpineOrder[i],
       Wheel((color1 + (spacing*Stripes[i]) % MaxColor))); }
    }
    strip.show();
    delay(500);
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
      morph_frame(GetDelayTime(wait));
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
  byte randcolor;
  
  for (int count = 0; count < cycles; count++) {
    if(!random(5)) { randcolor = 1; }  // White!
    else { randcolor = random(0, MaxColor); } // pick a random starting color
    
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
      setPixelColor(shuffle[i], Wheel(randcolor));
      morph_frame(GetDelayTime(wait));
    }
    // Turn off lights one at a time
    for (i=0; i < numLights; i++) {
      setPixelColor(shuffle[numLights-i-1], Color(0,0,0));
      morph_frame(GetDelayTime(wait));
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
  int frame = random(MaxColor);  // Starting position for pixel 0
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel(((i*MaxColor / strip.numPixels()) + frame) % MaxColor) );
    }  
    strip.show();   // write all the pixels out
    delay(50);
    if (frame+=15 > MaxColor) frame = frame - MaxColor;
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
  int randcolor;
  
  if(!random(5)) { randcolor = 1; }  // White!
  else { randcolor = random(0, MaxColor); } // pick a random starting color
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      for (int j=0; j < strip.numPixels(); j++) {
        if (i == j) { setPixelColor(j, Wheel(randcolor)); }
        else { setPixelColor(j, Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
      if (!random(100)) { if (wait++ > maxWait) wait = 0; }
      if (!random(10)) {
        if (randcolor+=5 > MaxColor) randcolor = randcolor - MaxColor;
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

void morph_frame(int msec) {
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
    strip.show();
    delay(STEP_LENGTH);
  }
  for (int i = 0; i < numLights; i++) {
    current_frame[i] = next_frame[i];
    strip.setPixelColor(i, current_frame[i]);
  }
  strip.show();
  delay(STEP_LENGTH);
}

void setPixelColor(int pos, uint32_t color) {
  next_frame[pos] = color;
}

void fill(uint32_t color) {
  for (int i = 0; i < numLights; i++) {
    setPixelColor(i, color);
  }
}

/*
 * Get Delay Time - Returns a delay time from an array.
 */
 
int GetDelayTime(int wait) {
  int DelayValues[maxWait] = { 10, 20, 40, 60, 80, 100, 150, 200, 300, 500, 750, 1000 };
  if (wait < maxWait) { return (DelayValues[wait]); } else { return (DelayValues[maxWait]); }
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

//Input a value 256 * 6 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(int color)
{
  return Gradient_Wheel(color, 1);  // Intensity = 1
}

//Input a value 255 * 6 to get a color value.
//The colours are a transition r - g -b - back to r
//Intensity must be 0 <= intensity <= 1
uint32_t Gradient_Wheel(int color, float intensity)
{
  int r,g,b;
  int channel, value;
  
  color = color % MaxColor;  // Keep colors within bounds
    
  channel = color / 255;
  value = color % 255;
  
  if (intensity > 1) intensity = 1;
  if (intensity < 0) intensity = 0;
  
  switch(channel)
  {
    case 0:
      r = 255;
      g = value;
      b = 0;        
      break;
    case 1:
      r = 255 - value;
      g = 255;
      b = 0;        
      break;
    case 2:
      r = 0;
      g = 255;
      b = value;        
      break;
    case 3:
      r = 0;
      g = 255 - value;
      b = 255;        
      break;
    case 4:
      r = value;
      g = 0;
      b = 255;        
      break;
    default:
      r = 255;
      g = 0;
      b = 255 - value;        
      break; 
   }
  return(Color(r * intensity, g * intensity, b * intensity));
}   
