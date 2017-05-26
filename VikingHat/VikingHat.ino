  //#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Viking Hat
//
//  Includes extended colors
//  Removes xBee hardware
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights.
// 
#define dataPin 8       // 'yellow' wire
#define clockPin 9      // 'green' wire

#define numLights 12
#define symLights 6

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// For random-fill show (nasty global)
byte shuffle[numLights];  // Will contain a shuffle of lights

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Light colors

int foreColor =   0;    // Starting foreground color
int backColor = 500;    // Starting background color
#define MaxColor 1530    // Colors are 6 * 255

// Shows

int show = 0;       // Starting show
#define MAX_SHOW 8  // Total number of shows

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
  
  for (int i=0; i<numLights; i++) {
    current_frame[i] = Color(0,0,0);        // Black
    next_frame[i] = Color(0,0,0);           // Black
    strip.setPixelColor(i, Color(0,0,0));
  }
  strip.show();
}

void loop() { 
   
  delay(20);   // The only delay!
  
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
    }
  }
  
  // Morph the display
  
  morph_frame();
  
  // Advance the clock
   
  if (morph++ >= GetDelayTime(wait)) {  // Finished morphing
    
    update = true;  // Force an update
    
    morph = 0;  // Reset morphClock
    
    // Advance the cycle clock
    
    if(cycle++ > 1000) cycle = 0;  
    
    // Update the frame display
  
    for (int i = 0; i < numLights; i++) {
      current_frame[i] = next_frame[i];
    }
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(10)) {
    foreColor = (foreColor + 5) % MaxColor;
    update = true;
  }
  
  if (!random(10)) {
    backColor -= 10;
    if (backColor < 0) backColor += MaxColor;
    update = true;
  }
  
  if (!random(10000)) {
    show = (show + 1) % MAX_SHOW;
    morph = 0;
    cycle = 0;
    clearWithFade();
    delay(1000 * random(5,30));  // Dark for 5-30 seconds
  }
  
  if (!random(1000)) {
    wait++;
    if (wait == MAX_WAIT) {
      wait = 0;
      morph = 0;
    }
  }
}

// clear
//
// set all cells to black but don't call show yet
// ignores buffering
// 
void clear() {
  for (int i=0; i<numLights; i++) {
    strip.setPixelColor(i, Color(0,0,0));
    setPixelColor(i, Color(0,0,0));
  }
}

// clear with fade
//
// Fades lights to black
//
void clearWithFade() {
  for (int i=0; i<numLights; i++) {
    setPixelColor(i, Color(0,0,0));
  }
}

