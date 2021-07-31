#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
//#include <ArduinoBlue.h>  // (ArduinoBlue)
//
//  Big Pentagon: 12 x 12 = 144 lights in a square/pentagon grid
//
//  6/17/21
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 20;  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
uint32_t timing[] = { millis(), millis() };  // last_time, last_connection
#define SMOOTHING 2   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 7
#define CLOCK_PIN 8

#define NUM_LEDS 144
#define NUMBER_SPACER_LEDS 15
#define TOTAL_LEDS  (NUM_LEDS + NUMBER_SPACER_LEDS)

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define ARE_CONNECTED false  // Are the pentagons talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

boolean is_lost = false;
#define MAX_SILENT_TIME  (3 * 1000)  // Time (in sec) without communication before marked as is_lost

#define SIZE  12

#define PIXEL_X_SPACING  (255 / (SIZE - 1))
#define PIXEL_Y_SPACING  (255 / (SIZE - 1))

#define XX  255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[NUM_LEDS];  // The Leds themselves

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 20
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type

// wait times
#define SHOW_DURATION 120  // seconds
uint8_t FADE_TIME = 100;  // seconds to fade in + out (Arduino Blue)
#define MAX_SMALL_CYCLE  (SHOW_DURATION * 2 * (1000 / DELAY_TIME))  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out
uint8_t freq_storage[] = { 60, 80 };  // variable storage for shows
#define RING_SPEED 10  // Lower is faster

// Game of Life
#define LIFE_DIMENSION  (SIZE + 3)
#define TOTAL_LIFE  (LIFE_DIMENSION * LIFE_DIMENSION)
#define LIFE_OFFSET  ((LIFE_DIMENSION - SIZE) / 2)
boolean LIFE_BOARD[TOTAL_LIFE];  // Make this a 2-bit compression

#define PENTAGON 5
#define BALL_HUE 150
#define BALL_SIZE 30

// ArduinoBlue
//ArduinoBlue phone(Serial2); // Blue Tx = pin 9; Blue Rx = pin 10
#define HUE_SLIDER        0
#define HUE_WIDTH_SLIDER  1
#define SPEED_SLIDER      2
#define BRIGHTNESS_SLIDER 3
#define FADE_TIME_SLIDER  4
#define BAM_BUTTON        0
#define BOLT_TIME        20

// xBee language
#define COMMAND_START      '+'
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_BRIGHTNESS 'I'
#define COMMAND_PALETTE    'H'
#define COMMAND_PAL_WIDTH  'J'
#define COMMAND_SPEED      'Y'
#define COMMAND_MASK       'M'
#define COMMAND_P_TYPE     'p'
#define COMMAND_W_TYPE     'w'
#define COMMAND_FADE       'R'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_SM_CYCLE   'C'
#define COMMAND_PATTERN    'P'
#define COMMAND_BOLT       'X'
#define COMMAND_CHANNEL_A  'x'
#define COMMAND_CHANNEL_B  'y'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define EMPTY_CHAR         ' '
#define MAX_MESSAGE       120
#define MAX_NUM             6  // To handle 65,535 of small_cycle
char message[MAX_MESSAGE];     // Incoming message buffer
char number_buffer[MAX_NUM];   // Incoming number buffer

// Lookup tables

