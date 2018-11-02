#include <Noise.h>
#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Triceratops - 45 cones on a coat
//
//  6/20/18
//
//  FastLED
//  1D Noise Library
//  Led Library
//  Shows from Library
//    pushed variables into Shows.h
//
#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define BODY_LEDS 42
#define NUM_LEDS 45
#define NUM_ROWS 17

#define MAX_COLOR 256   // Colors are 0-255

// Palettes
#define CAN_CHANGE_PALETTES true
#define PALETTE_DURATION 300  // seconds between palettes

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library

// Shows
#define NUM_SHOWS 21
uint8_t current_show = 0;

// wait times
#define SHOW_DURATION 30  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 2   // seconds to fade out
#define FADE_IN_TIME 2    // seconds to fade out
#define WAIT_DURATION 20 // second between increasing wait time

// noise
#define HAVE_NOISE false   // set to false to suppress noise
Noise noise = Noise(NUM_LEDS);

// Lookup tables

const uint8_t ConeLookUp[] PROGMEM = {
      44,
    42, 43,
  41, 40, 32,
    39, 33,
  38, 34, 31,
    35, 30,
  36, 29, 20,
37, 28, 21, 19,
  27, 22, 18,
26, 23, 17,  9,
  24, 16,  8,
25, 15,  7, 10,
  14,  6, 11,
     5, 12,
   4, 13,  0,
     3,  1,
       2
};

#define ROW_WIDTH  4
const int8_t ConeGrid[] PROGMEM = {
 -1,    44,   -1,-1,
 -1,  42, 43,    -1,
    41, 40, 32,  -1,
 -1,  39, 33,    -1,
    38, 34, 31,  -1,
 -1,  35, 30,    -1,
    36, 29, 20,  -1,
  37, 28, 21, 19,  
    27, 22, 18,  -1,
  26, 23, 17,  9,  
    24, 16,  8,  -1,
  25, 15,  7, 10,  
    14,  6, 11,  -1,
 -1,   5, 12,    -1,
     4, 13,  0,  -1,
 -1,   3,  1,    -1,
 -1,     2,   -1,-1,
};

const int8_t neighbors[] PROGMEM = {
  -1,-1,-1,1,13,12, // 0
  0,-1,-1,2,3,13,
  1,-1,-1,-1,-1,3,
  13,1,2,-1,-1,4,
  5,13,3,-1,-1,-1, // 4
  6,12,13,4,-1,14,
  7,11,12,5,14,15,
  8,10,11,6,15,16,
  9,-1,10,7,16,17, // 8
  -1,-1,-1,8,17,18,
  -1,-1,-1,11,7,8,
  10,-1,-1,12,6,7,
  11,-1,0,13,5,6, // 12
  12,0,1,3,4,5,
  15,6,5,-1,-1,25,
  16,7,6,14,25,24,
  17,8,7,15,24,23, // 16
  18,9,8,16,23,22,
  19,-1,9,17,22,21,
  -1,-1,-1,18,21,20,
  -1,-1,19,21,29,30, // 20
  20,19,18,22,28,29,
  21,18,17,23,27,28,
  22,17,16,24,26,27,
  23,16,15,25,-1,26, // 24
  24,15,14,-1,-1,-1,
  27,23,24,-1,-1,-1,
  28,22,23,26,-1,37,
  29,21,22,27,37,36, // 28
  30,20,21,28,36,35,
  31,-1,20,29,35,34,
  -1,-1,-1,30,34,33,
  -1,-1,-1,33,40,43, // 32
  32,-1,31,34,39,40,
  33,31,30,35,38,39,
  34,30,29,36,-1,38,
  35,29,28,37,-1,-1, // 36
  36,28,27,-1,-1,-1,
  39,34,35,-1,-1,-1,
  40,33,34,38,-1,41,
  43,32,33,39,41,42, // 40
  42,40,39,-1,-1,-1,
  44,43,40,41,-1,-1,
  -1,-1,32,40,42,44,
  -1,-1,43,42,-1,-1, // 44
};

