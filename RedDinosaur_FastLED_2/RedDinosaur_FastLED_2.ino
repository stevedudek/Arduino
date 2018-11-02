#include <FastLED.h>
//
//  Red Dinosaur
//
//  29 LEDs = 7 left side + 15 center + 7 right side
//
//  10/19/17
//
//  FastLED
//
#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 20 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 29
#define SYM_LEDS 22
#define SPINE_LEDS 14

CRGB leds[NUM_LEDS];
CRGB current_frame[NUM_LEDS]; // framebuffers
CRGB next_frame[NUM_LEDS];  // framebuffers

uint8_t shuffle[NUM_LEDS];  // For random-fill show (nasty global)

// Light colors
uint8_t foreColor =  0;    // Starting foreground color
uint8_t backColor = 99;   // Starting background color
uint8_t foreColorSpeed = 5; // Best (1-10) with 1=fast
uint8_t backColorSpeed = 5; // Best (1-10) with 1=fast
#define MAX_COLOR_SPEED 10   // Higher = slower
#define MAX_COLOR 255   // Colors are 0-255 (palette color)

// Palettes
CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;
TBlendType    currentBlending;
#define CAN_CHANGE_PALETTES true
#define DEFAULT_PALETTE RainbowColors_p // starting palette
#define PALETTE_DURATION 600  // seconds between palettes

// Possible palettes:
// RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, 
// ForestColors_p, and PartyColors_p

// Shows - now enumerated
typedef void (*ShowList[])();
ShowList Shows = { allOn, morphChain, randomfill, randomcolors, twocolor, lightwave, lightrunup, 
                   sawtooth, colorsize, brightsize, colorarrow, brightarrow };

uint8_t current_show = 0; // Index number of which pattern is current
#define SHOW_DURATION 60  // seconds between shows

// Clocks and time
uint8_t morph = 0;
uint16_t small_cycle = 0;
uint16_t cycle = 0;
uint8_t wait = 6;
#define MAX_WAIT 12 // Number of stored delay times
#define WAIT_DURATION 20 // second between increasing wait time
#define SHOW_DELAY 2000 // darkness (in ms) between shows

// center column | left side | right side
const uint8_t ConeLookUp[NUM_LEDS]  = { 0,1,3,5,6,7,8,19,18,17,16,15,26,27,28,
                                2,9,10,11,12,13,14,
                                4,20,21,22,23,24,25 };

const uint8_t ConeSize[NUM_LEDS] PROGMEM = { 2,1,2,3,2,1,1,1,1,1,1,2,3,4,5,
                      1,0,0,0,0,0,0,
                      1,0,0,0,0,0,0 };

const uint8_t ArrowPattern[NUM_LEDS] PROGMEM = 
{ 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                  2, 7, 6, 5, 4, 3, 2,
                  2, 7, 6, 5, 4, 3, 2 };

#define NUM_PATTERNS 6   // Total number of patterns

uint8_t PatternMatrix[NUM_PATTERNS][NUM_LEDS] = {
{ 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2, 1, 2, 1,
                  1, 2, 1, 2, 1, 2, 1,
                  1, 2, 1, 2, 1, 2, 1 },
                
{ 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
                  1, 2, 1, 2, 1, 2, 1,
                  2, 1, 2, 1, 2, 1, 2 },

{ 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
                  2, 1, 2, 1, 2, 1, 2,
                  1, 2, 1, 2, 1, 2, 1 },

{ 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 1, 2, 1, 2,
                1, 2, 1, 2, 1, 2, 1,
                1, 2, 1, 2, 1, 2, 1 },

{ 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 2,
                  1, 2, 2, 1, 2, 2, 1,
                  1, 2, 2, 1, 2, 2, 1 },

{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                  1, 1, 1, 1, 1, 1, 1,
                  1,  1, 1, 1, 1, 1, 1 },
                
};

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
  
  set_all_black();
}

