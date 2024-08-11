#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
//
//  Small Pentagon: 6 x 6 = 36 lights in a square/pentagon grid
//
//  4/26/24
//
//  On the Adafruit Feather - Modern Software
//
//  MESH network! No phone connection
//
//  Speaking & Hearing
//

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME 15  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.

#define DATA_PIN  0
#define CLOCK_PIN 2

#define NUM_LEDS 36
#define NUMBER_SPACER_LEDS 6
#define TOTAL_LEDS  (NUM_LEDS + NUMBER_SPACER_LEDS)

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define SIZE  6

// Smoothing constants - lower is slower smoothing
#define SMOOTHING_SHOWS_HUE    10  // Fastest full rainbow = DELAY_TIME * (255 / this value) = 150 ms
#define SMOOTHING_SHOWS_VALUE  30   // Fastest turn off/on = DELAY_TIME * (255 / this value) = 150 ms

#define PIXEL_X_SPACING  (255 / (SIZE - 1))
#define PIXEL_Y_SPACING  (255 / (SIZE - 1))

#define XX  255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CRGB leds[TOTAL_LEDS];  // The Leds themselves
CHSV led_buffer[NUM_LEDS];  // For smoothing

#define MAX_COLOR 256   // Colors are 0-255

uint8_t colors = 0;  // color scheme: 0 = all, 1 = red, 2 = blue, 3 = yellow

uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

// Shows
#define NUM_SHOWS 20
#define START_SHOW_CHANNEL_A  1  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  2
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type

// wait times
uint8_t show_duration = 50;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 255;  // 0 = no fading, to 255 = always be fading
uint32_t max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // *2! 50% = all on, 50% = all off, and rise + decay on edges

uint8_t freq_storage[] = { 60, 80 };  // 1-byte storage for shows
#define RING_SPEED 10  // Lower is faster

// Game of Life
#define LIFE_DIMENSION  10
#define TOTAL_LIFE  (LIFE_DIMENSION * LIFE_DIMENSION)
#define LIFE_OFFSET  2  //((LIFE_DIMENSION - SIZE) / 2)
boolean LIFE_BOARD[TOTAL_LIFE];  // Make this a 2-bit compression

#define PENTAGON 5
#define BALL_HUE 150
#define BALL_SIZE 80

//// Mesh parameters

// Messages on the mesh network are expensive, 
// prohibiting constant messages every 50ms or so
// Instead, initialize new shows properly on the show start
// with all the relevant parameters and then
// let the show runner do the right thing

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

#define ARE_CONNECTED false  // Are the pentagons talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = 0;
#define MAX_SILENT_TIME  10000  // Time (in cycles) without communication before marked as is_lost

painlessMesh mesh;
String message;  // String to send to other displays
String getReadings();  // Prototype for reading state of LEDs

//#define MESSAGE_REPEAT 3   // send this many duplicate messages
#define MESSAGE_SPACING 4   // wait this many DELAY_TIME (7 x 15 ms = every 0.105 s)

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

#define IR_MESSAGE  0
#define MESH_MESSAGE  1
#define PRESET_MESSAGE  2

#define EMPTY_COMMAND  0
#define BRIGHTNESS_COMMAND  1
#define HUE_COMMAND  2
#define HUE_WIDTH_COMMAND  3
#define SPEED_COMMAND  4
#define SHOW_DURATION_COMMAND  5
#define FADE_COMMAND 6
#define SATURATION_COMMAND 7

//// End Mesh parameters

// Lookup tables

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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, TOTAL_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 4 neighbors for every pixel
    shows[i] = Shows(&led[i], i);  // Show library - reinitialized for led mappings
    shows[i].setShowDuration(show_duration);
    shows[i].setShowSpeed(show_speed);
    shows[i].setDelayTime(DELAY_TIME);
    shows[i].setFadeAmount(fade_amount);
    shows[i].setColorSpeedMinMax();
    shows[i].pickCycleDuration();
    shows[i].fillForeBlack();
    led[i].push_frame();

    shows[i].setAsPentagon();
  }
  
  shows[CHANNEL_B].setStartShowTime(millis() - shows[CHANNEL_A].getElapsedShowTime() - (show_duration * 1000 / 2));

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskUpdateLeds);
  taskUpdateLeds.enable();
}