uint8_t neighbors[] PROGMEM = {
  12, 1, XX, XX, XX,
  13, 2, XX, 0, 12,
  14, 3, XX, XX, 1,
  15, 4, XX, 2, 14,
  16, 5, XX, XX, 3,
  17, 6, XX, 4, 16,
  18, 7, XX, XX, 5,
  19, 8, XX, 6, 18,
  20, 9, XX, XX, 7,
  21, 10, XX, 8, 20,
  22, 11, XX, XX, 9,
  23, XX, XX, 10, 22,
  24, 13, 1, 0, XX,
  25, 26, 14, 1, 12,
  26, 15, 3, 2, 13,
  27, 28, 16, 3, 14,
  28, 17, 5, 4, 15,
  29, 30, 18, 5, 16,
  30, 19, 7, 6, 17,
  31, 32, 20, 7, 18,
  32, 21, 9, 8, 19,
  33, 34, 22, 9, 20,
  34, 23, 11, 10, 21,
  35, XX, XX, 11, 22,
  36, 25, 12, XX, XX,
  37, 26, 13, 24, 36,
  38, 27, 14, 13, 25,
  39, 28, 15, 26, 38,
  40, 29, 16, 15, 27,
  41, 30, 17, 28, 40,
  42, 31, 18, 17, 29,
  43, 32, 19, 30, 42,
  44, 33, 20, 19, 31,
  45, 34, 21, 32, 44,
  46, 35, 22, 21, 33,
  47, XX, 23, 34, 46,
  48, 37, 25, 24, XX,
  49, 50, 38, 25, 36,
  50, 39, 27, 26, 37,
  51, 52, 40, 27, 38,
  52, 41, 29, 28, 39,
  53, 54, 42, 29, 40,
  54, 43, 31, 30, 41,
  55, 56, 44, 31, 42,
  56, 45, 33, 32, 43,
  57, 58, 46, 33, 44,
  58, 47, 35, 34, 45,
  59, XX, XX, 35, 46,
  60, 49, 36, XX, XX,
  61, 50, 37, 48, 60,
  62, 51, 38, 37, 49,
  63, 52, 39, 50, 62,
  64, 53, 40, 39, 51,
  65, 54, 41, 52, 64,
  66, 55, 42, 41, 53,
  67, 56, 43, 54, 66,
  68, 57, 44, 43, 55,
  69, 58, 45, 56, 68,
  70, 59, 46, 45, 57,
  71, XX, 47, 58, 70,
  72, 61, 49, 48, XX,
  73, 74, 62, 49, 60,
  74, 63, 51, 50, 61,
  75, 76, 64, 51, 62,
  76, 65, 53, 52, 63,
  77, 78, 66, 53, 64,
  78, 67, 55, 54, 65,
  79, 80, 68, 55, 66,
  80, 69, 57, 56, 67,
  81, 82, 70, 57, 68,
  82, 71, 59, 58, 69,
  83, XX, XX, 59, 70,
  84, 73, 60, XX, XX,
  85, 74, 61, 72, 84,
  86, 75, 62, 61, 73,
  87, 76, 63, 74, 86,
  88, 77, 64, 63, 75,
  89, 78, 65, 76, 88,
  90, 79, 66, 65, 77,
  91, 80, 67, 78, 90,
  92, 81, 68, 67, 79,
  93, 82, 69, 80, 92,
  94, 83, 70, 69, 81,
  95, XX, 71, 82, 94,
  96, 85, 73, 72, XX,
  97, 98, 86, 73, 84,
  98, 87, 75, 74, 85,
  99, 100, 88, 75, 86,
  100, 89, 77, 76, 87,
  101, 102, 90, 77, 88,
  102, 91, 79, 78, 89,
  103, 104, 92, 79, 90,
  104, 93, 81, 80, 91,
  105, 106, 94, 81, 92,
  106, 95, 83, 82, 93,
  107, XX, XX, 83, 94,
  108, 97, 84, XX, XX,
  109, 98, 85, 96, 108,
  110, 99, 86, 85, 97,
  111, 100, 87, 98, 110,
  112, 101, 88, 87, 99,
  113, 102, 89, 100, 112,
  114, 103, 90, 89, 101,
  115, 104, 91, 102, 114,
  116, 105, 92, 91, 103,
  117, 106, 93, 104, 116,
  118, 107, 94, 93, 105,
  119, XX, 95, 106, 118,
  120, 109, 97, 96, XX,
  121, 122, 110, 97, 108,
  122, 111, 99, 98, 109,
  123, 124, 112, 99, 110,
  124, 113, 101, 100, 111,
  125, 126, 114, 101, 112,
  126, 115, 103, 102, 113,
  127, 128, 116, 103, 114,
  128, 117, 105, 104, 115,
  129, 130, 118, 105, 116,
  130, 119, 107, 106, 117,
  131, XX, XX, 107, 118,
  132, 121, 108, XX, XX,
  133, 122, 109, 120, 132,
  134, 123, 110, 109, 121,
  135, 124, 111, 122, 134,
  136, 125, 112, 111, 123,
  137, 126, 113, 124, 136,
  138, 127, 114, 113, 125,
  139, 128, 115, 126, 138,
  140, 129, 116, 115, 127,
  141, 130, 117, 128, 140,
  142, 131, 118, 117, 129,
  143, XX, 119, 130, 142,
  XX, 133, 121, 120, XX,
  XX, XX, 134, 121, 132,
  XX, 135, 123, 122, 133,
  XX, XX, 136, 123, 134,
  XX, 137, 125, 124, 135,
  XX, XX, 138, 125, 136,
  XX, 139, 127, 126, 137,
  XX, XX, 140, 127, 138,
  XX, 141, 129, 128, 139,
  XX, XX, 142, 129, 140,
  XX, 143, 131, 130, 141,
  XX, XX, XX, 131, 142,
};

