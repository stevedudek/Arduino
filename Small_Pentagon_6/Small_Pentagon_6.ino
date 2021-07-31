#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>  // (ArduinoBlue)
//
//  Small Pentagon: 6 x 6 = 36 lights in a square/pentagon grid
//
//  9/22/20
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 20;  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
long last_time;
#define SMOOTHING 2   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 7
#define CLOCK_PIN 8

#define NUM_LEDS 36
#define NUMBER_SPACER_LEDS 6
#define TOTAL_LEDS  (NUM_LEDS + NUMBER_SPACER_LEDS)

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define SIZE  6

#define PIXEL_X_SPACING  (255 / (SIZE - 1))
#define PIXEL_Y_SPACING  (255 / (SIZE - 1))

#define XX  255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // The Leds themselves
CHSV led_buffer[TOTAL_LEDS];  // For smoothing

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 20
#define START_SHOW_CHANNEL_A  17  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  0
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type

// wait times
#define SHOW_DURATION 100  // seconds
uint8_t FADE_TIME = 50;  // seconds to fade in + out (Arduino Blue)
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out
uint8_t freq_storage[] = { 60, 80 };  // 1-byte storage for shows

// Game of Life
#define LIFE_DIMENSION  10
#define TOTAL_LIFE  (LIFE_DIMENSION * LIFE_DIMENSION)
#define LIFE_OFFSET  2  //((LIFE_DIMENSION - SIZE) / 2)
boolean LIFE_BOARD[TOTAL_LIFE];  // Make this a 2-bit compression

#define PENTAGON 5
#define BALL_HUE 150
#define BALL_SIZE 80

// ArduinoBlue
ArduinoBlue phone(Serial2); // Blue Tx = pin 9; Blue Rx = pin 10
#define HUE_SLIDER        0
#define HUE_WIDTH_SLIDER  1
#define SPEED_SLIDER      2
#define BRIGHTNESS_SLIDER 3
#define FADE_TIME_SLIDER  4
#define BAM_BUTTON        0
#define BOLT_TIME        20

// Lookup tables

#define NUMBER_SPACER_LEDS 6
const uint8_t spacer_leds[] PROGMEM = { 12, 13, 14, 27, 28, 29 };

const uint8_t LED_LOOKUP[] PROGMEM = {
    0,  2,  5,  6,  9, 10,
    1,  3,  4,  7,  8, 11,
   26, 23, 22, 19, 18, 16,
   25, 24, 21, 20, 17, 15,
   30, 32, 35, 36, 39, 40,
   31, 33, 34, 37, 38, 41
};

uint8_t neighbors[] PROGMEM = {
  6, 1, XX, XX, XX,
  7, 2, XX, 0, 6,
  8, 3, XX, XX, 1,
  9, 4, XX, 2, 8,
  10, 5, XX, XX, 3,
  11, XX, XX, 4, 10,
  12, 7, 1, 0, XX,
  13, 14, 8, 1, 6,
  14, 9, 3, 2, 7,
  15, 16, 10, 3, 8,
  16, 11, 5, 4, 9,
  17, XX, XX, 5, 10,
  18, 13, 6, XX, XX,
  19, 14, 7, 12, 18,
  20, 15, 8, 7, 13,
  21, 16, 9, 14, 20,
  22, 17, 10, 9, 15,
  23, XX, 11, 16, 22,
  24, 19, 13, 12, XX,
  25, 26, 20, 13, 18,
  26, 21, 15, 14, 19,
  27, 28, 22, 15, 20,
  28, 23, 17, 16, 21,
  29, XX, XX, 17, 22,
  30, 25, 18, XX, XX,
  31, 26, 19, 24, 30,
  32, 27, 20, 19, 25,
  33, 28, 21, 26, 32,
  34, 29, 22, 21, 27,
  35, XX, 23, 28, 34,
  XX, 31, 25, 24, XX,
  XX, XX, 32, 25, 30,
  XX, 33, 27, 26, 31,
  XX, XX, 34, 27, 32,
  XX, 35, 29, 28, 33,
  XX, XX, XX, 29, 34
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

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");
  Serial2.begin(9600);  // Serial2: Bluetooth serial (ArduinoBlue)

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, TOTAL_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 4 neighbors for every pixel
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    
    shows[i].setColorSpeedMinMax(6, 30);  // Make colors change faster (lower = faster)
    shows[i].setBandsBpm(10, 30);

    shows[i].setAsPentagon();
  }
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);
  last_time = millis();

  for (uint8_t i = 0; i < TOTAL_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }
  
  if (ONLY_RED) {  // (ArduinoBlue)
    hue_start = 192;
    hue_width = 124;
  }
}