//
// loop
//
void loop() {
  userScheduler.execute();  // Essential!
  if (ARE_CONNECTED) {
    mesh.update();
  }
}

void updateLeds() {
  
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
        shows[i].bounceGlowing();
//        bounceGlowing(i);
        break;
      case 4:
        shows[i].plinko(35);  // weird
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
        pendulum_wave(i, true);  // too fast
        break;
      case 9:
        pendulum_wave(i, false);  // too fast
        break;
      case 10:
        game_of_life(i, false);
        break;
      case 11:
        game_of_life(i, false);
        break;
      case 12:
        shows[i].lightRunUp();
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
        shows[i].confetti();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
    mirror_pixels(i);  // This flickers if done incorrectly
  }

  morph_channels();  // morph together the 2 leds channels and deposit on to Channel_A
  advance_clocks();  // advance the cycle clocks and check for next show
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].checkCycleClock();

    if (change_show(i)) {
      next_show(i);
      if (!ARE_CONNECTED || is_lost || (ARE_CONNECTED && IS_SPEAKING)) {
        pick_next_show(i);
      }
    }
    if (should_send_message(i)) {
      sendMessage(i);
    }
  }
  
  if (ARE_CONNECTED && !IS_SPEAKING) {
    if (last_connection++ > MAX_SILENT_TIME) {
      is_lost = true;
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
// should send message
//
bool should_send_message(uint8_t i) {
  if (ARE_CONNECTED && IS_SPEAKING) {
    return (shows[i].getCycle() == 0 && shows[i].getSmallCycle() % MESSAGE_SPACING == 0);
  }
  return false;
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  shows[i].resetAllClocks();
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  uint8_t mask = 0;
  uint8_t symmetry = 0;
  
  current_show[i] = (current_show[i] != 0 && !is_other_channel_show_zero(i)) ? 0 : random8(1, NUM_SHOWS) ;
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  show_variables[i] = random8(NUM_PATTERNS);
  mask = pick_random_mask(i);
  
  if (mask == 0) {
    symmetry = pick_random_symmetry();
  }
  show_variables[i + 6] = mask;
  show_variables[i + 4] = symmetry;
  show_variables[i + 2] = random(4);
  
  shows[i].pickRandomColorSpeeds();
//  log_status(i);  // For debugging
}

boolean is_other_channel_show_zero(uint8_t c) {
  if (c == CHANNEL_A) {
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
  uint8_t freq = (color_wipe) ? map8(show_speed, 1, 4) : map8(show_speed, 5, 15);
  return beatsin8(freq, 0, 255, 0, intensity);
}

/*

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

*/

//
// Cone Push
//
void cone_push(uint8_t i) {
  if (shows[i].isShowStart()) {
    shows[i].turnOffMorphing();
  }
  uint8_t beat = beatsin8(map8(show_speed, 2, 6), 0, 128, 0, 0);
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {
      uint8_t value = 255 - (abs(beat - get_distance_to_origin(x, y)) * 2);
      
      shows[i].setPixeltoColor(get_pixel_from_coord(x,y), led[i].gradient_wheel(shows[i].getForeColor(), value));
    }
  }
}

//
// Pendulum Wave
//
void pendulum_wave(uint8_t i, boolean smoothed) {
  uint8_t value, pos, dist;
  
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(20, 40);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < SIZE; x++) {
    value = beatsin8(freq_storage[i] - (3 * x));
    for (uint8_t y = 0; y < SIZE; y++) {
      pos = (y * 256 / SIZE) + (256 / (SIZE * 2));
      dist = (pos >= value) ? 255 - (pos - value) : 255 - (value - pos) ;
      if (smoothed == false) {
        dist = (dist > 230) ? dist : 0 ;
      }
      shows[i].setPixeltoColor(get_pixel_from_coord(x,y), CHSV(shows[i].getForeColor(), 255, dist) );
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
      shows[i].setPixeltoColor(get_pixel_from_coord(x,y), CHSV(shows[i].getForeColor(), 255, value) );
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
      shows[i].setPixeltoColor(get_pixel_from_coord(x,y), CHSV(shows[i].getForeColor(), 255, value) );
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
  }
  if (shows[i].isCycleStart()) { 
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



////// Speaking and Hearing

String getReadings (uint8_t c) {
  JSONVar jsonReadings;

  jsonReadings["type"] = MESH_MESSAGE;
  jsonReadings["c"] = c;
  jsonReadings["cycle"] = (const double)shows[c].getCycle();
  jsonReadings["cd"] = (const double)shows[c].getCycleDuration();
  jsonReadings["et"] = (const double)shows[c].getElapsedShowTime();
  jsonReadings["f"] = shows[c].getForeColor();
  jsonReadings["fspd"] = shows[c].getForeColorSpeed();
  jsonReadings["b"] = shows[c].getBackColor();
  jsonReadings["bspd"] = shows[c].getBackColorSpeed();
  jsonReadings["s"] = current_show[c];
  jsonReadings["p"] = show_variables[c];
  jsonReadings["m"] = show_variables[c + 6];
  jsonReadings["fill"] = show_variables[c + 8];
  jsonReadings["wp"] = show_variables[c + 10];
  jsonReadings["cs"] = colors;

  message = JSON.stringify(jsonReadings);
  return message;
}

void sendMessage (uint8_t c) {
  String msg = getReadings(c);
  mesh.sendBroadcast(msg);
  // Serial.println(msg);
}

void receivedCallback( uint32_t from, String &msg ) {
//  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  JSONVar myObject = JSON.parse(msg.c_str());

  switch (int(myObject["type"])) {
    case IR_MESSAGE:
      processIrMessage(myObject);
      break;
    case PRESET_MESSAGE:
      processPresetMessage(myObject);
      break;
    case MESH_MESSAGE:
      processMeshMessage(myObject);
      break;
  }
}

void processMeshMessage(JSONVar myObject) {
  last_connection = 0;
  is_lost = false;
  
  int c = int(myObject["c"]);
  shows[c].setCycle(double(myObject["cycle"]));
  shows[c].setCycleDuration(double(myObject["cd"]));
  shows[c].setElapsedShowTime(double(myObject["et"]));
  shows[c].setForeColor(int(myObject["f"]));
  shows[c].setForeColorSpeed(int(myObject["fspd"]));
  shows[c].setBackColor(int(myObject["b"]));
  shows[c].setBackColorSpeed(int(myObject["bspd"]));
  
  show_variables[c] = int(myObject["p"]);
  show_variables[c + 6] = int(myObject["m"]);
  show_variables[c + 8] = int(myObject["fill"]);
  show_variables[c + 10] = int(myObject["wp"]);

  int show_number = int(myObject["s"]);
  if (current_show[c] != show_number) {
    current_show[c] = show_number % NUM_SHOWS;
    next_show(c);
  }
  int colors_msg = int(myObject["cs"]);
  if (colors != colors_msg) {
    colors = colors_msg;
    set_color_scheme();
  }
}

void processPresetMessage(JSONVar myObject) {
  hue_center = int(myObject["hue"]);
  hue_width = int(myObject["width"]);
  updateShowSpeed(int(myObject["speed"]));
  updateShowDuration(int(myObject["duration"]));
  fade_amount = int(myObject["fade"]);
  saturation = int(myObject["saturation"]);
}

void processIrMessage(JSONVar myObject) {
  processIrParam(int(myObject["param1"]), int(myObject["value1"]));
  processIrParam(int(myObject["param2"]), int(myObject["value2"]));
}

void processIrParam(uint8_t param, uint8_t value) {
  
  switch (param) {
    case BRIGHTNESS_COMMAND:
      bright = value;
      FastLED.setBrightness( bright );
//      Serial.printf("brightness: %d\n", bright);
      break;
    case HUE_COMMAND:
      hue_center = value;
//      Serial.printf("hue: %d\n", hue_center);
      break;
    case HUE_WIDTH_COMMAND:
//      Serial.printf("hue width: %d\n", hue_width);
      hue_width = value;
      break;
    case SPEED_COMMAND:
      updateShowSpeed(value);
//      Serial.printf("speed: %d\n", show_speed);
      break;
    case SHOW_DURATION_COMMAND:
      updateShowDuration(value);
      break;
    case FADE_COMMAND:
      fade_amount = value;  // 0-255
//      Serial.printf("Fade: %d\n", fade_amount);
      break;
    case SATURATION_COMMAND:
      saturation = value;
//      Serial.printf("Saturation: %d\n", value);
      break;
    default:
      break;  // do nothing
  }
}

void updateShowSpeed(uint8_t spd) {
  show_speed = spd;
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].setShowSpeed(show_speed);
  }
}

void updateShowDuration(uint8_t duration) {
  show_duration = duration;
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].setShowDuration(show_duration);
  }  
}

