//#include <TimerOne.h>

#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Linear Lights with Threaded Shows
//
//  Standard program for a linear light strip
//
//  Set the number of lights with numLights below
//  Modern 8-bit color lights
//
//  Morphing is HSV-corrected (thanks, Greg)
//
//  No external controller like buttons or an X-Bee
//
//  Shows can be Threaded
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights.
// 
#define dataPin 8       // 'yellow' wire
#define clockPin 9      // 'green' wire

#define numLights 9     // Centurion helmet

// Thread buffers
#define MAX_THREADS 4

#define FULL_THREADS 99

#define LIFE_POS      0  // Position of variables for each thread
#define MORPH_POS     1
#define SHOW_POS      2
#define FORECOLOR_POS 3
#define BACKCOLOR_POS 4
#define WAIT_POS      5
#define UPDATE        6

int thread_vars[MAX_THREADS][7];    // See XXX_POS numbers above

// framebuffers
uint32_t current_frame[MAX_THREADS][numLights]; // Each thread has its own 2 frame buffers
uint32_t next_frame[MAX_THREADS][numLights];
uint32_t frame[numLights];  // Master buffer that sums up the threads
byte occupancy[numLights];  // How many colors stuffed into each light

// For random-fill show (nasty global)
byte shuffle[MAX_THREADS][numLights];  // Will contain a shuffle of lights

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Light colors - Fore and Back color handled by thread_vars above

#define MaxColor 255    // Colors go from 0 to 255

// Shows - show handled by thread_vars above - Dark = 0

#define MAX_SHOW 9  // Total number of shows

// Morphing (minute hand), cycles (hour hand), and update (boolean) handled by thread_vars above

// Wait handled by thread_vars above

#define MAX_WAIT 12 // Number of stored delay times 

//
// Setup
//

void setup() {
    
  Serial.begin(9600);
  //Serial.println("Start");
    
  // Start up the LED counter
  strip.begin();
  
  // Turn off all threads. life = 0 for all threads
  for (int i=0; i < MAX_THREADS; i++) thread_vars[i][LIFE_POS] = 0;
  
  startNewThread();  // Have a thread going on power on
}

