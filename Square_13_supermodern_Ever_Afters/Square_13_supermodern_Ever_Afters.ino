#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
//
//  Small Square: 6 x 6 = 36 lights in a square grid
//
//  4/24/24
//
//  Modern Software

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME 20 // in milliseconds

#define DATA_PIN 0
#define CLOCK_PIN 2

#define NUM_LEDS 36
#define HALF_LEDS (NUM_LEDS / 2)  // Half that number

#define SIZE  6

// Smoothing constants - lower is slower smoothing
#define SMOOTHING_SHOWS_HUE    4   // Fastest full rainbow = DELAY_TIME * (255 / this value) = 150 ms
#define SMOOTHING_SHOWS_VALUE  30   // Fastest turn off/on = DELAY_TIME * (255 / this value) = 150 ms

#define XX  255

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define PIXEL_X_SPACING  (255 / (SIZE - 1))
#define PIXEL_Y_SPACING  (255 / (SIZE - 1))

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[NUM_LEDS];  // The Leds themselves

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
#define BLACK  CHSV(0, 0, 0)

// Shows

#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type
uint8_t freq_storage[] = { 60, 80 };  // 1-byte storage for shows
#define NUM_SHOWS 23

// Clocks and time

uint8_t show_duration = 40;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 0;  // 0 = no fading, to 255 = always be fading

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

// Game of Life
#define LIFE_DIMENSION  10
#define TOTAL_LIFE  (LIFE_DIMENSION * LIFE_DIMENSION)
#define LIFE_OFFSET  2  //((LIFE_DIMENSION - SIZE) / 2)
boolean LIFE_BOARD[TOTAL_LIFE];  // Make this a 2-bit compression

// Lookup tables

uint8_t neighbors[] PROGMEM = {
  11, 1, XX, XX, 
  10, 2, XX, 0, 
  9, 3, XX, 1, 
  8, 4, XX, 2, 
  7, 5, XX, 3, 
  6, XX, XX, 4, 
  17, XX, 5, 7, 
  16, 6, 4, 8, 
  15, 7, 3, 9, 
  14, 8, 2, 10, 
  13, 9, 1, 11, 
  12, 10, 0, XX, 
  23, 13, 11, XX, 
  22, 14, 10, 12, 
  21, 15, 9, 13, 
  20, 16, 8, 14, 
  19, 17, 7, 15, 
  18, XX, 6, 16, 
  29, XX, 17, 19, 
  28, 18, 16, 20, 
  27, 19, 15, 21, 
  26, 20, 14, 22, 
  25, 21, 13, 23, 
  24, 22, 12, XX, 
  35, 25, 23, XX, 
  34, 26, 22, 24, 
  33, 27, 21, 25, 
  32, 28, 20, 26, 
  31, 29, 19, 27, 
  30, XX, 18, 28, 
  XX, XX, 29, 31, 
  XX, 30, 28, 32, 
  XX, 31, 27, 33, 
  XX, 32, 26, 34, 
  XX, 33, 25, 35, 
  XX, 34, 24, XX,
};


#define NUM_PATTERNS 19   // Total number of patterns, each 5 bytes wide

