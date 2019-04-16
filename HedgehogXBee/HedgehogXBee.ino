//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Hedgehog! - 50 Lights. This version does not use a keypad
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

#define numLights 50      // number of spikes
// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Hedgehog colors

int color1  = 85;   // Starting foreground color - Global!
int color2 = 170;  // Starting background color - Global!
#define MaxColor 255    // Colors go from 0 to 255

// Shows

int show=0;    // Show and number of initial show
int wait=6;    // index to keep track of delay time
#define maxWait 12     // Number of stored delay times 

// Timing variables for xBee communication

unsigned long OldTime;
unsigned long NewTime;
unsigned long TimeDiff;
int timeout;
int timeoutMax = 14; //If we don't hear anything for 14 seconds, then begin autonomous mode
char currentMode;
#define NO_DATA FFFF

//Depending on the unit, we need to run slightly different code
int I_AM_MASTER = 1; //Set to 1 if this code is meant for a master unit
int I_AM_SLAVE = 0;  //Set to 1 if this code is meant for a slave unit

/*
// Nearest Neighbors: mapping for the game of life
byte neighbors[][4] = {
  {51,52,3,1},
  {53,0,5,4},
  {0,3,6,5},
  {0,54,7,6},
  {55,1,8,59},
  {1,2,9,8},  //5
  {2,3,10,9},
  {3,56,60,10},
  {4,5,12,11},
  {5,6,13,12},
  {6,7,14,13}, //10
  {59,8,15,63},
  {8,9,16,15},
  {9,10,17,16},
  {10,60,64,17},
  {11,12,18,65}, //15
  {12,13,19,18},
  {13,14,66,19},
  {15,16,21,20},
  {16,17,22,21},
  {65,18,23,69}, //20
  {18,19,24,23}, 
  {19,66,70,24},
  {20,21,26,25},
  {21,22,27,26},
  {69,23,29,28}, //25
  {23,24,30,29},
  {24,70,31,30},
  {71,25,32,75},
  {25,26,33,32},
  {26,27,34,33}, //30
  {27,72,76,34},
  {28,29,36,35},
  {29,30,37,36},
  {30,31,38,37},
  {75,32,39,79}, //35
  {32,33,40,39},
  {33,34,41,40},
  {34,76,80,41},
  {35,36,43,42},
  {36,37,44,43}, //40
  {37,38,45,44},
  {79,39,46,83},
  {39,40,47,46},
  {40,41,48,47},
  {41,80,84,48}, //45
  {42,43,49,85},
  {43,44,86,49},
  {44,45,86,49},
  {46,47,88,87},
  {-1,-1,52,51}, //50
  {-1,50,8,53},
  {50,-1,54,0},
  {-1,51,1,55},
  {52,-1,56,3},
  {-1,53,4,57}, //55
  {54,-1,58,6},
  {-1,55,59,-1},
  {56,-1,-1,60},
  {57,4,11,61},
  {6,58,62,14}, //60
  {-1,59,63,-1},
  {60,-1,-1,64},
  {61,11,65,-1},
  {14,62,-1,66},
  {63,15,20,67}, //65
  {17,64,68,22},
  {-1,65,69,-1},
  {66,-1,-1,70},
  {67,20,25,71},
  {22,68,72,27}, //70
  {-1,69,28,73},
  {70,-1,74,31},
  {-1,71,75,-1},
  {72,-1,-1,76},
  {73,28,35,77}, //75
  {31,74,78,38},
  {-1,75,79,-1},
  {76,-1,-1,80},
  {77,35,42,81},
  {38,78,82,45}, //80
  {-1,79,83,-1},
  {80,-1,-1,84},
  {81,42,85,-1},
  {45,82,-1,86},
  {83,46,87,-1}, //85
  {47,84,-1,88},
  {85,49,89,-1},
  {49,86,-1,89},
  {87,88,-1,-1}
};
  
// framebuffer
//
// Different from the Greg/Robie buffer
// This buffer includes the outer edge
//
// 0: not lit
// 1: lit

#define NumLifePix 90
byte current_bigframe[NumLifePix];
byte next_bigframe[NumLifePix];
*/

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
  
  // xBee communication
  
  OldTime = 0;
  NewTime = 0;
  timeout = 0;
  
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
   
  // Start with a fun random show
  show = random(13);
  
  switch(show) {
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
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      setPixelColor(i, Wheel(color1));
    }
    morph_frame(GetDelayTime(wait));
  
    if (!random(100)) { if (wait++ > maxWait) wait = 0; }
    if (color1+=5 > MaxColor) color1 = color1 - MaxColor;  
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
  for (int count = 0; count < cycles; count++) {
   for (int i=0; i < numLights; i++) {
     if (centerstripe[i]) { setPixelColor(SpineOrder[i], Wheel(color1));
     } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
   }
   morph_frame(GetDelayTime(wait));
  
   if (!random(100)) { if (wait++ > maxWait) wait = 0; }
   if (color1+=10 > MaxColor) color1 = color1 - MaxColor;  
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
  int tempcolor;
  int colorspacing;
  
  tempcolor = color1;
  colorspacing = random(5, MaxColor/5);
  
  for (int i=0; i<4; i++) {
    colors[i] = tempcolor;
    tempcolor = tempcolor + colorspacing;
    if (tempcolor > MaxColor) tempcolor = tempcolor - MaxColor;
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
  int colorspacing;
  
  colorspacing = random(2, MaxColor/11);
  
  for (int i=0; i<10; i++) {
    colors[i] = ((i*colorspacing) + color1) % MaxColor;
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
      setPixelColor(shuffle[i], Wheel(color1));
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
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
      strip.setPixelColor(SpineOrder[i], Wheel(((i*256 / strip.numPixels()) + color1) % 256) );
    }  
    strip.show();   // write all the pixels out
    delay(50);
    if (color1+=15 > MaxColor) color1 = color1 - MaxColor;
  }
}

