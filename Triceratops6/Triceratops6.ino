#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Triceratops - 45 cones on a coat
//
//  5/13/17
//
//  Standard rgb colors - includes HSV
//
//  New shows: PLINKO and BOUNCE
//
//  Has BRIGHTNESS and ONLY_RED
//
/*****************************************************************************/
//
//  Set Up Pins
//
//  2 pins for clock and data of lights.
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define bodyLights 42
#define numLights 45
#define numRows 17

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// For random-fill show (nasty global)
int shuffle[numLights];  // Will contain a shuffle of lights

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Light colors

#define MaxColor 1530     // Colors are 6 * 255
int foreColor =     0;    // Starting foreground color
int backColor =   500;    // Starting background color

boolean ONLY_RED = false; // true = use only red colors
float BRIGHTNESS = 0.50;  // 0.0 - 1.0 brightness level

// Shows

byte curr_show = 1;       // Starting show
#define MAX_SHOW 20  // Total number of shows

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated

// Delays
int wait = 0;
#define MAX_WAIT 12 // Number of stored delay times 

// Plinko
#define NUM_PLINKO 5
byte plink_x[NUM_PLINKO] = { 0,0,0,0,0 };
byte plink_y[NUM_PLINKO] = { 18,18,18,18,18 };

// Bounce
int bounce_dir = random(0,6);
int bounce_pos = random(0,numLights);
int trail_size = 5;
int trail[] = { -1, -1, -1, -1, -1 };
int trail_colors[] = { 205, 145, 85, 45, 5 };
int glow_colors[] = { 255, 35, 9 };

// Lookup tables

byte ConeLookUp[45] = {
      44,
    42, 43,
  41, 40, 32,
    39, 33,
  38, 34, 31,
    35, 30,
  36, 29, 20,
37, 28, 21, 19,
  27, 22, 18,
26, 23, 17,  9,
  24, 16,  8,
25, 15,  7, 10,
  14,  6, 11,
     5, 12,
   4, 13,  0,
     3,  1,
       2
};

byte ConeGrid[17][4] = {
 {45,    44,   45,45},
 {45,  42, 43,    45},
 {   41, 40, 32,  45},
 {45,  39, 33,    45},
 {   38, 34, 31,  45},
 {45,  35, 30,    45},
 {   36, 29, 20,  45},
 { 37, 28, 21, 19,  },
 {   27, 22, 18,  45},
 { 26, 23, 17,  9,  },
 {   24, 16,  8,  45},
 { 25, 15,  7, 10,  },
 {   14,  6, 11,  45},
 {45,   5, 12,    45},
 {    4, 13,  0,  45},
 {45,   3,  1,    45},
 {45,     2,   45,45},
};

byte neighbors[][6] = {
  {88,88,88,1,13,12}, // 0
  {0,88,88,2,3,13},
  {1,88,88,88,88,3},
  {13,1,2,88,88,4},
  {5,13,3,88,88,88}, // 4
  {6,12,13,4,88,14},
  {7,11,12,5,14,15},
  {8,10,11,6,15,16},
  {9,88,10,7,16,17}, // 8
  {88,88,88,8,17,18},
  {88,88,88,11,7,8},
  {10,88,88,12,6,7},
  {11,88,0,13,5,6}, // 12
  {12,0,1,3,4,5},
  {15,6,5,88,88,25},
  {16,7,6,14,25,24},
  {17,8,7,15,24,23}, // 16
  {18,9,8,16,23,22},
  {19,88,9,17,22,21},
  {88,88,88,18,21,20},
  {88,88,19,21,29,30}, // 20
  {20,19,18,22,28,29},
  {21,18,17,23,27,28},
  {22,17,16,24,26,27},
  {23,16,15,25,88,26}, // 24
  {24,15,14,88,88,88},
  {27,23,24,88,88,88},
  {28,22,23,26,88,37},
  {29,21,22,27,37,36}, // 28
  {30,20,21,28,36,35},
  {31,88,20,29,35,34},
  {88,88,88,30,34,33},
  {88,88,88,33,40,43}, // 32
  {32,88,31,34,39,40},
  {33,31,30,35,38,39},
  {34,30,29,36,88,38},
  {35,29,28,37,88,88}, // 36
  {36,28,27,88,88,88},
  {39,34,35,88,88,88},
  {40,33,34,38,88,41},
  {43,32,33,39,41,42}, // 40
  {42,40,39,88,88,88},
  {44,43,40,41,88,88},
  {88,88,32,40,42,44},
  {88,88,43,42,88,88}, // 44
};