const uint8_t PatternMatrix[] PROGMEM = {
  0xcf, 0x33, 0xc, 0xcf, 0x30,
  0xa9, 0x5a, 0x95, 0xa9, 0x50,
  0x1, 0xe4, 0x92, 0x78, 0x0,
  0x1, 0xe7, 0x9e, 0x78, 0x0,
  0x91, 0x22, 0x64, 0x48, 0x90,
  0x30, 0xcf, 0xff, 0x30, 0xc0,
  0x0, 0x3, 0xc, 0x0, 0x0,
  0x81, 0x86, 0x6, 0x18, 0x10,
  0xc, 0x33, 0xc, 0xc3, 0x0,
  0xc3, 0x3, 0xc, 0xc, 0x30,
  0x30, 0xcc, 0xf3, 0x30, 0xc0,
  0x99, 0x96, 0x66, 0x99, 0x90,
  0x0, 0xf, 0xff, 0x0, 0x0,
  0x30, 0xc3, 0xc, 0x30, 0xc0,
  0x4b, 0x30, 0x0, 0xcd, 0x20,
  0xfc, 0x1f, 0x45, 0xd5, 0x50,
  0x62, 0xcd, 0x9b, 0x34, 0x60,
  0xc, 0x73, 0x9c, 0xe3, 0x0,
  0x30, 0xcf, 0x3c, 0x0, 0x0,
};  


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( bright );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
//    led[i].setNeighborMap(neighbors);  // 4 neighbors for every pixel
    led[i].setAsSquare();
    shows[i] = Shows(&led[i], i);  // Show library - reinitialized for led mappings
    shows[i].setShowDuration(show_duration);
    shows[i].setShowSpeed(show_speed);
    shows[i].setDelayTime(DELAY_TIME);
    shows[i].setFadeAmount(fade_amount);
    shows[i].setColorSpeedMinMax();
    shows[i].pickCycleDuration();
    shows[i].fillForeBlack();
    led[i].push_frame();
  }
  
  shows[CHANNEL_B].setStartShowTime(millis() - shows[CHANNEL_A].getElapsedShowTime() - (show_duration * 1000 / 2));

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  if (ONLY_RED) {
    hue_center = 0;
    hue_width = 124;
  }

  userScheduler.addTask(taskUpdateLeds);
  taskUpdateLeds.enable();
}

//
// loop
//
void loop() {
  userScheduler.execute();  // Essential!
}

//
// update leds - Moved to a task-scheduled event
//
void updateLeds() {

  for (uint8_t i = 0; i < DUAL; i++) {

    switch (current_show[i]) {
  
      case 0:
//        test_layout(i);
        patterns(i);   // works
        break;
      case 1:
        shows[i].morphChain();   // weird
        break;
      case 2:
        shows[i].bounce();  // no
        break;
      case 3:
        shows[i].bounceGlowing();  // very no.  blink, blink
        break;
      case 4:
        shows[i].plinko(2);  // weird
        break;
      case 5:
        shows[i].lightRunUp();
        break;
      case 6:
        shows[i].sinelon_fastled();
        break;
      case 7:
        windmill(i);
        break;
      case 8:
        windmill_smoothed(i);
        break;
      case 9:
        cone_push(i);
        break;
      case 10:
        pendulum_wave(i, true);
        break;
      case 11:
        pendulum_wave(i, false);
        break;
      case 12:
        game_of_life(i);
        break;
      case 13:
        shows[i].randomFlip();
        break;
      case 14:
        shows[i].stripes();
        break;
      case 15:
        shows[i].juggle_fastled();
        break;
      case 16:
        shows[i].randomTwoColorBlack();
        break;
      case 17:
        shows[i].randomOneColorBlack();
        break;
      case 18:
        shows[i].sawTooth();  // Vetted
        break;
      case 19:
        center_ring(i);
        break;
      case 20:
        corner_ring(i);
        break;
      case 21:
        well(i);
        break;
      default:
        shows[i].lightWave();
        break;
    }
  
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  morph_channels();
  advance_clocks();
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].checkCycleClock();
    if (change_show(i)) {
      next_show(i);
      pick_next_show(i);
    }
  }
}