//
// loop
//
void loop() { 
                   
  for (uint8_t i = 0; i < DUAL; i++) {

    switch (current_show[i]) {
  
      case 0:
        patterns(i);
        break;
      case 1:
        shows[i].morphChain();
        break;
      case 2:
        shows[i].bounce();
        break;
      case 3:
        bounceGlowing(i);
        break;
      case 4:
        shows[i].plinko(35);
        break;
      case 5:
        windmill(i);
        break;
      case 6:
        windmill_smoothed(i);
        break;
      case 7:
        cone_push(i);
        break;
      case 8:
        pendulum_wave(i, true);
        break;
      case 9:
        pendulum_wave(i, false);
        break;
      case 10:
        game_of_life(i, false);
        break;
      case 11:
        game_of_life(i, false);
        break;
      case 12:
        shows[i].randomFill();
        break;
      case 13:
        shows[i].juggle_fastled();
        break;
      case 14:
        shows[i].randomTwoColorBlack();
        break;
      case 15:
        shows[i].randomOneColorBlack();
        break;
      case 16:
        shows[i].sawTooth();  // Vetted
        break;
      case 17:
        center_ring(i);
        break;
      case 18:
        corner_ring(i);
        break;
      default:
        shows[i].bands();
        break;
    }
  
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  check_phone();  // Check the phone settings (ArduinoBlue)
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  fixed_delay();
  advance_clocks();  // advance the cycle clocks and check for next show
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { 
      next_show(i);
      pick_next_show(i);
    }
  }
  
  if (curr_lightning > 0 ) {  // (ArduinoBlue)
    curr_lightning--; // Reduce the current bolt
  }
}


