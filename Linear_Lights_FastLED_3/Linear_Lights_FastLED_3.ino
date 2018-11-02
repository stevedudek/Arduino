#include <FastLED.h>
//
//  Linear Lights with FastLED
//
//  Includes palettes
//  Switched to Intrinsic Morphing - DOESN'T WORK!
//  10/16/2017
//

#define NUM_LEDS 10

#define BRIGHTNESS  255 // (0-255)

#define DATA_PIN 9
#define CLOCK_PIN 8

CRGB leds[NUM_LEDS];

// For random-fill show (nasty global)
byte shuffle[NUM_LEDS];  // Will contain a shuffle of lights

// Light colors
byte foreColor =  0;    // Starting foreground color
byte backColor = 99;   // Starting background color
#define MAX_COLOR 255   // Colors are 0-255 (palette color)

// Palettes
CRGBPalette16 currentPalette;
TBlendType    currentBlending;
boolean can_change_palettes = false;  // randomly change palettes
#define DEFAULT_PALETTE RainbowColors_p // starting palette

// Possible palettes:
// RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, 
// ForestColors_p, and PartyColors_p

// Shows
int light_show = 5;       // Starting show
#define MAX_SHOW 8  // Total number of shows

int cycle = 0;      // Keeps track of animation
int wait = 11;
int UPDATES_PER_SECOND;
#define MAX_WAIT 12 // Number of stored delay times 
#define SHOW_DELAY 1000 // delay (in ms) between shows

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay
  
  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  currentPalette = DEFAULT_PALETTE;
  currentBlending = LINEARBLEND;
  
  // Initialize the strip, to start they are all 'off'
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void loop() { 
   
  switch(light_show) {
    
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

  FastLED.show();  // Update the display
  FastLED.delay(1000); // The only delay
//  FastLED.delay(1000 / UPDATES_PER_SECOND); // The only delay
  
  if (cycle++ > 1000) { // Advance the cycle clock
    cycle = 0;  
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(25)) {
    foreColor = (foreColor + 1) % MAX_COLOR;
  }
  
  if (!random(25)) {
    backColor -= 2;
    if (backColor < 0) backColor += MAX_COLOR;
  }
  
  if (!random(10000)) {  // Change the show
    light_show = random(MAX_SHOW);
    cycle = 0;
    set_all_black();
    FastLED.delay(SHOW_DELAY);
  }
  
  if (!random(1000)) {
    if (wait++ >= MAX_WAIT) {
      wait = 0;
    }
  }

  if (can_change_palettes && !random(10000)) {  // Change the palette
    change_palette();
  } 
}

//
// change_palette
//
void change_palette() {
  switch (random(6)) {
    case 0:
      currentPalette = RainbowColors_p;
      break;
    case 1:
      currentPalette = RainbowStripeColors_p;
      break;
    case 2:
      currentPalette = OceanColors_p;
      break;
    case 3:
      currentPalette = CloudColors_p;
      break;
    case 4:
      currentPalette = LavaColors_p;
      break;
    case 5:
      currentPalette = ForestColors_p;
      break;
    default:
      currentPalette = PartyColors_p;
      break;
  }
}

void set_all_black() {
  fill(CRGB::Black);
}

void fill(CRGB color) {
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
      setPixelColor(shuffle[NUM_LEDS-(i % NUM_LEDS)-1], CRGB::Black);  // Turning off lights one at a time
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
//
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
       setPixelColor(i, CRGB::Black);
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
      setPixelColor(i, CRGB::Black);   // black
    }
  }
}

//
// Set Delay Time - returns a delay time from an array
//
int setDelayTime(int wait) {
  int DelayValues[MAX_WAIT] = { 2, 3, 4, 6, 8, 10, 15, 20, 30, 50, 75, 100 };
  UPDATES_PER_SECOND = (10 * DelayValues[wait % MAX_WAIT]);
}

//
// setPixelColor
//
void setPixelColor(int i, CRGB color) {
  leds[i] = color;
}

//
//  Wheel - Input a hue (0-255) to get a color
//
CRGB Wheel(byte hue) {
  return Gradient_Wheel(hue, 1);  // Intensity = 1
}

//
//  Gradient_Wheel - Input a hue (0-255) and intensity (0-1) to get a CHSV from the palette
//
CRGB Gradient_Wheel(byte hue, float intensity) {
  return ColorFromPalette( currentPalette, hue, 255 * intensity, currentBlending);
}

//
// Interpolate - returns the fractional point from a to b
//
float interpolate(byte a, byte b, float fract)
{
  return(a + (fract*(b-a)));
}

//
// Interpolate Wrap
//
// Returns the fractional point from a to b
// checking both ways around a circle
//
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
//
boolean should_wrap(byte a, byte b) {
  if (a > b) {
    byte temp = a;  // basic a <-> b swap
    a = b;
    b = temp;
  }
  return(abs(b-a) > abs((a+255) - b));
}
