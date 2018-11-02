#include <Noise.h>
#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Cuddlefish - 42 cones on a hat
//
//  10/17/18
//
//  FastLED
//  2D Noise Library
//  Led Library
//  Shows from Library
//
#define BRIGHTNESS  48 // (0-255)

#define DELAY_TIME 20 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 42

#define MAX_XCOORD  7  // 7 tentacles
#define MAX_YCOORD  6  // each 6 cones

#define MAX_COLOR 256   // Colors are 0-255

#define CAN_CHANGE_PALETTES true  // Palettes

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library (will need to be reinitialized)

// Shows
#define NUM_SHOWS 24
uint8_t current_show = 0;

// wait times
#define SHOW_DURATION 30  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 2   // seconds to fade out
#define FADE_IN_TIME 1    // seconds to fade in

// noise
#define HAVE_NOISE false   // set to false to suppress noise
Noise noise = Noise(MAX_XCOORD, MAX_YCOORD);

// Lookup tables

const int8_t ConeLookUp[] PROGMEM = {
 17,11, 5,23,29,35,41,
 16,10, 4,22,28,34,40,
 15, 9, 3,21,27,33,39,
 14, 8, 2,20,26,32,38,
 13, 7, 1,19,25,31,37,
 12, 6, 0,18,24,30,36,
};

const int8_t ConeGrid[] PROGMEM = {
 17,11, 5,23,29,35,41,
 16,10, 4,22,28,34,40,
 15, 9, 3,21,27,33,39,
 14, 8, 2,20,26,32,38,
 13, 7, 1,19,25,31,37,
 12, 6, 0,18,24,30,36,
};

const int8_t neighbors[] PROGMEM = {
  7,1,19,18,6,-1, // 0
  8,2,0,9,0,7,
  9,3,21,20,1,8,
  10,4,22,21,2,9,
  11,5,23,22,3,10, // 4
  -1,-1,-1,23,4,11,
  12,7,0,-1,-1,-1,
  13,8,1,0,6,12,
  14,9,2,1,7,13, // 8
  15,10,3,2,8,14,
  16,11,4,3,9,15,
  17,-1,5,4,10,16,
  -1,13,7,6,-1,-1, // 12
  -1,14,8,7,12,-1,
  -1,15,0,8,13,-1,
  -1,16,10,9,14,-1,
  -1,17,11,10,15,-1, // 16
  -1,-1,-1,11,16,-1,
  0,19,24,-1,-1,-1,
  1,20,25,24,18,0,
  2,21,26,25,19,1, // 20
  3,22,27,26,20,2,
  4,23,28,27,21,3,
  5,-1,29,28,22,4,
  19,25,31,30,-1,18, // 24
  20,26,32,31,24,19,
  21,27,33,32,25,20,
  22,28,34,33,26,21,
  23,29,35,34,27,22, // 28
  -1,-1,-1,35,28,23,
  24,31,36,-1,-1,-1,
  25,32,37,36,30,24,
  26,33,38,37,31,25, // 32
  27,34,39,38,32,26,
  28,35,40,39,33,27,
  34,30,29,36,-1,38,
  31,37,-1,-1,-1,30, // 36
  32,38,-1,-1,36,31,
  33,39,-1,-1,37,32,
  34,40,-1,-1,38,33,
  35,41,-1,-1,39,34, // 40
  -1,-1,-1,-1,40,35,
};

const uint8_t PatternLookUp[45] = { 
  0,   2,   4,  6,
     1,   3,   5,
  7,   9,  11,  13,
     8,  10,  12,
  14,  16,  18,  20,
     15,  17,  19,
  21,  23,  25,  27,
     22,  24,  26,
  28,  30,  32,  34,
     29,  31,  33,
  35,  37,  39,  36,
     36,  38,  40,
};

const uint8_t Stripe_Pattern[45] PROGMEM = {
  0,  0,  1,  1,
    0,  1,  1,
  0,  1,  1,  0,
    1,  1,  0,
  1,  1,  0,  0,
    1,  0,  0,
  1,  0,  0,  1,
    0,  0,  1,
  0,  0,  1,  1,
    0,  1,  1,
  0,  1,  1,  0,
    1,  1,  0,
};