//
// fixed_delay - make every cycle the same time
//
void fixed_delay() {
  long new_time = millis();
  long time_delta = new_time - last_time;  // how much time has elapsed? Usually 3-5 milliseconds
  last_time = new_time;  // update the counter
  if (time_delta < DELAY_TIME) {  // if we have excess time,
    delay(DELAY_TIME - time_delta);  // delay for the excess
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
//  shows[i].fillForeBlack();
  led[i].push_frame();
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(5, 60);
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  uint8_t mask = 0;
  uint8_t symmetry = 0; 
  uint8_t rotation = 0;
  current_show[i] = is_other_channel_show_zero(i) ? random8(1, NUM_SHOWS) : 0 ;
  show_variables[i] = random8(NUM_PATTERNS);
  mask = pick_random_mask(i);
  if (mask == 0) {
    symmetry = pick_random_symmetry(i);
    if (symmetry == 0) {
      rotation = random8(4);
    }
  }
  show_variables[i + 6] = mask;
  show_variables[i + 4] = symmetry;
  show_variables[i + 2] = rotation;
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickRandomColorSpeeds();
//  shows[i].tweakColorSpeeds();
  shows[i].pickRandomWait();
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

uint8_t pick_random_pattern_type() {
  return random(12);
}

uint8_t pick_random_symmetry(uint8_t i) {
  uint8_t random_symmetry = random(10);
  
  if (random_symmetry < 2) {
    return 3;  // 4-fold
  } else if (random_symmetry < 4) {
    return 1;  // Horizontal mirroring
  } else if (random_symmetry < 6) {
    return 2;  // Vertical mirroring
  } else {
    return 0;  
  }
}

//
// get_pixel_from_coord - get LED i from x,y grid coordinate
//
uint8_t get_pixel_from_coord(uint8_t x, uint8_t y) {
  return ((y % SIZE) * SIZE) + (x % SIZE);
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
    show_variables[c + 8] = pick_random_pattern_type();   // Pick a fill algorithm
    show_variables[c + 10] = random(6);  // Pick a different wipes
  }
  uint8_t pattern = show_variables[c];
  uint8_t change_value = show_variables[c + 8] % 2;
  uint8_t pattern_type = show_variables[c + 8] / 2;
  
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

//
// Test Neighbors
//
void test_neighbors(uint8_t channel) {
  shows[channel].fillBlack();
  uint8_t i = shows[channel].getCycle() % NUM_LEDS;
  led[channel].setPixelHue(i, shows[channel].getForeColor());
  
  for (uint8_t j = 0; j < 5; j++) {
    uint8_t n = pgm_read_byte_near(neighbors + (i * 5) + j);
    if (n != XX) {
      led[channel].setPixelHue(n, shows[channel].getBackColor());
    }
  }
}

//
// Get Neighbor
//
uint8_t getNeighbor(uint8_t pixel, uint8_t dir) {
  return pgm_read_byte_near(neighbors + (pixel * 5) + (dir % PENTAGON));
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
// Bounce Glowing
//
void bounceGlowing(uint8_t channel) {
  
  if (shows[channel].isShowStart()) {
    shows[channel].num_balls = random8(1, 4);
    shows[channel].clearTrails();
  }

  if (shows[channel].getMorph() != 0) {
    return;
  }

  // Clear background
  shows[channel].fill(CHSV(BALL_HUE, 255, 20));

  for (uint8_t n = 0; n < shows[channel].num_balls; n++) {
    uint8_t ball_position = shows[channel].bounce_pos[n];
    uint8_t ball_x = ball_position % SIZE;
    uint8_t ball_y = ball_position / SIZE;

    // Draw Ball
    for (uint8_t x = 0; x < SIZE; x++) {
      for (uint8_t y = 0; y < SIZE; y++) {
        uint8_t distance = get_dist(x, y, ball_x, ball_y);
        if (distance < BALL_SIZE) {
          uint8_t intensity = map(distance, 0, BALL_SIZE, 255, 0);
          led[channel].addPixelColor(get_pixel_from_coord(x, y), CHSV(BALL_HUE, 255, intensity));
        }
      }
    }

    // Move Ball
    uint8_t max_attempts = 10;

    while (getNeighbor(ball_position, shows[channel].bounce_dir[n]) == XX || random(0, PENTAGON * 3) == 0) {
            shows[channel].bounce_dir[n] = random8(0, PENTAGON);
            if (max_attempts-- == 0) {
              break;
            }
          }
    shows[channel].bounce_pos[n] = getNeighbor(ball_position, shows[channel].bounce_dir[n]);
  }
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
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 0, 0, 3, 32, 164);
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
void game_of_life(uint8_t i, boolean faster) {
  if (shows[i].isShowStart() || count_life() < 10) {
    randomly_populate_life(random(20, 50));
    if (faster) { 
      shows[i].makeWaitFaster(2); 
    } else { 
      shows[i].makeWaitSlower(2); 
    }
  }
  if (shows[i].getMorph() == 0) { 
    grow_life(i);
  }
}

void grow_life(uint8_t i) {
  shows[i].fill(CHSV(200, 255, 0));
  
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
        uint8_t pixel = get_pixel_from_coord(x - LIFE_OFFSET, y - LIFE_OFFSET);
        if (value == false && hue == 0) {
          shows[i].setPixeltoColor(pixel, CHSV(200, 255, 0));
        } else {
          shows[i].setPixeltoHue(pixel, hue);
        }
      }
}

//// End specialized shows

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  uint8_t led_number;

  mirror_pixels(CHANNEL_A);
  mirror_pixels(CHANNEL_B);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    led_number = convert_pixel_to_led(i);
    if (led_number != XX) {
      CHSV color_b = mask(led[CHANNEL_B].getInterpFrameColor(rotate_pixel(i, CHANNEL_B)), i, CHANNEL_B);
      CHSV color_a = mask(led[CHANNEL_A].getInterpFrameColor(rotate_pixel(i, CHANNEL_A)), i, CHANNEL_A);
      CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, led_buffer[led_number], fract);  // interpolate a + b channels
      color = lightning(narrow_palette(color));  // (ArduinoBlue)
      if (SMOOTHING > 0) {
        color = led[CHANNEL_A].smooth_color(led_buffer[led_number], color, SMOOTHING);  // smoothing
      }
      leds[led_number] = color;
      led_buffer[led_number] = color;
    }
  }
  turn_off_spacer_leds();
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
// convert pixel to led: account for the spacer pixels
//
uint8_t convert_pixel_to_led(uint8_t i) {
  if (i == XX) {
    return XX;
  }
  return pgm_read_byte_near(LED_LOOKUP + (i % NUM_LEDS));
}

