#include <FastLED.h>
//
//  Linear Lights with FastLED
//
//  Palettes
//  Enumerated Shows
//  (Layer Effects)
//  (Noise)
//
//  10/18/2017
//

#define NUM_LEDS 10

#define BRIGHTNESS  125 // (0-255)

#define DELAY_TIME 20 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

CRGB leds[NUM_LEDS];

CRGB current_frame[NUM_LEDS]; // framebuffers
CRGB next_frame[NUM_LEDS];  // framebuffers

uint8_t shuffle[NUM_LEDS];  // For random-fill show (nasty global)

// Light colors
uint8_t foreColor =  0;    // Starting foreground color
uint8_t backColor = 99;   // Starting background color
#define MAX_COLOR 255   // Colors are 0-255 (palette color)

// Palettes
CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;
TBlendType    currentBlending;
boolean can_change_palettes = false;  // randomly change palettes
#define DEFAULT_PALETTE RainbowColors_p // starting palette

// Possible palettes:
// RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, 
// ForestColors_p, and PartyColors_p

// Shows - now enumerated
typedef void (*ShowList[])();
ShowList Shows = { allOn, morphChain, randomfill, randomcolors, twocolor, lightwave, lightrunup, sawtooth };
uint8_t current_show = 0; // Index number of which pattern is current

uint8_t morph = 0;       // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
uint8_t wait = 6;
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
  targetPalette = DEFAULT_PALETTE;
  currentBlending = LINEARBLEND;
  
  // Initialize the strip, to start they are all 'off'
  
  for (int i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = CRGB::Black;
    next_frame[i] = CRGB::Black;
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void loop() { 

  if (morph == 0) { 
    Shows[current_show]();  // Function call to the current show
  }

  morph_frame();  // Morph the display
  
  delay(DELAY_TIME); // The only delay
  
  // Advance the morph clock

  if (morph++ >= GetDelayTime(wait)) {  // Finished morphing

    change_it_up();
    
    morph = 0;  // Reset morphClock
    
    if (cycle++ > 1000) { cycle = 0; }  // Advance the cycle clock
    
    for (int i = 0; i < NUM_LEDS; i++) {
      current_frame[i] = next_frame[i]; // Update the frame display
    }

    // Palette blending
    uint8_t maxChanges = 24; 
    nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);
  }
}

//
// change_it_up - randomly change foreColor, backColor, show, and timing
//
void change_it_up() {
  int delay_time = GetDelayTime(wait);  // 1 = fast; 250 = slow
  
  if (!random(250 / delay_time)) { foreColor = IncColor(foreColor, 1); }
  if (!random(250 / delay_time)) {  backColor = IncColor(foreColor, -2); }
  if (!random(5000 / delay_time)) { next_show(); }
  if (!random(500 / delay_time)) {
    if (wait++ >= MAX_WAIT) {
      wait = 0;
      morph = 0;
    }
  }
}

//
// next_show
//
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void next_show() {
  current_show = random(ARRAY_SIZE( Shows));
  morph = 0;
  cycle = 0;
  set_all_black();

  if (can_change_palettes && !random(10)) {  // Change the palette
    change_palette();
  } 
}