byte PatternLookUp[45] = { 41,43,44,42,39,37,35,32,29,26,
                           33,36,38,40,34,31,28,25,22,19,
                           15,18,21,24,27,30,23,20,17,14,
                           12,10,5,7,9,11,13,16,8,6,
                           4,3,2,1,0 };

byte Stripe_Pattern[45] = {
       0,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
       0
};

byte Section_Pattern[45] = {
       2,
     2,  2,
   0,  0,  0,
     0,  0,
   0,  0,  0,
     0,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
     0,  0,
   2,  0,  2,
     2,  2,
       2
};

byte Explode_Pattern[45] = {
       5,
     5,  5,
   5,  4,  5,
     4,  4,
   4,  3,  4,
     3,  3,
   3,  2,  3,
 3,  1,  1,  3,
   2,  0,  2,
 3,  1,  1,  3,
   3,  2,  3,
 4,  3,  3,  4,
   4,  3,  4,
     4,  4,
   5,  4,  5,
     5,  5,
       5
};

byte Alternate_Pattern[45] = {
       0,
     1,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
 0,  0,  1,  0,
   1,  0,  0,
 1,  0,  1,  0,
   1,  0,  0,
 0,  0,  0,  1,
   0,  1,  0,
     0,  0,
   1,  0,  1,
     0,  0,
       1
};

byte SideSide_Pattern[45] = {
       3,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
       3
};

byte Diag_Pattern[45] = {
       0,
     1,  0,
   2,  1,  0,
     2,  1,
   3,  2,  1,
     3,  2,
   4,  3,  2,
 5,  4,  3,  2,
   5,  4,  3,
 6,  5,  4,  3,
   6,  5,  4,
 7,  6,  5,  4,
   7,  6,  5,
     7,  6,
   8,  7,  6,
     8,  7,
       8
};

byte ConeSize[45] = { 5,5,5,5,5,3,5,1,3,5,1,1,5,4,3,4,5,5,6,3,6,2,1,6,6,4,2,6,2,1,3,4,2,4,4,5,3,2,2,3,1,6,3,3,1 };

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
   
  delay(50);   // The only delay!
  
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
        lightwave();
        break;
      case 5:
        lightrunup();
        break;
      case 6:
        colorsize();
        break;
      case 7:
        brightsize();
        break;
      case 8:
        stripe();
        break;
      case 9:
        alternate();
        break;
      case 10:
        diagcolor();
        break;
      case 11:
        sidesidecolor();
        break;
      case 12:
        explodecolor();
        break;
      case 13:
        diagbright();
        break;
      case 14:
        sidesidebright();
        break;
      case 15:
        explodebright();
        break;
      case 16:
        plinko();
        break;
      case 17:
        bounce();
        break;
      case 18:
        bounce_glowing();
        break;    
      default:
        sectioncolor();
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
    foreColor = IncColor(foreColor, 1);
  }
  
  if (!random(2)) {
    backColor = IncColor(backColor, -1);
  }
  
  if (!random(2000)) {
    curr_show = random(MAX_SHOW);
    morph = 0;
    cycle = 0;
    clearWithFade();
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

// clear
//
// set all cells to black but don't call show yet
// ignores buffering
// 
void clear() {
  for (int i=0; i < numLights; i++) {
    strip.setPixelColor(i, Color(0,0,0));
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

// random fill
//
// randomfill: randomly fills in pixels from blank to all on
// then takes them away random until blank
//
 
void randomfill() {
  int i, j, save, pos;
  
  pos = cycle % (bodyLights*2);  // Where we are in the show
  if (pos >= bodyLights) {
    pos = (bodyLights*2) - pos;  // For a sawtooth effect
  }
  
  if (pos == 0) {  // Start of show
  
    // Shuffle sort to determine order to turn on lights
    for (i=0; i < bodyLights; i++) shuffle[i] = i; // before shuffle
    for (i=0; i < bodyLights; i++) {  // here's position
      j = random(bodyLights);         // there's position
      save = shuffle[i];
      shuffle[i] = shuffle[j];       // first swap
      shuffle[j] = save;             // second swap
    }
  }
  
  for (i=0; i < bodyLights; i++) {
    if (i < pos) {  
      setPixel(shuffle[i], foreColor);  // Turning on lights one at a time
    } else { 
      setPixelBlack(shuffle[i]);  // Turning off lights one at a time
    }
  }
  setHead(foreColor);
}  

// random colors
//
// randomcolors: turns each pixel on to a random color
//
void randomcolors() {
  int i;
  
  if (cycle == 0) {  // Start of show: assign lights to random colors
    for (i=0; i < numLights; i++) {
      shuffle[i] = random(MaxColor);
    }
  }
  
  // Otherwise, fill lights with their color
  for (i=0; i < numLights; i++) {
    setPixel(i, shuffle[i]);
  }
}

// colorsize
//
// Light each cone according to its cone size
//
void colorsize() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(foreColor, ((ConeSize[i]-1) * backColor) % MaxColor));
  }
  foreColor = IncColor(foreColor, 1);
}