const uint8_t Section_Pattern[45] PROGMEM = {
  0,  0,  0,  0,
    0,  1,  0,
  0,  1,  1,  0,
    1,  2,  1,
  1,  2,  2,  1,
    1,  2,  1,
  1,  2,  2,  1,
    1,  2,  1,
  0,  1,  1,  1,
    0,  1,  0,
  0,  0,  0,  0,
    0,  0,  0,
};

const uint8_t Explode_Pattern[45] PROGMEM = {
  5,  5,  5,  5,
    4,  4,  4,
  4,  3,  3,  4,
    3,  2,  1,
  3,  1,  1,  3,
    2,  0,  2,
  3,  1,  1,  3,
    3,  2,  3,
  4,  3,  3,  4,
    4,  4,  4,
  5,  5,  5,  5,
    5,  5,  5,
};

const uint8_t Alternate_Pattern[45] PROGMEM = {
  0,  0,  0,  0,
    1,  1,  1,
  0,  0,  0,  0,
    1,  1,  1,
  0,  0,  0,  0,
    1,  1,  1,
  0,  0,  0,  0,
    1,  1,  1,
  0,  0,  0,  0,
    1,  1,  1,
  0,  0,  0,  0,
    1,  1,  1,
};

const uint8_t SideSide_Pattern[45] PROGMEM = {
  0,  2,  4,  6,
    1,  3,  5,
  0,  2,  4,  6,
    1,  3,  5,
  0,  2,  4,  6,
    1,  3,  5,
  0,  2,  4,  6,
    1,  3,  5,
  0,  2,  4,  6,
    1,  3,  5,
  0,  2,  4,  6,
    1,  3,  5,
};

const uint8_t Diag_Pattern[45] PROGMEM = {
  0,  1,  2,  3,
    1,  2,  3,
  1,  2,  3,  4,
    2,  3,  4,
  2,  3,  4,  5,
    3,  4,  5,
  3,  4,  5,  6,
    4,  5,  6,
  4,  5,  6,  7,
    5,  6,  7,
  5,  6,  7,  8,
    6,  7,  8,
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
//  led.setOnlyRed();  // Optional for only red

  // Set up the various mappings (1D lists in PROGMEM)
  led.setLedMap(ConeLookUp);  // turned off mapping - it's handled explicitly here in update_leds()
  led.setCoordMap(MAX_YCOORD, ConeGrid);  // x,y grid of cones
  led.setNeighborMap(neighbors);  // 6 neighbors for every pixel
  shows = Shows(&led);  // Show library - reinitialized for led mappings
  
  led.fillBlack();
  led.push_frame();
}

void loop() { 
  
  switch (current_show) {
  
    case 0:
      morphcolor();
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
      stripe();
      break;
    case 6:
      alternate();
      break;
    case 7:
      diagcolor();
      break;
    case 8:
      sidesidecolor();
      break;
    case 9:
      explodecolor();
      break;
    case 10:
      diagbright();
      break;
    case 11:
      sidesidebright();
      break;
    case 12:
      explodebright();
      break;
    case 13:
      shows.plinko(15);
      break;
    case 14:
      shows.bounce();
      break;
    case 15:
      shows.bounceGlowing();
      break;
    case 16:
      sectioncolor();
      break;
    case 17:
      shows.bands();
      break;
    case 18:
      shows.packets();
      break;
    case 19:
      shows.packets_two();
      break;
    case 20:
      shows.sinelon_fastled();
      break;
    case 21:
      shows.bpm_fastled();
      break;
    case 22:
      shows.juggle_fastled();
      break;
    default:
      shows.lightWave();
      break;
  }
  
  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay

  noise.fillNoise();
  
  shows.advanceClock();
  
  if (shows.getSmallCycle() >= MAX_SMALL_CYCLE) { 
    next_show(); 
  }
}

//
// next_show
//
void next_show() {
  if (current_show == 0) {
    current_show = random(1,NUM_SHOWS);
  } else {
    current_show = 0;
  }
//   current_show = (current_show + 1) % NUM_SHOWS;  // For testing
  led.fillBlack();
  led.push_frame();
  shows.resetAllClocks();
  noise.setRandomNoiseParams();
  shows.pickRandomColorSpeeds();
  shows.pickRandomWait();
  led.randomizePalette();
}

