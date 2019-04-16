#include <Noise.h>
#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
//
//  Turtle! - 29 Lights in a hexagonal grid
//
//  8/2/18
//
//  FastLED
//  2D Noise Library
//  Led Library
//  Shows from Library

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 30 // in milliseconds

#define DATA_PIN 7  // Changed from 9 !
#define CLOCK_PIN 8

#define NUM_LEDS 29
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library (will need to be reinitialized)

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 11
uint8_t current_show = 0;
uint8_t current_pattern = 0;

// Palettes
#define CAN_CHANGE_PALETTES true
#define PALETTE_DURATION 300  // seconds between palettes

// wait times
#define SHOW_DURATION 30  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 2   // seconds to fade out
#define FADE_IN_TIME 1    // seconds to fade in

// noise
#define HAVE_NOISE false   // set to false to suppress noise
Noise noise = Noise(NUM_LEDS);

// Lookup tables

int8_t neighbors[] PROGMEM = {
  -1,1,8,9,-1,-1,
  -1,2,7,8,0,-1,
  -1,-1,3,7,1,-1,
  -1,-1,4,6,7,2,
  -1,-1,-1,5,6,3, // 4
  4,-1,-1,14,13,6,
  3,4,5,13,12,7,
  2,3,6,12,8,1,
  1,7,12,11,9,0,
  0,8,11,10,-1,-1,
  9,11,18,19,-1,-1,
  8,12,17,18,10,9,
  7,6,13,17,11,8, // 12
  6,5,14,16,17,12,
  5,-1,-1,15,16,13,
  14,-1,-1,24,23,16,
  13,14,15,23,22,17, // 16
  12,13,16,22,18,11,
  11,17,22,21,19,10,
  10,18,21,20,-1,-1,
  19,21,27,-1,-1,-1,
  18,22,26,27,20,19,
  17,16,23,26,21,18, // 22
  16,15,24,25,26,22,
  15,-1,-1,-1,25,23,
  23,24,-1,-1,28,26,
  22,23,25,28,27,21,
  21,26,28,-1,-1,20,
  26,25,-1,-1,-1,27 // 28
};

int8_t rings[][12] = {
  { 17, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },
  { 12, 13, 16, 22, 18, 11, -1,-1,-1,-1,-1,-1 },
  { 7, 6, 5, 14, 15, 23, 26, 21, 19, 10, 9, 8 },
  { 0, 1, 2, 3, 4, 24, 25, 28, 27, 20, -1, -1 } 
};

#define NUM_PATTERNS 9   // Total number of patterns

const uint8_t PatternMatrix[NUM_PATTERNS][29] = {
    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1 },
    { 1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,1 },
    { 1,1,1,1,1,1,2,2,2,1,1,2,1,2,1,1,2,2,2,1,1,2,1,2,1,1,2,1,1 },
    { 1,1,1,1,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,1,1 },
    { 1,1,1,1,1,2,2,2,2,2,2,1,1,1,2,2,1,1,1,2,1,2,1,2,1,1,2,1,1 },
    { 2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,2 },
    { 1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,1,1,1,2,1,2,1,2,1,1,2,1,1 },
    { 1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,2,1,2,1 } 
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
  
  if (HAVE_NOISE) {
    noise.turnNoiseOn();
  } else {
    noise.turnNoiseOff();
  }

  if (CAN_CHANGE_PALETTES) {
    led.setPalette();
  }

  // Set up the various mappings (1D lists in PROGMEM)
  led.setNeighborMap(neighbors);  // 6 neighbors for every pixel
  shows = Shows(&led);  // Show library - reinitialized for led mappings
  
  led.fillBlack();
  led.push_frame();
}

//
// loop
//
void loop() { 
                   
  switch (current_show) {
  
    case 0:
      patterns();
      break;
    case 1:
      warp1();
      break;
    case 2:
      warp2();
      break;
    case 3:
      rainbow_show();
      break;
    case 4:
      shows.morphChain();
      break;
    case 5:
      shows.bounce();
      break;
    case 6:
      shows.bounceGlowing();
      break;
    case 7:
      shows.plinko(2);
      break;
    case 8:
      shows.packets();
      break;
    case 9:
      shows.packets_two();
      break;
    default:
      shows.bands();
      break;
  }

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay

  noise.fillNoise();
  
  shows.advanceClock();
  
  if (shows.getSmallCycle() >= MAX_SMALL_CYCLE) { 
    next_show(); 
  }
  
  EVERY_N_SECONDS(PALETTE_DURATION) { 
    led.randomizePalette();
  }
}