#define NUM_PATTERNS 21   // Total number of patterns, each 18 bytes wide

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
//  Serial.begin(9600);
  Serial1.begin(9600);  // Serial1: xBee port
  Serial2.begin(9600);  // Serial2: Bluetooth serial (ArduinoBlue)
//  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, TOTAL_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 4 neighbors for every pixel
    led[i].fillBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    
//    shows[i].setColorSpeedMinMax(6,30);  // Make colors change faster (lower = faster)
    shows[i].setWaitRange(10, 100);
    shows[i].setBandsBpm(10, 30);

    shows[i].setAsPentagon();
  }
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);
  timing[0] = millis();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
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
        shows[i].plinko(119);
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
        shows[i].sawTooth();
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

//  check_phone();  // Check the phone settings (ArduinoBlue)
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  fixed_delay();
  advance_clocks();  // advance the cycle clocks and check for next show
//  speak_and_hear();  // speak out or hear in signals
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
  long time_delta = new_time - timing[0];  // how much time has elapsed? Usually 3-5 milliseconds
  timing[0] = new_time;  // update the counter
  if (time_delta < DELAY_TIME) {  // if we have excess time,
    delay(DELAY_TIME - time_delta);  // delay for the excess
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  led[i].push_frame();
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(10, 100);
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  uint8_t mask = 0;
  uint8_t symmetry = 0;
  
  current_show[i] = is_other_channel_show_zero(i) ? random(1, NUM_SHOWS) : 0 ;
  show_variables[i] = random(NUM_PATTERNS);
  mask = pick_random_mask(i);
  
  if (mask == 0) {
    symmetry = pick_random_symmetry();
  }
  
  show_variables[i + 6] = mask;
  show_variables[i + 4] = symmetry;
  show_variables[i + 2] = random(4);
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
  speak_and_hear();
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
  if (current_show[i] != 0 && random(1, 4) == 1) {
    return random(1, 5);
  } else {
    return 0;
  }
}

uint8_t pick_random_pattern_type() {
  return random(12);
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
            shows[c].setPixeltoHue(i, shows[c].IncColor(back_hue, intense / 2));
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
    shows[channel].num_balls = random(2, 4);
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
            shows[channel].bounce_dir[n] = random(0, PENTAGON);
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
        uint8_t pixel = get_pixel_from_coord(x - LIFE_OFFSET, y - LIFE_OFFSET);
        if (value == false && hue == 0) {
          shows[i].setPixeltoColor(pixel, CHSV(200, 255, 0));
        } else {
          shows[i].setPixeltoHue(pixel, hue);
        }
     }
}

//// End specialized shows


////// Speaking and Hearing

void speak_and_hear() {
  if (ARE_CONNECTED && IS_SPEAKING) { speak_commands(); }
  if (ARE_CONNECTED && !IS_SPEAKING) { hear_signal(); }  
}

void speak_commands() {
  for (uint8_t i = 0; i < DUAL; i++) {  // Send one channel, then the next
//    speak_channel_commands(i);   // High chatter
    if (shows[i].getMorph() == 0) {   // Lower chatter
      speak_channel_commands(i);
    }
  }
}