//// Start specialized shows

//
// morphcolor
//
void morphcolor() {
  uint16_t cycle = shows.getSmallCycle() % 500;
  
  if (cycle < NUM_LEDS) {
    for (uint8_t i=0; i < NUM_LEDS; i++) {
       if (i > cycle) {
         shows.setPixeltoForeColor(i);
       } else {
         shows.setPixeltoBackColor(i);
       }
    }
  } else if (cycle < 250) {
    shows.fillBackColor();
  
  } else if (cycle < 250 + NUM_LEDS) {
    for (uint8_t i=0; i < NUM_LEDS; i++) {
       if (i + 250 > cycle) {
         shows.setPixeltoBackColor(i);
       } else {
         shows.setPixeltoForeColor(i);
       }
    }
  } else {
    shows.fillForeColor();
    
  }
  led.push_frame();
}

//
// calcIntensity - use sine wave + cycle + variable to calculate intensity
//
// x ranges from 0 to max_x-1
// output is 0 to max_y
// speed spd (1, 2, 3, etc.) determines the rate of color change
//
uint8_t calcIntensity(uint8_t x, uint8_t max_x, uint8_t max_y, uint8_t spd) {
  uint8_t intense = map8(sin8_avr(map(x, 0, max_x, 0, 255) + (shows.getCycle() * spd)), 0, max_y);
  return intense;
}

//
// stripe
//
void stripe() {
  for (int i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Stripe_Pattern + i) == 0) {
      shows.setPixeltoForeColor(PatternLookUp[i]);
    } else {
      shows.setPixeltoBackColor(PatternLookUp[i]);
    }
  }
}

//
// alternate
//
void alternate() {
  for (int i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Alternate_Pattern + i)) {
      shows.setPixeltoForeColor(PatternLookUp[i]);
    } else {
      shows.setPixeltoBackColor(PatternLookUp[i]);
    }
  }
}

void diagcolor() {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(pgm_read_byte_near(Diag_Pattern + i), 9, 200, shows.getForeColorSpeed());
    led.setPixelHue(PatternLookUp[i], IncColor(shows.getForeColor(), change));
  }
}

void sectioncolor() {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(pgm_read_byte_near(Section_Pattern + i), 3, 128, shows.getForeColorSpeed());
    led.setPixelHue(PatternLookUp[i], IncColor(shows.getForeColor(), change));
  }
}

void sidesidecolor() {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(pgm_read_byte_near(SideSide_Pattern + i), 7, 256, shows.getForeColorSpeed());
    led.setPixelHue(PatternLookUp[i], IncColor(shows.getForeColor(), change));
  }
}

void explodecolor() {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(pgm_read_byte_near(Explode_Pattern + i), 6, 256, shows.getForeColorSpeed());
    led.setPixelHue(PatternLookUp[i], IncColor(shows.getForeColor(), change));
  }
}

void diagbright() {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(pgm_read_byte_near(Diag_Pattern + i), 9, 255, shows.getForeColorSpeed() * 3);
    led.setPixelColor(PatternLookUp[i], led.gradient_wheel(shows.getForeColor(), intense));
  }
}

void sidesidebright() {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(pgm_read_byte_near(SideSide_Pattern + i), 7, 255, shows.getForeColorSpeed() * 2);
    led.setPixelColor(PatternLookUp[i], led.gradient_wheel(shows.getForeColor(), intense));
  }
}

void explodebright() {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(pgm_read_byte_near(Explode_Pattern + i), 6, 255, shows.getForeColorSpeed() * 2);
    led.setPixelColor(PatternLookUp[i], led.gradient_wheel(shows.getForeColor(), intense));
  }
}

//// End specialized shows

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
    for (uint8_t x = 0; x < MAX_XCOORD; x++) {
      for (uint8_t y = 0; y < MAX_YCOORD; y++) {
        int8_t i = led.getLedFromCoord(x, y);
        if (i != -1) {
          led.setInterpFrameHue(i, noise.addNoiseAtoValue(x, y, led.getInterpFrameHue(i))); 
          led.setInterpFrameSat(i, noise.addNoiseBtoValueNoWrap(x, y, led.getInterpFrameSat(i)));
        } 
      }
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
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}
