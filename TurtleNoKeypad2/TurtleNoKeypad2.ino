#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Turtle! - 29 Lights in a hexagonal grid. This version does not use a keypad
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

byte numLights = 29;                 // Number of hexagonal pixels
byte halfLights = (numLights/2)+1;   // Half that number

#define numShows 5      // number of shows

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Turtle colors
byte currColor  = 85;   // Starting foreground color
byte currColor2 = 170;  // Starting background color
#define MaxColor 255    // Colors go from 0 to 255

int neighbors[][6] = {
  {-1,1,8,9,-1,-1},
  {-1,2,7,8,0,-1},
  {-1,-1,3,7,1,-1},
  {-1,-1,4,6,7,2},
  {-1,-1,-1,5,6,3}, // 4
  {4,-1,-1,14,13,6},
  {3,4,5,13,12,7},
  {2,3,6,12,8,1},
  {1,7,12,11,9,0},
  {0,8,11,10,-1,-1},
  {9,11,18,19,-1,-1},
  {8,12,17,18,10,9},
  {7,6,13,17,11,8}, // 12
  {6,5,14,16,17,12},
  {5,-1,-1,15,16,13},
  {14,-1,-1,24,23,16},
  {13,14,15,23,22,17}, // 16
  {12,13,16,22,18,11},
  {11,17,22,21,19,10},
  {10,18,21,20,-1,-1},
  {19,21,27,-1,-1,-1},
  {18,22,26,27,20,19},
  {17,16,23,26,21,18}, // 22
  {16,15,24,25,26,22},
  {15,-1,-1,-1,25,23},
  {23,24,-1,-1,28,26},
  {22,23,25,28,27,21},
  {21,26,28,-1,-1,20},
  {26,25,-1,-1,-1,27} // 28
};

int rings[][12] = {
  { 17, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },
  { 12, 13, 16, 22, 18, 11, -1,-1,-1,-1,-1,-1 },
  { 7, 6, 5, 14, 15, 23, 26, 21, 19, 10, 9, 8 },
  { 0, 1, 2, 3, 4, 24, 25, 28, 27, 20, -1, -1 } 
};

#define NUM_PATTERNS 9   // Total number of patterns
// Patterns
//
// 0 = Off
// 1 = Color 1
// 2 = Color 2
  
byte PatternMatrix[NUM_PATTERNS][29] = {
    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1 },
    { 1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,1 },
    { 1,1,1,1,1,1,2,2,2,1,1,2,1,2,1,1,2,2,2,1,1,2,1,2,1,1,2,1,1 },
    { 1,1,1,1,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,1,1 },
    { 1,1,1,1,1,1,2,2,2,2,2,1,1,1,2,2,1,1,1,2,1,2,1,2,1,1,2,1,1 },
    { 2,1,1,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,2 },
    { 1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,1,1,1,2,1,2,1,2,1,1,2,1,1 },
    { 1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,2,1,2,1 } 
};
  
// centering array
//
// 3 = pixel in the center
// 2 = pixel in the middle ring
// 1 = pixel in the outer ring

int centering[29] = { 1,1,1,1,1,1,2,2,2,1,1,2,3,2,1,1,2,3,2,1,1,2,3,2,1,1,2,1,1 };
  
// Shows
int show=6;    // default starting show
int f=0;       // indices to keep track of position within a show
int x=0;       // another counter
int temp;      // temporary place holder
int wait=6;    // index to keep track of delay time
#define maxWait 13     // Number of stored delay times 

