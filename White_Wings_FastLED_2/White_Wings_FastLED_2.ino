#include <FastLED.h>
//
//  Paired Linear Lights with FastLED
//
//  For the white wings
//
//  HSV - got rid of palettes
//  Enumerated Shows
//
//  4/15/2018
//

#define NUM_LEDS 84
#define SYM_LEDS (NUM_LEDS / 2)

#define BRIGHTNESS  24 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

CRGB leds[NUM_LEDS];
CHSV current_frame[NUM_LEDS]; // framebuffers
CHSV next_frame[NUM_LEDS];  // framebuffers

uint8_t shuffle[NUM_LEDS];  // For random-fill show (nasty global)

// Light colors
#define MAX_COLOR 255   // Colors are 0-255
uint8_t foreColor =  0;    // Starting foreground color
uint8_t backColor = MAX_COLOR / 2;   // Starting background color
CHSV BLACK = CHSV( 0, 0, 0);
#define MIN_COLOR_SPEED 2   // Higher = slower
#define MAX_COLOR_SPEED 10   // Higher = slower
uint8_t foreColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster
uint8_t backColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster

// Palettes
#define CAN_CHANGE_PALETTES false
uint8_t palette_start = 0; // 0-255 (hue)
uint8_t palette_width = 255;  // 255 = full RGB
#define PALETTE_DURATION 300  // seconds between palettes

// Shows
#define NUM_SHOWS 8
uint8_t current_show = 0;
#define SHOW_DURATION 60  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 3   // seconds to fade out
#define FADE_IN_TIME 3    // seconds to fade out

// Clocks and time
uint8_t morph = 0;
uint32_t small_cycle = 0;
uint16_t cycle = 0;
uint8_t bands_bpm_1, bands_bpm_2;
uint8_t band_min_1, band_min_2;
#define MIN_BPM 5
#define MAX_BPM 60

// wait times
#define WAIT_DURATION 4 // second between increasing wait time
#define MIN_WAIT   4  // Minimum number of morph steps in a cycle
#define MAX_WAIT  30  // Maximum number of morph steps in a cycle
#define NUM_WAIT_VALUES 50 // Number of stored delay times
uint8_t wait = NUM_WAIT_VALUES / 2;
uint8_t total_frames;


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  set_all_black();

  total_frames = getNumFrames(wait);
}

void loop() { 
  
  switch (current_show) {

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
      sawtooth();
      break;
    default:
      bands();
      break;
  }

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay
  
  morph++;
  small_cycle++;

  if (small_cycle % foreColorSpeed == 0) { foreColor = IncColor(foreColor, 1); }
  if (small_cycle % backColorSpeed == 0) { backColor = IncColor(backColor, 1); }
  
  if (morph >= total_frames) {  // Finished morphing
    
    morph = 0;
    
    if (cycle++ > 10000) { cycle = 0; }  // Advance the cycle clock
    
    push_frame();

    change_it_up();
  }

  if (small_cycle >= MAX_SMALL_CYCLE) { next_show(); }
  
}

//
// change_it_up
//
void change_it_up() {
  EVERY_N_SECONDS(PALETTE_DURATION) { change_palette(); }
  EVERY_N_SECONDS(WAIT_DURATION) { 
    wait = up_or_down(wait, 0, NUM_WAIT_VALUES);
    total_frames = getNumFrames(wait);
  }
}

//
// next_show
//
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void next_show() {
  current_show = random8(NUM_SHOWS);
  morph = 0;
  small_cycle = 0;
  cycle = 0;
  set_all_black();
  foreColorSpeed = up_or_down(foreColorSpeed, MIN_COLOR_SPEED, MAX_COLOR_SPEED);
  backColorSpeed = up_or_down(backColorSpeed, MIN_COLOR_SPEED, MAX_COLOR_SPEED);
}

//
// is_show_start
//
boolean is_show_start() {
  return (cycle == 0 && morph == 0);
}

//
// change_palette
//
void change_palette() {
  if (CAN_CHANGE_PALETTES) {
    palette_start = random8(255);
    palette_width = random8(20, 255);
  }
}

void set_all_black() {
  for (int i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = BLACK;
    next_frame[i] = BLACK;
    leds[i] = BLACK;
  }
  FastLED.show();
}

void fill(CHSV color) {
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
  
  for (i = 0; i < NUM_LEDS; i++) {
    if (i < pos) {  
      setPixelColor(shuffle[i], Wheel(foreColor));  // Turning on lights one at a time
    } else { 
      setPixelColor(shuffle[NUM_LEDS-(i % NUM_LEDS)-1], BLACK);  // Turning off lights one at a time
    }
  }
}

