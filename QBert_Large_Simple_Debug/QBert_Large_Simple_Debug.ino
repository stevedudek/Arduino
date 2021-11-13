#include <FastLED.h>
//
//  Linear Lights with FastLED
//
//  10/13/2017
//

#define NUM_LEDS 61  // 117

#define BRIGHTNESS  255  // (0-255)

#define DATA_PIN 0  // 11
#define CLOCK_PIN 2  // 13

CRGB leds[NUM_LEDS];

// framebuffers
uint32_t current_frame[NUM_LEDS];
uint32_t next_frame[NUM_LEDS];

// For random-fill show (nasty global)
//byte shuffle[NUM_LEDS];  // Will contain a shuffle of lights

// Light colors
byte foreColor =  0;    // Starting foreground color
byte backColor = 99;   // Starting background color
#define MAX_COLOR 255   // Colors are 0-255 (hue)

// Shows
int light_show = 0;       // Starting show
#define MAX_SHOW 6  // Total number of shows

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated
int wait = 2;
#define MAX_WAIT 12 // Number of stored delay times 
#define DELAY_TIME 20   // delay (in ms) between morphs
#define SHOW_DELAY 100000 // delay (in ms) between shows

//
// Setup
//
void setup() {
  
  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");
    
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  // Initialize the strip, to start they are all 'off'
  
  for (int i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = Color(0,0,0);        // Black
    next_frame[i] = Color(0,0,0);           // Black
    leds[i] = CHSV(0,0,0);                  // Black
  }
  FastLED.show();

  for (int hue = 0; hue < 256; hue++) {
    CHSV color = CHSV(hue, 255, 255);
    leds[0] = color;
    Serial.printf("%d: %d, %d, %d\n", hue, leds[0].r, leds[0].g, leds[0].b);
  }
}

void loop() { 

  delay(DELAY_TIME);  // The only delay!
 
  // Check if the lights need updating
  
  if (update) {
    
    update = false;
    
    switch(light_show) {
    
      case 0:    
        allOn();
        break;
      case 1:
        morphChain();
        break;
      case 2:
        twocolor();
        break;
      case 3:
        lightwave();
        break;
      case 4:
        lightrunup();
        break;
      case 5:
        sawtooth();
        break;
    }
  }

  morph_frame();  // Morph the display
  
  // Advance the morph clock
   
  if (morph++ >= GetDelayTime(wait)) {  // Finished morphing
    
    update = true;  // Force an update

    foreColor = (foreColor + 1) % MAX_COLOR;
    
    morph = 0;  // Reset morphClock
    
    // Advance the cycle clock
    
    if(cycle++ > 1000) cycle = 0;  
    
    // Update the frame display
  
    for (int i = 0; i < NUM_LEDS; i++) {
      current_frame[i] = next_frame[i];
    }
  }
  
  // Randomly change foreColor, backColor, show, and timing
  /*
  if (!random(25)) {
    foreColor = (foreColor + 1) % MAX_COLOR;
    update = true;
  }
  
  if (!random(25)) {
    backColor -= 2;
    if (backColor < 0) backColor += MAX_COLOR;
    update = true;
  }
  
  if (!random(10000)) {  // Change the show
    light_show = random(MAX_SHOW);
    morph = 0;
    cycle = 0;
    set_all_black();
    delay(SHOW_DELAY);
  }
  
  if (!random(1000)) {
    if (wait++ >= MAX_WAIT) {
      wait = 0;
      morph = 0;
    }
  }
  */
}

void set_all_black() {
  fill(Color(0,0,0));
}

void fill(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, color);
  }
}

//
// All On - turns all the pixels on the foreColor
// 
void allOn() {
   fill(Wheel(foreColor));
}

/*

//
// randomly fill in pixels from blank to all on, then takes them away
//
void randomfill() {
  int i, j, save, pos;
  
  pos = cycle % (NUM_LEDS*2);  // Where we are in the show
  
  if (pos == 0) {  // Start of show
    shuffle_leds();
  }
  
  for (i=0; i < NUM_LEDS; i++) {
    if (i < pos) {  
      setPixelColor(shuffle[i], Wheel(foreColor));  // Turning on lights one at a time
    } else { 
      setPixelColor(shuffle[NUM_LEDS-(i % NUM_LEDS)-1], Color(0,0,0));  // Turning off lights one at a time
    }
  }
}

//
// Shuffle LEDS
//
void shuffle_leds() {
  int i, j, save;
  
  for (i=0; i < NUM_LEDS; i++) {
    shuffle[i] = i; // before shuffle
  }
  for (i=0; i < NUM_LEDS; i++) {  // here's position
    j = random(NUM_LEDS);         // there's position
    save = shuffle[i];
    shuffle[i] = shuffle[j];       // first swap
    shuffle[j] = save;             // second swap
  }
}

//    
// random colors - turns each pixel on to a random color
//
void randomcolors() {
  if (cycle == 0) {  // Start of show: assign lights to random colors
    for (int i=0; i < NUM_LEDS; i++) {
      shuffle[i] = random(MAX_COLOR);
    }
  }
  
  // Otherwise, fill lights with their color
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(shuffle[i]));
  }
}

*/

//
// two color - alternates the color of pixels between two colors
//
void twocolor() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % 2) {
      setPixelColor(i, Wheel(foreColor));
    } else {
      setPixelColor(i, Wheel(backColor));
    }
  }
}