void setup() {
//  Serial.begin(9600);

  // not sure if this is necessary but try it anyway
  randomSeed(analogRead(0));
     
  // Start up the LED counter
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
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
 * visit each light in order and illuminate its neighbors
 * ignores buffering
 */
void unit_test() {
  int tmp=29;
  
  for (int i=0; i<tmp; i++) {
    clear();
    strip.setPixelColor(i, Color(255,0,0));
    
    int *n = neighbors[i];
    for (int j=0; j<6; j++) {
      if (n[j] != -1) {
        strip.setPixelColor(n[j], Color(0,255,0));
        
        strip.show();
        delay(100);
      }
    }
    delay(500);    
  }
}

void bounce(int cycles) {
  int ticks = 0;
  
  int dir = random(0,6);
  int pos = random(0,numLights);
  int trail_size = 5;
  int trail[] = { -1, -1, -1, -1, -1 };
  int trail_colors[] = { 205, 145, 85, 45, 5 };
  
  while (ticks++ < cycles) {
    fill(Color(0,0,2));
    for (int i = trail_size - 1; i >= 0; i--) {
      if (trail[i] == -1) continue;
      setPixelColor(trail[i], Color(0, 0, trail_colors[i]));
    }
    setPixelColor(pos, Color(200, 0, 128));
    morph_frame(250, 8);
    
    int *n = neighbors[pos];
    int old_dir = dir;
    while (n[dir] == -1 || (dir + 3) % 6 == old_dir) {
      dir = random(0,6);
    }
    for (int i = trail_size - 1; i >= 0; i--) {
      trail[i] = trail[i - 1];
    }
    trail[0] = pos;
    pos = n[dir];
  }  
}

void bounce_glowing(int cycles) {
  int ticks=0;
  
  int dir = random(0, 6);
  int pos = random(0, numLights);
  int glow_colors[] = { 255, 35, 9 };
  
  while (ticks++ < cycles) {
    fill(Color(0, 2, 2));
    for (int i = 0; i < 6; i++) {
      int x = neighbors[pos][i];
      if (x == -1) continue;
      for (int j = 0; j < 6; j++) {
        int xx = neighbors[x][j];
        if (xx == -1) continue;
        setPixelColor(xx, Color(0, glow_colors[2], glow_colors[2]));
      }
    }
    for (int i = 0; i < 6; i++) {
      int x = neighbors[pos][i];
      if (x == -1) continue;
      setPixelColor(x, Color(0, glow_colors[1], glow_colors[1]));
    }
    setPixelColor(pos, Color(0, glow_colors[0], glow_colors[0]));
    morph_frame(300, 25);
    
    int *n = neighbors[pos];
    int old_dir = dir;
    while (n[dir] == -1 || (dir + 3) % 6 == old_dir) {
      dir = random(0,6);
    }
    pos = n[dir];
  }  
}

void draw_ring(int i, uint32_t color) {
  int *r = rings[i];
  for (int j=0; j<12; j++) {
    setPixelColor(r[j], color);
  }
}

/*
 Colored ring animating outwards
 c1 is the primary color, c2 is a trail color
 bg is the background color
*/
void tunnelvision(uint32_t c1, uint32_t c2, uint32_t bg) {  
  for (int i=0; i<5; i++) {
    fill(bg);
    
    if (i < 4) {
      draw_ring(i, c1);
    }      
    if (i != 0) {
        draw_ring(i-1, c2);
    }
    
    draw_frame(200);
// XXX morph_frame seems broken with this pattern
//    morph_frame(250,8); 
  }
}

// framebuffer
uint32_t current_frame[29];
uint32_t next_frame[29];

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

void morph_frame(int msec, int steps) {
  int deltas[29][3];
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
    delay(msec / steps);
  }
  for (int i = 0; i < numLights; i++) {
    current_frame[i] = next_frame[i];
    strip.setPixelColor(i, current_frame[i]);
  }
  strip.show();
  delay(msec / steps);
}

void setPixelColor(int pos, uint32_t color) {
  next_frame[pos] = color;
}

void fill(uint32_t color) {
  for (int i = 0; i < numLights; i++) {
    setPixelColor(i, color);
  }
}

// colors on a black field
void warp1(int cycles) {
  for (int i=0; i<cycles; i++) {
    tunnelvision(Color(0, 255, 0), Color(0,40,0), Color(0,0,0));
    tunnelvision(Color(0,0,255), Color(0,0,40), Color(0,0,0));
    tunnelvision(Color(0,255,255), Color(0,40,40), Color(0,0,0));
    tunnelvision(Color(255,255,0), Color(40,40,0), Color(0,0,0));  
    tunnelvision(Color(255,0,0), Color(40,0,0), Color(0,0,0));
    tunnelvision(Color(255,0,255), Color(40,0,40), Color(0,0,0));  
  }
}

