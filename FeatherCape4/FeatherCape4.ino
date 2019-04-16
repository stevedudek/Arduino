#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Feather Cape - 38 Petals on a cape
//
//  5/23/17
//
//  Standard rgb colors - no HSV
//
//  Has a brightness setting: BRIGHTNESS
//
/*****************************************************************************/
//
//  Set Up Pins
//
//  2 pins for clock and data of lights.
// 
#define dataPin 9       // 'MAIN_COLOR' wire
#define clockPin 8      // 'green' wire

#define numLights 38
#define numRows 11
#define numCols 4

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// For random-fill show (nasty global)
int shuffle[numLights];  // Will contain a shuffle of lights

// Set the first variable to the NUMBER of pixels: here 48: 10 spacer pixels + 38 petals
Adafruit_WS2801 strip = Adafruit_WS2801(48, dataPin, clockPin);

// User-adjustable toggle values
boolean ONLY_RED = false; // true = use only red colors
#define MAIN_COLOR 120  // MAIN_COLOR=120, green=450
boolean USE_DIMMER = false;  // true = use dimmer
float BRIGHTNESS = 0.75;  // 0.0 - 1.0 brightness level
int BLINK_FACTOR = 1;     // how fast blinking (1 = fast, higher = slower)

// Light colors

#define MAIN_COLOR 120  // MAIN_COLOR=120, green=450
#define maxColor 1530     // Colors are 6 * 255
int foreColor =   MAIN_COLOR;    // Starting foreground color
int backColor =   0;    // Starting background color

// Dimmer
#define dimmerPin 5
int dimmerValue = 1024;
#define MIN_DIMMER_CHANGE 0.1 // fractional minumum recognized by the dimmer

// Shows

#define MAX_SHOW 15  // Total number of shows
byte curr_show = random(MAX_SHOW); // Starting show

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated

// Delays
int wait = 0;
#define MAX_WAIT 12 // Number of stored delay times 

// Plinko
#define NUM_PLINKO 5
byte plink_x[NUM_PLINKO] = { 0,0,0,0,0 };
byte plink_y[NUM_PLINKO] = { 12,12,12,12,12 };

// Bounce
int bounce_dir = random(0,6);
int bounce_pos = random(0,numLights);
int trail_size = 5;
int trail[] = { -1, -1, -1, -1, -1 };
int trail_colors[] = { 205, 145, 85, 45, 5 };
int glow_colors[] = { 255, 35, 9 };
  
// Lookup tables

byte PetalLookUp[38] = {
     0,  1,  2,
   7,  6,  5,  4,
     9, 10, 11,
  16, 15, 14, 13,
    18, 19, 20,
  25, 24, 23, 22,
    27, 28, 29,
  34, 33, 32, 31,
    36, 37, 38,
  43, 42, 41, 40,
    45, 46, 47
};

#define NUM_SPACERS 10
byte SpacerPixels[NUM_SPACERS] = { 3, 8, 12, 17, 21, 26, 30, 35, 39, 44 };

byte PetalGrid[11][4] = {
 {    0,  1,  2,   38},
 {  3,  4,  5,  6,   },
 {    7,  8,  9,   38},
 { 10, 11, 12, 13,   },
 {   14, 15, 16,   38},
 { 17, 18, 19, 20,   },
 {   21, 22, 23,   38},
 { 24, 25, 26, 27,   },
 {   28, 29, 30,   38},
 { 31, 32, 33, 34,   },
 {   35, 36, 37,   38}
};

int neighbors[][6] = {
  {-1,1,4,3,-1,-1}, // 0
  {-1,2,5,4,0,-1},
  {-1,-1,6,5,1,-1},
  {0,4,7,-1,-1,-1},
  {1,5,8,7,3,0}, // 4
  {2,6,9,8,4,1},
  {-1,-1,-1,9,5,2},
  {4,8,11,10,-1,3},
  {5,9,12,11,7,4}, // 8
  {6,-1,13,12,8,5},
  {7,11,14,-1,-1,-1},
  {8,12,15,14,10,7},
  {9,13,16,15,11,8}, // 12
  {-1,-1,-1,16,12,9},
  {11,15,18,17,-1,10},
  {12,16,19,18,14,11},
  {13,-1,20,19,15,12}, // 16
  {14,18,21,-1,-1,-1},
  {15,19,22,21,17,14},
  {16,20,23,22,18,15},
  {-1,-1,-1,23,19,16}, // 20
  {18,22,25,24,-1,17},
  {19,23,26,25,21,18},
  {20,-1,27,26,22,19},
  {21,25,28,-1,-1,-1}, // 24
  {22,26,29,28,24,21},
  {23,27,30,29,25,22},
  {-1,-1,-1,30,26,23},
  {25,29,32,31,-1,24}, // 28
  {26,30,33,32,28,25},
  {27,-1,34,33,29,26},
  {28,32,35,-1,-1,-1},
  {29,33,36,35,31,28}, // 32
  {30,34,37,36,32,29},
  {-1,-1,-1,37,33,30},
  {32,36,-1,-1,-1,31},
  {33,37,-1,-1,35,32}, // 36
  {34,-1,-1,-1,36,33}
};