//
// change show
//
bool change_show(uint8_t i) {
  if (i == CHANNEL_A) {
    return shows[i].hasShowFinished();
  } else {
    return (shows[CHANNEL_A].getElapsedShowTime() > show_duration * 500 && shows[CHANNEL_B].getElapsedShowTime() > show_duration * 500);
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  shows[i].resetAllClocks();
  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  uint8_t mask = 0;
  uint8_t symmetry = 0;
  
//  current_show[i] = (current_show[i] != 0 && !is_other_channel_show_zero(i)) ? 0 : random8(1, NUM_SHOWS) ;
  show_variables[i] = random8(NUM_PATTERNS);
  mask = pick_random_mask(i);
  
  if (mask == 0) {
    symmetry = pick_random_symmetry();
  }
  show_variables[i + 6] = mask;
  show_variables[i + 4] = symmetry;
  show_variables[i + 2] = random(4);
  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
//  log_status(i);  // For debugging
}

boolean is_other_channel_show_zero(uint8_t channel) {
  if (channel == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

uint8_t pick_random_mask(uint8_t i) {
  // 2 bit identities x 2 color/black = 4 possibilities
  if (current_show[i] != 0 && random8(1, 4) == 1) {
    return random8(1, 5);
  } else {
    return 0;
  }
}

uint8_t pick_random_symmetry() {
  uint8_t random_symmetry = random(12);
  
  if (random_symmetry <= 6) {
    return random_symmetry;
  }
  return 0;  // No symmetrizing
}

//
// get_pixel_from_coord - get LED i from x,y grid coordinate
//
uint8_t get_pixel_from_coord(uint8_t x, uint8_t y) {
  x = y % 2 ? 6 - x - 1 : x ;
  return (y * 6) + x;
}

//
// get_dist - normalized to 0 - 255 (far corner)
//
uint8_t get_dist(uint8_t x1, uint8_t y1, float x2, float y2) {
  uint8_t mult = 180 / SIZE;
  uint16_t dx = sq(uint8_t(x2 * mult) - (x1 * mult));
  uint16_t dy = sq(uint8_t(y2 * mult) - (y1 * mult));
  return sqrt( dx + dy );
}

//
// patterns shows
//
void patterns(uint8_t c) {
  // Reset a lot of variables at the start of the show
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();  // Because we are using a beatsin8
    show_variables[c] = random(NUM_PATTERNS);  // Pick a pattern
    show_variables[c + 8] = random(12);   // Pick a fill algorithm
    show_variables[c + 10] = random(6);  // Pick a different wipes
  }
  uint8_t pattern = show_variables[c];
  uint8_t change_value = show_variables[c + 8] % 2;
  // Too much pulsing of patterns
  uint8_t pattern_type = show_variables[c + 8] / 6; // show_variables[c + 8] / 2;  // restrict to just 0 or 1
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {
      boolean value = get_bit_from_pattern_number((y * SIZE) + x, pattern);
      if (change_value == 1) {
        value = !value;
      }
      uint8_t i = get_pixel_from_coord(x,y);

      if (value) {
        if (pattern_type % 2 == 1) {
          shows[c].setPixeltoForeColor(i);
        } else {
          shows[c].setPixeltoForeBlack(i);
        }
        
      } else {
        if (pattern_type < 2) {
          shows[c].setPixeltoBackColor(i);
        } else {
          uint8_t intense = get_wipe_intensity(x, y, show_variables[c + 10], c, (pattern_type < 4));
          uint8_t back_hue = shows[c].getBackColor();
          if (pattern_type < 4) {
            shows[c].setPixeltoHue(i, shows[c].IncColor(back_hue, intense));
          } else {
            shows[c].setPixeltoColor(i, led[c].gradient_wheel(back_hue, map8(intense, 64, 255)));
          }
        }
      }
    }
  }
}

void test_layout(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  uint8_t pixel= shows[c].getCycle();
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (i == pixel % NUM_LEDS) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoForeBlack(i);
    }
  }
}


//
// Get Wipe Intensity
//
uint8_t get_wipe_intensity(uint8_t x, uint8_t y, uint8_t wipe_number, uint8_t c, boolean color_wipe) {
  uint8_t intensity;

  switch (wipe_number) {
    case 0:
      intensity = x * 4;  // coefficient = thickness
      break;
    case 1: 
      intensity = y * 4;
      break;
    case 2:
      intensity = (x * 2) + (y * 2);
      break;
    case 3:
      intensity = (x * 2) + ((SIZE - y - 1) * 2);
      break;
    case 4:
      intensity = 255 - get_distance_to_origin(x, y);  // center wipe
      break;
    default:
      intensity = (255 - get_distance_to_coord(x, y, 0, 0)) / 2;  // corner wipe
      break;
  }
  uint8_t freq = (color_wipe) ? 1 : 10;
  return beatsin8(freq, 0, 255, 0, intensity);
}

