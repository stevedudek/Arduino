#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
//
//  Many Small Squares, each 6 x 6 = 36 lights
//
//  10/9/24
//
//  Modern Software, built on the Dragon software

#define SIZE  6  // edge of a small square
#define SQUARE_LEDS  (SIZE * SIZE)

#define NUM_SQUARES  16
#define NUM_LEDS  (NUM_SQUARES * SQUARE_LEDS)

uint8_t square_location[] = {  // x,y grid location for each square
  0, 0,  1, 1,  1, 2,  1, 3,
  2, 0,  2, 1,  2, 2,  3, 3,
  3, 4,  3, 2,  3, 0,  4, 4,
  4, 1,  4, 3,  5, 5,  5, 2
};

#define MAX_SQUARE_X  8  // none of the above coordinates can be bigger than this!
#define MAX_SQUARE_Y  7  // none of the above coordinates can be bigger than this!

uint8_t square_grid[MAX_SQUARE_X * MAX_SQUARE_Y]; // where each square sits in the grid

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 64;  // (0 = fast, 255 = slow)

#define DELAY_TIME 10 // in milliseconds

#define DATA_PIN 0
#define CLOCK_PIN 2

// Smoothing constants - lower is slower smoothing
#define SMOOTHING_SHOWS_HUE    1  // 4   // Fastest full rainbow = DELAY_TIME * (255 / this value) = 150 ms
#define SMOOTHING_SHOWS_VALUE  1  // 30   // Fastest turn off/on = DELAY_TIME * (255 / this value) = 150 ms

#define XX    255
#define XXXX  900

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[NUM_LEDS];  // The Leds themselves

uint8_t colors = 0;  // color scheme: 0 = all, 1 = red, 2 = blue, 3 = yellow

uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
#define BLACK  CHSV(0, 0, 0)

// Shows

#define SNAKE_SHOW_START   23  // Show this number to grided use the snake format
#define GRIDED_SHOW_START  30  // Show this number and above use the grid format

#define NUM_SHOWS 38

#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_type[] = { 0, 0 };  // whether all same, different, or on a big grid

#define SHOW_TYPE_ALL_SAME    0
#define SHOW_TYPE_DIFFERENT   1
#define SHOW_TYPE_BIG_SNAKE   2
#define SHOW_TYPE_BIG_GRID    3

#define NUM_SHOW_VARIABLES  (2 * NUM_SQUARES * 6)
#define VAR_PATTERN       0
#define VAR_ROTATE        1
#define VAR_SYMMETRY      2
#define VAR_MASK          3
#define VAR_PATTERN_TYPE  4
#define VAR_WIPE_TYPE     5
uint8_t show_variables[NUM_SHOW_VARIABLES];  // 6: pattern, rotate, symmetry, mask, pattern_type, wipe_type

uint8_t freq_storage[] = { 60, 80 };  // 1-byte storage for shows


// Clocks and time

uint8_t show_duration = 50;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 255;  // 0 = no fading, to 255 = always be fading

// Game of Life

#define LIFE_DIMENSION  10
#define TOTAL_LIFE  (LIFE_DIMENSION * LIFE_DIMENSION)
#define LIFE_OFFSET  2  //((LIFE_DIMENSION - SIZE) / 2)
boolean LIFE_BOARD[TOTAL_LIFE];  // Make this a 2-bit compression

//// Mesh parameters

// Messages on the mesh network are expensive, 
// prohibiting constant messages every 50ms or so
// Instead, initialize new shows properly on the show start
// with all the relevant parameters and then
// let the show runner do the right thing

// MESH Details
#define   MESH_PREFIX     "SQUARE" // name for your MESH
#define   MESH_PASSWORD   "roarsquare" // password for your MESH
#define   MESH_PORT       5556  //default port

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
void updateLeds(), changePalette();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);
Task taskChangePalette(TASK_MINUTE * 20, TASK_FOREVER, &changePalette);

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

//// End Mesh parameterss

//  0  1  2  3  4  5
//  6  7  8  9 10 11
// 12 13 14 15 16 17
// 18 19 20 21 22 23
// 24 25 26 27 28 29
// 30 31 32 33 34 35