#define NUM_PATTERNS 8   // Total number of patterns
// Patterns
//
// 0 = Off
// 1 = Color 1
// 2 = Color 2

byte PatternMatrix[NUM_PATTERNS][38] = {
  {  1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1    },
  
  {  1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1    },
     
  {  2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2    },
  
  {  1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2,
   2,  1,  1,  2,
     1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2,
   2,  1,  1,  2,
     1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2    },
  
  {  1,  1,  1,
   1,  1,  1,  1,
     1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   1,  2,  2,  1,
     1,  2,  1,
   1,  1,  1,  1,
     1,  1,  1    },
     
  {  1,  2,  1,
   2,  1,  2,  1,
     2,  1,  2,
   1,  2,  1,  2,
     1,  2,  1,
   2,  1,  2,  1,
     2,  1,  2,
   1,  2,  1,  2,
     1,  2,  1,
   2,  1,  2,  1,
     2,  1,  2    },
   
  {  1,  2,  1,
   1,  2,  1,  2,
     2,  1,  2,
   2,  1,  2,  1,
     1,  2,  1,
   1,  2,  1,  2,
     2,  1,  2,
   2,  1,  2,  1,
     1,  2,  1,
   1,  2,  1,  2,
     2,  1,  2    },
  
  {  1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1    },
     
};

//
// Setup
//
void setup() {
  
  Serial.begin(9600);
  Serial.println("Start");
  
  randomSeed(analogRead(0));
  
  // Start up the LED counter
  strip.begin();

  // Update the strip, to start they are all 'off'
  clear();
  strip.show();
}

void loop() { 
   
  delay(10);   // The only delay!

  check_dimmer();
  
  // Check if the lights need updating
  
  if (update) {
    
    update = false;
    
    switch(curr_show) {      
      case 0:    
        allOn();
        break;
      case 1:
        randomfill();
        break;
      case 2:
        randomcolors();
        break;
      case 3:
        twocolor();
        break;
      case 4:
        lightrunup();
        break;
      case 5:
        rainbow();
        break;
      case 6:
        updown();
        break;
      case 7:
        updownfill();
        break;
      case 8:
        leftright();
        break;
      case 9:
        leftrightfill();
        break;
      case 10:
        plinko();
        break;
      case 11:
        rainbow_cycle();
        break;
      case 12:
        bounce();
        break;
      case 13:
        bounce_glowing();
        break;
      case 14:
        scales();
        break;
    }
  }
  
  // Morph the display
  
  morph_frame();
  
  // Advance the clock
  
  if (morph++ >= GetDelayTime(wait)) {  // Finished morphing
    update = true;  // Force an update
    push_frame();
    morph = 0;  // Reset morphClock

    if (cycle++ > 1000) cycle = 0;  // Advance the cycle clock
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(10)) {
    foreColor = IncColor(foreColor, 10);
  }
  
  if (!random(2)) {
    backColor = IncColor(backColor, -5);
  }
  
  if (!random(10000)) {
    curr_show = random(MAX_SHOW);
    morph = 0;
    cycle = 0;
    clearWithFade();
    patterns(random(5,10) * 100); // Pattern shows!
  }
  
  if (!random(1000)) {
    wait++;
    if (wait >= MAX_WAIT) {
      wait = 0;
      morph = 0;
    }
  }
}

//
// push_frame()
//
void push_frame() {
  for (int i = 0; i < numLights; i++) {
    current_frame[i] = next_frame[i];
  }
}