//
// Shuffle LEDS
//
void shuffle_leds() {
  int i, j, save;
  
  for (i = 0; i < NUM_LEDS; i++) {
    shuffle[i] = i; // before shuffle
  }
  for (i = 0; i < NUM_LEDS; i++) {  // here's position
    j = random8(NUM_LEDS);         // there's position
    save = shuffle[i];
    shuffle[i] = shuffle[j];       // first swap
    shuffle[j] = save;             // second swap
  }
}

//    
// random colors - turns each pixel on to a random color
//
void randomcolors() {
  if (is_show_start()) {  // Start of show: assign lights to random colors
    for (int i = 0; i < NUM_LEDS; i++) {
      shuffle[i] = random8(MAX_COLOR);
    }
  }
  
  // Otherwise, fill lights with their color
  for (int i = 0; i < NUM_LEDS; i++) {
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
    } else {
      setSymPixelColor(i, Wheel(backColor));
    }
  }
}

//
// Morph Chain - morphs color 1 from position x to color 2 at position x+n
//
void morphChain() {
  for (int i = 0; i < SYM_LEDS; i++) {
    uint8_t fract = map((i+(cycle % SYM_LEDS)) % SYM_LEDS, 0, SYM_LEDS-1, 0, 255);
    setSymPixelColor(i, Wheel(interpolate_wrap(foreColor, backColor, fract)));
  }
}

//
// Saw tooth - Fills in pixels with a sawtooth of intensity
//
void sawtooth() {
  
  for (int i = 0; i < SYM_LEDS; i++) {
    uint16_t fract = map((i+(cycle % SYM_LEDS)) % SYM_LEDS, 0, SYM_LEDS-1, 0, 512);
    if (fract >= 256) {
      fract = 512 - fract;  // the subtraction creates the sawtooth
    }
    // "i" will have pattern move up; "SYM_LEDS-i-1'' will have pattern move down
    setSymPixelColor(SYM_LEDS-i-1, Gradient_Wheel(foreColor, fract));
  }
}

// 
// bands
//
void bands() {
  if (is_show_start()) {
    noise_hue_intense = 0;  // Turn off regular noise
    noise_sat_intense = 0;
    bands_bpm_1 = random8(MIN_BPM, MAX_BPM);
    bands_bpm_2 = random8(MIN_BPM, MAX_BPM);
    band_min_1 = random8(64, 192);
    band_min_2 = random8(64, 192);
  }

  uint8_t wave;
  
  for (int i=0; i < NUM_LEDS; i++) {
    wave = beatsin8(bands_bpm_1, 0, 255, 0, map(i, 0, NUM_LEDS-1, 0, 255) );
    wave = (wave > band_min_1) ? map(wave, band_min_1, 255, 0, 255) : 0;
    if (wave > 0) {
      setPixelColor(i, CHSV(foreColor, 255, wave) );
    } else {
      setPixelColor(i, BLACK);
    }

    wave = beatsin8(bands_bpm_2, 0, 255, 0, map(i, 0, NUM_LEDS-1, 0, 255) );
    wave = (wave > band_min_2) ? map(wave, band_min_2, 255, 0, 255) : 0;
    if (wave > 0) {
      addPixelColor(NUM_LEDS - i - 1, CHSV(backColor, 255, wave) );
    }
  }
}

//
// lightwave - just one pixel traveling along the chain
//
void lightwave() {
  for (int i=0; i < SYM_LEDS; i++) {
     if (i == cycle % SYM_LEDS) {
       setSymPixelColor(i, Wheel(foreColor));
     } else {
       setSymPixelColor(i, BLACK);
     }
  }
}

