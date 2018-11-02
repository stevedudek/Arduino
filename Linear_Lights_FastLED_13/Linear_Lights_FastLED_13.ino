#include <Noise.h>
#include <FastLED.h>
#include <Led.h>

//
//  Linear Lights with FastLED
//
//  Leds from Library
//  Noise from Library
//
//  BLINKS AT SHOW END
//
//  6/19/2018
//
#define NUM_LEDS 9

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

uint8_t shuffle[NUM_LEDS];  // For random-fill show (nasty global)

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
uint8_t foreColor =  0;    // Starting foreground color
uint8_t backColor = MAX_COLOR / 2;   // Starting background color
#define BLACK  CHSV( 0, 0, 0)
#define MIN_COLOR_SPEED 2   // Higher = slower
#define MAX_COLOR_SPEED 10   // Higher = slower
uint8_t foreColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster
uint8_t backColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library

// Palettes
#define CAN_CHANGE_PALETTES false
uint8_t palette_start = 0; // 0-255 (hue)
uint8_t palette_width = 255;  // 255 = full RGB
#define PALETTE_DURATION 300  // seconds between palettes

// Shows - now enumerated
#define NUM_SHOWS 9
uint8_t current_show = 0;
#define SHOW_DURATION 20  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 2   // seconds to fade out
#define FADE_IN_TIME 2    // seconds to fade out

// Clocks and time
uint8_t morph = 0;
uint32_t small_cycle = 0;
uint16_t cycle = 0;
uint8_t bands_bpm_1, bands_bpm_2;
uint8_t band_min_1, band_min_2;
#define MIN_BPM 5
#define MAX_BPM 60

// wait times
#define WAIT_DURATION 20 // second between increasing wait time
#define MIN_WAIT   2  // Minimum number of morph steps in a cycle
#define MAX_WAIT  50  // Maximum number of morph steps in a cycle
#define NUM_WAIT_VALUES 10 // Number of stored delay times
uint8_t wait = NUM_WAIT_VALUES / 2;
uint8_t total_frames;

// noise
#define HAVE_NOISE false    // set to false to suppress noise
#define MAX_HUE_NOISE 64   // 255 max 
#define MAX_SAT_NOISE 64   // 255 max
Noise noise = Noise(NUM_LEDS);

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
  
  if (HAVE_NOISE) {
    noise.setMaxNoise(MAX_HUE_NOISE, MAX_SAT_NOISE);
    noise.turnNoiseOn();
  } else {
    noise.turnNoiseOff();
  }

  if (CAN_CHANGE_PALETTES) {
    led.setPalette(palette_start, palette_width);
  }
  
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
    case 7:
      noisyshow();
      break;
    default:
      bands();
      break;
  }

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay
  
  morph++;
  small_cycle++;
  noise.fillNoise();

  if (small_cycle % foreColorSpeed == 0) { foreColor = IncColor(foreColor, 1); }
  if (small_cycle % backColorSpeed == 0) { backColor = IncColor(backColor, 1); }
  
  if (morph >= total_frames) {  // Finished morphing
    
    morph = 0;
    
    if (cycle++ > 10000) { 
      cycle = 0;  // Advance the cycle clock
    }
    led.push_frame();

    change_it_up();
  }

  if (small_cycle >= MAX_SMALL_CYCLE) { 
    next_show(); 
  }
}

//
// change_it_up
//
void change_it_up() {
  EVERY_N_SECONDS(PALETTE_DURATION) { 
    led.randomizePalette(); 
  }
  EVERY_N_SECONDS(WAIT_DURATION) { 
    wait = up_or_down(wait, 0, MAX_WAIT);
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
  noise.setRandomNoiseParams();
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
// set_all_black - turn all to black and update leds
//
void set_all_black() {
  led.fillBlack();
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = BLACK;
  }
  FastLED.show();
}

//
// All On - turns all the pixels on the foreColor
// 
void allOn() {
   led.fillHue(foreColor);
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
      led.setPixelHue(shuffle[i], foreColor);  // Turning on lights one at a time
    } else { 
      led.setPixelBlack(shuffle[NUM_LEDS-(i % NUM_LEDS)-1]);  // Turning off lights one at a time
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
    for (int i=0; i < NUM_LEDS; i++) {
      shuffle[i] = random8(MAX_COLOR);
    }
  }
  
  // Otherwise, fill lights with their color
  for (int i=0; i < NUM_LEDS; i++) {
    led.setPixelHue(i, shuffle[i]);
  }
}