//
// patterns shows
//
void patterns(int cycles) {
  int pattern = random(0, NUM_PATTERNS);    // Pick a random pattern
  int i, ticks = 0;                         // One color is always MAIN_COLOR
  
  while (ticks++ < cycles) {
      if (!random(10)) {       // The not-MAIN_COLOR color
        foreColor = IncColor(foreColor, 10); 
      }
        
      for (i=0; i < numLights; i++) {
        switch (PatternMatrix[pattern][i]) {
          case 0: {        // Off (black)
            strip.setPixelColor(PetalLookUp[i], Color(0,0,0));
            break;
          }
          case 1: {        // MAIN_COLOR = 511
            strip.setPixelColor(PetalLookUp[i], Wheel(MAIN_COLOR));
            break;
          }
          case 2: {        // The other color
            strip.setPixelColor(PetalLookUp[i], Wheel(foreColor));
            break;
          }
        }
      }
      strip.show();
      delay(10);
  }  
}

// clear
//
// set all cells to black but don't call show yet
// ignores buffering
// 
void clear() {
  for (int i=0; i < numLights; i++) {
    strip.setPixelColor(PetalLookUp[i], Color(0,0,0));
    current_frame[i] = Color(0,0,0);
    next_frame[i] = Color(0,0,0);
  }
}

//
// fill
//
void fill(uint32_t color) {
  for (int i = 0; i < numLights; i++) {
    setPixelColor(i, color);
  }
}

// clear with fade
//
// Fades lights to black
//
void clearWithFade() {
  fill(Color(0,0,0));
}

//
// All On
//
// Simply turns all the pixels on to one color
// 
void allOn() {
  fill(Wheel(foreColor));
}

//
// rainbow
//
void rainbow() {
   for (int i=0; i < numLights; i++) {
     setPixel(i, (foreColor + (i * 20)) % maxColor );
   }
}

//
// rainbow_cycle - break up a rainbow and partition evenly
//
void rainbow_cycle() {
  int frame = (cycle * 6) % maxColor;
  int division = wait + 1;
  
  for (int i=0; i < (numLights / 2); i++) {
     setPixel(i, ((i*256*division / numLights) + frame) % 256);
     setPixel(numLights - 1 - i, ((i*256*division / numLights) + frame) % 256);
  }  
}

// random fill
//
// randomfill: randomly fills in pixels from blank to all on
// then takes them away random until blank
//
 
void randomfill() {
  int i, j, save, pos;
  
  pos = cycle % (numLights*2);  // Where we are in the show
  if (pos >= numLights) {
    pos = (numLights*2) - pos;  // For a sawtooth effect
  }
  
  if (pos == 0) {  // Start of show
    shuffle_lights();
  }
  
  for (i=0; i < numLights; i++) {
    if (i < pos) {  
      setPixel(shuffle[i], foreColor);  // Turning on lights one at a time
    } else { 
      setPixelBlack(shuffle[i]);  // Turning off lights one at a time
    }
  }
}

//
// scales
//
void scales() {
  if (cycle == 0) {
    shuffle_lights();
  }
  fill(Wheel(MAIN_COLOR));
  
  int num_scales = (shuffle[0] / 3) + 3;
  for (int i=0; i < num_scales; i++) {
    setPixel(shuffle[i], foreColor);
  }
}

//
// shuffle_lights
//
void shuffle_lights() {
  // Shuffle sort to determine order to turn on lights
  int i,j, save;
  
  for (i=0; i < numLights; i++) {
    shuffle[i] = i; // before shuffle
  }
  for (i=0; i < numLights; i++) {  // here's position
    j = random(numLights);         // there's position
    save = shuffle[i];
    shuffle[i] = shuffle[j];       // first swap
    shuffle[j] = save;             // second swap
  }
}


//
// bounce
//
void bounce() {  
  fill(Color(0,0,2));
  
  for (int i = trail_size - 1; i >= 0; i--) {
    if (trail[i] == -1) continue;
    setPixelColor(trail[i], Color(0, 0, trail_colors[i]));
  }
  setPixelColor(bounce_pos, Color(200, 0, 128));
  
  int *n = neighbors[bounce_pos];
  int old_dir = bounce_dir;
  while (n[bounce_dir] == -1 || (bounce_dir + 3) % 6 == old_dir) {
    bounce_dir = random(0,6);
  }
  for (int i = trail_size - 1; i >= 0; i--) {
    trail[i] = trail[i - 1];
  }
  trail[0] = bounce_pos;
  bounce_pos = n[bounce_dir];
}