//
// change_palette
//
void change_palette() {
  switch (random(6)) {
    case 0:
      targetPalette = RainbowColors_p;
      break;
    case 1:
      targetPalette = RainbowStripeColors_p;
      break;
    case 2:
      targetPalette = OceanColors_p;
      break;
    case 3:
      targetPalette = CloudColors_p;
      break;
    case 4:
      targetPalette = LavaColors_p;
      break;
    case 5:
      targetPalette = ForestColors_p;
      break;
    default:
      targetPalette = PartyColors_p;
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

  if (pos > NUM_LEDS) {
    pos = (2 * NUM_LEDS) - pos;
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
// Saw tooth - Fills in pixels with a sawtooth of intensity
//
void sawtooth() {
  float attenuation;
  
  for (int i=0; i < NUM_LEDS; i++) {
    attenuation = 2*(((i+(cycle%NUM_LEDS)) % NUM_LEDS)/(float)(NUM_LEDS-1));
    if (attenuation > 1) {
      attenuation = 2 - attenuation;  // the '2 -' creates the sawtooth
    }
    // "i" will have pattern move up; "NUM_LEDS-i-1'' will have pattern move down
    setPixelColor(NUM_LEDS-i-1, Gradient_Wheel(foreColor, 255 * attenuation));
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
// morph_frame
//
void morph_frame() {
   for (int i = 0; i < NUM_LEDS; i++) {
     setLEDinterpCRGB(i, current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait));
   }
   FastLED.show();  // Update the display 
}

void setPixelColor(int pos, CRGB color) {
  next_frame[pos] = color;
}

//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t color, int amount) {
  int value = color + amount;
  while (value < 0) value += MAX_COLOR;
  while (value >= MAX_COLOR) value -= MAX_COLOR;
  return value;
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

//
//  Wheel - Input a hue (0-255) to get a color
//
CRGB Wheel(uint8_t hue)
{
  return Gradient_Wheel(hue, 255);  // 255 = full brightness
}

//
//  Gradient_Wheel - Input a hue and intensity to get a CHSV from the palette
//
//  Intensity must be 0 <= intensity <= 255
//
CRGB Gradient_Wheel(uint8_t hue, uint8_t intensity)
{
  return ColorFromPalette( currentPalette, hue, intensity, currentBlending);
}

//
//  setLEDinterpCRGB - Set LED i to the interpolate of two CRGB colors 
//
void setLEDinterpCRGB(int i, CRGB c1, CRGB c2, float fract)
{  
  if (c1 == c2) {
    leds[i] = c1;
    return;
  } else if (fract <= 0.0) {
    leds[i] = c1;
    return;
  } else if (fract >= 1.0) { 
    leds[i] = c2;
    return;
  } else if (is_black(c1)) {
    leds[i] = c2;
    leds[i].fadeToBlackBy( 255 * (1.0 - fract) );
    return;
  } else if (is_black(c2)) {
    leds[i] = c1;
    leds[i].fadeToBlackBy( 255 * fract );
    return;
  } else {
    leds[i] = c1.lerp8(c2, uint8_t(255 * fract));
//    leds[i] = HSVinterp(c1, c2, fract);
    return;
  }
}

// is_black
boolean is_black(CRGB color) {
  return (color.r == 0 && color.g == 0 && color.b == 0);
}

//
// Interpolate between 2 CRGB colors using HSV
//
CRGB HSVinterp(CRGB a, CRGB b, float fract) {
  CRGB new_color;
  CHSV c1 = rgb2hsv_approximate(a); // convert to HSV
  CHSV c2 = rgb2hsv_approximate(b); // convert to HSV
  
  uint8_t h = interpolate_wrap(c1.h, c2.h, fract);
  uint8_t s = interpolate(c1.s, c2.s, fract);
  uint8_t v = interpolate(c1.v, c2.v, fract);
  
  return new_color.setHSV(h,s,v);
}

//
// Interpolate - returns the fractional point from a to b
//
uint8_t interpolate(uint8_t a, uint8_t b, float fract)
{
  return(a + (fract*(b-a)));
}

//
// Interpolate Wrap
//
// Returns the fractional point from a to b
// checking both ways around a circle
//
uint8_t interpolate_wrap(uint8_t a, uint8_t b, float fract)
{
  if(should_wrap(a,b)) {
    if(a < b) a += 255;
    else b += 255;
  }
  uint8_t result = interpolate(a,b,fract);
  if (result > 255) result -= 255;
  return result;
}

//
// should_wrap
//
// determines whether the negative way
// around the circle is shorter
//
boolean should_wrap(uint8_t a, uint8_t b) {
  if (a > b) {
    uint8_t temp = a;  // basic a <-> b swap
    a = b;
    b = temp;
  }
  return(abs(b-a) > abs((a+255) - b));
}