void newConnectionCallback(uint32_t nodeId) {
//  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
//  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
//  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

////// End Speaking and Hearing



//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  boolean update_leds = true;  // update every cycle
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B
  
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = mask(led[CHANNEL_B].getCurrFrameColor(rotate_pixel(i, CHANNEL_B)), i, CHANNEL_B);
    CHSV color_a = mask(led[CHANNEL_A].getCurrFrameColor(rotate_pixel(i, CHANNEL_A)), i, CHANNEL_A);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    if (hue_width < 255 || saturation < 255) {
     color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
    }
    
    // smoothing backstop. smooth constants should be large.
    color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);

    if (led_buffer[i].h == color.h && led_buffer[i].s == color.s && led_buffer[i].v == color.v) {
      continue;
    } else {
      leds[convert_pixel_to_led(i)] = color;
      led_buffer[i] = color;
      update_leds = true;
    }
  }
  turn_off_spacer_leds();

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
// convert pixel to led: account for the spacer pixels
//
uint8_t convert_pixel_to_led(uint8_t i) {
  const uint8_t LED_LOOKUP[] = {
    0,  2,  5,  6,  9, 10,
    1,  3,  4,  7,  8, 11,
   26, 23, 22, 19, 18, 16,
   25, 24, 21, 20, 17, 15,
   30, 32, 35, 36, 39, 40,
   31, 33, 34, 37, 38, 41
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
          led[channel].setCurrentFrame(get_pixel_from_coord(x, SIZE - y - 1), led[channel].getCurrFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }
  
  if (symmetry == 2 || symmetry == 3) {  // Vertical mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (x >= SIZE / 2) {
          led[channel].setCurrentFrame(get_pixel_from_coord(SIZE - x - 1, y), led[channel].getCurrFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }

  if (symmetry == 4 || symmetry == 6) {  // Diagonal 1 mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (x > y) {
          led[channel].setCurrentFrame(get_pixel_from_coord(y,x), led[channel].getCurrFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }
  
  if (symmetry == 5 || symmetry == 6) {  // Diagonal 2 mirroring
    for (uint8_t y = 0 ; y < SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        if (x + y < SIZE - 1) {
          led[channel].setCurrentFrame(get_pixel_from_coord(SIZE - y - 1, SIZE - x - 1), led[channel].getCurrFrameColor(get_pixel_from_coord(x,y)));
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
  uint8_t spacer_leds[] = { 12, 13, 14, 27, 28, 29 };
  
  for (uint8_t i = 0; i < NUMBER_SPACER_LEDS; i++) {
    leds[spacer_leds[i]] = CRGB(0, 0, 0);
  }
}


//// End DUAL SHOW LOGIC

//
// set_color_scheme
//
void set_color_scheme() {
  
  switch(colors) {
    
    case 1:  // red
      hue_center = 240;
      hue_width = 128;
      break;
    case 2:  // blue
      hue_center = 170;
      hue_width = 64;
      break;
    case 3:  // yellow
      hue_center = 40;
      hue_width = 48;
      break;
    default: // all
      hue_center = 0;
      hue_width = 255;
      break;
  }
}


//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  const uint8_t PatternMatrix[] = {
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
  pattern_number = pattern_number % NUM_PATTERNS;
  uint8_t pattern_byte = PatternMatrix[(pattern_number * 5) + (n / 8)];
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