//
// bounce_glowing
//
void bounce_glowing() {  
  fill(Color(0, 2, 2));
  
  for (int i = 0; i < 6; i++) {
    int x = neighbors[bounce_pos][i];
    if (x == -1) continue;
    for (int j = 0; j < 6; j++) {
      int xx = neighbors[x][j];
      if (xx == -1) continue;
      setPixelColor(xx, Color(0, glow_colors[2], glow_colors[2]));
    }
  }
  for (int i = 0; i < 6; i++) {
    int x = neighbors[bounce_pos][i];
    if (x == -1) continue;
    setPixelColor(x, Color(0, glow_colors[1], glow_colors[1]));
  }
  setPixelColor(bounce_pos, Color(0, glow_colors[0], glow_colors[0]));
  
  int *n = neighbors[bounce_pos];
  int old_dir = bounce_dir;
  while (n[bounce_dir] == -1 || (bounce_dir + 3) % 6 == old_dir) {
    bounce_dir = random(0,6);
  }
  bounce_pos = n[bounce_dir]; 
}


// random colors
//
// randomcolors: turns each pixel on to a random color
//
void randomcolors() {
  int i;
  
  if (cycle == 0) {  // Start of show: assign lights to random colors
    for (i=0; i < numLights; i++) {
      shuffle[i] = random(maxColor);
    }
  }
  
  // Otherwise, fill lights with their color
  for (i=0; i < numLights; i++) {
    setPixel(i, shuffle[i]);
  }
}

// calcIntensity
//
// Use sine wave + cycle + variable to calculate intensity
float calcIntensity(byte x, byte max_x) {
  return sin(3.14 * ((cycle + x) % max_x) / (max_x - 1));
}


// two color
//
// alternates the color of pixels between two colors
//
void twocolor() {
  for (int i=0; i < numLights; i++) {
    if (i % 2) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, backColor);
    }
  }
}


//
// lightrunup
//
// Wave filling in and out

void lightrunup() {
  byte x, y, pos;
  
  pos = cycle % (numRows*2);  // Where we are in the show
  if (pos >= numRows) {
    pos = (numRows*2) - pos;  // For a sawtooth effect
  }
  
  for (y=0; y < numRows; y++) {
    for (x=0; x < getRowWidth(y); x++) {
      if (y >= pos) {
        setPixelBlack(PetalGrid[y][x]);
      } else {
        setPixel(PetalGrid[y][x], IncColor(foreColor, 10 * y));
      }
    }
  }
}

//
// updown: lights moving up & down in opposite columns
//
void updown() {
  byte x, y, pos;
  
  pos = cycle % (numRows*2);  // Where we are in the show
  if (pos >= numRows) {
    pos = (numRows*2) - pos;  // For a sawtooth effect
  }
  
  for (y = 0; y < numRows; y++) {
    for (x = 0; x < 4; x++) {
      if (x % 2 == 0 && y == pos) {
        setPixel(PetalGrid[y][x], foreColor);      
      } else if (x % 2 == 1 && numRows-y-1 == pos) {
        setPixel(PetalGrid[y][x], foreColor);      
      } else {
        setPixelBlack(PetalGrid[y][x]);
      }
    }
  }
}

//
// updownfill: lights filling up & down in opposite columns
//
void updownfill() {
  byte x, y, pos;
  
  pos = cycle % (numRows*2);  // Where we are in the show
  if (pos >= numRows) {
    pos = (numRows*2) - pos;  // For a sawtooth effect
  }
  
  for (y=0; y < numRows; y++) {
    for (x=0; x < 4; x++) {
      if (x % 2 == 0 && y >= pos) {
        setPixel(PetalGrid[y][x], foreColor);      
      } else if (x % 2 == 1 && (numRows-y-1) >= pos) {
        setPixel(PetalGrid[y][x], backColor);      
      } else {
        setPixelBlack(PetalGrid[y][x]);
      }
    }
  }
}

