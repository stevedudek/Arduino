//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Dimetrodon - 1 strand of 15 lights on a sail
//             - Sail is symmetric (7 + 1 + 7)
//             - No keypad
//
/*****************************************************************************/
//
// 10/22/14
//
// Modern full colors
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights  144   // 15 segments to the sail

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Sail colors

int foreColor  = 84;   // Starting foreground color
int backColor = 168;   // Starting background color
#define MaxColor 1536    // Colors are 6 * 256


// Shows

int wait=6;    // index to keep track of delay time
#define maxWait 12     // Number of stored delay times 

//
// Setup
//

void setup() {

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
  
  switch(0) {//random(8)) {
    
    case 0:    
      allOn(random(20,100));
      break;
    case 1:
      randomfill(random(5,20));
      break;
    case 2:
      twocolor(random(1,30)*10000);
      break;
    case 3:
      rainbowshow(random(100,500));
      break;
    case 4:
      lightwave(random(10,50));
      break;
    case 5:
      twowave(random(10,50));
      break;
    case 6:
      lightrunup(random(10,40));
      break;
    case 7:
      randomcolors(random(5,30)*10000);
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
    delay(GetDelayTime(wait));
    
    // Turn on lights one at a time
    for (i=0; i < numLights; i++) {
      setPixelColor(shuffle[i], Wheel(foreColor));
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
  backColor = (backColor + 50) % MaxColor; 
}

/*
 * rainbowshow: fills the spines with a gradient rainbow
 * over time, the starting color changes
 */
 
void rainbowshow(int cycles) {

  for (int count = 0; count < cycles; count++) {
    //int diff = abs(foreColor - backColor);
    int diff = MaxColor;
    for (int i=0; i < strip.numPixels(); i++) {
      setPixelColor(i, Wheel(((i*diff / strip.numPixels())+foreColor) % diff));
    }  
    morph_frame(GetDelayTime(wait));
    foreColor = (foreColor + 20) % MaxColor;
    
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
 * lightwave - Just one pixel traveling from 0 to numLights
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
 * twowave - Two pixels traveling in opposite directions
 */
 
void twowave(int cycles) {
 
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < numLights; i++) {
      for (int j=0; j < numLights; j++) {
        if (j != i && j != (numLights-i-1)) {
          setPixelColor(j, Color(0,0,0));
        } 
        if (j == i) {
          setPixelColor(j, Wheel(foreColor));
        }
        if (j == (numLights-i-1)) {
          setPixelColor(j, Wheel(backColor));
        }
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
        if (!random(10)) foreColor = (foreColor + 20) % MaxColor;
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

//Input a value 256 * 6 to get a color value.
//The colours are a transition r - g -b - back to r
//Intensity must be 0 <= intensity <= 1
uint32_t Gradient_Wheel(int color, float intensity)
{
  int r,g,b;
  int channel, value;
  
  color = color % MaxColor;  // Keep colors within bounds
    
  channel = color / 256;
  value = color % 256;
  
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