// calcIntensity
//
// Use sine wave + cycle + variable to calculate intensity
float calcIntensity(byte x, byte max_x) {
  return sin(3.14 * ((cycle + x) % max_x) / (max_x - 1));
}

// brightsize
//
// Light just one cone size
//
void brightsize() {
  for (int i=0; i < bodyLights; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(ConeSize[i]-1, 5)));
  }
  setHead(foreColor);
}

void stripe() {
  for (int i=0; i < numLights; i++) {
    if (Stripe_Pattern[PatternLookUp[i]] == 0) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, backColor);
    }
  }
  backColor = IncColor(backColor, -3);
}

void alternate() {
  for (int i=0; i < numLights; i++) {
    if (Alternate_Pattern[PatternLookUp[i]]) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, backColor);
    }
  }
  //foreColor = IncColor(foreColor, 75);
}

void diagcolor() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(foreColor, (backColor * Diag_Pattern[PatternLookUp[ConeLookUp[i]]] / 5) % MaxColor));
  }
}

void sectioncolor() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(foreColor, (backColor * Section_Pattern[PatternLookUp[ConeLookUp[i]]] / 10) % MaxColor));
  }
}

void sidesidecolor() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(backColor, (foreColor * SideSide_Pattern[PatternLookUp[ConeLookUp[i]]]) % MaxColor));
  }
}

void explodecolor() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(backColor, (foreColor * Explode_Pattern[PatternLookUp[ConeLookUp[i]]]) % MaxColor));
  }
}

void diagbright() {
  for (int i=0; i < bodyLights; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(Diag_Pattern[PatternLookUp[i]], 9)));
  }
  setHead(foreColor);
}

void sidesidebright() {
  for (int i=0; i < bodyLights; i++) {
    setPixelColor(i, Gradient_Wheel(foreColor, calcIntensity(SideSide_Pattern[PatternLookUp[i]], 7)));
  }
  setHead(foreColor);
}

