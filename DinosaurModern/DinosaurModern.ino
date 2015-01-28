#include <TimerOne.h>
#include "LPD6803.h"

//
//  Modern dinosaur.
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights.
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 25      // number of spikes

// framebuffers
uint16_t current_frame[numLights];
uint16_t next_frame[numLights];

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
LPD6803 strip = LPD6803(numLights, dataPin, clockPin);

// Dinosaur colors

uint8_t foreColor  = 57;  // Starting foreground color
uint8_t backColor =  57;  // Starting background color
#define MaxColor 96   // Colors go from 0 to 96!

// Shows

int wait=6;    // index to keep track of delay time
#define maxWait 12     // Number of stored delay times 

//
// Setup
//

void setup() {
  
  // The Arduino needs to clock out the data to the pixels
  // this happens in interrupt timer 1, we can change how often
  // to call the interrupt. setting CPUmax to 100 will take nearly all all the
  // time to do the pixel updates and a nicer/faster display, 
  // especially with strands of over 100 dots.
  // (Note that the max is 'pessimistic', its probably 10% or 20% less in reality)
  
  strip.setCPUmax(50);  // start with 50% CPU usage. up this if the strand flickers or is slow

  // Start up the LED counter
  strip.begin();
  
  // Update the strip, to start they are all 'off'
  clear();
  strip.show();
}


void loop() {
   
  // Start with a fun random show
  
  switch(random(5)) {
  //switch(0) {  
    case 0:    
      allOn(random(100,500));
      break;
    case 1:
      twocolor(random(20,100)*10000);
      break;
    case 2:
      rainbowshow(random(100,500));
      break;
    case 3:
      lightwave(random(20,100));
      break;
    case 4:
      lightrunup(random(10,40));
      break;
  }
  
  // Then a break with all the lights off
  clear();
  strip.show();
  delay(random(2,5)*10000);
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
   for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      //strip.setPixelColor(i, Wheel(foreColor));
      setPixelColor(i, Wheel(foreColor));
    }
    //strip.show();
    morph_frame(GetDelayTime(wait));
    //delay(10000);
    if (!random(1000)) { if (wait++ > maxWait) wait = 0; }
    foreColor = foreColor++ % MaxColor;  
  }
  clearWithFade(1000);
}

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
 * twocolor: alternates the color of pixels between two colors
 */

void twocolor(int cycles) {
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < numLights; i++) {
      if (i%2) { setPixelColor(i, Wheel(foreColor)); }
      else { setPixelColor(i, Wheel(backColor)); }
    }
    morph_frame(GetDelayTime(wait));
  
    if (!random(1000)) { if (wait++ > maxWait) wait = 0; }
    foreColor = foreColor++ % MaxColor;
    backColor = backColor - 2;
    if (backColor < 0) backColor = backColor + MaxColor;
    
  }
  clearWithFade(1000);
}

/*
 * rainbowshow: fills the spines with a gradient rainbow
 * over time, the starting color changes
 */
 
void rainbowshow(int cycles) {

  for (int count = 0; count < cycles; count++) {
    int diff = abs(foreColor - backColor);
    for (int i=0; i < strip.numPixels(); i++) {
      setPixelColor(i, Wheel(((i*diff / strip.numPixels())+foreColor) % diff));
    }  
    morph_frame(GetDelayTime(wait));
    foreColor = foreColor++ % MaxColor;
    
    // Possibly new delay
    if (!random(1000)) { if (wait++ > maxWait) wait = 0; }
  }
  clearWithFade(1000);
}

/*
 * visit each light in order and illuminate its neighbors
 * ignores buffering
 */
void unit_test() {
  
  for (int i=0; i<numLights; i++) {
    clear();
    strip.setPixelColor(i, Color(255,0,0));
    strip.show();
    delay(500);    
  }
}

/*
 * lightwave - Just one pixel traveling from 0 to 18
 */
 
void lightwave(int cycles) {
 
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < numLights; i++) {
      for (int j=0; j < numLights; j++) {
        if (i == j) { setPixelColor(j, Wheel(foreColor)); }
        else { setPixelColor(j, Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
      if (!random(1000)) wait = (wait + 1) % maxWait;
      foreColor = (foreColor + 5) % MaxColor;
    }
  }
}

/*
 * lightrunup. Show where the lights fill up one at time
 */
 
void lightrunup(int cycles) {
  int i,j,k,count;
  
  for (count = 0; count < cycles; count++) {
    for (k=numLights; k > 0; k--) {
      for (i=0; i < k; i++) {
        for (j=0; j < k; j++) {
          if (i == j) { setPixelColor(j, Wheel(foreColor)); }
          else { setPixelColor(j, Color(0,0,0)); }
        }
        morph_frame(GetDelayTime(wait));
        if (!random(1009)) wait = (wait + 1) % maxWait;
        foreColor = (foreColor + 2) % MaxColor;
      }
    }
  }
  clearWithFade(1000);
}

    
//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code from Greg and Robie
//

void draw_frame(int delay_ms) {
  for (int i = 0; i < numLights; i++) {
    strip.setPixelColor(i, next_frame[i] & 0x7fff);
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
  strip.show();
  delay(msec);
  /*
  int steps = msec / STEP_LENGTH;
  uint8_t deltas[strip.numPixels()][3];
  for (int i = 0; i < strip.numPixels(); i++) {
    for (int j = 0; j < 3; j++) {
      uint8_t old_color = (current_frame[i] >> (j * 5)) & 0x1f; 
      uint8_t new_color = (next_frame[i] >> (j * 5)) & 0x1f;
      deltas[i][j] = (uint8_t)new_color - (uint8_t)old_color;
    }
  }

  for (int t = 1; t < steps; t++) {
    for (int i = 0; i < strip.numPixels(); i++) {
      uint16_t color = 0;
      for (int j = 0; j < 3; j++) {
        uint8_t old_color = (current_frame[i] >> (j * 5)) & 0x1f;
        color |= ((old_color + (deltas[i][j] * t / steps)) & 0x1f) << (5 * j);
      }
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(STEP_LENGTH);
  }
  for (int i = 0; i < strip.numPixels(); i++) {
    current_frame[i] = next_frame[i];
    strip.setPixelColor(i, current_frame[i]);
  }
  strip.show();
  delay(STEP_LENGTH);
  */
}

void setPixelColor(int pos, uint16_t color) {
  strip.setPixelColor(pos,color);
  //next_frame[pos] = color;
}

void fill(uint16_t color) {
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

// Create a 15 bit color value from R,G,B
unsigned int Color(byte r, byte g, byte b)
{ 
   //Take the lowest 5 bits of each value and append them end to end
  return( ((unsigned int)g & 0x1F )<<10 | ((unsigned int)b & 0x1F)<<5 | (unsigned int)r & 0x1F);
}

//Input a value 0 to 127 to get a color value.
//The colours are a transition r - g -b - back to r
unsigned int Wheel(byte WheelPos)
{
  byte r,g,b;
  switch(WheelPos >> 5)
  {
    case 0:
      r=31- WheelPos % 32;   //Red down
      g=WheelPos % 32;      // Green up
      b=0;                  //blue off
      break; 
    case 1:
      g=31- WheelPos % 32;  //green down
      b=WheelPos % 32;      //blue up
      r=0;                  //red off
      break; 
    case 2:
      b=31- WheelPos % 32;  //blue down 
      r=WheelPos % 32;      //red up
      g=0;                  //green off
      break; 
  }  
  return(Color(r,g,b));
}
