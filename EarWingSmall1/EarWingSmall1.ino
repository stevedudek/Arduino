
#include <Adafruit_DotStar.h>

//
//  Small Ear Wings
//
//  3/20/17
//
//  Dotstar strip lights on corrugated Plastic
//  No X-bee, No HSV
//  Has brightness and red toggle
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights.
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 28
#define symLights 14

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// For random-fill show (nasty global)
byte shuffle[numLights];  // Will contain a shuffle of lights

// Set the first variable to the NUMBER of pixels.
Adafruit_DotStar strip = Adafruit_DotStar(numLights, dataPin, clockPin, DOTSTAR_BRG);

// Light colors

int foreColor =   0;    // Starting foreground color
int backColor = 500;    // Starting background color
#define MaxColor 1536    // Colors are 6 * 255

float BRIGHTNESS = 0.20;  // 0.0 - 1.0 brightness level
boolean ONLY_RED = true; // Set true for only Reds

// Shows

int curr_show = 0;       // Starting show
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

  shuffle_lights();
  
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
    
    switch(curr_show) {
    
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
    push_frame();
    morph = 0;  // Reset morphClock
    
    if (cycle++ > 1000) cycle = 0;  // Advance the cycle clock
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(10)) {
    foreColor = IncColor(foreColor, 10);
  }
  
  if (!random(10)) {
    backColor = IncColor(backColor, -5);
  }
  
  if (!random(10000)) {
    curr_show = random(MAX_SHOW);
    morph = 0;
    cycle = 0;
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

// clear
//
// set all cells to black but don't call show yet
// ignores buffering
// 
void clear() {
  for (int i=0; i<numLights; i++) {
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
  fill(Color(0,0,0));
}

//
// fill
//
void fill(uint32_t color) {
  for (int i = 0; i < numLights; i++) {
    setPixelColor(i, color);
  }
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
  
  pos = cycle % (numLights*2);  // Where we are in the show
  if (pos >= numLights) {
    pos = (numLights*2) - pos;  // For a sawtooth effect
  }
  
  if (pos == 0) {  // Start of show
    shuffle_lights();
  }
  
  for (i=0; i < numLights; i++) {
    if (i < pos) {  
      setPixelColor(shuffle[i], Wheel(foreColor));  // Turning on lights one at a time
    } else { 
      setPixelColor(shuffle[numLights-(i % numLights)-1], Color(0,0,0));  // Turning off lights one at a time
    }
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
    setPixelColor(i, Wheel(shuffle[i]));
  }
}

// two color
//
// alternates the color of pixels between two colors
//
void twocolor() {
  for (int i=0; i < symLights; i++) {
    if (i%2) {
      setSymPixelColor(i, Wheel(foreColor));
    }
    else {
      setSymPixelColor(i, Wheel(backColor));
    }
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
    setSymPixelColor(i, RGBinter24(Wheel(foreColor),Wheel(backColor), attenuation));
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
    setSymPixelColor(i, RGBinter24(Color(0,0,0), Wheel(foreColor), attenuation));
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
     if (i == symLights-(cycle % symLights)-1) {
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
  int pos = cycle % (symLights*2);  // Where we are in the show
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
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code initially from Greg and Robie
//

void morph_frame() {
   for (int i = 0; i < numLights; i++) {
     strip.setPixelColor(i, RGBinter24(current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait)));
   }
   strip.show();  // Update the display 
}

//
// set SymPixelColor(int pos, uint32_t color) {
//
// Forces the two earwings to have symmetric pixels

void setSymPixelColor(int pos, uint32_t color) {
  pos = pos % symLights;
  
  next_frame[pos] = color;
  next_frame[numLights - pos - 1] = color; 
}

void setPixelColor(int pos, uint32_t color) {
  next_frame[pos] = color;
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
  int DelayValues[MAX_WAIT] = { 4, 8, 12, 20, 30, 40, 50, 60, 75, 100, 125, 150 };
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
  
  color = color % MaxColor;  // Keep colors within bounds  
  
  // Red party
  if (ONLY_RED) {
    color = color * 2 / 3;  // Break color into 4 (not 6) channels
  }
  
  channel = color / 256;
  value = color % 256;
  
  if (intensity > 1) intensity = 1;
  if (intensity < 0) intensity = 0;
  
  intensity *= BRIGHTNESS;
  
  // Red party - These values are different
  if (ONLY_RED) {
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
  }
   
  return(Color(r * intensity, g * intensity, b * intensity));
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
