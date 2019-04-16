#include "SPI.h"
#include "Adafruit_WS2801.h"

//  USE THIS VERSION
//
//  Triceratops - 45 cones on a coat
//
//  5/20/16
//
//  Standard rgb colors - Abandoned FastLED
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

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// For random-fill show (nasty global)
int shuffle[numLights];  // Will contain a shuffle of lights

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Light colors

#define maxColor 1530     // Colors are 6 * 255
int foreColor =     0;    // Starting foreground color
int backColor =   500;    // Starting background color

// Shows

byte curr_show = 0;       // Starting show
#define MAX_SHOW 18  // Total number of shows

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated

// Delays
int wait = 0;
#define MAX_WAIT 12 // Number of stored delay times 

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
        sectioncolor();
        break;
      default:
        clearWithFade();
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
  
  if (!random(100)) {
    foreColor = IncColor(foreColor, 10);
    update = true;
  }
  
  if (!random(200)) {
    backColor = IncColor(backColor, -20);
    update = true;
  }
  
  if (!random(4000)) {
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

// clear with fade
//
// Fades lights to black
//
void clearWithFade() {
  for (int i=0; i < numLights; i++) {
    setPixelBlack(i);
  }
}

//
// All On
//
// Simply turns all the pixels on to one color
// 
void allOn() {
   for (int i=0; i < numLights; i++) {
     setPixel(i, foreColor);
   }
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
      shuffle[i] = random(maxColor);
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
    setPixel(i, IncColor(foreColor, ((ConeSize[i]-1) * backColor) % maxColor));
  }
  foreColor = IncColor(foreColor, 40);
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
  float intensity;
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
  backColor = IncColor(backColor, 50);
}

void alternate() {
  for (int i=0; i < numLights; i++) {
    if (Alternate_Pattern[PatternLookUp[i]]) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, backColor);
    }
  }
  foreColor = IncColor(foreColor, 75);
}

void diagcolor() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(foreColor, (backColor * PatternLookUp[ConeLookUp[i]]) % maxColor));
  }
}

void sectioncolor() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(foreColor, (backColor * PatternLookUp[ConeLookUp[i]]) % maxColor));
  }
}

void sidesidecolor() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(backColor, (foreColor * PatternLookUp[ConeLookUp[i]]) % maxColor));
  }
}

void explodecolor() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(backColor, (foreColor * PatternLookUp[ConeLookUp[i]]) % maxColor));
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
    if (i%2) {
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
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye
//
void morph_frame() {
   for (int i = 0; i < numLights; i++) {
     strip.setPixelColor(i, HSVinter24(current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait)));
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
  int DelayValues[MAX_WAIT] = { 6, 8, 10, 15, 20, 30, 50, 75, 100, 150, 200, 250 };
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

// Input a value 256 * 6 to get a color value.
// The colours are a transition r - g -b - back to r
// Intensity must be 0 <= intensity <= 1
uint32_t Gradient_Wheel(int color, float intensity)
{
  int r,g,b;
  int channel, value;
  
  color = color % maxColor;  // Keep colors within bounds  
  
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
 
  *v = MAX;		        // v

  float delta = MAX - MIN;

  if (MAX != 0 ) *s = delta / MAX;	// s
  else { // r = g = b = 0		// s = 0, v is undefined
    *s = 0;
    *h = -1;
    return;
  }
  if( r == MAX ) *h = 60.0 * ( g - b ) / delta; // between yellow & magenta
  else {
    if( g == MAX ) {
      *h = 120.0 + 60.0 * ( b - r ) / delta; // between cyan & yellow
    } else {
      *h = 240.0 + 60.0 * ( r - g ) / delta;	// between magenta & cyan
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
  
  h /= 60;			// sector 0 to 5
  i = floor( h );
  f = h - i;			// factorial part of h
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
    default:		// case 5:
      *r = v * 255;
      *g = p * 255;
      *b = q * 255;
      break;
    }
}

//
//  HSV Interpolate Wheel
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 0-255 wheel colors

uint32_t HSVinterWheel(int c1, int c2, float fract)
{
  return(HSVinter24(Wheel(c1),Wheel(c2),fract));
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
  
  float BRIGHTNESS = 0.25;
  
  return(Color(r * BRIGHTNESS, g * BRIGHTNESS, b * BRIGHTNESS));
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