// colors on a green field
void warp2(int cycles) {
  for (int i=0; i<cycles; i++) {
    tunnelvision(Color(0,255,100), Color(0,80,40), Color(0,40,0));
    tunnelvision(Color(0,200,100), Color(0,80,40), Color(0,40,0));
    tunnelvision(Color(0,150,100), Color(0,80,40), Color(0,40,0));
    tunnelvision(Color(0,100,100), Color(0,80,40), Color(0,40,0));
  
    tunnelvision(Color(255,255,0), Color(80,80,0), Color(0,40,0));
    tunnelvision(Color(200,200,0), Color(80,80,0), Color(0,40,0));
    tunnelvision(Color(150,150,0), Color(80,80,0), Color(0,40,0));
    tunnelvision(Color(100,100,0), Color(80,80,0), Color(0,40,0));  
  }
}

void loop() {
   // Start with a boring turtle shell for a while
  patterns(random(1,10)*1000); // 100 is about 6 seconds
  
  // Then a fun random shows
  switch(random(0,6)) {
    case 0:    
      warp1(1);
      break;
    case 1:
      rainbow_show(5000); // 5000 is about 3.5 minutes (210 sec)
      break;
    case 2:
      warp2(1);
      break;
    case 3:
      bounce(100);
      break;
    case 4:
      bounce_glowing(100);
      break;
    case 5:
      color_gradient_show(1000); // 100 is about 15 seconds
      break;
  }
  
  clear();
  strip.show();   
}
/* Not used
void running_lights_show(int cycles) { // show 0
  int ticks = 0;
  
  f=0;  // reset the frame position to 0
  x=0;  // reset the floating index to 0
  
  while (ticks++ < cycles) {
      
      if (x<1 || x>halfLights) x=1;  // Sets bounds for number of running lights
      if (!random(1000)) {  // CENTER = add another running light
        if (x++>halfLights) x=1;
      }
      if (!random(1000)) { // UP = slow speed
        if (wait++ >= maxWait) wait=0;
      }
      if (!random(2)) {       // LEFT = change foreground color
        if (currColor++ >= MaxColor) currColor = 0;
      }
      if (!random(3)) {     // RIGHT = change background color
        if (currColor2++ >= MaxColor) currColor2 = 0;
      }
      LightRunUp(f, x, Wheel(currColor), Wheel(currColor2), GetDelayTime(wait)); // post frame of show
      if (f++ >= halfLights) f=0;      // advance frame. Set to 0 if at end
        
  }
}
*/
void rainbow_show(int cycles) {
  int ticks = 0;
  
  f=0;  // reset the frame position to 0
  x=0;  // reset the floating index to 0
 
  while (ticks++ < cycles) {
      if (x<1 || x>halfLights/2) x=1;  // Sets bounds for number of color divisions
      if (f++ >= MaxColor) f=0;      // advance frame. Set to 0 if at end
      
      if (!random(1000)) { // LEFT = slow speed
        if (wait < maxWait) wait++ ;
      }
      if (!random(1000)) { // RIGHT = increase speed
        if (wait > 1) wait--;
      }
      if(!random(1000)) { x++; }  // UP = increase color divisions
      
      rainbowCycle(f, x, GetDelayTime(wait)/5);  //  run Rainbow show
  }  
}
/* Not used
void lightwaves_show(int cycles) {
  int ticks = 0;
  
  f=0;  // reset the frame position to 0
  x=0;  // reset the floating index to 0
    
  while (ticks++ < cycles) {
      if (x<0 || x>=halfLights) x=0;  // Sets bounds for number of running lights
      if (f<0 || f>=halfLights) f=0;  // Sets bounds for frame of show
 
      if (!random(2)) {       // UP = change color
        if (currColor++ >= MaxColor) currColor = 0;
      }
      if (!random(1000)) { // LEFT = slow speed
        if (wait++ >= maxWait) wait=0;
      }
      if (!random(3)) { // RIGHT = change background color
        if (currColor2++ >= MaxColor) currColor2 = 0;
      }
      
      LightWave(f, x, Wheel(currColor), Wheel(currColor2), GetDelayTime(wait)); // run the light wave
      f++;
      if (f>=(halfLights-x)) {   // At end of frame
        x++;      // increment x
        f=0;      // reset frame to x
        if (x>=halfLights) {  // Filled up all pixels
          x=0;  // start over
          f=0;  // start at first fram
          if (currColor++ >= MaxColor) currColor = 0;
        }
      }
  }
}
*/
void patterns(int cycles) {
  int pattern = random(0, NUM_PATTERNS);    // Pick a random pattern
  int i, ticks = 0;                         // One color is always green
  
  while (ticks++ < cycles) {
      if (!random(10)) {       // The not-green color
        if (currColor++ >= MaxColor) currColor = 0;  
      }
        
      for (i=0; i < strip.numPixels(); i++) {
        switch (PatternMatrix[pattern][i]) {
          case 0: {        // Off (black)
            strip.setPixelColor(i, Color(0,0,0));
            break;
          }
          case 1: {        // Green
            strip.setPixelColor(i, GradientWheel(Color(0,255,0),centering[i]));
            break;
          }
          case 2: {        // The other color
            strip.setPixelColor(i, GradientWheel(currColor,centering[i]));
            break;
          }
        }  // Gradient Wheel turns a Color into r,g,b but attenuates from center to edge
      }
      strip.show();
      delay(wait);
  }  
}