void loop() { 
   
  delay(20);  // The only delay!
 
  clearFrame(); // Empty the lights
  
  checkThreads(); // Check each thread and add together thread's light intensity 
  
  writeFrame(); // Push the frame on to the lights
  
  /*
  // Check if the lights need updating
  
  if (update) {
    
    update = false;
    
    switch(show) {
    
      case 0:
        clearWithFade();
        break;
      case 1:    
        allOn();
        break;
      case 2:
        morphChain();
        break;
      case 3:
        randomfill();
        break;
      case 4:
        randomcolors();
        break;
      case 5:
        twocolor();
        break;
      case 6:
        lightwave();
        break;
      case 7:
        lightrunup();
        break;
      case 8:
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
  */
  
  randomizeThreads(); // Change the active threads slightly
  
  if (!random(1000) ) startNewThread(); // Randomly start a new threads
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

// Turn all one color
//
// Turn all pixels foreceably one color, then delays
//
void TurnAllOneColor(uint32_t color, int rest) {
  for (int i=0; i<numLights; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
  delay(rest);
}

// Turn all white
//
// Turns all pixels forceably white, then delays
//
void TurnAllWhite(int rest) {
  TurnAllOneColor(Color(255,255,255), rest);
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
  for (int i=0; i < numLights; i++) {
    if (i%2) setPixelColor(i, Wheel(foreColor));
    else setPixelColor(i, Wheel(backColor));
  }
}

//
// Morph Chain
//
// Morphs color 1 from position x to
// color 2 at position x+n

void morphChain() {
  float attenuation;
  
  for (int i=0; i < numLights; i++) {
    attenuation = ((i+(cycle%numLights)) % numLights)/(float)(numLights-1);
    setPixelColor(i, HSVinter24(Wheel(foreColor),Wheel(backColor), attenuation));
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
  
  for (int i=0; i < numLights; i++) {
    attenuation = 2*(((i+(cycle%numLights)) % numLights)/(float)(numLights-1));
    if (attenuation > 1) attenuation = 2 - attenuation;  // the '2 -' creates the sawtooth
    attenuation = attenuation * attenuation;  // magnify effect - color brightness is not linear
    setPixelColor(i, HSVinter24(Color(0,0,0), Wheel(foreColor), attenuation));
  }
}

//
// rainbowshow: fills the spines with a gradient rainbow
// over time, the starting color changes
//
 
void rainbowshow(int cycles) {
  int diff = abs(foreColor - backColor);
  
  for (int i=0; i < numLights; i++) {
    setPixelColor( (i+(cycle%numLights)) % numLights, Wheel(((i*diff / numLights)+foreColor) % diff) );
  }
}

//
// lightwave
//
// Just one pixel traveling along the chain
 
void lightwave() {
 
  for (int i=0; i < numLights; i++) {
     if (i == numLights-(cycle % numLights)-1) setPixelColor(i, Wheel(foreColor));
     else setPixelColor(i, Color(0,0,0));
  }
}

//
// lightrunup
//
// lights fill up one at time
// 
void lightrunup() {
  int pos = cycle % (numLights*2);  // Where we are in the show
  if (pos >= numLights) pos = (numLights*2) - pos - 1;
  
  for (int i=0; i < numLights; i++) {
    if (i <= pos) {
      setPixelColor(i, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setPixelColor(i, Color(0,0,0));   // black
    }
  }
}

//
// Thread Routines
//

// countThreads
//
// Return the number of active threads (life > 0)
byte countThreads() {
  int threads = 0;
  
  for (int i=0; i < MAX_THREADS; i++) {
    if (thread_vars[i][LIFE_POS] > 0) threads++;
  }
  return (threads);
}

// firstEmptyThread
//
// Return the position of the first empty thread
// If all positions are filled, returns FULL_THREADS
byte firstEmptyThread() {
  for (int i=0; i < MAX_THREADS; i++) {
    if (thread_vars[i][LIFE_POS] == 0) return (i);
  }
  return (FULL_THREADS);
}

// startNewThread
//
// Kicks off a new thread
void startNewThread() {
  int newThreadPos = firstEmptyThread();
  if (newThreadPos == FULL_THREADS) return;  // All threads full
  
  // Pick random values for the new thread
  thread_vars[newThreadPos][LIFE_POS] =       random(100,1000);  // How long a thread will live
  thread_vars[newThreadPos][MORPH_POS] = 0;
  thread_vars[newThreadPos][SHOW_POS] =       random(1,MAX_SHOW);  // Show 0 = All Black
  thread_vars[newThreadPos][FORECOLOR_POS] =  random(MaxColor);
  thread_vars[newThreadPos][BACKCOLOR_POS] =  random(MaxColor);
  thread_vars[newThreadPos][WAIT_POS] =       random(MAX_WAIT);
  thread_vars[newThreadPos][UPDATE] = true;
}

//
// Check Threads
//
void checkThreads() {
  for (int i=0; i < MAX_THREADS; i++) {
    checkThread(i);
  }
}

//
// Check Thread
//
void checkThread(byte thread) {
  if (thread_vars[thread][LIFE_POS] == 0) return;  // Thread is dead
  
  if (thread_vars[thread][UPDATE] == true) {
    thread_vars[thread][UPDATE] = false;
    
    //check show
  }
  
  // Morph the display
  
  morph_frame(thread);
  
  // Advance the morph clock
   
  if (thread_vars[thread][MORPH_POS]++ >= GetDelayTime(thread_vars[thread][WAIT_POS])) {  // Finished morphing
    
    thread_vars[thread][UPDATE] = true;  // Force an update
    
    thread_vars[thread][MORPH_POS] = 0;  // Reset morphClock
    
    // Decrease the thread's life clock
    
    if(thread_vars[thread][LIFE_POS] > 0) thread_vars[thread][LIFE_POS]--;  
    
    // Update the thread's frame display
  
    for (int i = 0; i < numLights; i++) {
      current_frame[thread][i] = next_frame[thread][i];
    }
  }
}

// randomizeThreads
//
// Slowly randomizes all active threads
//
void randomizeThreads() {
  for (int i=0; i < MAX_THREADS; i++) {
    randomizeThread(i);
  }
}
  
// randomize Thread
//
// Slowly randomizes a thread to make life more interesting
// Ignores dead threads

void randomizeThread(byte thread) {
  if (thread_vars[thread][LIFE_POS] == 0) return;  // Thread is dead
  
  // foreColor
  if (!random(25)) {
    thread_vars[thread][FORECOLOR_POS] = (thread_vars[thread][FORECOLOR_POS] + 1) % MaxColor;
    thread_vars[thread][UPDATE] = true;
  }
  
  // backColor
  if (!random(25)) {
    thread_vars[thread][BACKCOLOR_POS] -= 2;
    if (thread_vars[thread][BACKCOLOR_POS] < 0) thread_vars[thread][BACKCOLOR_POS] += MaxColor;
    thread_vars[thread][UPDATE] = true;
  }
  
  // wait
  if (!random(1000)) {
    thread_vars[thread][WAIT_POS]++;
    if (thread_vars[thread][WAIT_POS] >= MAX_WAIT) {
      thread_vars[thread][WAIT_POS] = 0;
      thread_vars[thread][MORPH_POS] = 0;
    }
  }
}  


//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code initially from Greg and Robie
//

//
// Clear Frame
//
// Empty the master light Frame
//
void clearFrame() {
  for (int i = 0; i < numLights; i++) {
    frame[i] = 0;
    occupancy[i] = 0;
  }
}

//
// Write Frame
//
// Push the master frame on to the actual lights
//
void writeFrame() {
  for (int i = 0; i < numLights; i++) {
    strip.setPixelColor(i, frame[i]);
  }
  strip.show();  // Update the display
}

//
// Morph Frame
//
void morph_frame(byte thread) {
   uint32_t color;
   float fract;
   
   for (int i = 0; i < numLights; i++) {
     // Determine the interpolated fraction
     fract = (float)thread_vars[thread][MORPH_POS]/GetDelayTime(thread_vars[thread][WAIT_POS]);
     // get the desired morphed color for the particular thread
     color = HSVinter24(current_frame[thread][i], next_frame[thread][i], fract);
     // Push color into global frame
     addColor(i, color);
   } 
}

//
// Add Color
//
void addColor(int pos, uint32_t color) {
  float fract;
  occupancy[pos]++;
  
  if (occupancy[pos] == 1) {  // This is the first color
    frame[pos] = color;
  } else {
    frame[pos] = HSVinter24(frame[pos], color, (float)1/occupancy[pos] );
  }
}

/*
void setPixelColor(int pos, uint32_t color) {
  next_frame[pos] = color;
}
*/

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


//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
  int r,g,b;
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
  return Color(r,g,b);
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
