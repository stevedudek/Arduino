#include <Noise.h>
#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Linear Lights with FastLED
//
//  Leds from Library
//  Noise from Library
//  Shows from Library
//
//  9/10/2018
//
#define NUM_LEDS 55

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 30 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library

#define CAN_CHANGE_PALETTES true  // Palettes

// Shows
#define NUM_SHOWS 12
uint8_t current_show = 0;
uint8_t current_pattern = 0;
boolean has_black_pattern = true;

// Clocks and time
#define SHOW_DURATION 30  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 1   // seconds to fade out
#define FADE_IN_TIME 1    // seconds to fade out

// noise
#define HAVE_NOISE true    // set to false to suppress noise
Noise noise = Noise(NUM_LEDS);

// symmetries - 6 arms - 9 leds per arm - center is +1
#define NUM_SYMMETRIES 5
uint8_t symmetry = 3;  // 0-4
boolean invert = false;  // Whether to invert odd arms
boolean is_rotating = false;  // Whether arms rotate
const uint8_t symmetries[] = { 1, 2, 3, 6, 6 };
const uint8_t numSymLeds[] = { 55, 31, 16, 15, 16 };  // Determined empirically

//
// PATTERNS - 8 hex bytes per pattern (see Fish.ipynb)
//
#define NUM_PATTERNS 5   // Total number of patterns
const uint8_t pattern_matrix[] PROGMEM = {
  //               Top
  0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0,
  1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0,
  0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0,
  1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0,
  1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0,
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
    led.setPalette();  // turns on palettes with default valets
  }

  // Set up the various mappings here (1D lists in PROGMEM)
  //  led.setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  shows = Shows(&led);  // Show library - reinitialized for led mappings
  
  led.fillBlack();
  led.push_frame();

  set_symmetry();

  shows.setColorSpeedMinMax(2, 10); // Speed up color changing
}

void loop() {
  
  switch (current_show) {
  
    case 0:
      patternsBW();
      break;
    case 1:
      shows.allOn();
      break;
    case 2:
      shows.morphChain();
      break;
    case 3:
      shows.randomFill();  // Check color changing
      break;
    case 4:
      shows.randomColors();
      break;
    case 5:
      shows.twoColor();
      break;
    case 6:
      shows.lightWave();
      break;
    case 7:
      shows.sawTooth();  // Stuck on just 1- and 2- ; check symmetry
      break;
    case 8:
      shows.lightRunUp();  // Change colors faster
      break;
    case 9:
      shows.packets();  // Stuck on just 1- and 2- ; check symmetry
      break;
    case 10:
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
}

//
// next_show
//
void next_show() {
  // Switch between a patterns show and all the other shows
  current_pattern = random8(NUM_PATTERNS);
  has_black_pattern = (random8(2)) ? true : false;
  is_rotating = (random8(8) == 1) ? true : false;
  
  if (current_show < 2) {
    current_show++;
  } else if (current_show == 2) {
    current_show = random8(3, NUM_SHOWS);
  } else {
    current_show = 0;
  }
  
//  current_show = (current_show + 1) % NUM_SHOWS;  // For debugging
  set_symmetry();
  led.fillBlack();
  led.push_frame();
  shows.resetAllClocks();
  noise.setRandomNoiseParams();
  shows.pickRandomColorSpeeds();
  shows.pickRandomWait();
  led.randomizePalette(); 
}

//
// patterns shows
//
void patternsBW() {
  map_pattern(get_bit_from_pattern_number(0, current_pattern), 0);  // Center
  map_pattern(get_bit_from_pattern_number(1, current_pattern), 1);  // Start
  
  for (int i = 2; i < NUM_LEDS; i++) {
    map_pattern(get_bit_from_pattern_number(((i+2) % 9)+2, current_pattern), i);
  }
}

void map_pattern(boolean isOn, uint8_t i) {
  if (isOn) {
    shows.setPixeltoForeColor(i);
  } else {
    shows.setPixeltoBlack(i);
//    shows.setPixeltoBackColor(i);
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
// set_symmetry - pick a random symmetry for the duration of the show
//
void set_symmetry() {
  symmetry = random8(NUM_SYMMETRIES); // 0-4
  shows.resetNumLeds(numSymLeds[symmetry]);  // resets the virtual number of LEDs
  invert = (random8(2) && !has_black_pattern ) ? true : false;
}

//
// update_leds - push the interp_frame on to the leds - adjust for SYMMETRY!
//
void update_leds() {
  draw_center();
  draw_hub();
  draw_spokes();
}

//
// draw_center - fill in the center circle - pixel 54 maps to virtual led 0
//
void draw_center() {
  leds[54] = led.getInterpFrameColor(0);
}

//
// draw_hub - fill in the 6 petals around the center (48-53) with virtual leds (1+)
//
void draw_hub() {
  uint8_t pixel;
  uint8_t pitch = 6 / symmetries[symmetry];
  for (uint8_t i = 0; i < pitch; i++) {
    for (uint8_t j = 0; j < symmetries[symmetry]; j++) {
      pixel = try_rotate(48 + i + (j * pitch));
      leds[pixel] = led.getInterpFrameColor(1 + i);
    }
  }
}

//
// draw_spokes - draw the six 8-pixel spokes
//
// spoke virtual leds start at 7
// actual leds are 0-47
//
void draw_spokes() {
  uint8_t adj_i, pixel, pitch;
  
  if (symmetry < 4) {
    pitch = 48 / symmetries[symmetry];
    
    for (uint8_t i = 0; i < pitch; i++) {      
      for (uint8_t j = 0; j < symmetries[symmetry]; j++) {
        adj_i = (invert && j % 2 && symmetry == 3) ? ((pitch / 2) - i) % pitch : i;
        pixel = try_rotate(shift_one(adj_i + (j * pitch)));
        leds[pixel] = led.getInterpFrameColor(7 + i);
      }
    }
  } else {  // For 12-fold symmetry
    for (uint8_t i = 0; i < 5; i++) {
      for (uint8_t spoke = 0; spoke < 6; spoke++) {
        leds[shift_one((8 - i) + (spoke * 8))] = led.getInterpFrameColor(7 + i);
        leds[shift_one(i + (spoke * 8))] = led.getInterpFrameColor(7 + i);        
      }
    }
  }
}

//
// shift_one - shift the LED one over to fix in software the hardware layout
//
uint8_t shift_one(uint8_t i) {
  return (i + 47) % 48;
}

//
// try_rotate
//
uint8_t try_rotate(uint8_t i) {
  uint8_t new_i = (is_rotating) ? rotate(shows.getCycle() % 6, i) : i;
  return new_i
}

//
// rotate - rotate a pixel (i) by 0-5 arms (r)
//
uint8_t rotate(uint8_t r, uint8_t i) {
  if (i == 54) {  // center
    return i;
  } else if (i >= 48) {  // spoke hub
    return (((i-48) + r) % 6) + 48;
  } else {  // spoke
    return (i + (r*8)) % 48;
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
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern_byte = pgm_read_byte_near(pattern_matrix + (pattern_number * 11) + n);
  return (pattern_byte == 1);
}
