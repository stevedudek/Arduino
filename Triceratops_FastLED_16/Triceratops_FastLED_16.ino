#include <Noise.h>
#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Triceratops - 45 cones on a coat
//
//  8/10/18
//
//  FastLED
//  2D Noise Library
//  Led Library
//  Shows from Library
//
#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 45

#define MAX_XCOORD  4
#define MAX_YCOORD  17

#define MAX_COLOR 256   // Colors are 0-255

#define CAN_CHANGE_PALETTES true  // Palettes

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library (will need to be reinitialized)

// Shows
#define NUM_SHOWS 23
uint8_t current_show = 21;

// wait times
#define SHOW_DURATION 30  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 2   // seconds to fade out
#define FADE_IN_TIME 1    // seconds to fade in

// noise
#define HAVE_NOISE false   // set to false to suppress noise
Noise noise = Noise(MAX_XCOORD, MAX_YCOORD);

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
      shows.plinko(40);
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
    case 20:
      shows.packets();
      break;
    case 21:
      shows.packets_two();
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
  current_show = random8(NUM_SHOWS);
  // current_show = (current_show + 1) % NUM_SHOWS;  // For testing
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
// colorsize - light each cone according to its cone size
//
void colorsize() {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(ConeSize[i]-1, 5, sin8_avr(shows.getBackColor()), shows.getForeColorSpeed());
    led.setPixelHue(i, IncColor(shows.getForeColor(), change));
  }
}

//
// brightsize - light just one cone size
//
void brightsize() {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(ConeSize[i]-1, 5, 255, shows.getForeColorSpeed());
    led.setPixelColor(i, led.gradient_wheel(shows.getBackColor(), intense));
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
}

void diagcolor() {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(pgm_read_byte_near(Diag_Pattern + i), 9, 200, shows.getForeColorSpeed());
    led.setPixelHue(i, IncColor(shows.getForeColor(), change));
  }
}

void sectioncolor() {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(pgm_read_byte_near(Section_Pattern + i), 3, 128, shows.getForeColorSpeed());
    led.setPixelHue(i, IncColor(shows.getForeColor(), change));
  }
}

void sidesidecolor() {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(pgm_read_byte_near(SideSide_Pattern + i), 7, 256, shows.getForeColorSpeed());
    led.setPixelHue(i, IncColor(shows.getForeColor(), change));
  }
}

void explodecolor() {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(pgm_read_byte_near(Explode_Pattern + i), 6, 256, shows.getForeColorSpeed());
    led.setPixelHue(i, IncColor(shows.getForeColor(), change));
  }
}

void diagbright() {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(pgm_read_byte_near(Diag_Pattern + i), 9, 255, shows.getForeColorSpeed() * 3);
    led.setPixelColor(i, led.gradient_wheel(shows.getForeColor(), intense));
  }
  setHead(shows.getForeColor());
}

void sidesidebright() {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(pgm_read_byte_near(SideSide_Pattern + i), 7, 255, shows.getForeColorSpeed() * 2);
    led.setPixelColor(i, led.gradient_wheel(shows.getForeColor(), intense));
  }
  setHead(shows.getForeColor());
}

void explodebright() {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(pgm_read_byte_near(Explode_Pattern + i), 6, 255, shows.getForeColorSpeed() * 2);
    led.setPixelColor(i, led.gradient_wheel(shows.getForeColor(), intense));
  }
  setHead(shows.getForeColor());
}

//// End specialized shows

//
// lookup_leds - convert cones into LEDs - this is where mapping happens
//
int8_t lookup_leds(uint8_t i) {
  return pgm_read_byte_near(ConeLookUp + i);
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

void setHead(uint8_t hue) {
  led.setPixelHue(0, hue);
  led.setPixelHue(1, hue);
  led.setPixelHue(2, hue);
}
 
//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}