void explodebright() {
  for (int i=0; i < bodyLights; i++) {
    setPixelColor(i, Gradient_Wheel(foreColor, calcIntensity(5 - Explode_Pattern[PatternLookUp[i]], 6)));
  }
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
// lightwave
//
// Just one pixel traveling along the chain
 
void lightwave() {
  for (int i=0; i < bodyLights; i++) {
     if (i == bodyLights-(cycle % bodyLights)-1) {
       setPixel(ConeLookUp[i], foreColor);
     } else if (i == cycle % bodyLights) {
       setPixel(ConeLookUp[i], backColor);
     } else {
       setPixelBlack(ConeLookUp[i]);
     }
  }
  setHead(foreColor);
}

//
// lightrunup
//
// Wave filling in and out

void lightrunup() {
  int i, pos;
  
  pos = cycle % (bodyLights*2);  // Where we are in the show
  if (pos >= bodyLights) {
    pos = (bodyLights*2) - pos;  // For a sawtooth effect
  }
  
  for (i=0; i < bodyLights; i++) {
    if (i < pos) {
      setPixelBlack(ConeLookUp[i]);
    } else {
      setPixel(ConeLookUp[i], foreColor);
    }
  }
  setHead(foreColor);
}

//
// bounce
//
void bounce() {  
  fill(Color(0,0,2));
  
  for (int i = trail_size - 1; i >= 0; i--) {
    if (trail[i] == 88) continue;
    setPixelColor(trail[i], Color(0, 0, trail_colors[i]));
  }
  setPixelColor(bounce_pos, Color(200, 0, 128));
  
  byte *n = neighbors[bounce_pos];
  int old_dir = bounce_dir;
  while (n[bounce_dir] == 88 || (bounce_dir + 3) % 6 == old_dir) {
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
    if (x == 88) continue;
    for (int j = 0; j < 6; j++) {
      int xx = neighbors[x][j];
      if (xx == 88) continue;
      setPixelColor(xx, Color(0, glow_colors[2], glow_colors[2]));
    }
  }
  for (int i = 0; i < 6; i++) {
    int x = neighbors[bounce_pos][i];
    if (x == 88) continue;
    setPixelColor(x, Color(0, glow_colors[1], glow_colors[1]));
  }
  setPixelColor(bounce_pos, Color(0, glow_colors[0], glow_colors[0]));
  
  byte *n = neighbors[bounce_pos];
  int old_dir = bounce_dir;
  while (n[bounce_dir] == 88 || (bounce_dir + 3) % 6 == old_dir) {
    bounce_dir = random(0,6);
  }
  bounce_pos = n[bounce_dir]; 
}

//
// plinko
//
void plinko() {
  // Start a new plinko
  if (!random(8)) {
    for (int i = 0; i < NUM_PLINKO; i++) {
      if (plink_y[i] >= numRows) {  // off the board
        plink_x[i] = 1;  // Start at center
        plink_y[i] = 0;  // Start at top
        break;
      }
    }
  }
  
  // Move existing plinko
  clearWithFade();
  
  for (int i = 0; i < NUM_PLINKO; i++) {
    if (plink_y[i] < numRows) {  // is on the board?
      if (ConeGrid[plink_y[i]][plink_x[i]] != 88) {
        setPixel(ConeGrid[plink_y[i]][plink_x[i]], IncColor(foreColor, 100 * i)); // Draw the plinko
      }
      
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
     strip.setPixelColor(i, HSVinter24(current_frame[i], next_frame[i], fract));
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

void setHead(int color) {
  next_frame[42] = Wheel(color);
  next_frame[43] = Wheel(color);
  next_frame[44] = Wheel(color);
}
 
//
// IncColor
//
// Adds amount to color
//
int IncColor(int color, int amount) {
  int value = color + amount;
  while (value < 0) value += MaxColor;
  while (value >= MaxColor) value -= MaxColor;
  return value;
}

//
// Get Delay Time
//
// Returns a delay time from an array
//
 
int GetDelayTime(int wait) {
  int DelayValues[MAX_WAIT] = { 2, 4, 6, 10, 15, 20, 30, 50, 75, 100, 125, 150 };
  return (DelayValues[wait % MAX_WAIT]);
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
  
  color = color % MaxColor;  // Keep colors within bounds  
  
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
// HSV CODE
//

// r,g,b values are from 0 to 255
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
// 
// code from http://www.cs.rit.edu/~ncs/color/t_convert.html

void RGBtoHSV( byte red, byte green, byte blue, float *h, float *s, float *v )
{
  float r = red/float(255);
  float g = green/float(255);
  float b = blue/float(255);
  
  float MIN = min(r, min(g,b));  // min(r,g,b)
  float MAX = max(r, max(g,b));  // max(r,g,b)
 
  *v = MAX;            // v

  float delta = MAX - MIN;

  if (MAX != 0 ) *s = delta / MAX;  // s
  else { // r = g = b = 0   // s = 0, v is undefined
    *s = 0;
    *h = -1;
    return;
  }
  if( r == MAX ) *h = 60.0 * ( g - b ) / delta; // between yellow & magenta
  else {
    if( g == MAX ) {
      *h = 120.0 + 60.0 * ( b - r ) / delta; // between cyan & yellow
    } else {
      *h = 240.0 + 60.0 * ( r - g ) / delta;  // between magenta & cyan
    }
  }
  if( *h < 0 ) *h += 360;
}

// r,g,b values are from 0 to 255
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
//
// code from http://www.cs.rit.edu/~ncs/color/t_convert.html

void HSVtoRGB( byte *r, byte *g, byte *b, float h, float s, float v )
{
  int i;
  float f, p, q, t;
  
  if( s == 0 ) {
    // achromatic (grey)
    *r = *g = *b = (v*255);
    return;
  }
  
  h /= 60;      // sector 0 to 5
  i = floor( h );
  f = h - i;      // factorial part of h
  p = v * ( 1 - s );
  q = v * ( 1 - s * f );
  t = v * ( 1 - s * ( 1 - f ) );
  
  switch( i ) {
    case 0:
      *r = v * 255;
      *g = t * 255;
      *b = p * 255;
      break;
    case 1:
      *r = q * 255;
      *g = v * 255;
      *b = p * 255;
      break;
    case 2:
      *r = p * 255;
      *g = v * 255;
      *b = t * 255;
      break;
    case 3:
      *r = p * 255;
      *g = q * 255;
      *b = v * 255;
      break;
    case 4:
      *r = t * 255;
      *g = p * 255;
      *b = v * 255;
      break;
    default:    // case 5:
      *r = v * 255;
      *g = p * 255;
      *b = q * 255;
      break;
    }
}

//
//  HSV Interpolate 24-bit
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 24-bit colors

uint32_t HSVinter24(uint32_t c1, uint32_t c2, float fract)
{
  return(HSVinterRGB(GetRed(c1),GetGreen(c1),GetBlue(c1), GetRed(c2),GetGreen(c2),GetBlue(c2), fract));
}

//  HSV Interpolate RGB
//
//  Given a start rgb, an end rgb, and a fractional distance (0-1)
//  This function converts start and end colors to hsv
//  and interpolates between the two points
//  The function returns the properly interpolated rgb
//  as a 24-bit rgb color
//  Whew.

uint32_t HSVinterRGB(byte r1, byte g1, byte b1, byte r2, byte g2, byte b2, float fract)
{  
  if (fract < 0) {
     fract = 0;
  }
  if (fract > 1) {
    fract = 1;
  }
  // Check to see if either 1 or 2 are black. If black, just attenuate the other color.
   
  if (r1+g1+b1 == 0) return(Color(r2*fract, g2*fract, b2*fract));
  if (r2+g2+b2 == 0) return(Color(r1*(1-fract), g1*(1-fract), b1*(1-fract)));
   
  // Set up HSV1 and HSV2 variables and pointers
  
  float h1,s1,v1,h2,s2,v2,hi,si,vi;
  float* p_h1;
  float* p_s1;
  float* p_v1;
  p_h1 = &h1;
  p_s1 = &s1;
  p_v1 = &v1;
  float* p_h2;
  float* p_s2;
  float* p_v2;
  p_h2 = &h2;
  p_s2 = &s2;
  p_v2 = &v2;
  
  // Calculate HSV1 and HSV2
  
  RGBtoHSV(r1,g1,b1,p_h1,p_s1,p_v1);
  RGBtoHSV(r2,g2,b2,p_h2,p_s2,p_v2);
  
  // Calculate the interpolated HSVi
  
  hi = interpolateWrap(h1,h2,fract);
  si = interpolate(s1,s2,fract);
  vi = interpolate(v1,v2,fract);
  
  // Convert back to rgb via pointers
  
  byte r,g,b;
  byte* p_r;
  byte* p_g;
  byte* p_b;
  p_r = &r;
  p_g = &g;
  p_b = &b;
  
  HSVtoRGB(p_r,p_g,p_b,hi,si,vi);
  
  return(Color(r,g,b));
}

//
// End HSV Code
//

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