//
// leftright
//
//
void leftright() {
  byte x, y, pos;
  
  clearWithFade();
  
  // 3-wide rows
  pos = cycle % 4;
  
  for (y = 0; y < 6; y++) {
    if (pos == 3) {
      setPixel(PetalGrid[y*2][1], IncColor(foreColor, 20 * y));
    } else {
      setPixel(PetalGrid[y*2][pos], IncColor(foreColor, 20 * y));
    }
  }
  
  // 4-wide rows
  pos = cycle % 6;
  
  for (y = 0; y < 5; y++) {
    if (pos < 4) {
      setPixel(PetalGrid[(y*2)+1][pos], IncColor(foreColor, -20 * y));
    } else {
      setPixel(PetalGrid[(y*2)+1][6 - pos], IncColor(foreColor, -20 * y));
    }
  }
}


//
// updownfill: lights filling up & down in opposite columns
//
void leftrightfill() {
  byte x, y, pos;
  
  clearWithFade();
  
  pos = cycle % 6;  // 3-wide rows
  if (pos > 3) {
    pos = 6 - pos;
  }
  for (y = 0; y < 6; y++) {
    for (x = 0; x < pos; x++) {
      setPixel(PetalGrid[y*2][x], foreColor);
    }
  }
  
  pos = cycle % 8;  // 4-wide rows
  if (pos > 4) {
    pos = 8 - pos;
  }
  for (y = 0; y < 5; y++) {
    for (x = 0; x < pos; x++) {
        setPixel(PetalGrid[(y*2)+1][3-x], backColor);
    }
  }
}

//
// plinko
//
void plinko() {
  // Start a new plinko
  if (!random(8)) {
    for (int i = 0; i < NUM_PLINKO; i++) {
      if (plink_y[i] >= numRows) {  // off the board
        plink_x[i] = random(4);
        plink_y[i] = 0;  // Start at top
        break;
      }
    }
  }
  
  // Move existing plinko
  clearWithFade();
  
  for (int i = 0; i < NUM_PLINKO; i++) {
    if (plink_y[i] < numRows) {  // is on the board?
      setPixel(PetalGrid[plink_y[i]][plink_x[i]], IncColor(foreColor, 100 * i)); // Draw the plinko
      
      if (getRowWidth(plink_y[i]) == 3) {
        plink_x[i] = plink_x[i] + random(0,2);
      } else {
        switch (plink_x[i]) {
          case 0:
            plink_x[i] = 0;
            break;
          case 3:
            plink_x[i] = 2;
            break;
          default:
            plink_x[i] = plink_x[i] - random(0,2);
            break;
        }     
      }
      plink_y[i] = plink_y[i] + 1;
    }
  }
}
      
      
//
// getRowWidth
//
byte getRowWidth(byte row) {
  if (row % 2 == 0) {
    return 3;
  } else {
    return 4;
  }
}

//
// check_dimmer - see whether the dimmer pot has changed - if so, adjust BRIGHTNESS
//
void check_dimmer() {
  if (USE_DIMMER) {
    int new_value = analogRead(dimmerPin);
    if (((abs(new_value - dimmerValue) / 1024.0) >= MIN_DIMMER_CHANGE) || ((new_value >= 1020) && (dimmerValue < 1000)) || ((new_value < 20) && (dimmerValue > 50))) {
      dimmerValue = new_value;
      BRIGHTNESS = dimmerValue / 1024.0;
    }
  }
}

//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye
//
void morph_frame() {
   float fract = (float)morph/GetDelayTime(wait);
   if (fract > 1.0) {
     fract = 1.0;
   } else if (fract < 0) {
     fract = 0;
   }
   for (int i = 0; i < numLights; i++) {
     strip.setPixelColor(PetalLookUp[i], RGBinter24(current_frame[i], next_frame[i], fract));
   }
   // Make sure spacer pixels are off
   for (int i = 0; i < NUM_SPACERS; i++) {
     strip.setPixelColor(SpacerPixels[i], 0);
   }
   strip.show();  // Update the display 
}

//
// setPixel
//
void setPixel(byte pos, int color) {
  next_frame[pos] = Wheel(color);
}

void setPixelColor(byte pos, uint32_t color) {
  next_frame[pos] = color;
}

void setPixelBlack(byte pos) {
  next_frame[pos] = Color(0,0,0);
}
 
//
// IncColor
//
// Adds amount to color
//
int IncColor(int color, int amount) {
  int value = color + amount;
  while (value < 0) value += maxColor;
  while (value >= maxColor) value -= maxColor;
  return value;
}