void speak_channel_commands(uint8_t i) {
  // Speak all the commands for a particular channel
  uint8_t m = 0;  // where we are in the send-out message string

  m = speak_start(m);
  m = speak_channel(i, m);  // Send the channel A or B prompt
  
  m = speak_command(COMMAND_SM_CYCLE, shows[CHANNEL_A].getSmallCycle(), m);
  m = speak_command(COMMAND_FORE, shows[i].getForeColor(), m);
  m = speak_command(COMMAND_BACK, shows[i].getBackColor(), m);
  m = speak_command(COMMAND_SHOW, current_show[i], m);
  m = speak_command(COMMAND_WAIT, shows[i].getWait(), m);
  m = speak_command(COMMAND_MASK, show_variables[i + 6], m);
  m = speak_command(COMMAND_P_TYPE, show_variables[i + 8], m);
  m = speak_command(COMMAND_W_TYPE, show_variables[i + 10], m);
  
  message[m++] = COMMAND_PERIOD;  // Terminate message with period character
  message[m++] = '\0';  // Terminate message with null (new-line) character
  Serial1.print(message);  // Only done once
//  Serial.println(message);  // For debugging
}

void speak_arduinoblue_commands() {
  uint8_t m = 0;  // where we are in the send-out message string

  m = speak_start(m);
  m = speak_command(COMMAND_BRIGHTNESS, BRIGHTNESS, m);
  m = speak_command(COMMAND_PALETTE, hue_start, m);
  m = speak_command(COMMAND_PAL_WIDTH, hue_width, m);
  m = speak_command(COMMAND_SPEED, DELAY_TIME, m);
  m = speak_command(COMMAND_FADE, FADE_TIME, m);
  m = speak_command(COMMAND_BOLT, curr_lightning, m);
  
  message[m++] = COMMAND_PERIOD;  // Terminate message with null character
  message[m++] = '\0';  // Terminate message with null character
  Serial1.print(message);  // Only done once
//  Serial.println(message);  // For debugging
}

uint8_t speak_start(uint8_t m) {
  message[m++] = COMMAND_START;
  return m;
}

uint8_t speak_command(char cmd, int value, uint8_t m) {
  message[m++] = cmd;

  String value_str = String(value);
  for (uint8_t i = 0; i < value_str.length(); i++) {
    message[m++] = value_str[i];
  }

  message[m++] = COMMAND_COMMA;
  return m;
}

uint8_t speak_channel(int i, uint8_t m) {
  char command_channel = (i == 0) ? COMMAND_CHANNEL_A : COMMAND_CHANNEL_B ;
  message[m++] = command_channel;
  return m;
}

////// Hearing

void hear_signal() {
  if (Serial1.available()) {  // Heard a signal!
    ResetLostCounter();    
    boolean msg_received = GetMessage(message);  // Load global message buffer by pointer
    if (msg_received) {
      Serial.println(message);  // For debugging
      ProcessMessage(message);
    }
  } else {
    if (!is_lost && (millis() - timing[1] > MAX_SILENT_TIME) ) {
      is_lost = true;
    }
  }
}

//
// ResetLostCounter - board is not lost
//
void ResetLostCounter() {
  is_lost = false;
  timing[1] = millis();
}

//
// GetMessage - pulls the whole serial buffer until a period
//
boolean GetMessage(char* message) {
  char tmp;
  boolean have_start_signal = false;
  uint8_t tries = 0;
  uint16_t MAX_TRIES = 2000;
  uint8_t i = 0;

  while (tries++ < MAX_TRIES) {
    if (Serial1.available()) {
      tries = 0;
      tmp = Serial1.read();
//      Serial.print(tmp);  // For debugging

      if (!have_start_signal) {
        if (tmp == COMMAND_START) { have_start_signal = true; }  // Start of message
        else { } // discard prefix garbage
      } else {
        if (tmp == COMMAND_PERIOD) {
          message[i++] = tmp;  // End of message
          message[i++] = '\0';  // End of string
          return true;
        }  
        if (isAscii(tmp)) {
          message[i++] = tmp;
          if (i >= MAX_MESSAGE) {
            Serial.println("Message too long!");  // Ran out of message space
            return false;
          }
        }
      }
    }
  }
  Serial.println("Ran out of tries");
  return false;  // discard message
}