uint8_t neighbors[] PROGMEM = {
  XX, 1, 6, XX,  // 0
  XX, 2, 7, 0, 
  XX, 3, 8, 1, 
  XX, 4, 9, 2,
  XX, 5, 10, 3,
  XX, XX, 11, 4,  // 5
  0, 7, 12, XX,
  1, 8, 13, 6,
  2, 9, 14, 7,
  3, 10, 15, 8,
  4, 11, 16, 9,  // 10
  5, XX, 17, 10,
  6, 13, 18, XX,
  7, 14, 19, 12, 
  8, 15, 20, 13, 
  9, 16, 21, 14,  // 15 
  10, 17, 22, 15,
  11, XX, 23, 16,
  12, 19, 24, XX,
  13, 20, 25, 18,
  14, 21, 26, 19,  // 20 
  15, 22, 27, 20,
  16, 23, 28, 21, 
  17, XX, 29, 22, 
  18, 25, 30, XX, 
  19, 26, 31, 24,  // 25  
  20, 27, 32, 25,
  21, 28, 33, 26, 
  22, 29, 34, 27, 
  23, XX, 35, 28, 
  24, 31, XX, XX,  // 30 
  25, 32, XX, 30,
  26, 33, XX, 31, 
  27, 34, XX, 32, 
  28, 35, XX, 33, 
  29, XX, XX, 34,  // 35
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );

  fill_square_grid();

  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 4 neighbors for every pixel
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

  clear_all_show_variables();  // blank out the show variables
  
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  set_color_scheme();

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskUpdateLeds);
  taskUpdateLeds.enable();

  userScheduler.addTask(taskChangePalette);
  taskChangePalette.enable();

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