void loop() { 

  Shows[current_show]();  // Function call to the current show

  morph_frame();  // Morph the display and update the LEDs
  
  delay(DELAY_TIME); // The only delay

  morph++;
  small_cycle++;
  
  // Adjust colors
  if (small_cycle % foreColorSpeed == 0) { foreColor = (foreColor + 1) % MAX_COLOR; }
  if (small_cycle % backColorSpeed == 0) { backColor = (backColor + 1) % MAX_COLOR; }
  
  if (morph >= GetDelayTime(wait)) {  // Finished morphing

    morph = 0;
    
    if (cycle++ > 10000) { cycle = 0; }  // Advance the cycle clock
    
    for (int i = 0; i < NUM_LEDS; i++) {
      current_frame[i] = next_frame[i]; // Push the frame
    }
    // blend_palette();

    change_it_up();
  }
}

//
// change_it_up
//
void change_it_up() {
  EVERY_N_SECONDS(SHOW_DURATION) { next_show(); }
  EVERY_N_SECONDS(PALETTE_DURATION) { change_palette(); }
  EVERY_N_SECONDS(WAIT_DURATION) { wait = (wait + 1) % MAX_WAIT; }
}

//
// Blend Palette
//
void blend_palette() {
  uint8_t maxChanges = 24; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);
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
  foreColorSpeed = random(1, MAX_COLOR_SPEED);
  backColorSpeed = random(1, MAX_COLOR_SPEED);
  delay(SHOW_DELAY);
}

//
// change_palette
//
void change_palette() {
  if (!CAN_CHANGE_PALETTES) return;
  
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
  for (int i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = CRGB::Black;
    next_frame[i] = CRGB::Black;
    leds[i] = CRGB::Black;
  }
  FastLED.show();
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
  if (cycle == 0 && morph == 0) {  // Start of show: assign lights to random colors
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
  for (int i = 0; i < SYM_LEDS; i++) {
    if (i % 2) {
      setSymPixelColor(i, Wheel(foreColor));
    }
    else {
      setSymPixelColor(i, Wheel(backColor));
    }
  }
}

//
// patterns shows - not part of the usual show loop
//
void patterns(int cycles) {
  int pattern = random(NUM_PATTERNS);    // Pick a random pattern
  int i, ticks = 0;
  CRGB color;
  
  while (ticks++ < cycles) {
      if (!random(10)) {       // The not-MAIN_COLOR color
        foreColor = IncColor(foreColor, 10); 
      }
        
      for (i=0; i < NUM_LEDS; i++) {
        switch (PatternMatrix[pattern][i]) {
          case 0: {        // Off (black)
            color = CRGB::Black;
            break;
          }
          case 1: {        // backColor
            color = Wheel(backColor);
            break;
          }
          case 2: {        // foreColor
            color = Wheel(foreColor);
            break;
          }
        }
        leds[ConeLookUp[i]] = color;
      }
      FastLED.show();  // Update the display 
      delay(DELAY_TIME);
  }  
}

//
// Morph Chain - Morphs color 1 from position x to color 2 at position x+n
//
void morphChain() {
  float attenuation;
  
  for (int i = 0; i < SYM_LEDS; i++) {
    attenuation = ((i + (cycle % SYM_LEDS)) % SYM_LEDS) / (float)(SYM_LEDS - 1);
    setSymPixelColor(i, Wheel(interpolate_wrap(foreColor, backColor, attenuation)));
  }
}

//
// Saw tooth - fills in pixels with a sawtooth of intensity
//
void sawtooth() {
  float attenuation;
  
  for (int i = 0; i < SYM_LEDS; i++) {
    attenuation = 2 * (((i + (cycle % SYM_LEDS)) % SYM_LEDS) / (float)(SYM_LEDS - 1));
    if (attenuation > 1) {
      attenuation = 2 - attenuation;  // the '2 -' creates the sawtooth
    }
    setSymPixelColor(i, Gradient_Wheel(foreColor, 255 * attenuation));
  }
}

//
// colorsize - light each cone according to its cone size
//
void colorsize() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(IncColor(foreColor, pgm_read_byte_near(ArrowPattern + i) * backColor)));
  }
  foreColor = IncColor(foreColor, 40);
}

