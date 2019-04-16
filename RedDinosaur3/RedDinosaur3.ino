#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Red Dinosaur
//
//  26 LEDs = 6 left side + 14 center + 6 right side
//
//  Has a brightness setting: BRIGHTNESS and a dimmer
//
//  Has an ONLY_RED boolean
//
//  Restored HSV - If you only want RGB, use RedDinosaur2
//
//  6/13/17
//
/*****************************************************************************/
//
//  Set Up Pins
//
//  2 pins for clock and data of lights.
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 26
#define symLights 20
#define spineLights 14

// User-adjustable toggle values
boolean ONLY_RED = false; // true = use only red colors
boolean USE_DIMMER = false;  // true = use dimmer
float BRIGHTNESS = 0.40;  // 0.0 - 1.0 brightness level
int BLINK_FACTOR = 2;     // how fast blinking (1 = fast, higher = slower)

// Dimmer
#define dimmerPin 5
int dimmerValue = 1024;
#define MIN_DIMMER_CHANGE 0.1 // fractional minumum recognized by the dimmer

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// For random-fill show (nasty global)
byte shuffle[numLights];  // Will contain a shuffle of lights

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// center column | left side | right side
byte ConeLookUp[numLights] = { 0,1,2,3,4,5,16,15,14,13,12,23,24,25,
                                6,7,8,9,10,11,
                                17,18,19,20,21,22 };

byte ConeSize[numLights] = { 5,4,3,2,1,1,1,1,1,1,2,3,4,5,
                      0,0,0,0,0,0,
                      0,0,0,0,0,0 };

byte ArrowPattern[numLights] = 
{ 2, 1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                  7, 6, 5, 4, 3, 2,
                  7, 6, 5, 4, 3, 2 };

#define NUM_PATTERNS 6   // Total number of patterns

byte PatternMatrix[NUM_PATTERNS][numLights] = {
{ 1, 1, 1, 1, 1, 2, 1, 2, 1, 2, 1, 2, 1,
                  2, 1, 2, 1, 2, 1,
                  2, 1, 2, 1, 2, 1 },
                
{ 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
                  2, 1, 2, 1, 2, 1,
                  1, 2, 1, 2, 1, 2 },

{ 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
                  1, 2, 1, 2, 1, 2,
                  2, 1, 2, 1, 2, 1 },

{ 2, 2, 2, 2, 1, 2, 1, 2, 1, 2, 1, 2,
                2, 1, 2, 1, 2, 1,
                2, 1, 2, 1, 2, 1 },

{ 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 2,
                  2, 2, 1, 2, 2, 1,
                  2, 2, 1, 2, 2, 1 },

{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                  1, 1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1, 1 },
                
};

                
// Light colors

int foreColor =   0;     // Starting foreground color
int backColor = 500;     // Starting background color
#define MaxColor 1530    // Colors are 6 * 255

// Shows

int show = 0;       // Starting show
#define MAX_SHOW 12  // Total number of shows

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated

// Delays
int wait = 6;
#define MAX_WAIT 12 // Number of stored delay times 

//
// Setup
//

void setup() {
  
  //Serial.begin(9600);
  //Serial.println("Start");
    
  // Start up the LED counter
  strip.begin();
  
  // Initialize the strip, to start they are all 'off'
  
  for (int i = 0; i < numLights; i++) {
    current_frame[i] = Color(0,0,0);        // Black
    next_frame[i] = Color(0,0,0);           // Black
    strip.setPixelColor(i, Color(0,0,0));
  }
  strip.show();
}