//
// update leds - Moved to a task-scheduled event
//
void updateLeds() {
  
  for (uint8_t i = 0; i < DUAL; i++) {

    if (current_show[i] > 0 && current_show[i] < SNAKE_SHOW_START) {
      shows[i].resetNumLeds(SQUARE_LEDS);  // show is on just the first square 
    }
    if (current_show[i] >= SNAKE_SHOW_START) {
      check_speed[i];  // make sure the show is going fast
    }
    
    switch (current_show[i]) {
      
      case 0:
        patterns(i);
//        check_wiring(i);
        break;
      case 1:
        shows[i].lightWave();
        break;
      case 2:
        shows[i].morphChain();
        break;
      case 3:
        shows[i].bounce();
        break;
      case 4:
        shows[i].bounceGlowing();
        break;
      case 5:
        shows[i].plinko(5);
        break;
      case 6:
        shows[i].lightRunUp();  // good
        break;
      case 7:
        shows[i].sinelon_fastled();  // good
        break;
      case 8:
        windmill(i);  // good
        break;
      case 9:
        windmill_smoothed(i);  // good
        break;
      case 10:
        pendulum_wave(i, true);  // good
        break;
      case 11:
        pendulum_wave(i, false);  // good
        break;
      case 12:
        game_of_life(i); // ???
        break;
      case 13:
        shows[i].randomFlip();  // good
        break;
      case 14:
        shows[i].stripes();  // good
        break;
      case 15:
        shows[i].juggle_fastled();  // good
        break;
      case 16:
        shows[i].randomTwoColorBlack();  // good
        break;
      case 17:
        shows[i].randomOneColorBlack();  // good
        break;
      case 18:
        shows[i].sawTooth();  // good
        break;
      case 19:
        center_ring(i);  // good
        break;
      case 20:
        corner_ring(i);  // ???
        break;
      case 21:
        well(i);  // good
        break;
      //////////////////// Snake shows
      case 22:
        shows[i].lightWave();  // good
        break;
      case 23:
        shows[i].morphChain();
        break;
      case 24:
        shows[i].lightRunUp();  // good
        break;
      case 25:
        shows[i].sinelon_fastled();  // good
        break;
      case 26:
        shows[i].juggle_fastled();  // good
        break;
      case 27:
        shows[i].sawTooth();  // good
        break;
      case 28:
        shows[i].lightWave();  // good
        break;
      //////////////////// Grid shows
      case 29:
        windmill(i);  // good
        break;
      case 30:
        windmill_smoothed(i);  // good
        break;
      case 31:
        pendulum_wave(i, true);  // good
        break;
      case 32:
        pendulum_wave(i, false);  // good
        break;
      case 33:
        center_ring(i);  // good
        break;
      case 34:
        corner_ring(i);  // ???
        break;
      default:
        well(i);  // good
        break;
    }

    if (current_show[i] > 0 && current_show[i] < SNAKE_SHOW_START) {
      copy_first_square(i); 
    }
    
    shows[i].morphFrame();  // calculate interp_frame 2. adjust palette
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
  
  current_show[i] = (current_show[i] != 0 && !is_other_channel_show_zero(i)) ? 0 : random8(1, NUM_SHOWS) ;
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging

  if (current_show[i] < SNAKE_SHOW_START) {
//    show_type[i] = SHOW_TYPE_ALL_SAME;  // good for debugging
    set_show_variables_one_square(i, 0);
    show_type[i] = (random(4) == 1) ? SHOW_TYPE_ALL_SAME : SHOW_TYPE_DIFFERENT;
    if (show_type[i] == SHOW_TYPE_DIFFERENT) {
      for (uint8_t s = 1; s < NUM_SQUARES; s++) {
        set_show_variables_one_square(i, s);
      }
    }
  } else if (current_show[i] < GRIDED_SHOW_START) {
    show_type[i] = SHOW_TYPE_BIG_SNAKE;  // squares stacked adjacent like a snake
  } else {
    show_type[i] = SHOW_TYPE_BIG_GRID;  // squares put in a holey matrix
  }
  
  shows[i].pickRandomColorSpeeds();
  log_status(i);  // For debugging
}

void set_show_variables_one_square(uint8_t c, uint8_t s) {
  uint8_t mask = pick_random_mask(c);
  uint8_t symmetry = (mask == 0) ? pick_random_symmetry() : 0;
  
  set_show_variable(s, c, VAR_PATTERN, random8(NUM_PATTERNS));
  set_show_variable(s, c, VAR_MASK, mask);
  set_show_variable(s, c, VAR_ROTATE, random(4));
  set_show_variable(s, c, VAR_SYMMETRY, symmetry);
  set_show_variable(s, c, VAR_PATTERN_TYPE, random(12));
  set_show_variable(s, c, VAR_WIPE_TYPE, random(6));
}

boolean is_other_channel_show_zero(uint8_t c) {
  if (c == CHANNEL_A) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

void check_speed(uint8_t c) {
  // Make sure the show is moving quickly
  uint16_t spd = 90 / NUM_SQUARES;
  if (shows[c].getCycleDuration() > spd) {
    shows[c].pickRandomCycleDuration(spd / 3, spd);
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
//  copy_first_square - copy first square onto all the other squares
//
void copy_first_square(uint8_t c) {
  shows[c].resetNumLeds(NUM_LEDS);  // reopen led size to access all pixels
  
  for (uint16_t i = 0; i < SQUARE_LEDS; i++) {
    CHSV color = led[c].getNextFrameColor(i);
    for (uint16_t s = 1; s < NUM_SQUARES; s++) {
      shows[c].setPixeltoColor(i + (s * SQUARE_LEDS), color);
    }
  }
}

//
// get_pixel_from_coord - get LED i from x,y grid coordinate
//
uint16_t get_pixel_from_coord(uint8_t x, uint8_t y) {
  if (x < SIZE && y < SIZE) {
    x = y % 2 ? 6 - x - 1 : x ;  // serpentine for some reason
    return (y * 6) + x;
  
  } else {
    
    uint8_t big_x = x / SIZE;
    uint8_t big_y = y / SIZE;
    uint8_t big_s = square_grid[(big_y * MAX_SQUARE_X) + big_x];
    if (big_s == XX) {
      return XXXX; // not on the grid
    } else {
      x = x % SIZE;
      x = y % 2 ? 6 - x - 1 : x ;  // serpentine for some reason
      return ((y * 6) + x) + (big_s * SQUARE_LEDS);
    }
  }
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
// check wiring
//
void check_wiring(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }
  shows[c].fillForeBlack();
  shows[c].setPixeltoForeColor(shows[c].getCycle() % SQUARE_LEDS);
}

//
// patterns shows
//
void patterns(uint8_t c) {
  // Reset a lot of variables at the start of the show
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();  // Because we are using a beatsin8
  }

  for (uint8_t s = 0; s < NUM_SQUARES; s++) {
    
    uint8_t pattern = get_show_variable(s, c, VAR_PATTERN);
    uint8_t change_value = get_show_variable(s, c, VAR_PATTERN_TYPE)  % 2;
    uint8_t pattern_type = get_show_variable(s, c, VAR_PATTERN_TYPE) / 2;
    uint8_t wipe_type = get_show_variable(s, c, VAR_WIPE_TYPE);
    
    for (uint8_t x = 0; x < SIZE; x++) {
      for (uint8_t y = 0; y < SIZE; y++) {
        boolean value = get_bit_from_pattern_number((y * SIZE) + x, pattern);
        if (change_value == 1) {
          value = !value;
        }
        uint16_t i = get_pixel_from_coord(x,y) + (s * SQUARE_LEDS);
  
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
            uint8_t intense = get_wipe_intensity(x, y, wipe_type, c, (pattern_type < 4));
            uint8_t back_hue = shows[c].getBackColor();
            if (pattern_type < 4) {
              shows[c].setPixeltoHue(i, shows[c].IncColor(back_hue, intense));
            } else {
              shows[c].setPixeltoColor(i, led[c].gradient_wheel(back_hue, map8(intense, 64, 128)));
            }
          }
        }
      }
    }
    if (show_type[c] == SHOW_TYPE_ALL_SAME) {
      copy_first_square(c);
      return;
    }
  }
}

void test_layout(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  shows[c].fillForeBlack();
  uint8_t pixel = shows[c].getCycle()  % NUM_LEDS;
  
  shows[c].setPixeltoForeColor(pixel);
  Serial.print(pixel);
  Serial.print(": ");
  for (uint8_t d = 0; d < 4; d++) {
    shows[c].setPixeltoBackColor(led[c].getNeighbor(pixel, d));
    Serial.print(led[c].getNeighbor(pixel, d));
    Serial.print(", ");
  }
  Serial.println("");
}

//
// get_x_size | get_y_size - check the type of show to determine the size of the grid
//
uint8_t get_x_size(uint8_t c) {
  if (show_type[c] == SHOW_TYPE_BIG_GRID) {
    return SIZE * MAX_SQUARE_X;
  } else {
    return SIZE;
  }
}

uint8_t get_y_size(uint8_t c) {
  if (show_type[c] == SHOW_TYPE_BIG_GRID) {
    return SIZE * MAX_SQUARE_Y;
  } else {
    return SIZE;
  }
}

uint8_t get_x_spacing(uint8_t c) {
  return (255 / (get_x_size(c) - 1));
}

uint8_t get_y_spacing(uint8_t c) {
  return (255 / (get_y_size(c) - 1));
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
      intensity = 255 - get_distance_to_origin(x, y, c);  // center wipe
      break;
    default:
      intensity = (255 - get_distance_to_coord(x, y, 0, 0, c)) / 2;  // corner wipe
      break;
  }
  uint8_t freq = (color_wipe) ? map8(show_speed, 1, 4) : map8(show_speed, 5, 15);
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
      uint8_t value = 255 - (abs(beat - get_distance_to_origin(x, y, i)) * 2);
      
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
    freq_storage[i] = random(20, 40);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < get_x_size(i); x++) {
    value = beatsin8(freq_storage[i] - (3 * x));
    for (uint8_t y = 0; y < get_y_size(i); y++) {
      pos = (y * 256 / get_y_size(i)) + (256 / (get_y_size(i) * 2));
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
  
  for (uint8_t x = 0; x < get_x_size(i); x++) {
    for (uint8_t y = 0; y < get_y_size(i); y++) {
      uint8_t value = beatsin8(freq_storage[i] - (4 * y), 0, 255, 0, x * 128 / get_x_size(i) );
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
  
  for (uint8_t x = 0; x < get_x_size(i); x++) {
    for (uint8_t y = 0; y < get_y_size(i); y++) {
      uint8_t value = beatsin8(freq_storage[i] - (4 * y), 0, 255, 0, x * 255 / get_x_size(i) );
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

  for (uint8_t y = 0; y < get_y_size(c); y++) {
    for (uint8_t x = 0; x < get_x_size(c); x++) {
      uint8_t delta = abs(get_distance_to_coord(x, y, center_x, center_y, c) - value) % ring_freq;
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
uint8_t get_distance_to_origin(uint8_t x, uint8_t y, uint8_t c) {
  return get_distance_to_coord(x, y, 128, 128, c);
}

uint8_t get_distance_to_coord(uint8_t x, uint8_t y, uint8_t x_coord, uint8_t y_coord, uint8_t c) {
  uint8_t x_pos = 0;
  uint8_t y_pos = 0;
  get_coord_position(x, y, &x_pos, &y_pos, c);
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
void get_coord_position(uint8_t x, uint8_t y, uint8_t *x_pos, uint8_t *y_pos, uint8_t c) {
  *x_pos = get_x_spacing(c) * x;
  *y_pos = get_y_spacing(c) * y;
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
  boolean update_leds = false;
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B
  uint16_t pixel;

  for (uint8_t s = 0; s < NUM_SQUARES; s++) {
    for (uint8_t i = 0; i < SQUARE_LEDS; i++) {
      pixel = (s * SQUARE_LEDS) + i;

      CHSV color = led[CHANNEL_A].getInterpHSV(get_color_from_pixel(CHANNEL_B, s, i),
                                               get_color_from_pixel(CHANNEL_A, s, i), 
                                               fract);  // interpolate a + b channels
      if (hue_width < 255 || saturation < 255) {
       color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
      }
  
      // smoothing backstop. smooth constants should be large.
      color = led[CHANNEL_A].smooth_color(led_buffer[pixel], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);
  
      if (led_buffer[pixel].h == color.h && led_buffer[pixel].s == color.s && led_buffer[pixel].v == color.v) {
        continue;
      } else {
        leds[(s * SQUARE_LEDS) + convert_pixel_to_led(i)] = color;   // put HSV color on to LEDs
        led_buffer[pixel] = color;  // push new color into led_buffer (both HSV)
        update_leds = true;
      }
    }
  }

  if (update_leds) { 
    FastLED.show();
  }
}

//
// get_color_from_pixel
//
CHSV get_color_from_pixel(uint8_t c, uint8_t s, uint8_t i) {

  if (show_type[c] == SHOW_TYPE_BIG_GRID) {
    return led[c].getCurrFrameColor((s * SQUARE_LEDS) + i);
  } else {
    if (show_type[c] == SHOW_TYPE_ALL_SAME) {
      s = 0;
    }
    // mirror then rotate
    uint8_t new_i = rotate_pixel(c, s, mirror_pixel(c, s, i));
    return mask(led[c].getCurrFrameColor((s * SQUARE_LEDS) + new_i), c, s, i);
  }
}

//
// mask
//
CHSV mask(CHSV color, uint8_t c, uint8_t s, uint8_t i) {
  uint8_t mask_value = get_show_variable(s, c, VAR_MASK);
  if (mask_value == 0 || current_show[i] == 0) {
    return color;  // no masking
  }
  boolean value = get_bit_from_pattern_number(i, get_show_variable(s, c, VAR_PATTERN));
  if (mask_value % 2 == 0) {
    value = !value;
  }
  if (value) {
    return (mask_value > 2) ? shows[c].getBackBlack() : led[c].wheel(shows[c].getBackColor());
  } else {
    return color;
  }
}

//
// convert pixel to led
//
uint8_t convert_pixel_to_led(uint8_t i) {
  const uint8_t LED_LOOKUP[] = {
    1,  2,  5,  6,  9, 10,
    0,  3,  4,  7,  8, 11,
   22, 21, 18, 17, 14, 12,
   23, 20, 19, 16, 15, 13,
   24, 26, 29, 30, 33, 34,  // 25, 26, 29, 30, 33, 34,
   25, 27, 28, 31, 32, 35  // 24, 27, 28, 31, 32, 35
  };
  return LED_LOOKUP[i % SQUARE_LEDS];
}

//
// mirror_pixel
//
// changing this up away from writing out data
// instead, return the location of the mirrored pixel
//
uint8_t mirror_pixel(uint8_t c, uint8_t s, uint8_t i) {
  uint8_t symmetry = get_show_variable(s, c, VAR_SYMMETRY);

  if (symmetry == 0) {
    return i;
  }
  
  uint8_t x = i % SIZE;
  uint8_t y = i / SIZE;
    
  if (symmetry == 1 || symmetry == 3) {  // Horizontal mirroring
    if (y >= SIZE / 2) {
      y = SIZE - y - 1;
    }
  }
  
  if (symmetry == 2 || symmetry == 3) {  // Vertical mirroring
    if (x >= SIZE / 2) {
      x = SIZE - x - 1;
    }
  }

  if (symmetry == 4 || symmetry == 6) {  // Diagonal 1 mirroring
    if (x > y) {
      uint8_t temp = x;
      x = y;
      y = temp;
    }
  }
  
  if (symmetry == 5 || symmetry == 6) {  // Diagonal 2 mirroring
    if (x + y < SIZE - 1) {
      uint8_t temp = x;
      x = SIZE - y - 1;
      y = SIZE - temp - 1;
    }
  }

  return ((y * SIZE) + x) % SQUARE_LEDS;  // reconstitute i
}

//
// rotate_pixel - rotate square grid 90-degrees for each "r"
//
uint8_t rotate_pixel(uint8_t c, uint8_t s, uint8_t i) {
  uint8_t rotation = get_show_variable(s, c, VAR_ROTATE);
  if (rotation == 0) {
    return i;
  }
  uint8_t new_x, new_y;
  uint8_t x = i % SIZE;
  uint8_t y = i / SIZE;

  for (uint8_t r = rotation; r > 0; r--) {
    new_x = SIZE - y - 1;
    new_y = x;
    x = new_x;
    y = new_y;
  }

  return ((y * SIZE) + x) % SQUARE_LEDS;
}


//// End DUAL SHOW LOGIC


//// Square Grids

//
// get_show_variable
//
uint8_t get_show_variable(uint8_t square, uint8_t c, uint8_t variable) {
  return show_variables[((square * 6 * 2) + (c * 6) + variable)];
}

void set_show_variable(uint8_t square, uint8_t c, uint8_t variable, uint8_t value) {
  show_variables[((square * 6 * 2) + (c * 6) + variable)] = value;
}

void clear_all_show_variables() {
  for (uint8_t i = 0; i < NUM_SHOW_VARIABLES; i++) {
    show_variables[i] = 0;
  }
}

//
// fill_square_grid
//
void fill_square_grid() {
  // empty the square grid
  for (uint8_t i = 0; i < MAX_SQUARE_X * MAX_SQUARE_Y; i++) {
    square_grid[i] = XX;
  }
  
  for (uint8_t i = 0; i < NUM_SQUARES; i++) {
    square_grid[(MAX_SQUARE_X * square_location[(i*2)+1]) + square_location[(i*2)]] = i;
  }
}


//
// set_color_scheme
//
void set_color_scheme() {
  
  switch(colors) {
    
    case 1:  // red
      hue_center = 240;
      hue_width = 128;
      break;
    case 2:  // all
      hue_center = 0;
      hue_width = 255;
      break;
    case 3:  // yellow
      hue_center = 40;
      hue_width = 48;
      break;
    case 4:  // all
      hue_center = 0;
      hue_width = 255;
      break;
    case 5:  // blue
      hue_center = 170;
      hue_width = 64;
      break;
    default: // all
      hue_center = 0;
      hue_width = 255;
      break;
  }
}

void changePalette() {
  colors = (colors + 1) % 6;
  set_color_scheme();
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
  Serial.println(".");
}