void color_gradient_show(int cycles) {
  int ticks = 0;
  
  f=0;  // reset the frame position to 0
  x=0;  // reset the floating index to 0  
    
  while (ticks++ < cycles) {
      if (f<0 || f>=halfLights) f=0;  // Sets bounds for frame of show
      if (!random(1000)) {     // CENTER = slow speed
        if (wait++ >= maxWait) wait=0;
      }
      if(!random(2)) {          // UP = change start of gradient
        if (currColor++ >= MaxColor) currColor = 0;
      }
     
      if (!random(3)) {     // RIGHT = increase width of gradient
        if (currColor2++ >= MaxColor) currColor2 = 0;
      }
      GradientLights(f, currColor, currColor2, GetDelayTime(wait));  // Last number is a delay
      f++;    
  }
}

// XXX old stuff below

/*
 * GradientLights - Makes a gradient of light from one color
 *                  to the other
 */
void GradientLights(uint8_t frame, byte cinit, byte clength, uint8_t wait) {
  int i;
  int c = cinit;
  byte cspacing = clength/halfLights;
  
  for (i=0; i< halfLights; i++) {
    strip.setPixelColor(frame, Wheel(c));
    strip.setPixelColor(numLights-1-frame, Wheel(c));
    if (frame++ >= halfLights) { frame = 0; }
    c = ((c + cspacing) % 255);
  }
  strip.show();
  delay(wait);
}

/*
 * LightWave - Lights a selection
 */ 
void LightWave(uint8_t frame, uint8_t wavepos, uint32_t c, uint32_t c2, uint8_t wait) {
  int i;
  for (i=0; i < halfLights; i++) {
    if (i>(halfLights-wavepos-1) || i==frame) {
      strip.setPixelColor(i, c);
      strip.setPixelColor(numLights-1-i, c);
    } 
    else {
      strip.setPixelColor(i, c2);
      strip.setPixelColor(numLights-1-i, c2);
    }
  }
  strip.show();
  delay(wait);
}

/*
 * Get Delay Time - Returns a delay time from an array.
 */
 
int GetDelayTime(int wait) {
  int DelayValues[maxWait] = { 10, 20, 40, 60, 80, 100, 150, 200, 300, 500, 750, 1000, 2000 };
  if (wait < maxWait) { return (DelayValues[wait]); } else { return (DelayValues[maxWait]); }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t frame, uint8_t division, uint8_t wait) {
  int i;
  for (i=0; i < halfLights; i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / numLights part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
     strip.setPixelColor(i, Wheel(((i*256*division / numLights) + frame) % 256) );
     strip.setPixelColor(numLights-1-i, Wheel(((i*256*division / numLights) + frame) % 256) );
  }  
  strip.show();   // write all the pixels out
  delay(wait);
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint32_t c, uint8_t wait) {
  int i;
  
  for (i=0; i < halfLights; i++) {
      strip.setPixelColor(i, c);
      strip.setPixelColor(numLights-1-i, c);
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
//3=full strength, 2=2/3rd strength, 1=1/3rd strength, 0=off
uint32_t GradientWheel(byte WheelPos, int strength)
{
  int r,g,b;
  int s[4] = { 0, 2, 10, 100 };  // This is the attenuation percent
  
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