void loop() { 
   
  delay(10);   // The only delay!

  check_dimmer();

  // Check if the lights need updating
  
  if (update) {
    
    update = false;
    
    switch(show) {
    
      case 0:    
        allOn();
        break;
      case 1:
        morphChain();
        break;
      case 2:
        randomfill();
        break;
      case 3:
        randomcolors();
        break;
      case 4:
        twocolor();
        break;
      case 5:
        lightwave();
        break;
      case 6:
        lightrunup();
        break;
      case 7:
        sawtooth();
        break;
      case 8:
        colorsize();
        break;
      case 9:
        brightsize();
        break;
      case 10:
        colorarrow();
        break;
      case 11:
        brightarrow();
        break;
    }
  }
  
  // Morph the display
  
  morph_frame();
  
  // Advance the clock
   
  if (morph++ >= GetDelayTime(wait)) {  // Finished morphing
    
    update = true;  // Force an update
    morph = 0;  // Reset morphClock
    push_frame();

    if (cycle++ > 1000) cycle = 0;  // Advance the cycle clock
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(10)) {
    foreColor = IncColor(foreColor, 5);
  }
  
  if (!random(10)) {
    backColor = IncColor(backColor, -10);
  }
  
  if (!random(10000)) {
    show = random(MAX_SHOW);
    morph = 0;
    cycle = 0;
    clearWithFade();
    patterns(random(5,10) * 100); // Pattern shows!
    delay(random(50,100) * 1000);
    clearWithFade();
  }
  
  if (!random(1000)) {
    wait++;
    if (wait == MAX_WAIT) {
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
// clear - set all pixels to black
// 
void clear() {
  for (int i = 0; i < numLights; i++) {
    strip.setPixelColor(i, Color(0,0,0));
    setPixelColor(i, Color(0,0,0));
  }
}

//
// clear and fade lights to black
//
void clearWithFade() {
  fill(Color(0,0,0));
}

//
// fill - fill all pixels to a color
//
void fill(uint32_t color) {
  for (int i = 0; i < numLights; i++) {
    setPixelColor(i, color);
  }
}

//
// All On - turns all the pixels on to one color
// 
void allOn() {
   fill(Wheel(foreColor));
}

// random fill
//
// randomly fills in pixels from black to all on, then takes them away until blank
//
void randomfill() {
  int i, j, save, pos;
  
  pos = cycle % (numLights * 2);  // Where we are in the show
  if (pos >= numLights) {
    pos = (numLights*2) - pos;  // For a sawtooth effect
  }
  
  if (pos == 0) {  // Start of show
  
    // Shuffle sort to determine order to turn on lights
    for (i = 0; i < numLights; i++) {
      shuffle[i] = i; // before shuffle
    }
    for (i = 0; i < numLights; i++) {  // here's position
      j = random(numLights);           // there's position
      save = shuffle[i];
      shuffle[i] = shuffle[j];         // first swap
      shuffle[j] = save;               // second swap
    }
  }
  
  for (i=0; i < numLights; i++) {
    if (i < pos) {  
      setPixelColor(shuffle[i], Wheel(foreColor));  // Turning on lights one at a time
    } else { 
      setPixelBlack(shuffle[i]);  // Turning off lights one at a time
    }
  }
}  

//
// random colors - turns each pixel on to a random color
//
void randomcolors() {
  int i;
  
  if (cycle == 0) {  // Start of show: assign lights to random colors
    for (i = 0; i < numLights; i++) {
      shuffle[i] = random(MaxColor);
    }
  }
  
  // Otherwise, fill lights with their color
  for (i = 0; i < numLights; i++) {
    setPixelColor(i, Wheel(shuffle[i]));
  }
}

//
// two color - alternates the color of pixels between two colors
//
void twocolor() {
  for (int i = 0; i < symLights; i++) {
    if (i % 2) setSymPixelColor(i, Wheel(foreColor));
    else setSymPixelColor(i, Wheel(backColor));
  }
}

//
// patterns shows
//
void patterns(int cycles) {
  int pattern = random(0, NUM_PATTERNS);    // Pick a random pattern
  int i, ticks = 0;
  
  while (ticks++ < cycles) {
      if (!random(10)) {       // The not-MAIN_COLOR color
        foreColor = IncColor(foreColor, 10); 
      }
        
      for (i=0; i < numLights; i++) {
        switch (PatternMatrix[pattern][i]) {
          case 0: {        // Off (black)
            strip.setPixelColor(ConeLookUp[i], Color(0,0,0));
            break;
          }
          case 1: {        // backColor
            strip.setPixelColor(ConeLookUp[i], Wheel(backColor));
            break;
          }
          case 2: {        // foreColor
            strip.setPixelColor(ConeLookUp[i], Wheel(foreColor));
            break;
          }
        }
      }
      strip.show();
      delay(10);
  }  
}

//
// Morph Chain - Morphs color 1 from position x to color 2 at position x+n
//
void morphChain() {
  float attenuation;
  
  for (int i = 0; i < symLights; i++) {
    attenuation = ((i + (cycle % symLights)) % symLights) / (float)(symLights - 1);
    setSymPixelColor(i, HSVinter24(Wheel(foreColor), Wheel(backColor), attenuation));
  }
}

//
// Saw tooth - fills in pixels with a sawtooth of intensity
//
// Peak of the sawtooth moves with the cycle
//
void sawtooth() {
  float attenuation;
  
  for (int i = 0; i < symLights; i++) {
    attenuation = 2 * (((i + (cycle % symLights)) % symLights) / (float)(symLights - 1));
    if (attenuation > 1) {
      attenuation = 2 - attenuation;  // the '2 -' creates the sawtooth
    }
    attenuation = attenuation * attenuation;  // magnify effect - color brightness is not linear
    setSymPixelColor(i, HSVinter24(Color(0,0,0), Wheel(foreColor), attenuation));
  }
}

//
// colorsize - light each cone according to its cone size
//
void colorsize() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(foreColor, ((ArrowPattern[i]) * backColor) % MaxColor));
  }
  foreColor = IncColor(foreColor, 40);
}