const uint8_t PatternLookUp[45] = { 41,43,44,42,39,37,35,32,29,26,
                           33,36,38,40,34,31,28,25,22,19,
                           15,18,21,24,27,30,23,20,17,14,
                           12,10,5,7,9,11,13,16,8,6,
                           4,3,2,1,0 };

const uint8_t Stripe_Pattern[45] PROGMEM = {
       0,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
       0
};

const uint8_t Section_Pattern[45] PROGMEM = {
       2,
     2,  2,
   0,  0,  0,
     0,  0,
   0,  0,  0,
     0,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
     0,  0,
   2,  0,  2,
     2,  2,
       2
};

const uint8_t Explode_Pattern[45] PROGMEM = {
       5,
     5,  5,
   5,  4,  5,
     4,  4,
   4,  3,  4,
     3,  3,
   3,  2,  3,
 3,  1,  1,  3,
   2,  0,  2,
 3,  1,  1,  3,
   3,  2,  3,
 4,  3,  3,  4,
   4,  3,  4,
     4,  4,
   5,  4,  5,
     5,  5,
       5
};

const uint8_t Alternate_Pattern[45] PROGMEM = {
       0,
     1,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
 0,  0,  1,  0,
   1,  0,  0,
 1,  0,  1,  0,
   1,  0,  0,
 0,  0,  0,  1,
   0,  1,  0,
     0,  0,
   1,  0,  1,
     0,  0,
       1
};

const uint8_t SideSide_Pattern[45] PROGMEM = {
       3,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
       3
};

const uint8_t Diag_Pattern[45] PROGMEM = {
       0,
     1,  0,
   2,  1,  0,
     2,  1,
   3,  2,  1,
     3,  2,
   4,  3,  2,
 5,  4,  3,  2,
   5,  4,  3,
 6,  5,  4,  3,
   6,  5,  4,
 7,  6,  5,  4,
   7,  6,  5,
     7,  6,
   8,  7,  6,
     8,  7,
       8
};

const uint8_t ConeSize[45]  = { 5,5,5,5,5,3,5,1,3,5,1,1,5,4,3,4,5,5,6,3,6,2,1,6,6,4,2,6,2,1,3,4,2,4,4,5,3,2,2,3,1,6,3,3,1 };

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
  led.setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  led.setCoordMap(ROW_WIDTH, ConeGrid);  // x,y grid of cones
  led.setNeighborMap(neighbors);  // 6 neighbors for every pixel
  
  set_all_black();
}

void loop() { 
 
  switch (current_show) {
  
    case 0:
      shows.allOn();
      break;
    case 1:
      shows.randomFill();
      break;
    case 2:
      shows.randomColors();
      break;
    case 3:
      shows.twoColor();
      break;
    case 4:
      shows.lightRunUp();
      break;
    case 5:
      colorsize();
      break;
    case 6:
      brightsize();
      break;
    case 7:
      stripe();
      break;
    case 8:
      alternate();
      break;
    case 9:
      diagcolor();
      break;
    case 10:
      sidesidecolor();
      break;
    case 11:
      explodecolor();
      break;
    case 12:
      diagbright();
      break;
    case 13:
      sidesidebright();
      break;
    case 14:
      explodebright();
      break;
    case 15:
      shows.plinko(40);  // 40 = starting plinko pixel
      break;
    case 16:
      shows.bounce();
      break;
    case 17:
      shows.bounceGlowing();
      break;
    case 18:
      sectioncolor();
      break;
    case 19:
      shows.bands();
      break;
    default:
      noisyshow();
      break;
  }
  
  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay

  noise.fillNoise();
  
  shows.advanceClock();
  
  if (shows.getSmallCycle() >= MAX_SMALL_CYCLE) { 
    next_show(); 
  }
  
  change_it_up();  // change palettes and wait times
  
}

//
// change_it_up
//
void change_it_up() {
  EVERY_N_SECONDS(PALETTE_DURATION) { 
    led.randomizePalette();
  }
  EVERY_N_SECONDS(WAIT_DURATION) { 
    shows.tweakWait();
  }
}

//
// next_show
//
void next_show() {
  current_show = random8(NUM_SHOWS);
  shows.resetAllClocks();
  noise.setRandomNoiseParams();
//  set_all_black();
  shows.tweakColorSpeeds();
}

