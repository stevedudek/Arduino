//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Great Eyeball Coat - 113 lights
//
/*****************************************************************************/
//
// 1/15/15
//
// Colors are f----d up due to storage constraints
// The arduino microprocessor is running out of RAM so
// colors are stored reduced as bytes instead of uint32_t's
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 113      // Lots! Causes memory storage

// framebuffers now stored in program memory - strange bugs otherwise
byte current_frame[numLights];
byte next_frame[numLights];

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Hat colors

#define MaxColor 252    // Colors are 6 * 42 : REDUCED SET!
#define BLCK     255    // Hack for black/off color
#define WHTE     254    // hack for white color

int foreColor  = 84;   // Starting foreground color
int backColor = 168;   // Starting background color

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
  
  switch (random(8)) {
    
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
      randomColors(random(5,30)*10000);
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
    setPixelColor(i, BLCK);
  }
}

/*
 * Fades lights to black over fade time
 */
void clearWithFade(int fadeTime) {
  for (int i=0; i<numLights; i++) {
    setPixelColor(i, BLCK);
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
    for (int i=0; i < numLights; i++) {
      setPixelColor(i, foreColor);
    }
    morph_frame(GetDelayTime(wait));
  
    if (!random(100)) {
      if (wait++ > maxWait) wait = 0;
    }
    foreColor = (foreColor + 1) % MaxColor;  
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
      setPixelColor(shuffle[i], foreColor);
      morph_frame(GetDelayTime(wait));
    }
    // Turn off lights one at a time
    for (i=0; i < numLights; i++) {
      setPixelColor(shuffle[numLights-i-1], BLCK);
      morph_frame(GetDelayTime(wait));
    }
    // Possibly new delay
    if (!random(2)) { wait = random(0,maxWait); }
  }
}  

/*
 * randomcolors: turns each pixel on to a random color
 */

void randomColors(long time) {
  for (int i=0; i < numLights; i++) setPixelColor(i, random(MaxColor));
  morph_frame(1000);
  delay(time);
  clearWithFade(1000);
}

/*
 * twocolor: alternates the color of pixels between two colors
 */

void twocolor(long time) {
  for (int i=0; i < numLights; i++) {
    if (i%2) { setPixelColor(i, foreColor); }
    else { setPixelColor(i, backColor); }
  }
  morph_frame(1000);
  delay(time);
  clearWithFade(1000);
  
  if (backColor <= 0) backColor = MaxColor;
  backColor -= 1;
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
      setPixelColor(i, ((i*diff / strip.numPixels())+foreColor) % diff);
    }  
    morph_frame(GetDelayTime(wait));
    foreColor = (foreColor + 2) % MaxColor;
    
    // Possibly new delay
    if (!random(10)) { if (wait++ > maxWait) wait = 0; }
  }
  clearWithFade(1000);
}

/*
 * lightwave - Just one pixel traveling from 0 to numLights
 */
 
void lightwave(int cycles) {
  int i,j,count;
  
  for (count = 0; count < cycles; count++) {
    for (i=0; i < numLights; i++) {
      for (j=0; j < numLights; j++) {
        if (i == j) {
          setPixelColor(j, foreColor);
        } else {
          setPixelColor(j, BLCK);
        }
      }
      morph_frame(GetDelayTime(wait));
      if (!random(100)) wait = (wait + 1) % maxWait;
      if (!random(10)) {
        if (foreColor <= 0) foreColor = MaxColor;
        foreColor -= 1;
      }
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
          setPixelColor(j, BLCK);
        } 
        if (j == i) {
          setPixelColor(j, foreColor);
        }
        if (j == (numLights-i-1)) {
          setPixelColor(j, backColor);
        }
      }
      morph_frame(GetDelayTime(wait));
      if (!random(100)) wait = (wait + 1) % maxWait;
      if (!random(10)) foreColor = (foreColor + 1) % MaxColor;
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
          if (i == j) { setPixelColor(j, foreColor); }
          else { setPixelColor(j, BLCK); }
        }
        morph_frame(GetDelayTime(wait));
        if (!random(100)) wait = (wait + 1) % maxWait;
        if (!random(10)) foreColor = (foreColor + 2) % MaxColor;
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
    setPixelColor(i, next_frame[i]);
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
  
  /*
  int deltas[numLights][3];
  
  for (int i = 0; i < numLights; i++) {
    for (int j = 0; j < 3; j++) {
      uint32_t old_color = (Wheel(current_frame[i]) >> (j * 8)) & 0xff; 
      uint32_t new_color = (Wheel(next_frame[i]) >> (j * 8)) & 0xff;
      deltas[i][j] = (int)new_color - (int)old_color;
    }
  }
  */

  for (int t = 1; t < steps; t++) {
    for (int i = 0; i < numLights; i++) {
      uint32_t color = 0;
      for (int j = 0; j < 3; j++) {
        uint32_t old_color = (Wheel(current_frame[i]) >> (j * 8)) & 0xff;
        uint32_t new_color = (Wheel(next_frame[i]) >> (j * 8)) & 0xff;
        int delta = (int)new_color - (int)old_color;
        color |= ((old_color + (delta * t / steps)) & 0xff) << (8 * j);
      }
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(STEP_LENGTH);
  }
  
  for (int i = 0; i < numLights; i++) {
    current_frame[i] = next_frame[i];
  }
}

void setPixelColor(int pos, byte color) {
  next_frame[pos] = color;
}



void fill(byte color) {
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

// Input a value 42 * 6 to get a color value.
// The colours are a transition r - g -b - back to r
// Intensity is scaled down with increasing number of lights
// to prevent overloading the power supply

uint32_t Wheel(byte color)
{
  return Gradient_Wheel(color, 25.0 / numLights);
}

//Input a value 24 * 6 to get a color value.
//The colours are a transition r - g -b - back to r
//Intensity must be 0 <= intensity <= 1
uint32_t Gradient_Wheel(byte color, float intensity)
{
  byte r,g,b;
  byte channel, value;
 
  if (intensity > 1) intensity = 1;
  if (intensity < 0) intensity = 0;
  
  if (color == BLCK) return (Color(0,0,0));
  if (color == WHTE) {
    return (Color(255*intensity, 255*intensity, 255*intensity));
  }
  
  color = color % MaxColor;  // Keep colors within bounds
  
  channel = color / 42;
  value = (color % 42) * 6.22;
  
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