//
// Cone Push
//
void cone_push(uint8_t i) {
  if (shows[i].isShowStart()) {
    shows[i].turnOffMorphing();
  }
  uint8_t beat = beatsin8(10, 0, 128, 0, 0);
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {
      uint8_t value = 255 - (abs(beat - get_distance_to_origin(x, y)) * 2);
      
      led[i].setPixelColor(get_pixel_from_coord(x,y), led[i].gradient_wheel(shows[i].getForeColor(), value));
    }
  }
}

//
// Pendulum Wave
//
void pendulum_wave(uint8_t i, boolean smoothed) {
  uint8_t value, pos, dist;
  
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(40, 100);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < SIZE; x++) {
    value = beatsin8(freq_storage[i] - (4 * x));
    for (uint8_t y = 0; y < SIZE; y++) {
      pos = (y * 256 / SIZE) + (256 / (SIZE * 2));
      dist = (pos >= value) ? 255 - (pos - value) : 255 - (value - pos) ;
      if (smoothed == false) {
        dist = (dist > 230) ? dist : 0 ;
      }
      led[i].setPixelColor(get_pixel_from_coord(x,y), CHSV(shows[i].getForeColor(), 255, dist) );
    }
  }
}

//
// Windmill
//
void windmill(uint8_t i) {
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(40, 100);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {
      uint8_t value = beatsin8(freq_storage[i] - (4 * y), 0, 255, 0, x * 128 / SIZE );
      value = (value > 235) ? value : 0 ;
      led[i].setPixelColor(get_pixel_from_coord(x,y), CHSV(shows[i].getForeColor(), 255, value) );
    }
  }
}

void windmill_smoothed(uint8_t i) {
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(40, 100);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {
      uint8_t value = beatsin8(freq_storage[i] - (4 * y), 0, 255, 0, x * 255 / SIZE );
      led[i].setPixelColor(get_pixel_from_coord(x,y), CHSV(shows[i].getForeColor(), 255, value) );
    }
  }
}

//
// rings
//
void center_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 128, 128, 1, 32, 128);
}

void well(uint8_t c) {
  ring(c, shows[c].getForeColor(), led[c].wheel(shows[c].getBackColor()), 128, 128, 2, 192, 192);
}

void corner_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 0, 0, 3, 32, 128);
}

void ring(uint8_t c, uint8_t color, CHSV background, uint8_t center_x, uint8_t center_y, uint8_t ring_speed, uint8_t cutoff, uint8_t ring_freq) {
  // center_x, center_y = ring center coordinates; (5,5) is hex centers
  // ring_speed = 1+. Higher = faster.
  // cutoff = ring thickness with higher values = thicker
  // ring_freq = (255: 1 ring at a time; 128: 2 rings at a time)
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();
  }
  CHSV foreColor = led[c].wheel(color);
  led[c].fill(background);
  uint8_t value = shows[c].getCycle() / ring_speed;

  for (uint8_t y = 0; y < SIZE; y++) {
    for (uint8_t x = 0; x < SIZE; x++) {
      uint8_t delta = abs(get_distance_to_coord(x, y, center_x, center_y) - value) % ring_freq;
      if (delta < cutoff) {
        uint8_t intensity = map(delta, 0, cutoff, 255, 0);
        shows[c].setPixeltoColor(get_pixel_from_coord(x, y), led[c].getInterpHSV(background, foreColor, intensity));
      }
    }
  }
}

///// Distance functions

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t get_distance_to_origin(uint8_t x, uint8_t y) {
  return get_distance_to_coord(x, y, 128, 128);
}

uint8_t get_distance_to_coord(uint8_t x, uint8_t y, uint8_t x_coord, uint8_t y_coord) {
  uint8_t x_pos = 0;
  uint8_t y_pos = 0;
  get_coord_position(x, y, &x_pos, &y_pos);
  return get_pos_distance(x_pos, y_pos, x_coord, y_coord);
}

uint8_t get_pos_distance(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  // distance = sqrt(x^2 + y^2)
  // input: 0-255 coordinates
  // output: 0-255 distance
  // dx and dy should be no larger than 180 each
  uint8_t dx = map8(abs(x2 - x1), 0, 100);
  uint8_t dy = map8(abs(y2 - y1), 0, 100);
  return sqrt16((dx * dx) + (dy * dy));
}