//
// set_all_black - turn all to black and update leds
//
void set_all_black() {
  led.fillBlack();
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV( 0, 0, 0);
  }
  FastLED.show();
  led.push_frame();
}

//// Start specialized shows

//
// colorsize - light each cone according to its cone size
//
void colorsize() {
  for (int i=0; i < NUM_LEDS; i++) {
    led.setPixelHue(i, IncColor(shows.getForeColor(), ((ConeSize[i]-1) * sin8(shows.getBackColor())) % MAX_COLOR));
  }
  shows.IncForeColor(1);
}

//
// calcIntensity - use sine wave + cycle + variable to calculate intensity
//
uint8_t calcIntensity(uint8_t x, uint8_t max_x) {
  return sin8_C( 255 * ((shows.getCycle() + x) % max_x) / (max_x - 1));
}

//
// brightsize - light just one cone size
//
void brightsize() {
  for (int i=0; i < BODY_LEDS; i++) {
    led.setPixelColor(i, led.gradient_wheel(shows.getBackColor(), calcIntensity(ConeSize[i]-1, 5)));
  }
  setHead(shows.getForeColor());
}

//
// stripe
//
void stripe() {
  for (int i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Stripe_Pattern + PatternLookUp[i]) == 0) {
      shows.setPixeltoForeColor(i);
    } else {
      shows.setPixeltoBackColor(i);
    }
  }
  shows.IncBackColor(1);
}

//
// alternate
//
void alternate() {
  for (int i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Alternate_Pattern + PatternLookUp[i])) {
      shows.setPixeltoForeColor(i);
    } else {
      shows.setPixeltoBackColor(i);
    }
  }
  shows.IncForeColor(1);
}

void diagcolor() {
  for (int i=0; i < NUM_LEDS; i++) {
    led.setPixelHue(i, IncColor(shows.getForeColor(), sin8(shows.getBackColor() * pgm_read_byte_near(Diag_Pattern + PatternLookUp[ConeLookUp[i]])) / 5));
  }
}

void sectioncolor() {
  for (int i=0; i < NUM_LEDS; i++) {
    led.setPixelHue(i, IncColor(shows.getForeColor(), sin8(shows.getBackColor() * pgm_read_byte_near(Section_Pattern + PatternLookUp[ConeLookUp[i]])) / 10));
  }
}

void sidesidecolor() {
  for (int i=0; i < NUM_LEDS; i++) {
    led.setPixelHue(i, IncColor(shows.getBackColor(), sin8(shows.getForeColor() * pgm_read_byte_near(SideSide_Pattern + PatternLookUp[ConeLookUp[i]]))));
  }
}

void explodecolor() {
  for (int i=0; i < NUM_LEDS; i++) {
    led.setPixelHue(i, IncColor(shows.getBackColor(), sin8(shows.getForeColor() * pgm_read_byte_near(Explode_Pattern + PatternLookUp[ConeLookUp[i]]))));
  }
}

void diagbright() {
  for (int i=0; i < BODY_LEDS; i++) {
    led.setPixelColor(i, led.gradient_wheel(shows.getBackColor(), calcIntensity(pgm_read_byte_near(Diag_Pattern + PatternLookUp[i]), 9)));
  }
  setHead(shows.getForeColor());
}

void sidesidebright() {
  for (int i=0; i < BODY_LEDS; i++) {
    led.setPixelColor(i, led.gradient_wheel(shows.getForeColor(), calcIntensity(pgm_read_byte_near(SideSide_Pattern + PatternLookUp[i]), 7)));
  }
  setHead(shows.getForeColor());
}

void explodebright() {
  for (int i=0; i < BODY_LEDS; i++) {
    led.setPixelColor(i, led.gradient_wheel(shows.getForeColor(), calcIntensity(5 - pgm_read_byte_near(Explode_Pattern + PatternLookUp[i]), 6)));
  }
}

//// End specialized shows

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

void setHead(uint8_t hue) {
  led.setPixelHue(42, hue);
  led.setPixelHue(43, hue);
  led.setPixelHue(44, hue);
}
 
//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}