//
// Morph Chain - morphs color 1 from position x to color 2 at position x+n
//
void morphChain() {
  float attenuation;
  
  for (int i=0; i < NUM_LEDS; i++) {
    attenuation = ((i+(cycle%NUM_LEDS)) % NUM_LEDS)/(float)(NUM_LEDS-1);
    setPixelColor(NUM_LEDS-i-1, Wheel(interpolate_wrap(foreColor, backColor, attenuation)));
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
  
  for (int i=0; i < NUM_LEDS; i++) {
    attenuation = 2*(((i+(cycle%NUM_LEDS)) % NUM_LEDS)/(float)(NUM_LEDS-1));
    if (attenuation > 1) {
      attenuation = 2 - attenuation;  // the '2 -' creates the sawtooth
    }
    // "i" will have pattern move up; "NUM_LEDS-i-1'' will have pattern move down
    setPixelColor(NUM_LEDS-i-1, Gradient_Wheel(foreColor, attenuation));
  }
}

//
// lightwave - just one pixel traveling along the chain
//
void lightwave() {
  for (int i=0; i < NUM_LEDS; i++) {
     if (i == cycle % NUM_LEDS) {
       setPixelColor(i, Wheel(foreColor));
     } else {
       setPixelColor(i, Color(0,0,0));
     }
  }
}

//
// lightrunup -lights fill up one at time
// 
void lightrunup() {
  int pos = cycle % (NUM_LEDS*2);  // Where we are in the show
  if (pos >= NUM_LEDS) {
    pos = (NUM_LEDS*2) - pos - 1;
  }
  
  for (int i=0; i < NUM_LEDS; i++) {
    if (i <= pos) {
      setPixelColor(i, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setPixelColor(i, Color(0,0,0));   // black
    }
  }
}
    
//
// morph_frame
//
void morph_frame() {
   for (int i = 0; i < NUM_LEDS; i++) {
     HSVinter24(i, current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait));
   }
   FastLED.show();  // Update the display 
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
// Create a 24-bit color value from H,S,V
//
uint32_t Color(byte h, byte s, byte v)
{
  uint32_t c = 0;
  c = h;
  c <<= 8;
  c |= s;
  c <<= 8;
  c |= v;
  return c;
}

// Extracts the red part of a 24-bit color
byte GetHue(uint32_t c)
{
  return ((c >> 16) & 0xff);
}

// Extracts the green part of a 24-bit color
byte GetSat(uint32_t c)
{
  return ((c >> 8) & 0xff);
}

// Extracts the green part of a 24-bit color
byte GetValue(uint32_t c)
{
   return (c & 0xff);
}

// Wheel - Input a hue (0-255) to get a color
uint32_t Wheel(byte hue)
{
  return Gradient_Wheel(hue, 1);  // Intensity = 1
}

//Input a value 255 * 6 to get a color value.
//The colours are a transition r - g -b - back to r
//Intensity must be 0 <= intensity <= 1
uint32_t Gradient_Wheel(byte hue, float value)
{
  // h = hue
  // s = 255
  // v = intensity
  return(Color(hue, 255, value * 255));
}

//
//  HSV Interpolate 24-bit
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 24-bit colors

void HSVinter24(int i, uint32_t c1, uint32_t c2, float fract)
{ 
  uint32_t color;
  if (c1 == c2) {
    color = c1;
  } else if (fract <= 0.0) {
    color = c1;
  } else if (fract >= 1.0) { 
    color = c2;
  } else if (c1 == 0) {
    leds[i] = CHSV(GetHue(c2), GetSat(c2), GetValue(c2));
    leds[i].fadeToBlackBy( 256 * (1.0 - fract) );
    return;
  } else if (c2 == 0) {
    leds[i] = CHSV(GetHue(c1), GetSat(c1), GetValue(c1));
    leds[i].fadeToBlackBy( 256 * fract );
    return;
  } else {
    color = interpolate_colors(c1, c2, fract);
  }
  leds[i] = CHSV(GetHue(color), GetSat(color), GetValue(color));
}

//
// interpolate_colors
//
uint32_t interpolate_colors(uint32_t c1, uint32_t c2, float fract)
{
  byte h = interpolate_wrap(GetHue(c1), GetHue(c2), fract);
  byte s = interpolate(GetSat(c1), GetSat(c2), fract);
  byte v = interpolate(GetValue(c1), GetValue(c2), fract);

  return Color(h,s,v);
}

//
// Interpolate
//
// Returns the fractional point from a to b

float interpolate(byte a, byte b, float fract)
{
  return(a + (fract*(b-a)));
}

//
// Interpolate Wrap
//
// Returns the fractional point from a to b
// checking both ways around a circle

float interpolate_wrap(byte a, byte b, float fract)
{
  if(should_wrap(a,b)) {
    if(a < b) a += 255;
    else b += 255;
  }
  byte result = interpolate(a,b,fract);
  if (result > 255) result -= 255;
  return(result);
}

//
// should_wrap
//
// determines whether the negative way
// around the circle is shorter

boolean should_wrap(byte a, byte b) {
  if (a > b) {
    byte temp = a;  // basic a <-> b swap
    a = b;
    b = temp;
  }
  return(abs(b-a) > abs((a+255) - b));
}