//
// ProcessMessage
//
void ProcessMessage(char* message) {
  uint8_t i = 0;
  uint8_t channel = 0;  // Default to channel 0
  char cmd;
  
  while (i < MAX_MESSAGE) {
    
    cmd = message[i];  // Get one-letter command

    if (cmd == COMMAND_PERIOD) {
      return;
    } else if (!isAscii(cmd)) {
      return;
    } else if (cmd == COMMAND_CHANNEL_A) {
      i++;
      channel = 0;
    } else if (cmd == COMMAND_CHANNEL_B) {
      i++;
      channel = 1;
    } else if (isAlpha(cmd)) {
      for (uint8_t j = 0; j < MAX_NUM; j++) { 
        number_buffer[j] = EMPTY_CHAR;  // Clear number buffer
      }
      
      int numsiz = ReadNum(message, i+1, number_buffer);  // Read in the number
      i = i + 2 + numsiz; // 2 = leap beginning command and trailing comma
      
      ExecuteOrder(cmd, atoi(number_buffer), channel);
    } else {
      return;  // Garbage
    }
  }
}

//
// ReadNum - reads numbers in a string
//
uint8_t ReadNum(char* msg, uint8_t place, char* number) {
  uint8_t i = 0; // Start of number
  char tmp;
  
  while (i < MAX_NUM) {
    tmp = msg[place];
    if (tmp == COMMAND_COMMA) {
      return (i);
    } else {
      number[i] = msg[place];
      i++;
      place++;
      if (place >= MAX_MESSAGE) { 
        break;  
      }
    }
  }
  Serial.println("Number too long");
  return (0); // Number too long
}

//
// Execute Order - execute a letter command followed by a number
//
void ExecuteOrder(char command, uint16_t value, uint8_t i) {
      
  switch (command) {
    
    case COMMAND_FORE:
      shows[i].setForeColor(value);
      break;
    
    case COMMAND_BACK:
      shows[i].setBackColor(value);
      break;

    case COMMAND_BRIGHTNESS:
      if (BRIGHTNESS != value) {
        BRIGHTNESS = value;
        FastLED.setBrightness( BRIGHTNESS );
      }
      break;
    
    case COMMAND_WAIT:
      if (shows[i].getWait() != value) { 
        shows[i].setWait(value);
      }
      break;
    
    case COMMAND_SHOW:  // Show
      if (current_show[i] != value) {
        current_show[i] = value % NUM_SHOWS;
        next_show(i);
      }
      break;

    case COMMAND_MASK:
      show_variables[i + 6] = value;
      break;

    case COMMAND_P_TYPE:
      show_variables[i + 8] = value;
      break;

    case COMMAND_W_TYPE:
      show_variables[i + 10] = value;
      break;

    case COMMAND_SM_CYCLE: // small_cycle
      shows[i].setSmallCycle(value);
      break;
  }
}

////// End Speaking & Hearing


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  mirror_pixels(CHANNEL_A);
  mirror_pixels(CHANNEL_B);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = mask(led[CHANNEL_B].getInterpFrameColor(rotate_pixel(i, CHANNEL_B)), i, CHANNEL_B);
    CHSV color_a = mask(led[CHANNEL_A].getInterpFrameColor(rotate_pixel(i, CHANNEL_A)), i, CHANNEL_A);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels

    color = narrow_palette(color);
//    color = lightning(narrow_palette(color));  // (ArduinoBlue)

    if (SMOOTHING > 0 && i >= 15) {  // Smoothing
      // Figure out why this doesn't work with certain pixels
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }
    led_buffer[i] = color;
    leds[convert_pixel_to_led(i)] = color;  // Send color to the actual LED
  }
  turn_off_spacer_leds();  // Try removing this?
}