//
// mirror_pixels
//
void mirror_pixels(uint8_t channel) {
  uint8_t symmetry = show_variables[4 + channel];
  
  if (symmetry == 1 || symmetry == 3) {  // Horizontal mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (x > y) {
          led[channel].setInterpFrame(get_pixel_from_coord(y,x), led[channel].getInterpFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }
  
  if (symmetry == 2 || symmetry == 3) {  // Vertical mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (x + y < SIZE - 1) {
          uint8_t new_y = SIZE - x - 1;
          uint8_t new_x = x - y + new_y;
          led[channel].setInterpFrame(get_pixel_from_coord(new_x, new_y), led[channel].getInterpFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }
}

//
// rotate_pixel
//
uint8_t rotate_pixel(uint8_t i, uint8_t channel) {
  uint8_t x = i % SIZE;
  uint8_t y = i / SIZE;
  uint8_t rotation = show_variables[2 + channel];  // Very strange memory bug here

  if (rotation > 1) {
    y = SIZE - y - 1;
  }
  if (rotation == 1 || rotation == 2) {
    x = SIZE - x - 1;
  }

  return ((y * SIZE) + x) % NUM_LEDS;
}

//
// turn off spacer leds - blacken the 16 space pixels
//
void turn_off_spacer_leds() {
  for (uint8_t i = 0; i < NUMBER_SPACER_LEDS; i++) {
    leds[pgm_read_byte_near(spacer_leds + i)] = CRGB(0, 0, 0);
  }
}



//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();

  // Similar logic to check_fades (deprecated)
  if (small_cycle < FADE_CYCLES) {
    intensity = map(small_cycle, 0, FADE_CYCLES, 0, 255);  // rise
  } else if (small_cycle <= (MAX_SMALL_CYCLE / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((MAX_SMALL_CYCLE / 2) + FADE_CYCLES)) {
    intensity = map(small_cycle - (MAX_SMALL_CYCLE / 2), 0, FADE_CYCLES, 255, 0);  // decay
  } else {
    intensity = 0;
  }
  return ease8InOutQuad(intensity);
}

//// End DUAL SHOW LOGIC

//
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  color.h = map8(color.h, hue_start, (hue_start + hue_width) % MAX_COLOR );
  return color;
}

//
// lightning - ramp all pixels quickly up to white (down sat & up value) and back down
//
CHSV lightning(CHSV color) {  // (ArduinoBlue)
//  if (curr_lightning > 0) {
//    uint8_t increase = 255 - cos8( map(curr_lightning, 0, BOLT_TIME, 0, 255));
//    color.s -= increase;
//    color.v += increase;
//  }
  return color;
}

//
// check_phone - poll the phone for updated values  (ArduinoBlue)
//
void check_phone() {
  int8_t sliderId = phone.getSliderId();  // ID of the slider moved
  int8_t buttonId = phone.getButton();  // ID of the button

  if (sliderId != -1) {
    int16_t sliderVal = phone.getSliderVal();  // Slider value goes from 0 to 200
    sliderVal = map(sliderVal, 0, 200, 0, 255);  // Recast to 0-255

    switch (sliderId) {
      case BRIGHTNESS_SLIDER:
        BRIGHTNESS = sliderVal;
        FastLED.setBrightness( BRIGHTNESS );
        break;
      case HUE_SLIDER:
        hue_start = sliderVal;
        break;
      case HUE_WIDTH_SLIDER:
        hue_width = sliderVal;
        break;
      case SPEED_SLIDER:
        DELAY_TIME = map8(sliderVal, 10, 100);
        break;
      case FADE_TIME_SLIDER:
        FADE_TIME = map8(sliderVal, 0, SHOW_DURATION);
        break;
    }
  }
  
  if (buttonId == BAM_BUTTON) { curr_lightning = BOLT_TIME; }
}

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
  Serial.print(", Wait: ");
  Serial.print(shows[i].getNumFrames());
  Serial.println(".");
}
