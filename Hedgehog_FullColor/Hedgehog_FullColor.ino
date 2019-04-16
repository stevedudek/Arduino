//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Hedgehog! - 50 Lights. Colors adjusted
//
/*****************************************************************************/
//
// 11/6/2014
//

#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 50      // number of spikes
// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Hedgehog colors

int currColor  = 85;   // Starting foreground color
int currColor2 = 170;  // Starting background color
#define MaxColor 1536    // Colors are 6 * 256

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

// starburstpattern: from center to edge
byte starburstpattern[50] = {
      9,
    9,8,9,
   8,7,7,8,
    6,5,6,
   5,4,4,5,
    3,2,3,
     1,1,
    2,0,2,
     1,1,
    2,1,2,
   4,3,3,4,
    5,4,5,
   6,5,5,6,
    7,6,7,
   8,7,7,8,
    9,8,9,
      9,
};

// centerstripe: a center stripe
byte centerstripe[50] = {
      1,
    0,1,0,
   0,0,1,0,
    0,1,0,
   0,1,0,0,
    0,1,0,
     0,1,
    0,1,0,
     1,0,
    0,1,0,
   0,0,1,0,
    0,1,0,
   0,1,0,0,
    0,1,0,
   0,0,1,0,
    0,1,0,
      1,
}; 

void setup() {
  //Serial.begin(9600);
  //Serial.println("Start");
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
  
  switch(random(13)) {
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
      randomcolors(random(5,30)*10000);
      break;
    case 4:
      twocolor(random(1,30)*10000);
      break;
    case 5:
      rainbowshow(random(100,500));
      break;
    case 6:
      hogshell(random(10,30));
      break;
    case 7:
      lightwave(random(2,10));
      break;
    case 8:
      bullseye(random(1,30)*10000);
      break;
    case 9:
      chevronrainbow(random(1,30)*10000);
      break;
    case 10:
      stripe(random(20,100));
      break;
    case 11:
      chevronfill(random(25,100));
      break;
    case 12:
      starburst(random(50,200));
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
  
    if (!random(20)) { if (wait++ > maxWait) wait = 0; }
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
 
void stripe(int cycles) {
  int randcolor = random(MaxColor); // pick a random starting color
  
  for (int count = 0; count < cycles; count++) {
   for (int i=0; i < numLights; i++) {
     if (centerstripe[i]) { setPixelColor(SpineOrder[i], Wheel(randcolor));
     } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
   }
   morph_frame(GetDelayTime(wait));
  
   if (!random(10)) { if (wait++ > maxWait) wait = 0; }
   if (randcolor+=10 > MaxColor) randcolor = randcolor - MaxColor;  
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
 * twocolor: turns each pixel on to either of two random colors
 */

void twocolor(long time) {
  int color1, color2, maxspacing;
  color1 = random(MaxColor);  // Set up color1
  color2 = color1 + random(50,MaxColor/2);  // Set up color2
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
      setPixelColor(SpineOrder[j], Wheel(colors[centering[j]]));
  }
  morph_frame(1000);
  delay(time);
  clearWithFade(1000);
}

/*
 * chevronrainbow: lights up the chevron with a rainbow pattern with 10 colors
 */

void chevronrainbow(long time) {
  int colors[10];
  int randcolor;
  int colorspacing;
  
  randcolor = random(MaxColor); // Initial color is random
  colorspacing = random(2, MaxColor/11);
  
  for (int i=0; i<10; i++) {
    colors[i] = ((i*colorspacing) + randcolor) % MaxColor;
  }
  
  for (int j=0; j < strip.numPixels(); j++) {
      setPixelColor(SpineOrder[j], Wheel(colors[chevpattern[j]]));
  }
  morph_frame(1000);
  delay(time);
  clearWithFade(1000);
}

/*
 * randomfill: randomly fills in pixels from blank to all on
 * then takes them away random until blank
 */
 
void randomfill(int cycles) {
  int i, j, save;
  byte shuffle[numLights];  // Will contain a shuffle of 0 to totPixels
  int randcolor;
  
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
      strip.setPixelColor(SpineOrder[i], Wheel(((i*256 / strip.numPixels()) + frame) % 256) );
    }  
    strip.show();   // write all the pixels out
    delay(50);
    if (frame+=15 > MaxColor) frame = frame - MaxColor;
  }
}

/*
 * starburst: burst from center to outer ring. Code is identical to chevrons
 */
 
void starburst(int cycles) {
  int chev, i, randcolor;
  randcolor = random(0, MaxColor); // pick a random starting color
    
  for (int count = 0; count < cycles; count++) {
    for (chev=0; chev < 11; chev++) {
      for (i=0; i < numLights; i++) {
        if (starburstpattern[i] == chev) {
          setPixelColor(SpineOrder[i], Wheel((randcolor+(starburstpattern[i]*3))%MaxColor));
        } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
    }
    if (!random(3)) { if (wait++ > maxWait) wait = 0; }
  }
}

/*
 * chevrons: upward chevrons
 */
 
void chevrons(int cycles) {
  int chev, i, randcolor;
  randcolor = random(0, MaxColor); // pick a random starting color
    
  for (int count = 0; count < cycles; count++) {
    for (chev = 0; chev < 11; chev++) {
      for (i=0; i < numLights; i++) {
        if (chevpattern[i] == chev) {
          setPixelColor(SpineOrder[i], Wheel(randcolor));
        } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
    }
    if (!random(3)) { if (wait++ > maxWait) wait = 0; }
  }
}

/*
 * chevronfill: upward chevrons that fill and fall back
 */
 
void chevronfill(int cycles) {
  int chev, i, randcolor;
  randcolor = random(MaxColor); // pick a random starting color
    
  for (int count = 0; count < cycles; count++) {
    for (chev=-1; chev < 10; chev++) {
      for (i=0; i < numLights; i++) {
        if (chev >= chevpattern[i]) {
          setPixelColor(SpineOrder[i], Wheel((randcolor+(chevpattern[i]*2))%MaxColor));
        } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
    }
    for (chev=8; chev >= 0; chev--) {
      for (i=0; i < numLights; i++) {
        if (chev >= chevpattern[i]) {
          setPixelColor(SpineOrder[i], Wheel((randcolor+(chevpattern[i]*2))%MaxColor));
        } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
    }
    if (!random(3)) { if (wait++ > maxWait) wait = 0; }
  }
}

void hogshell(int cycles) {
  int randcolor = random(MaxColor); // pick a random starting color
  int s[4] = { 2, 10, 50, 255 };
  float intensity;
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      intensity = s[centering[i]] / 255.0;
      setPixelColor(SpineOrder[i], Gradient_Wheel(randcolor, intensity));
    }
    morph_frame(GetDelayTime(wait));
    if (!random(100)) { if (wait++ > maxWait) wait = 0; }
    if (randcolor++ > MaxColor) randcolor = randcolor - MaxColor; 
  }
  clearWithFade(1000);
}

/*
 * lightwave - Just one pixel traveling from 0 to 49
 */
 
void lightwave(int cycles) {
  int randcolor = random(MaxColor); // pick a random starting color
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      for (int j=0; j < strip.numPixels(); j++) {
        if (i == j) { setPixelColor(SpineOrder[j], Wheel(randcolor)); }
        else { setPixelColor(SpineOrder[j], Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
      if (!random(100)) { if (wait++ > maxWait) wait = 0; }
      if (!random(2)) {
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
  
  intensity *= 0.25;
  
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
