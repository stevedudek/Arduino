#include <Noise.h>
#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
//
//  Hedgehog! - 50 Lights
//
//  8/1/18
//
//  FastLED
//  1D Noise from Library
//  Led Library
//  Shows from Library
//
#define BRIGHTNESS  125 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 50

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library (will need to be reinitialized)

// Shows
#define NUM_SHOWS 15
uint8_t current_show = 0;

// Palettes
#define CAN_CHANGE_PALETTES true
#define PALETTE_DURATION 300  // seconds between palettes

// wait times
#define SHOW_DURATION 30  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 2   // seconds to fade out
#define FADE_IN_TIME 2    // seconds to fade out

// noise
#define HAVE_NOISE false   // set to false to suppress noise
Noise noise = Noise(NUM_LEDS);

// Lookup tables

const uint8_t ConeLookUp[50] PROGMEM= {
      8,
    9,13,7,
   10,12,14,6,
    11,15,5,
   17,16,3,4,
    18,2,0,
     19,1,
    20,22,24,
     21,23,
    40,25,27,
   41,39,26,28,
    42,38,29,
   43,37,31,30,
    44,36,32,
   45,47,35,33,
    46,48,34,
      49
};

// Centering: creates a center bright ring
const uint8_t centering[50] PROGMEM = {
      0,
    0,1,0,
   0,1,1,0,
    0,2,0,
   0,2,2,0,
    1,2,1,
     2,2,
    2,3,2,
     3,3,
    2,3,2,
   2,3,3,2,
    2,3,2,
   1,2,2,1,
    0,2,0,
   0,1,1,0,
    0,1,0,
      0,
};

// chevpattern: pattern of chevrons
const uint8_t chevpattern[50] PROGMEM = {
      9,
    9,8,9,
   9,8,8,9,
    8,7,8,
   8,7,7,8,
    7,6,7,
     6,6,
    6,5,6,
     5,5,
    5,4,5,
   5,4,4,5,
    4,3,4,
   4,3,3,4,
    3,2,3,
   3,2,2,3,
    2,1,2,
      0,
};

// starburstpattern: from center to edge
const uint8_t starburstpattern[50] PROGMEM = {
      9,
    9,8,9,
   8,7,7,8,
    6,5,6,
   5,4,4,5,
    3,2,3,
     1,1,
    2,0,2,
     1,1,
    2,1,2,
   4,3,3,4,
    5,4,5,
   6,5,5,6,
    7,6,7,
   8,7,7,8,
    9,8,9,
      9,
};

// centerstripe: a center stripe
const uint8_t centerstripe[50] PROGMEM = {
      1,
    0,1,0,
   0,0,1,0,
    0,1,0,
   0,1,0,0,
    0,1,0,
     0,1,
    0,1,0,
     1,0,
    0,1,0,
   0,0,1,0,
    0,1,0,
   0,1,0,0,
    0,1,0,
   0,0,1,0,
    0,1,0,
      1,
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
  led.setLedMap(ConeLookUp);  // turned off mapping - it's handled explicitly here in update_leds()
  shows = Shows(&led);  // Show library - reinitialized for led mappings
  shows.setWaitRange(4, 20, 16);
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
      chevrons();
      break;
    case 6:
      hogshell();
      break;
    case 7:
      bullseye();
      break;
    case 8:
      chevronrainbow();
      break;
    case 9:
      stripe();
      break;
    case 10:
      chevronfill();
      break;
    case 11:
      starburst();
      break;
    case 12:
      shows.morphChain();
      break;
    case 13:
      shows.lightWave();
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
}

//
// next_show
//
void next_show() {
  current_show = random8(NUM_SHOWS);
//  current_show = (current_show + 1) % NUM_SHOWS;  // For testing
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
// stripe
//
void stripe() {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(centerstripe + i)) {
      shows.setPixeltoForeColor(i);
    } else {
      shows.setPixeltoBlack(i);
    }
  }
}

//
// bullseye
//
void bullseye() {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    led.setPixelHue(i, IncColor(shows.getForeColor(), sin8((shows.getBackColor() / 5) * pgm_read_byte_near(centering + i))));
  }
}

//
// starburst
//
void starburst() {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(pgm_read_byte_near(starburstpattern + i), 10, 255, shows.getForeColorSpeed() * 3);
    led.setPixelColor(i, led.gradient_wheel(shows.getBackColor(), intense));
  }
}

//
// chevrons
//
void chevrons() {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(pgm_read_byte_near(chevpattern + i), 10, 255, shows.getForeColorSpeed() * 3);
    led.setPixelColor(i, led.gradient_wheel(shows.getBackColor(), intense));
  }
}

//
// chevronrainbow
//
void chevronrainbow() {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(pgm_read_byte_near(chevpattern + i), 10, 255, shows.getForeColorSpeed());
    led.setPixelHue(i, IncColor(shows.getForeColor(), change));
  }
}

//
// chevronfill
//
void chevronfill() {
  uint8_t pos, x;
  pos = shows.getCycle() % (11 * 2);  // Where we are in the show
  if (pos >= 11) {
    pos = (11 * 2) - pos;  // For a sawtooth effect
  }
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    x = pgm_read_byte_near(chevpattern + i);
    if (x < pos) {
      shows.setPixeltoBlack(i);
    } else {
      shows.setPixeltoForeColor(i);
    }
  }
}

//
// hogshell
//
void hogshell() {
  uint8_t s[4] = { 2, 10, 50, 255 };
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t x = pgm_read_byte_near(centering + i);
    led.setPixelColor(i, CHSV(shows.getForeColor(), 255, s[x]));
  }
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
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}