/*
 * starburst: burst from center to outer ring. Code is identical to chevrons
 */
 
void starburst(int cycles) {
  int chev, i;
    
  for (int count = 0; count < cycles; count++) {
    for (chev=0; chev < 11; chev++) {
      for (i=0; i < numLights; i++) {
        if (starburstpattern[i] == chev) {
          setPixelColor(SpineOrder[i], Wheel((color1+(starburstpattern[i]*3))%MaxColor));
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
  int chev, i;
    
  for (int count = 0; count < cycles; count++) {
    for (chev = 0; chev < 11; chev++) {
      for (i=0; i < numLights; i++) {
        if (chevpattern[i] == chev) {
          setPixelColor(SpineOrder[i], Wheel(color1));
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
  int chev, i;
   
  for (int count = 0; count < cycles; count++) {
    for (chev=-1; chev < 10; chev++) {
      for (i=0; i < numLights; i++) {
        if (chev >= chevpattern[i]) {
          setPixelColor(SpineOrder[i], Wheel((color1+(chevpattern[i]*2))%MaxColor));
        } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
    }
    for (chev=8; chev >= 0; chev--) {
      for (i=0; i < numLights; i++) {
        if (chev >= chevpattern[i]) {
          setPixelColor(SpineOrder[i], Wheel((color1+(chevpattern[i]*2))%MaxColor));
        } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
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
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      setPixelColor(SpineOrder[i], GradientWheel(color1,centering[i]));
    }
    morph_frame(GetDelayTime(wait));
    if (!random(100)) { if (wait++ > maxWait) wait = 0; }
    if (color1++ > MaxColor) color1 = color1 - MaxColor; 
  }
  clearWithFade(1000);
}

/*
 * lightwave - Just one pixel traveling from 0 to 49
 */
 
void lightwave(int cycles) {
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      for (int j=0; j < strip.numPixels(); j++) {
        if (i == j) { setPixelColor(SpineOrder[j], Wheel(color1)); }
        else { setPixelColor(SpineOrder[j], Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
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
// Code from Greg and Robey
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

//
// Exchange Info
//
// If Master, routine sends out "#", then show number, delay number, color1, and color2
//
// If Slave, routine looks for a "#", then receives show number, delay number, color1, and color2
//

void ExchangeInfo() {
  int heardNum;
  
  if (I_AM_MASTER) {
    Serial.print('#');     // Symbol to start communication
    Serial.print(show);    // Send show information
    Serial.print('$');     // Space character
    Serial.print(wait);    // Send delay information
    Serial.print('$');     // Space character
    Serial.print(color1);  // Send color1
    Serial.print('$');     // Space character
    Serial.print(color2);  // Send color2
    Serial.print('$');     // Space character
  }
  
  if (I_AM_SLAVE) {
    if (Serial.available()) {
        char incoming1 = Serial.read();
        if (incoming1 == '#') {  // Heard from the master
          heardNum = HearNum();
          if (heardNum != NO_DATA) show = heardNum; else return;
          
          heardNum = HearNum();
          if (heardNum != NO_DATA) wait = heardNum; else return;
          
          heardNum = HearNum();
          if (heardNum != NO_DATA) color1 = heardNum; else return;
          
          heardNum = HearNum();
          if (heardNum != NO_DATA) color2 = heardNum; else return;
        }
    }
  }
}

//
// HearNum
//
// Converts xBee characters into a number
// '$' character terminates the number string
//
// If no number, returns NO_DATA signifier

/*int HearNum1() {
  int i=0;
  char buf[BUFSIZE];
  while (Serial.available() && i < BUFSIZE) {
    char tmp = Serial.read();
    delay(1);
    
    if (tmp != '$') {
      buf[i] = tmp;
    } else {
      break;
    }
  }
  return atoi(buf);
}*/

int HearNum() {
  #define BUFSIZE 6
  
  int i=0;
  char buf[BUFSIZE];
  boolean finish_number = false;  // Set to true on reaching '$' and the end of a number
  unsigned long timeout_time = millis() + 100;  // 100 msec is max we will wait for next character
  
  while (i < BUFSIZE && millis() < timeout_time) {
    if (Serial.available()) {
      char tmp = Serial.read();
      if (tmp != '$') {
        buf[i] = tmp;
        i++;
      } else {
        finish_number = true;  // successfully reached the end of a number
        break;
      }
    } else {
      delay(1); 
    }
  }
  if (finish_number && i>0) return atoi(buf); else return(NO_DATA);
}

//
// game of life
//

void game_of_life(int MaxTime) {
  int currTime = 0;
  int randcolor = random(MaxColor); // pick a random starting color

  clear_bigframe();  // Set big frame pixels all to 0
  
  while (currTime++ < MaxTime) {   // Go for maxTime number of cycles, and then exit
    if (CountFramePixels() < 3) randomize_start(numLights/2); // Number is #pixels to turn on at beginning
    life_step();
    draw_bigframe(randcolor, 200);  // number is delay between life stages
    if (randcolor++ > MaxColor) randcolor = randcolor - MaxColor;  // Slowly change the color
  }
}

//
// life_step
//
// Calculate life and death for the frame

void life_step() {
  int pixelScore;
  byte outcome;
  
  for (int currPix = 0; currPix < NumLifePix; currPix++ ) {
    pixelScore = num_near_neigh(currPix);
    
    if (current_bigframe[currPix]) { // Pixel is currently alive
      if (pixelScore > 3 && pixelScore < 7) outcome = 1;  // still alive
      else outcome = 0;  // death
    } else {                         // Pixel is currently dead
      if (pixelScore > 4 && pixelScore < 7 ) outcome = 1;  // spring to life
      else outcome = 0;  // still dead
    }
    next_bigframe[currPix] = outcome;
  }
}

// num_near_neigh
//
// Calculates how many nearest (20) and next-nearest (6) neighbors are alive
// Out of bounds neighbors (10) and next-nearest (3) are considered half alive
// Values are *20 to avoid fractions

int num_near_neigh(byte pos) {
  int nTot = 0; // running total of neighbor score
  byte *n = neighbors[pos]; // neighboring pixels
  
  for (int i=0; i < 4; i++) {
    // Calculate the near neighbors
    if (n[i] == -1) nTot++;  // 1pts. for out-of bounds near neighbor
    else nTot = nTot + (2*current_bigframe[n[i]]); // 2pts. for live neighbor
  }
  return(nTot);
}

// CountFramePixels
//
// Counts the number of lit pixels

int CountFramePixels() {
  int OnPixels = 0;
  for (int i = 0; i < numLights; i++) {
    if (current_bigframe[i]) OnPixels++;
  }
 return (OnPixels);
}

// draw_bigframe
//
// For the shown pixels (#0-49)
// 0 = Black, Anything else = color c

void draw_bigframe(byte c, int delay_ms) {
  for (int i = 0; i < NumLifePix; i++) {
    current_bigframe[i] = next_bigframe[i];
    if (i < numLights) {
      if (next_bigframe[i]) { strip.setPixelColor(SpineOrder[i], Wheel(c)); }
      else { strip.setPixelColor(SpineOrder[i], Color(0,0,0)); }
    }
  }
  strip.show();
  delay(delay_ms);
}

// randomize_start
//
// Turns the number of pixels on
// Position is choosen randomly by a shuffle sort

void randomize_start(int OnPixels) {
  int i, j, save;
  int shuffle[NumLifePix];  // Will contain a shuffle of 0 to totPixels
  
  for (i=0; i < NumLifePix; i++) shuffle[i] = i; // before shuffle
  for (i=0; i < NumLifePix; i++) {  // here's position
    j = random(NumLifePix);       // there's position
    save = shuffle[i];
    shuffle[i] = shuffle[j];     // first swap
    shuffle[j] = save;           // second swap
  }
  for (i=0; i < OnPixels; i++) {
    current_bigframe[shuffle[i]] = 1;
    next_bigframe[shuffle[i]] = 1;
  }
}

//
// clear_bigframe()
//
// Sets all pixels including buffer ring to 0
void clear_bigframe(void) {
 for (int i=0; i < NumLifePix; i++) current_bigframe[i] = 0; // Sets all pixels in big frame to 0
}
*/

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