//
// get coord position - convert (x, y) pixel to (x_pos, y_pos) coord, each (0-255)
void get_coord_position(uint8_t x, uint8_t y, uint8_t *x_pos, uint8_t *y_pos) {
  *x_pos = PIXEL_X_SPACING * x;
  *y_pos = PIXEL_Y_SPACING * y;
}

//
// Game of Life
//
void game_of_life(uint8_t i) {
  if (shows[i].isShowStart() || count_life() < 10) {
    randomly_populate_life(random(20, 50));
  }
  if (shows[i].isCycleStart()) { 
    grow_life(i);
  }
}

void grow_life(uint8_t i) {
  shows[i].fillBlack();
  
  for (uint8_t x = 0; x < LIFE_DIMENSION; x++) {
    for (uint8_t y = 0; y < LIFE_DIMENSION; y++) {
      uint8_t neighbors = count_life_neighbors(x,y);
      
      if (has_life(x,y) == 0) {
        if (neighbors == 3) {
          set_life_coord(x,y, true, 180, i);  // spontaneously grow a pixel
        } else {
          set_life_coord(x,y, false, 0, i);  // Turn pixel off
        }
      } else {
        
        switch(neighbors) {
          case 2:
            set_life_coord(x,y, true, 160, i);
            break;
          case 3:
            set_life_coord(x,y, true, 160, i);
            break;
          default:
            set_life_coord(x,y, false, 200, i);
            break;
        }
      }
    }
  }
}

uint8_t count_life() {
  uint8_t life = 0;
  for (uint8_t x = 0; x < LIFE_DIMENSION; x++) {
    for (uint8_t y = 0; y < LIFE_DIMENSION; y++) {
      life += has_life(x,y);
    }
  }
  return life;
}

uint8_t count_life_neighbors(uint8_t x, uint8_t y) {
  uint8_t neighbors = 0;
  
  for (uint8_t dx = 0; dx < 3; dx++) {  // Look at grid of 9 pixels
    for (uint8_t dy = 0; dy < 3; dy++) {
      if (dx == 1 && dy == 1) { continue; }  // Center (identity) square
      neighbors += has_life(x + dx - 1, y + dy - 1);
    }
  }
  return neighbors;
}

uint8_t has_life(uint8_t x, uint8_t y) {
  if (x < 0 || x >= LIFE_DIMENSION || y < 0 || y >= LIFE_DIMENSION || get_life_from_coord(x,y) == false) {
    return 0;
  }
  return 1;
}

void randomly_populate_life(uint8_t seeds) {
  for (uint8_t i = 0; i < TOTAL_LIFE; i++) { set_life(i, false); }
  for (uint8_t i = 0; i < seeds; i++) { set_life(random8(TOTAL_LIFE), true); }
}

void print_life_board() {
  for (uint8_t x = 0; x < LIFE_DIMENSION; x++) {
    for (uint8_t y = 0; y < LIFE_DIMENSION; y++) {
      Serial.print(has_life(x,y));
    }
    Serial.println();
  }
}

uint8_t get_life_pix_from_coord(uint8_t x, uint8_t y) {
  return (y * LIFE_DIMENSION) + x;
}

boolean get_life_from_coord(uint8_t x, uint8_t y) {
  boolean life = LIFE_BOARD[get_life_pix_from_coord(x,y)];
  return life;
}

boolean get_life(uint8_t i) {
  return LIFE_BOARD[i];
}

void set_life(uint8_t i, boolean value) {
  LIFE_BOARD[i] = value;
}

void set_life_coord(uint8_t x, uint8_t y, boolean value, uint8_t hue, uint8_t i) {
  set_life(get_life_pix_from_coord(x,y), value);
  
  if ((x >= LIFE_OFFSET) && (x < (LIFE_DIMENSION - LIFE_OFFSET)) && 
      (y >= LIFE_OFFSET) && (y < (LIFE_DIMENSION - LIFE_OFFSET))) {
        if (value == false && hue == 0) {
          shows[i].setPixeltoBlack(get_pixel_from_coord(x - LIFE_OFFSET, y - LIFE_OFFSET));
        } else {
          shows[i].setPixeltoHue(get_pixel_from_coord(x - LIFE_OFFSET, y - LIFE_OFFSET), hue);
        }
      }
  
}