//
// next_show
//
void next_show() {
  // Switch between a patterns show and all the other shows
  
//  current_show = (current_show + 1) % NUM_SHOWS;  // For debugging
  
  if (current_show == 0) {
    current_show = random8(1, NUM_SHOWS);
  } else {
    current_show = 0;
    current_pattern = random8(NUM_PATTERNS);
  }

  led.fillBlack();
  led.push_frame();
  shows.resetAllClocks();
  noise.setRandomNoiseParams();
  shows.pickRandomColorSpeeds();
  shows.pickRandomWait();
}

//
// patterns shows
//
void patterns() {
  for (int i=0; i < NUM_LEDS; i++) {
    switch (PatternMatrix[current_pattern][i]) {
      case 0: {        // Off
        led.setPixelBlack(i);
        break;
      }
      case 1: {        // always green
        led.setPixelColor(i, CHSV(96, 255, 255));
        break;
      }
      case 2: {        // the other color
        led.setPixelHue(i, shows.getForeColor());
        break;
      }
    }
  }
}

//
// draw_ring
//
void draw_ring(uint8_t i, CHSV color) {
  int8_t *r = rings[i];
  for (int j = 0; j < 12; j++) {
    led.setPixelColor(r[j], color);
  }
}

//
// tunnel vision
//
// Colored ring animating outwards
// color1 is the primary color, colo22 is a trail color
// background is the background color
//
void tunnelvision(CHSV color1, CHSV color2, CHSV background) {  
  int i = shows.getCycle() % 5;
  if (i < 4) { draw_ring(i, color1); }      
  if (i != 0) { draw_ring(i-1, color2); }
}

//
// warp1 - colors on a black field
// 
void warp1() {
  switch ((shows.getCycle() / 5) % 6) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0, 255, 0)), rgb_to_hsv(CRGB(0,40,0)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,0,255)), rgb_to_hsv(CRGB(0,0,40)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,255,255)), rgb_to_hsv(CRGB(0,40,40)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(40,40,0)), rgb_to_hsv(CRGB(0,0,0)));  
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,0,0)), rgb_to_hsv(CRGB(40,0,0)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(255,0,255)), rgb_to_hsv(CRGB(40,0,40)), rgb_to_hsv(CRGB(0,0,0)));  
      break;
  }
}

//
// warp2 - colors on a green field
//
void warp2() {
  switch ((shows.getCycle() / 5) % 8) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0,255,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,200,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,150,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(0,100,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 5:
      tunnelvision(rgb_to_hsv(CRGB(200,200,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 6:
      tunnelvision(rgb_to_hsv(CRGB(150,150,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(100,100,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)));  
      break;
  }
}

//
// rainbow show - distribute a rainbow wheel equally distributed along the chain
//
void rainbow_show() {
  for (uint8_t i = 0; i < HALF_LEDS; i++) {
    uint8_t hue = ((shows.getForeColorSpeed() * i) + shows.getBackColorSpeed() + shows.getCycle()) % MAX_COLOR;
    led.setPixelHue(i, hue);
    led.setPixelHue(NUM_LEDS - i, hue);
  }  
}

//
// noisyshow
//
void noisyshow() {
  if (shows.isShowStart()) {
    noise.makeVeryNoisy();
  }
  shows.allOn();
}

//
// morph_frame
//
void morph_frame() {
  shows.morphFrame();  // 1. calculate interp_frame 2. adjust palette
  add_noise();   // Use the noise library
  update_leds();  // push the interp_frame on to the leds
  check_fades();  // Fade start and end of shows
  FastLED.show();  // Update the display 
}

//
// add_noise - from library - uses led.library getters and setters
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
  uint32_t small_cycle = shows.getSmallCycle();
  
  if (small_cycle <= (FADE_IN_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map(small_cycle, 0, (FADE_IN_TIME * 1000 / DELAY_TIME), 255, 0);
  } else if ((MAX_SMALL_CYCLE - small_cycle) <= (FADE_OUT_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map((MAX_SMALL_CYCLE - small_cycle), 0, (FADE_OUT_TIME * 1000 / DELAY_TIME), 255, 0);
  }
    
  if (fade_amount > 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(fade_amount);
    }
  }
}

//
// RGB to HSV - save having to translate RGB colors by hand
//
CHSV rgb_to_hsv( CRGB color) {
  return led.rgb_to_hsv(color);
}
  