//
// two color - alternates the color of pixels between two colors
//
void twocolor() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % 2) {
      led.setPixelHue(i, foreColor);
    } else {
      led.setPixelHue(i, backColor);
    }
  }
}

//
// Morph Chain - morphs color 1 from position x to color 2 at position x+n
//
void morphChain() {
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t fract = map((i+(cycle % NUM_LEDS)) % NUM_LEDS, 0, NUM_LEDS, 0, 255);
    led.setPixelHue(NUM_LEDS-i-1, led.interpolate_wrap(foreColor, backColor, fract));
  }
}

//
// Saw tooth - Fills in pixels with a sawtooth of intensity
//
void sawtooth() {
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint16_t fract = map((i+(cycle % NUM_LEDS)) % NUM_LEDS, 0, NUM_LEDS, 0, 512);
    if (fract >= 256) {
      fract = 512 - fract;  // the subtraction creates the sawtooth
    }
    // "i" will have pattern move up; "NUM_LEDS-i-1'' will have pattern move down
    led.setPixelColor(NUM_LEDS-i-1, led.gradient_wheel(foreColor, fract));
  }
}

// 
// bands
//
void bands() {
  if (is_show_start()) {
    bands_bpm_1 = random8(MIN_BPM, MAX_BPM);
    bands_bpm_2 = random8(MIN_BPM, MAX_BPM);
    band_min_1 = random8(64, 192);
    band_min_2 = random8(64, 192);
  }

  uint8_t wave;
  
  for (int i=0; i < NUM_LEDS; i++) {
    wave = beatsin8(bands_bpm_1, 0, 255, 0, map(i, 0, NUM_LEDS, 0, 255) );
    wave = (wave > band_min_1) ? map(wave, band_min_1, 255, 0, 255) : 0;
    if (wave > 0) {
      led.setPixelColor(i, CHSV(foreColor, 255, wave) );
    } else {
      led.setPixelBlack(i);
    }

    wave = beatsin8(bands_bpm_2, 0, 255, 0, map(i, 0, NUM_LEDS, 0, 255) );
    wave = (wave > band_min_2) ? map(wave, band_min_2, 255, 0, 255) : 0;
    if (wave > 0) {
      led.addPixelColor(NUM_LEDS - i - 1, CHSV(backColor, 255, wave) );
    }
  }
}

//
// noisyshow
//
void noisyshow() {
  if (is_show_start()) {
    noise.makeVeryNoisy();
  }
  allOn();
}

//
// lightwave - just one pixel traveling along the chain
//
void lightwave() {
  for (int i=0; i < NUM_LEDS; i++) {
     if (i == cycle % NUM_LEDS) {
       led.setPixelHue(i, foreColor);
     } else {
       led.setPixelBlack(i);
     }
  }
}

//
// lightrunup -lights fill up one at time
// 
void lightrunup() {
  int pos = cycle % (NUM_LEDS*2);  // Where we are in the show
  if (pos >= NUM_LEDS) {
    pos = (NUM_LEDS*2) - pos;
  }
  
  for (int i=0; i < NUM_LEDS; i++) {
    if (i < pos) {
      led.setPixelHue(i, foreColor);  // Turning on lights one at a time
    } else {
      led.setPixelBlack(i);
    }
  }
}
    
//
// morph_frame
//
void morph_frame() {
  led.morph_frame(morph, total_frames);  // 1. calculate interp_frame 2. adjust palette
  add_noise();   // Use the noise library
  update_leds();  // push the interp_frame on to the leds
  check_fades();  // Fade start and end of shows
  FastLED.show();  // Update the display 
}

//
// add_noise - from library - uses led. getters and setters
//
void add_noise() {
  if (HAVE_NOISE) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      led.setInterpFrameHue(i, noise.addNoiseAtoValue(i, led.getInterpFrameHue(i))); 
      led.setInterpFrameSat(i, noise.addNoiseBtoValueNoWrap(i, led.getInterpFrameSat(i)));
    }
  }
}

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = led.getInterpFrameColor(i);
  }
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