//
// mask
//
CHSV mask(CHSV color, uint8_t i, uint8_t channel) {
  if (show_variables[channel + 6] == 0 || current_show[i] == 0) {
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
  const uint8_t LED_LOOKUP[] = {
     0,  2,  5,  6,  9, 10, 13, 14, 17, 18, 21, 22,
     1,  3,  4,  7,  8, 11, 12, 15, 16, 19, 20, 23,
    50, 47, 46, 43, 42, 39, 38, 35, 34, 31, 30, 28,
    49, 48, 45, 44, 41, 40, 37, 36, 33, 32, 29, 27,
    54, 56, 59, 60, 63, 64, 67, 68, 71, 72, 75, 76,
    55, 57, 58, 61, 62, 65, 66, 69, 70, 73, 74, 77,
   104,101,100, 97, 96, 93, 92, 89, 88, 85, 84, 82,
   103,102, 99, 98, 95, 94, 91, 90, 87, 86, 83, 81,
   108,110,113,114,117,118,121,122,125,126,129,130,
   109,111,112,115,116,119,120,123,124,127,128,131,
   158,155,154,151,150,147,146,143,142,139,138,136,
   157,156,153,152,149,148,145,144,141,140,137,135
  };
  return LED_LOOKUP[i % NUM_LEDS];
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
          led[channel].setInterpFrame(get_pixel_from_coord(x, SIZE - y - 1), led[channel].getInterpFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }
  
  if (symmetry == 2 || symmetry == 3) {  // Vertical mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (x >= SIZE / 2) {
          led[channel].setInterpFrame(get_pixel_from_coord(SIZE - x - 1, y), led[channel].getInterpFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }

  if (symmetry == 4 || symmetry == 6) {  // Diagonal 1 mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (x > y) {
          led[channel].setInterpFrame(get_pixel_from_coord(y,x), led[channel].getInterpFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }
  
  if (symmetry == 5 || symmetry == 6) {  // Diagonal 2 mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (x + y < SIZE - 1) {
          led[channel].setInterpFrame(get_pixel_from_coord(SIZE - y - 1, SIZE - x - 1), led[channel].getInterpFrameColor(get_pixel_from_coord(x,y)));
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

//
// turn off spacer leds - blacken the 16 space pixels
//
void turn_off_spacer_leds() {
  const uint8_t spacer_leds[] = { 24, 25, 26, 51, 52, 53, 78, 79, 80, 105, 106, 107, 132, 133, 134 };
  for (uint8_t i = 0; i < NUMBER_SPACER_LEDS; i++) {
    leds[spacer_leds[i]] = CRGB(0, 0, 0);
  }
}



//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();

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
  if (curr_lightning > 0) {
    uint8_t increase = 255 - cos8( map(curr_lightning, 0, BOLT_TIME, 0, 255));
    color.s -= increase;
    color.v += increase;
  }
  return color;
}

//
// check_phone - poll the phone for updated values  (ArduinoBlue)
//
//void check_phone() {
//  int8_t sliderId = phone.getSliderId();  // ID of the slider moved
//  int8_t buttonId = phone.getButton();  // ID of the button
//
//  if (sliderId != -1) {
//    int16_t sliderVal = phone.getSliderVal();  // Slider value goes from 0 to 200
//    sliderVal = map(sliderVal, 0, 200, 0, 255);  // Recast to 0-255
//
//    switch (sliderId) {
//      case BRIGHTNESS_SLIDER:
//        BRIGHTNESS = sliderVal;
//        FastLED.setBrightness( BRIGHTNESS );
//        break;
//      case HUE_SLIDER:
//        hue_start = sliderVal;
//        break;
//      case HUE_WIDTH_SLIDER:
//        hue_width = sliderVal;
//        break;
//      case SPEED_SLIDER:
//        DELAY_TIME = map8(sliderVal, 10, 100);
//        break;
//      case FADE_TIME_SLIDER:
//        FADE_TIME = map8(sliderVal, 0, SHOW_DURATION);
//        break;
//    }
//  }
//  
//  if (buttonId == BAM_BUTTON) { curr_lightning = BOLT_TIME; }
//}

//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  const uint8_t PatternMatrix[] = {
    0xc0, 0xc, 0x0, 0x30, 0x3, 0x0, 0xc, 0x0, 0xc0, 0x3, 0x0, 0x30, 0x0, 0xc0, 0xc, 0x0, 0x30, 0x3,
    0xc0, 0xc, 0x6, 0x30, 0x63, 0x18, 0xd, 0x80, 0xc0, 0x3, 0x1, 0xb0, 0x18, 0xc6, 0xc, 0x60, 0x30, 0x3,
    0xf, 0x0, 0xf0, 0xf, 0x0, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf, 0x0, 0xf0, 0xf, 0x0, 0xf0,
    0x99, 0x96, 0x66, 0x66, 0x69, 0x99, 0x99, 0x96, 0x66, 0x66, 0x69, 0x99, 0x19, 0x96, 0x66, 0x66, 0x69, 0x99,
    0x0, 0x6, 0x0, 0x60, 0xc1, 0x8c, 0x18, 0x0, 0x60, 0x6, 0x0, 0x18, 0x31, 0x83, 0x6, 0x0, 0x60, 0x0,
    0x0, 0x0, 0x60, 0x39, 0x3, 0x8, 0x20, 0x44, 0x62, 0x46, 0x22, 0x4, 0x10, 0xc0, 0x9c, 0x6, 0x0, 0x0,
    0x6, 0x0, 0x60, 0x0, 0xc, 0x3, 0xc0, 0x33, 0x4c, 0x32, 0xcc, 0x3, 0xc0, 0x30, 0x0, 0x6, 0x0, 0x60,
    0x55, 0x5a, 0xaa, 0x55, 0x5a, 0xaa, 0x55, 0x5a, 0xaa, 0x55, 0x5a, 0xaa, 0x55, 0x5a, 0xaa, 0x55, 0x5a, 0xaa,
    0xe3, 0x8e, 0x38, 0xe3, 0x81, 0xc7, 0x1c, 0x71, 0xc7, 0xe3, 0x8e, 0x38, 0xe3, 0x81, 0xc7, 0x1c, 0x71, 0xc7,
    0x30, 0xc3, 0xc, 0xf0, 0xff, 0xf, 0x8, 0x0, 0x60, 0x6, 0x0, 0x10, 0xf0, 0xff, 0xf, 0x30, 0xc3, 0xc,
    0x40, 0x8c, 0x66, 0x26, 0x61, 0x81, 0x18, 0x6, 0x6, 0x60, 0x60, 0x18, 0x81, 0x86, 0x64, 0x66, 0x31, 0x2,
    0x20, 0x63, 0x7, 0xc0, 0x74, 0x60, 0x6, 0x1, 0xf8, 0x1f, 0x80, 0x60, 0x6, 0x2e, 0x3, 0xe0, 0xc6, 0x4,
    0x20, 0x63, 0x9, 0xc0, 0x94, 0xc6, 0x4, 0x0, 0x40, 0x4, 0x0, 0x40, 0x66, 0x29, 0x3, 0x90, 0xc6, 0x4,
    0x0, 0x0, 0x0, 0x0, 0xc0, 0xc, 0xf, 0x0, 0x90, 0x9, 0x0, 0xf0, 0x30, 0x3, 0x0, 0x0, 0x0, 0x0,
    0x60, 0x7e, 0x9, 0xe0, 0x91, 0xce, 0x18, 0x1, 0x0, 0x0, 0x80, 0x18, 0x73, 0x89, 0x7, 0x90, 0x7e, 0x6,
    0x1, 0x81, 0x98, 0x18, 0x0, 0x0, 0x60, 0x66, 0x6, 0x0, 0x0, 0x60, 0x6, 0x0, 0x0, 0x1, 0x80, 0x18,
    0x20, 0x62, 0x19, 0xe1, 0x90, 0x6, 0x3, 0x60, 0x30, 0xc, 0x6, 0xc0, 0x60, 0x9, 0x87, 0x98, 0x46, 0x4,
    0x19, 0x87, 0x90, 0x78, 0x2e, 0x3, 0xec, 0x0, 0xc0, 0x3, 0x0, 0x37, 0xc0, 0x74, 0x1e, 0x9, 0xe1, 0x98,
    0x1, 0x81, 0x88, 0x8, 0x0, 0xc, 0x60, 0x42, 0x0, 0x0, 0x0, 0xc0, 0xc4, 0x4, 0x18, 0x8, 0x81, 0x80,
    0x24, 0x94, 0x92, 0x92, 0x42, 0x49, 0x49, 0x29, 0x24, 0x24, 0x94, 0x92, 0x92, 0x42, 0x49, 0x49, 0x29, 0x24,
    0x0, 0x6, 0x0, 0x60, 0x0, 0x78, 0x4, 0x81, 0xc8, 0x13, 0x81, 0x20, 0x1e, 0x0, 0x6, 0x0, 0x60, 0x0
  };
  pattern_number = pattern_number % NUM_PATTERNS;
  uint8_t pattern_byte = PatternMatrix[(pattern_number * 18) + (n / 8)];
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