//
// lightrunup -lights fill up one at time
// 
void lightrunup() {
  int pos = cycle % (SYM_LEDS*2);  // Where we are in the show
  if (pos >= SYM_LEDS) {
    pos = (SYM_LEDS*2) - pos;
  }
  
  for (int i=0; i < SYM_LEDS; i++) {
    if (i < pos) {
      setSymPixelColor(i, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setSymPixelColor(i, BLACK);   // black
    }
  }
}
    
//
// morph_frame
//
void morph_frame() {
   uint8_t fract = map(morph, 0, total_frames-1, 0, 255);  // 0 - 255
   
   for (int i = 0; i < NUM_LEDS; i++) {
     setLEDinterpHSV(i, current_frame[i], next_frame[i], fract);
   }
   check_fades();
   FastLED.show();  // Update the display 
}

//
// push_frame
//
void push_frame() {
  for (int i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = next_frame[i];
  }
}

//
// constrain_palette
//
CHSV constrain_palette(uint8_t i, CHSV color) {
  color.h = map8(sin8(color.h), palette_start, (palette_start + palette_width) % 256);
  return color;
}

//
// check_fades - check the fade-to-blacks at beginning and end of show
//
void check_fades() {
  uint8_t fade_amount = 0;
  
  if (small_cycle <= (FADE_IN_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map(small_cycle, 0, (FADE_IN_TIME * 1000 / DELAY_TIME), 256, 0);
  } else if ((MAX_SMALL_CYCLE - small_cycle) <= (FADE_OUT_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map((MAX_SMALL_CYCLE - small_cycle), 0, (FADE_OUT_TIME * 1000 / DELAY_TIME), 256, 0);
  }
    
  if (fade_amount > 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(fade_amount);
    }
  }
}

void setPixelColor(int pos, CHSV color) {
  next_frame[pos] = color;
}

void setSymPixelColor(int pos, CHSV color) {
  pos = pos % SYM_LEDS;
  
  next_frame[pos] = color;
  next_frame[NUM_LEDS - pos - 1] = color; 
}

//
// addPixelColor - add color to the existing color in next_frame
//
void addPixelColor(int pos, CHSV c2) {
  CHSV c1 = next_frame[pos];
  
  if (c1.v > c2.v) {
      next_frame[pos] = c1;
    } else {
      next_frame[pos] = c2;
    }
}


//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint8_t amount) {
  return (c + amount ) % MAX_COLOR;
}

//
// up_or_down - increase on decrease a counter randomly within bounds
//
uint8_t up_or_down(uint8_t counter, uint8_t min_value, uint8_t max_value) {
  counter += (random8(2) * 2 - 1);  // 0-1 -> 0,2 -> -1 or +1
  if (counter < min_value) {
    return min_value + 1;
  } else if (counter > max_value) {
    return max_value - 1;
  } else {
    return counter;
  }
}

//
// getNumFrames - convert a wait value into a number of morph frames
//
uint8_t getNumFrames(uint8_t wait_value) {
  return map(wait_value, 0, NUM_WAIT_VALUES-1, MIN_WAIT, MAX_WAIT);
}

//
//  Wheel - Input a hue (0-255) to get a color
//
CHSV Wheel(uint8_t hue)
{
  return Gradient_Wheel(hue, 255);  // 255 = full brightness
}

//
//  Gradient_Wheel - Input a hue and intensity (0-255) to get a CHSV from the palette
//
CHSV Gradient_Wheel(uint8_t hue, uint8_t intensity)
{
  return CHSV(hue, 255, intensity);
}

//
//  setLEDinterpHSV - Set LED i to the interpolate of two HSV colors 
//
void setLEDinterpHSV(int i, CHSV c1, CHSV c2, uint8_t fract)
{ 
  if (c1 == c2) {
    leds[i] = constrain_palette(i, c1);
  } else if (fract == 0) {
    leds[i] = constrain_palette(i, c1);
    return;
  } else if (fract == 255) { 
    leds[i] = constrain_palette(i, c2);
    return;
  } else if (is_black(c1)) {
    leds[i] = constrain_palette(i, c2);
    leds[i].fadeToBlackBy(255 - fract);
    return;
  } else if (is_black(c2)) {
    leds[i] = constrain_palette(i, c1);
    leds[i].fadeToBlackBy(fract);
    return;
  } else {
    if (c1.v > c2.v) {
      // Can also try: leds[i] = constrain_palette(i, c1);
      leds[i] = constrain_palette(i, CHSV(c1.h, interpolate(c1.s, c2.s, fract), interpolate(c1.v, c2.v, fract)));
    } else {
      // Can also try: leds[i] = constrain_palette(i, c2);
      leds[i] = constrain_palette(i, CHSV(c2.h, interpolate(c1.s, c2.s, fract), interpolate(c1.v, c2.v, fract)));
    }
    return;
  }
}

// is_black
boolean is_black(CHSV color) {
  return color == BLACK;
}

//
// Interpolate - returns the fractional point from a to b
//
float interpolate(uint8_t a, uint8_t b, uint8_t fract)
{
  return lerp8by8(a, b, fract);
}

//
// Interpolate Wrap - returns the fractional point from a to b, checking both ways around a circle
//
uint8_t interpolate_wrap(uint8_t a, uint8_t b, uint8_t fract)
{
  uint8_t distCCW, distCW;

  if (a >= b) {
    distCW = 256 + b - a;
    distCCW = a - b; 
  } else {
    distCW = b - a;
    distCCW = 256 + a - b;
  }
  if (distCW <= distCCW) {
    return a + lerp8by8 (0, distCW, fract);
  } else {
    return a - lerp8by8 (0, distCCW, fract);
  }
}
