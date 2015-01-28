//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

// Accelerometer

#include <I2C.h>
#include <MMA8453_n0m1.h>

MMA8453_n0m1 accel;

//
//  Mohawk! - Only 10 Lights. This version uses an ACCELEROMETER
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights.
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 10      // number of spikes

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Mohawk colors

int foreColor  = 85;   // Starting foreground color
int backColor = 170;   // Starting background color
#define MaxColor 255   // Colors go from 0 to 255

int accelx, accely, accelz;

// Shows

int wait=6;    // index to keep track of delay time
#define maxWait 12     // Number of stored delay times 

//
// Setup
//

void setup() {
  // Accelerometer
  accel.setI2CAddr(0x1D);  //change your device address if necessary, default is 0x1C
  accel.dataMode(true, 2); //enable highRes 10bit, 2g range [2g,4g,8g]

  //Serial.begin(9600);
  //Serial.println("Start");
    
  // Start up the LED counter
  strip.begin();
  
  // Update the strip, to start they are all 'off'
  clear();
  strip.show();
}


void loop() {
   
  // Start with a fun random show
  
  switch(random(3)) {
    
    case 0:
      twocolor(random(2,5)*1000);
      break;
    case 1:
      rainbowshow(random(10,20));
      break;
    case 2:
      lightrunup(random(1,2));
      break;
  }
  
  // Then the accelerometer show
  accelshow(random(20,100));
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
      setPixelColor(i, Wheel(foreColor));
    }
    morph_frame(GetDelayTime(wait));
  
    if (!random(100)) { if (wait++ > maxWait) wait = 0; }
    foreColor = (foreColor + 5) % MaxColor;  
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

void twocolor(long time) {
  for (int i=0; i < numLights; i++) {
    if (i%2) { setPixelColor(i, Wheel(foreColor)); }
    else { setPixelColor(i, Wheel(backColor)); }
  }
  morph_frame(1000);
  delay(time);
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
    foreColor = (foreColor + 5) % MaxColor;
    
    // Possibly new delay
    if (!random(10)) { if (wait++ > maxWait) wait = 0; }
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
 * lightwave - Just one pixel traveling from 0 to 10
 */
 
void lightwave(int cycles) {
 
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < numLights; i++) {
      for (int j=0; j < numLights; j++) {
        if (i == j) { setPixelColor(j, Wheel(foreColor)); }
        else { setPixelColor(j, Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
      if (!random(100)) wait = (wait + 1) % maxWait;
      if (!random(10)) foreColor = (foreColor + 5) % MaxColor;
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
        if (!random(100)) wait = (wait + 1) % maxWait;
        if (!random(10)) backColor = (backColor + 5) % MaxColor;
      }
    }
  }
  clearWithFade(1000);
}

/*
 * accelShow - Fill the lights based on the accelerometer values
 */
 
void accelshow(int cycles) {
  int numfill;
  int minXaccel = -150;
  int maxXaccel = 100;
  int oldXaccel;
  
  accel.update();
  oldXaccel=accel.x();
  
  // Check to see whether accelerometer is plugged in
  
  if (accel.x() == 13 && accel.y() == 13) { // Not plugged in
    delay(100 * cycles);
    return;
  }
  
  for (int count = 0; count < cycles; count++) {
    pollAccel(100);
    
    // Look at the Z for a jump or a fall
    /*
    if (accelz < 175) {
      colorWipe(Color(255,255,255), 1000);
    } else {
    */
      // Look at the Y to change colors 
      if (accely > 100) foreColor = (foreColor + 2) % MaxColor;
      if (accely < -50) foreColor = (foreColor + MaxColor - 2) % MaxColor;
      
      // See whether Z has changed
      if (abs(accelx - oldXaccel) > 20) { oldXaccel = accelx; } else { accelx = oldXaccel; }
      if (accelx < minXaccel) accelx = minXaccel;
      if (accelx > maxXaccel) accelx = maxXaccel;
      
    
      numfill =  (accelx-minXaccel) / ((maxXaccel-minXaccel) / strip.numPixels());
      for (int i=0; i < strip.numPixels(); i++) { 
        if (i < numfill) { strip.setPixelColor(i, Wheel(foreColor)); }
        else { strip.setPixelColor(i, Color(0,0,0)); }
      }
      strip.show();   // write all the pixels out
    //}
  }
}

//
// Poll accel. Averages the accel values over a accelstep time.
//

void pollAccel(int acceltime) {
  long totaccelx, totaccely, totaccelz;
  int accelstep = 5;
  int count = 0;
  
  totaccelx = 0;  // zero counters
  totaccely = 0;
  totaccelz = 0;

  for (int t = acceltime; t >= accelstep; t=t-accelstep) {
    accel.update();
    totaccelx = totaccelx + accel.x();
    totaccely = totaccely + accel.y();
    totaccelz = totaccelz + accel.z();
    count++;
    delay(accelstep);
  }
  accelx = totaccelx/count;
  accely = totaccely/count;
  accelz = totaccelz/count;
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