//
// Get Delay Time
//
// Returns a delay time from an array
//
 
int GetDelayTime(int wait) {
  int DelayValues[MAX_WAIT] = { 4, 8, 12, 20, 30, 40, 50, 60, 75, 100, 125, 150 };
  return (DelayValues[(wait % MAX_WAIT)] * BLINK_FACTOR );
}

/* Helper functions */

// Color
//
// Create a 24-bit color value from R,G,B
//
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c = 0;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

// Extracts the red part of a 24-bit color
byte GetRed(uint32_t c)
{
  return ((c >> 16) & 0xff);
}

// Extracts the green part of a 24-bit color
byte GetGreen(uint32_t c)
{
  return ((c >> 8) & 0xff);
}

// Extracts the green part of a 24-bit color
byte GetBlue(uint32_t c)
{
  return (c & 0xff);
}

// If r=g=b=0, return true
boolean IsBlack(byte r, byte g, byte b)
{
  if(r=g=b=0) return (true);
  else return (false);
}


//Input a value 256 * 6 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(int color)
{
  return Gradient_Wheel(color, 1);  // Intensity = 1
}

//
// Gradient Wheel - Input a value (255 * 6) to get a color value.
// Modulate by intensity (0.0 - 1.0)
//
uint32_t Gradient_Wheel(int color, float intensity)
{
  int r,g,b;
  int channel, value;
  
  color = color % maxColor;  // Keep colors within bounds  
  
  channel = color / 255;
  value = color % 255;

  if (ONLY_RED) {
    color = color * 2 / 3;  // Break color into 4 (not 6) channels
  }
  
  if (intensity > 1) intensity = 1;
  if (intensity < 0) intensity = 0;

  intensity *= BRIGHTNESS;

  if (ONLY_RED) {       // RED PARTY
    switch(channel)
    {
      case 0:
        r = 255;
        g = value;
        b = 0;        
        break;
      case 1:
        r = 255;
        g = 255 - value;
        b = 0;        
        break;
      case 2:
        r = 255;
        g = 0;
        b = value;        
        break;
      default:
        r = 255;
        g = 0;
        b = 255 - value;        
        break;
     }
     
  } else {
    
    switch(channel)       // Regular colors
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
  }
  return(Color(r * intensity, g * intensity, b * intensity));
}
 
//
//  RGB Interpolate Wheel
//
//  Wrapper for RGB Interpolate RGB below
//  start and end colors are 0-255 wheel colors

uint32_t RGBinterWheel(int c1, int c2, float fract)
{
  return(RGBinter24(Wheel(c1),Wheel(c2),fract));
}

//
//  RGB Interpolate 24-bit
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 24-bit colors

uint32_t RGBinter24(uint32_t c1, uint32_t c2, float fract)
{
  return(RGBinterRGB(GetRed(c1),GetGreen(c1),GetBlue(c1), GetRed(c2),GetGreen(c2),GetBlue(c2), fract));
}

//  RGB Interpolate RGB
//
//  Given a start rgb, an end rgb, and a fractional distance (0-1)
//  This function interpolates between the two points
//  and returns the properly interpolated rgb
//  as a 24-bit rgb color

uint32_t RGBinterRGB(byte r1, byte g1, byte b1, byte r2, byte g2, byte b2, float fract)
{
  if (fract < 0) return(Color(r1,g1,b1));
  if (fract > 1) return(Color(r2,g2,b2));
  return(Color(interpolate(r1,r2,fract), interpolate(g1,g2,fract), interpolate(b1,b2,fract) ));
}
//
// Interpolate
//
// Returns the fractional point from a to b

float interpolate(float a, float b, float fract)
{
  return(a + (fract*(b-a)));
}

//
// Interpolate Wrap
//
// Returns the fractional point from a to b
// checking both ways around a circle

float interpolateWrap(float a, float b, float fract)
{
  if(should_wrap(a,b)) {
    if(a<b) a += 360;
    else b += 360;
  }
  float result = interpolate(a,b,fract);
  if (result>360) result -= 360;
  return(result);
}

//
// should_wrap
//
// determines whether the negative way
// around the circle is shorter

boolean should_wrap(float a, float b) {
  if (a>b) {
    float temp = a;  // basic a <-> b swap
    a = b;
    b = temp;
  }
  return(abs(b-a) > abs((a+360) - b));
}