//// End specialized shows


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  uint8_t led_number;
  boolean update_leds = false;
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B

//  mirror_pixels(CHANNEL_A);
//  mirror_pixels(CHANNEL_B);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    led_number = i;
    
    if (led_number != XX) {
      CHSV color_b = mask(led[CHANNEL_B].getCurrFrameColor(rotate_pixel(i, CHANNEL_B)), i, CHANNEL_B);
      CHSV color_a = mask(led[CHANNEL_A].getCurrFrameColor(rotate_pixel(i, CHANNEL_A)), i, CHANNEL_A);
      CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
      if (hue_width < 255 || saturation < 255) {
       color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
      }

      // smoothing backstop. smooth constants should be large.
      color = led[CHANNEL_A].smooth_color(led_buffer[led_number], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);

      if (led_buffer[led_number].h == color.h && led_buffer[led_number].s == color.s && led_buffer[led_number].v == color.v) {
        continue;
      } else {
        leds[led_number] = color;   // put HSV color on to LEDs
        led_buffer[led_number] = color;  // push new color into led_buffer (both HSV)
        update_leds = true;
      }
    }
  }

  if (update_leds) { 
    FastLED.show();
  }
}

//
// mask
//
CHSV mask(CHSV color, uint8_t i, uint8_t channel) {
  if (show_variables[channel + 6] == 0 || current_show[i] == 0) {  // ToDo: Check this
    return color;  // no masking
  }
  uint8_t mask_value = show_variables[channel + 6] - 1;
  boolean value = get_bit_from_pattern_number(i, show_variables[channel]);
  if (mask_value % 2 == 0) {
    value = !value;
  }
  if (value) {
    return (mask_value > 2) ? shows[channel].getBackBlack() : led[channel].wheel(shows[channel].getBackColor());
  } else {
    return color;
  }
}

//
// mirror_pixels
//
void mirror_pixels(uint8_t channel) {  
  uint8_t symmetry = show_variables[4 + channel];
  
  if (symmetry == 1 || symmetry == 3) {  // Horizontal mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (y >= SIZE / 2) {
          led[channel].setPixelColor(get_pixel_from_coord(x, SIZE - y - 1), led[channel].getCurrFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }
  
  if (symmetry == 2 || symmetry == 3) {  // Vertical mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (x >= SIZE / 2) {
          led[channel].setPixelColor(get_pixel_from_coord(SIZE - x - 1, y), led[channel].getCurrFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }

  if (symmetry == 4 || symmetry == 6) {  // Diagonal 1 mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (x > y) {
          led[channel].setPixelColor(get_pixel_from_coord(y,x), led[channel].getCurrFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }
  
  if (symmetry == 5 || symmetry == 6) {  // Diagonal 2 mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (x + y < SIZE - 1) {
          led[channel].setPixelColor(get_pixel_from_coord(SIZE - y - 1, SIZE - x - 1), led[channel].getCurrFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }
}

//
// rotate_pixel - rotate square grid 90-degrees for each "r"
//
uint8_t rotate_pixel(uint8_t i, uint8_t channel) {
  uint8_t new_x, new_y;
  uint8_t x = i % SIZE;
  uint8_t y = i / SIZE;
  uint8_t rotation = show_variables[2 + channel];

  for (uint8_t r = rotation; r > 0; r--) {
    new_x = SIZE - y - 1;
    new_y = x;
    x = new_x;
    y = new_y;
  }

  return ((y * SIZE) + x) % NUM_LEDS;
}


//// End DUAL SHOW LOGIC


//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern_byte = pgm_read_byte_near(PatternMatrix + (pattern_number * 5) + (n / 8) );
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

//
// log status
//
void log_status(uint8_t i) {
  Serial.print("Channel: ");
  Serial.print(i);
  Serial.print(", Show: ");
  Serial.print(current_show[i]);
  Serial.println(".");
}