//
// All On
//
// Simply turns all the pixels on to one color
// 
void allOn() {
   for (int i=0; i < strip.numPixels(); i++) {
     setPixelColor(i, Wheel(foreColor));
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
  
  if (pos == 0) {  // Start of show
  
    // Shuffle sort to determine order to turn on lights
    for (i=0; i < numLights; i++) shuffle[i] = i; // before shuffle
    for (i=0; i < numLights; i++) {  // here's position
      j = random(numLights);         // there's position
      save = shuffle[i];
      shuffle[i] = shuffle[j];       // first swap
      shuffle[j] = save;             // second swap
    }
  }
  
  for (i=0; i < numLights; i++) {
    if (i < pos) {  
      setPixelColor(shuffle[i], Wheel(foreColor));  // Turning on lights one at a time
    } else { 
      setPixelColor(shuffle[numLights-(i % numLights)-1], Color(0,0,0));  // Turning off lights one at a time
    }
  }
}  

// random colors
//
// randomcolors: turns each pixel on to a random color
//
void randomcolors() {
  int i;
  
  if (cycle == 0) {  // Start of show: assign lights to random colors
    for (i=0; i < numLights; i++) shuffle[i] = random(MaxColor);
  }
  
  // Otherwise, fill lights with their color
  for (i=0; i < numLights; i++) {
    setPixelColor(i, Wheel(shuffle[i]));
  }
}

// two color
//
// alternates the color of pixels between two colors
//
void twocolor() {
  for (int i=0; i < symLights; i++) {
    if (i%2) setSymPixelColor(i, Wheel(foreColor));
    else setSymPixelColor(i, Wheel(backColor));
  }
}

//
// Morph Chain
//
// Morphs color 1 from position x to
// color 2 at position x+n

void morphChain() {
  float attenuation;
  
  for (int i=0; i < symLights; i++) {
    attenuation = ((i+(cycle%symLights)) % symLights)/(float)(symLights-1);
    setSymPixelColor(i, HSVinter24(Wheel(foreColor),Wheel(backColor), attenuation));
  }
}

//
// Saw tooth
//
// Fills in pixels with a sawtooth of intensity
//
// Peak of the sawtooth moves with the cycle

void sawtooth() {
  float attenuation;
  
  for (int i=0; i < symLights; i++) {
    attenuation = 2*(((i+(cycle%symLights)) % symLights)/(float)(symLights-1));
    if (attenuation > 1) attenuation = 2 - attenuation;  // the '2 -' creates the sawtooth
    attenuation = attenuation * attenuation;  // magnify effect - color brightness is not linear
    setSymPixelColor(i, HSVinter24(Color(0,0,0), Wheel(foreColor), attenuation));
  }
}

//
// rainbowshow: fills the spines with a gradient rainbow
// over time, the starting color changes
//
 
void rainbowshow(int cycles) {
  int diff = abs(foreColor - backColor);
  
  for (int i=0; i < symLights; i++) {
    setSymPixelColor( (i+(cycle%symLights)) % symLights, Wheel(((i*diff / symLights)+foreColor) % diff) );
  }
}

//
// lightwave
//
// Just one pixel traveling along the chain
 
void lightwave() {
 
  for (int i=0; i < symLights; i++) {
     if (i == symLights-(cycle % symLights)-1) setSymPixelColor(i, Wheel(foreColor));
     else setSymPixelColor(i, Color(0,0,0));
  }
}

//
// lightrunup
//
// lights fill up one at time
// 
void lightrunup() {
  int pos = cycle % (symLights*2);  // Where we are in the show
  if (pos >= symLights) pos = (symLights*2) - pos - 1;
  
  for (int i=0; i < symLights; i++) {
    if (i <= pos) {
      setSymPixelColor(i, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setSymPixelColor(i, Color(0,0,0));   // black
    }
  }
}
    
//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code initially from Greg and Robie
//

void morph_frame() {
   for (int i = 0; i < numLights; i++) {
     strip.setPixelColor(i, HSVinter24(current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait)));
   }
   strip.show();  // Update the display 
}

//
// set SymPixelColor(int pos, uint32_t color) {
//
// Forces the two poles to have symmetric pixels

void setSymPixelColor(int pos, uint32_t color) {
  pos = pos % symLights;
  
  next_frame[pos] = color;
  next_frame[numLights - pos - 1] = color;
  
  if (pos == symLights - 1) next_frame[symLights] = color;
}

void setPixelColor(int pos, uint32_t color) {
  next_frame[pos] = color;
}

//
// Get Delay Time
//
// Returns a delay time from an array
//
 
int GetDelayTime(int wait) {
  int DelayValues[MAX_WAIT] = { 2, 3, 4, 6, 8, 10, 15, 20, 30, 50, 75, 100 };
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

//Input a value 255 * 6 to get a color value.
//The colours are a transition r - g -b - back to r
//Intensity must be 0 <= intensity <= 1
uint32_t Gradient_Wheel(int color, float intensity)
{
  int r,g,b;
  int channel, value;
  
  color = color % MaxColor;  // Keep colors within bounds  
  
  channel = color / 255;
  value = color % 255;
  
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

uint32_t HSVinterWheel(byte c1, byte c2, float fract)
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
  //Serial.print("r1 = ");
  //Serial.print(r1);
  //Serial.print(", g1 = ");
  //Serial.print(g1);
  //Serial.print(", b1 = ");
  //Serial.print(b1);
  //Serial.print("r2 = ");
  //Serial.print(r2);
  //Serial.print(", g2 = ");
  //Serial.print(g2);
  //Serial.print(", b2 = ");
  //Serial.println(b2);
  
  if (fract < 0 || fract > 1) {
     Serial.print("fract = ");
     Serial.println(fract);
     return(Color(255,0,0));  // Fract is out of 0-1 bounds. Color defaults to red
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
  
  // Print test bed
  /*
  Serial.print("r1 = ");
  Serial.print(r1);
  Serial.print(", g1 = ");
  Serial.print(g1);
  Serial.print(", b1 = ");
  Serial.print(b1);
  Serial.print(", r = ");
  Serial.print(r);
  Serial.print(", g = ");
  Serial.print(g);
  Serial.print(", b = ");
  Serial.println(b);
  */
  // Return the proper 24-bit color
   
  return(Color(r,g,b));
}

//
//  RGB Interpolate Wheel
//
//  Wrapper for RGB Interpolate RGB below
//  start and end colors are 0-255 wheel colors

uint32_t RGBinterWheel(byte c1, byte c2, float fract)
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