//
// brightsize -Light just one cone size at a time
//
void brightsize() {
  float intensity;
  for (int i=0; i < numLights; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(ArrowPattern[i], 9)));
  }
}

//
// colorarrow - light each cone according to its arrow position
//
void colorarrow() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(foreColor, ((ConeSize[i]) * backColor) % MaxColor));
  }
  foreColor = IncColor(foreColor, 40);
}

//
// brightarrow -Light just one arrow piece at a time
//
void brightarrow() {
  float intensity;
  for (int i=0; i < numLights; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(ConeSize[i], 5)));
  }
}

//
// calcIntensity - use sine wave + cycle + variable to calculate intensity
//
float calcIntensity(byte x, byte max_x) {
  return sin(3.14 * ((cycle + x) % max_x) / (max_x - 1));
}

//
// rainbowshow - fills with a gradient rainbow; over time, the starting color changes
//
void rainbowshow(int cycles) {
  int diff = abs(foreColor - backColor);
  
  for (int i = 0; i < symLights; i++) {
    setSymPixelColor( (i+(cycle % symLights)) % symLights, Wheel(((i*diff / symLights)+foreColor) % diff) );
  }
}

//
// lightwave - just one pixel traveling along the chain
//
void lightwave() {
  for (int i = 0; i < symLights; i++) {
     if (i == symLights - (cycle % symLights) - 1) {
      setSymPixelColor(i, Wheel(foreColor));
     }
     else {
      setSymPixelColor(i, Color(0,0,0));
     }
  }
}

//
// lightrunup
//
// lights fill up one at time
// 
void lightrunup() {
  int pos = cycle % (symLights * 2);  // Where we are in the show
  if (pos >= symLights) {
    pos = (symLights*2) - pos - 1;
  }
  
  for (int i=0; i < symLights; i++) {
    if (i <= pos) {
      setSymPixelColor(i, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setSymPixelColor(i, Color(0,0,0));   // black
    }
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
// morph_frame - gradually changes image from previous frame to next frame.
//
void morph_frame() {
   float fract = (float)morph/GetDelayTime(wait);
   for (int i = 0; i < numLights; i++) {
     strip.setPixelColor(i, HSVinter24(current_frame[i], next_frame[i], fract));
   }
   strip.show();  // Update the display 
}

//
// set SymPixelColor - forces the two sides to have symmetric pixels
//
void setSymPixelColor(int pos, uint32_t color) {
  // 0 - 13 : just light the center spine
  // 14 - 20: light the left side and light the right side the same
  // 20 - 25: ignore the right side
  if (pos < symLights) {
    next_frame[ConeLookUp[pos]] = color;

    if (pos >= spineLights) {
      next_frame[ConeLookUp[pos + 6]] = color;
    }
  } 
}

void setPixel(byte pos, int color) {
  next_frame[ConeLookUp[pos]] = Wheel(color);
}

void setPixelColor(int pos, uint32_t color) {
  next_frame[ConeLookUp[pos]] = color;
}

void setPixelBlack(byte pos) {
  next_frame[ConeLookUp[pos]] = Color(0,0,0);
}

//
// IncColor - adds amount to color
//
int IncColor(int color, int amount) {
  int value = color + amount;
  while (value < 0) value += MaxColor;
  while (value >= MaxColor) value -= MaxColor;
  return value;
}

//
// Get Delay Time - returns a delay time from an array
//
int GetDelayTime(int wait) {
  int DelayValues[MAX_WAIT] = { 4, 6, 8, 12, 20, 30, 50, 75, 100, 150, 200, 250 };
  return (DelayValues[(wait % MAX_WAIT)] * BLINK_FACTOR );
}


//
// Color - Create a 24-bit color value from R,G,B
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

//
// Wheel - Input a value (256 * 6) to get a color value
//
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
  
  float BRIGHTNESS = 0.25;
  
  return(Color(r * BRIGHTNESS, g * BRIGHTNESS, b * BRIGHTNESS));
}

//
// End HSV Code
//

//
// Interpolate
//
// Returns the fractional point from a to b
//
float interpolate(float a, float b, float fract)
{
  return(a + (fract*(b-a)));
}

//
// Interpolate Wrap
//
// Returns the fractional point from a to b
// checking both ways around a circle
//
float interpolateWrap(float a, float b, float fract)
{
  if(should_wrap(a,b)) {
    if(a < b) a += 360;
    else b += 360;
  }
  float result = interpolate(a, b, fract);
  if (result>360) result -= 360;
  return(result);
}

//
// should_wrap
//
// determines whether the negative way
// around the circle is shorter
//
boolean should_wrap(float a, float b) {
  if (a > b) {
    float temp = a;  // basic a <-> b swap
    a = b;
    b = temp;
  }
  return(abs(b - a) > abs((a + 360) - b));
}