//
// brightsize -Light just one cone size at a time
//
void brightsize() {
  float intensity;
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(pgm_read_byte_near(ArrowPattern + i), 9)));
  }
}

//
// colorarrow - light each cone according to its arrow position
//
void colorarrow() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(IncColor(foreColor, pgm_read_byte_near(ConeSize + i) * backColor)));
  }
  foreColor = IncColor(foreColor, 40);
}

//
// brightarrow -Light just one arrow piece at a time
//
void brightarrow() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(pgm_read_byte_near(ConeSize + i), 5)));
  }
}

//
// calcIntensity - use sine wave + cycle + variable to calculate intensity
//
uint8_t calcIntensity(uint8_t x, uint8_t max_x) {
  return sin8_C( 255 * ((cycle + x) % max_x) / (max_x - 1));
}

//
// rainbowshow - fills with a gradient rainbow; over time, the starting color changes
//
void rainbowshow(int cycles) {
  uint8_t diff = abs(foreColor - backColor);
  
  for (int i = 0; i < SYM_LEDS; i++) {
    setSymPixelColor( (i+(cycle % SYM_LEDS)) % SYM_LEDS, Wheel(((i*diff / SYM_LEDS)+foreColor) % diff) );
  }
}

//
// lightwave - just one pixel traveling along the chain
//
void lightwave() {
  for (int i = 0; i < SYM_LEDS; i++) {
     if (i == SYM_LEDS - (cycle % SYM_LEDS) - 1) {
      setSymPixelColor(i, Wheel(foreColor));
     }
     else {
      setSymPixelColor(i,CRGB::Black);
     }
  }
}

//
// lightrunup - lights fill up one at time
// 
void lightrunup() {
  int pos = cycle % (SYM_LEDS * 2);  // Where we are in the show
  if (pos >= SYM_LEDS) {
    pos = (SYM_LEDS*2) - pos;
  }
  
  for (int i=0; i < SYM_LEDS; i++) {
    if (i < pos) {
      setSymPixelColor(i, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setSymPixelColor(i, CRGB::Black);   // black
    }
  }
}

//
// morph_frame
//
void morph_frame() {
   float fract = float(morph % (GetDelayTime(wait) + 1)) / GetDelayTime(wait);  // 0 - 1

   for (int i = 0; i < NUM_LEDS; i++) {
     setLEDinterpCRGB(i, current_frame[i], next_frame[i], fract);
   }
   FastLED.show();  // Update the display 
}

//
// set SymPixelColor - forces the two sides to have symmetric pixels
//
void setSymPixelColor(int pos, CRGB color) {
  // 0 - 13 : just light the center spine
  // 14 - 20: light the left side and light the right side the same
  // 20 - 25: ignore the right side
  
  if (pos < SYM_LEDS) {
    next_frame[ConeLookUp[pos]] = color;

    if (pos >= SPINE_LEDS) {
      next_frame[ConeLookUp[pos + 6]] = color;
    }
  } 
}

void setPixelColor(int pos, CRGB color) {
  next_frame[ConeLookUp[pos]] = color;
}

//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t color, int amount) {
  int value = color + amount;
  while (value < 0) value += MAX_COLOR;
  while (value >= MAX_COLOR) value -= MAX_COLOR;
  Serial.print(value);
  Serial.print(" ");
  return value;
}

//
// Get Delay Time - returns a delay time from an array
//
int GetDelayTime(int wait) {
  int DelayValues[MAX_WAIT] = { 4, 6, 8, 12, 20, 30, 50, 75, 100, 150, 200, 250 };
  return (DelayValues[(wait % MAX_WAIT)]);
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
    leds[i].fadeToBlackBy( 256 * (1.0 - fract) );
    return;
  } else if (is_black(c2)) {
    leds[i] = c1;
    leds[i].fadeToBlackBy( 256 * fract );
    return;
  } else {
    //leds[i] = c1.lerp8(c2, uint8_t(255 * fract));
    leds[i] = HSVinterp(c1, c2, fract);
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
