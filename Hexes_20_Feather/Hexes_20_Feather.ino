#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <math.h>
//
//  Hexes - 3 Large hexes
//
//  11/19/21
//
//  FastLED - On the Adafruit Feather
//
//  MESH network!
//
//  Listens to the Network and an IR Controller
//
//  For Shows & LED Libraries: Look in Documents/Arduino/libraries/Shows
//

/////// START HERE
//
//  Fri Nov 19: This compiles
//  See whether this runs on a Hex
//  The show_spd is not mapped to shows that use beatsin8
//  Speaking and Hearing do not well communicate mask numbers and show variables
//
////////


#define HEX_NUM   0
#define MAX_HEX   1

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME 12  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

// Hex Grid - Each Hex is 11 x 11
// MAX_XGRID, MAX_YGRID must be the same for all HEX
#define HEX_XGRID  0  // x-coord (upper left)
#define HEX_YGRID  0  // y-coord (upper left)
#define MAX_XGRID  11  //
#define MAX_YGRID  33  //

#define DATA_PIN  0
#define CLOCK_PIN 2

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define NUM_LEDS 91
#define ACTUAL_LEDS 106
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number

#define SIZE 11

#define XX  255

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
CHSV BLACK = CHSV(0, 0, 0);

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

// Shows
#define START_SHOW_CHANNEL_A  0  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B  1  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 27
#define NUM_PATTERNS 18   // Total number of patterns

uint8_t show_var[] = { 0, 0 };  // generic show variable - use carefully
uint8_t pattern[] = { 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, mask, pattern_type, wipe_type
uint8_t *show_storage[2];  // Array of SIZE length
uint8_t rotate_amount[] = { 0, 0 };  // 0-5 how much to rotate the hexes
uint8_t symmetry[] = { 0, 0 };  // 1 = no reflection, 2 = two halves, 3 = three pieces, 6 = six pieces

// wait times
uint8_t show_duration = 40;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 196; // 148;  // 0 = no fading, to 255 = always be fading
uint32_t max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // *2! 50% = all on, 50% = all off, and rise + decay on edges

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

#define ARE_CONNECTED true// Are the pentagons talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = 0;
#define MAX_SILENT_TIME  10000  // Time (in cycles) without communication before marked as is_lost

String message;  // String to send to other displays

#define MESSAGE_REPEAT 3   // send this many duplicate messages
#define MESSAGE_SPACING 5   // wait this many cycles
Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

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

// User stub
//void sendMessage();
void updateLeds();
String getReadings();  // Prototype for reading state of LEDs

//Task taskSendMessage(MSG_FREQUENCY, TASK_FOREVER, &sendMessage);
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

//// End Mesh parameters

#define NUMBER_SPACER_LEDS 15

const uint8_t coords[] PROGMEM = {
  XX, XX, XX, XX, XX,  0,  1,  2,  3,  4,  5, 
  XX, XX, XX, XX,  6,  7,  8,  9, 10, 11, 12, 
  XX, XX, XX, 13, 14, 15, 16, 17, 18, 19, 20, 
  XX, XX, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
  XX, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 
  40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 
  51, 52, 53, 54, 55, 56, 57, 58, 59, 60, XX, 
  61, 62, 63, 64, 65, 66, 67, 68, 69, XX, XX, 
  70, 71, 72, 73, 74, 75, 76, 77, XX, XX, XX, 
  78, 79, 80, 81, 82, 83, 84, XX, XX, XX, XX, 
  85, 86, 87, 88, 89, 90, XX, XX, XX, XX, XX
};


// 21 pixels
const uint8_t one_wedge[] PROGMEM = {
  0, 7, 15, 24, 34, 45, 6, 14, 23, 33, 44, 13, 22, 32, 43, 21, 31, 42, 30, 41, 40
};
// 36 pixels
const uint8_t two_wedges[] PROGMEM = {
  0, 7, 15, 24, 34, 45, 6, 14, 23, 33, 44, 13, 22, 32, 43, 21, 31, 42, 30, 41, 40,
  1, 8, 16, 25, 35, 2, 9, 17, 26, 3, 10, 18, 4, 11, 5
};
// 51 pixels
const uint8_t three_wedges[] PROGMEM = {
  0, 7, 15, 24, 34, 45, 6, 14, 23, 33, 44, 13, 22, 32, 43, 21, 31, 42, 30, 41, 40,
  1, 8, 16, 25, 35, 2, 9, 17, 26, 3, 10, 18, 4, 11, 5,
  12, 19, 27, 36, 46, 20, 28, 37, 47, 29, 38, 48, 39, 49, 50
};

#define LIFE_MAP_SIZE 13
#define LIFE_NUM_CELLS  (LIFE_MAP_SIZE * LIFE_MAP_SIZE)
boolean *game_of_life_curr_map = (boolean *)calloc(LIFE_NUM_CELLS * DUAL, sizeof(boolean));
boolean *game_of_life_next_map = (boolean *)calloc(LIFE_NUM_CELLS * DUAL, sizeof(boolean));

//
// Pixel neighbors
//
const uint8_t neighbors[] PROGMEM = {
7, 6, XX, XX, XX, 1,
8, 7, 0, XX, XX, 2,
9, 8, 1, XX, XX, 3,
10, 9, 2, XX, XX, 4,
11, 10, 3, XX, XX, 5,
12, 11, 4, XX, XX, XX,
14, 13, XX, XX, 0, 7,
15, 14, 6, 0, 1, 8,
16, 15, 7, 1, 2, 9,
17, 16, 8, 2, 3, 10,
18, 17, 9, 3, 4, 11,
19, 18, 10, 4, 5, 12,
20, 19, 11, 5, XX, XX,
22, 21, XX, XX, 6, 14,
23, 22, 13, 6, 7, 15,
24, 23, 14, 7, 8, 16,
25, 24, 15, 8, 9, 17,
26, 25, 16, 9, 10, 18,
27, 26, 17, 10, 11, 19,
28, 27, 18, 11, 12, 20,
29, 28, 19, 12, XX, XX,
31, 30, XX, XX, 13, 22,
32, 31, 21, 13, 14, 23,
33, 32, 22, 14, 15, 24,
34, 33, 23, 15, 16, 25,
35, 34, 24, 16, 17, 26,
36, 35, 25, 17, 18, 27,
37, 36, 26, 18, 19, 28,
38, 37, 27, 19, 20, 29,
39, 38, 28, 20, XX, XX,
41, 40, XX, XX, 21, 31,
42, 41, 30, 21, 22, 32,
43, 42, 31, 22, 23, 33,
44, 43, 32, 23, 24, 34,
45, 44, 33, 24, 25, 35,
46, 45, 34, 25, 26, 36,
47, 46, 35, 26, 27, 37,
48, 47, 36, 27, 28, 38,
49, 48, 37, 28, 29, 39,
50, 49, 38, 29, XX, XX,
51, XX, XX, XX, 30, 41,
52, 51, 40, 30, 31, 42,
53, 52, 41, 31, 32, 43,
54, 53, 42, 32, 33, 44,
55, 54, 43, 33, 34, 45,
56, 55, 44, 34, 35, 46,
57, 56, 45, 35, 36, 47,
58, 57, 46, 36, 37, 48,
59, 58, 47, 37, 38, 49,
60, 59, 48, 38, 39, 50,
XX, 60, 49, 39, XX, XX,
61, XX, XX, 40, 41, 52,
62, 61, 51, 41, 42, 53,
63, 62, 52, 42, 43, 54,
64, 63, 53, 43, 44, 55,
65, 64, 54, 44, 45, 56,
66, 65, 55, 45, 46, 57,
67, 66, 56, 46, 47, 58,
68, 67, 57, 47, 48, 59,
69, 68, 58, 48, 49, 60,
XX, 69, 59, 49, 50, XX,
70, XX, XX, 51, 52, 62,
71, 70, 61, 52, 53, 63,
72, 71, 62, 53, 54, 64,
73, 72, 63, 54, 55, 65,
74, 73, 64, 55, 56, 66,
75, 74, 65, 56, 57, 67,
76, 75, 66, 57, 58, 68,
77, 76, 67, 58, 59, 69,
XX, 77, 68, 59, 60, XX,
78, XX, XX, 61, 62, 71,
79, 78, 70, 62, 63, 72,
80, 79, 71, 63, 64, 73,
81, 80, 72, 64, 65, 74,
82, 81, 73, 65, 66, 75,
83, 82, 74, 66, 67, 76,
84, 83, 75, 67, 68, 77,
XX, 84, 76, 68, 69, XX,
85, XX, XX, 70, 71, 79,
86, 85, 78, 71, 72, 80,
87, 86, 79, 72, 73, 81,
88, 87, 80, 73, 74, 82,
89, 88, 81, 74, 75, 83,
90, 89, 82, 75, 76, 84,
XX, 90, 83, 76, 77, XX,
XX, XX, XX, 78, 79, 86,
XX, XX, 85, 79, 80, 87,
XX, XX, 86, 80, 81, 88,
XX, XX, 87, 81, 82, 89,
XX, XX, 88, 82, 83, 90,
XX, XX, 89, 83, 84, XX,
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
  
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );

  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setCoordMap(SIZE, coords);  // x, y grid of hexes
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    
    shows[i].setColorSpeedMinMax(show_speed);
    shows[i].setWaitRange(show_speed);
    
    show_storage[i] = (uint8_t *)calloc(SIZE, sizeof(uint8_t));
    set_show_variables(i);
  }
  
  if (ONLY_RED) {  // (ArduinoBlue)
    hue_center = 0;
    hue_width = 124;
  }

  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);  // Start Channel B offset at halfway through show

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
  // Moved to a task-scheduled event
  for (uint8_t i = 0; i < DUAL; i++) {
    
    switch (current_show[i]) {

      case 0:
        patterns(i);
        break;
      case 1:
        shows[i].randomFill();
        break;
      case 2:
        shows[i].juggle_fastled();
        break;
      case 3:
        shows[i].randomTwoColorBlack();
        break;
      case 4:
        shows[i].randomOneColorBlack();
        break;
      case 5:
        shows[i].sawTooth();
        break;
      case 6:
        pendulum_wave(i);
        break;
      case 7:
        shows[i].bounce();
        break;
      case 8:
        shows[i].bounceGlowing();
        break;
      case 9:
        shows[i].plinko(90);
        break;
      case 10:
        shows[i].bands();
        break;
      case 11:
        vert_back_forth_dots(i);
        break;
      case 12:
        vert_back_forth_bands(i);  // grided
        break;
      case 13:
        vert_back_forth_colors(i);  // grided
        break;
      case 14:
        diag_back_forth_dots(i);  // grided
        break;
      case 15:
        diag_back_forth_bands(i);  // grided
        break;
      case 16:
        diag_back_forth_colors(i);
        break;
      case 17:
        shows[i].sinelon_fastled();  // vetted
        break;
      case 18:
        center_ring(i);  // vetted
        break;
      case 19:
        corner_ring(i);  // vetted
        break;
      case 20:
        game_of_life_one_color(i);
        break;
      case 21:
        rain(i);  // vetted
        break;
      case 22:
        well(i);  // vetted
        break;
      case 23:
        radar(i);  // vetted
        break;
      case 24:
        wipers(i);  // vetted
        break;
      case 25:
        orbits(i);  // vetted
        break;
      default:
        game_of_life_two_color(i);
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }
  
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show 
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= max_small_cycle) { 
      next_show(i);
      if (!ARE_CONNECTED || is_lost || (ARE_CONNECTED && IS_SPEAKING)) {
        pick_next_show(i);
      }
    }
    if (ARE_CONNECTED && IS_SPEAKING) {
      // Send MESSAGE_REPEAT duplicate messages at the start of a show, then be quiet
      uint32_t smallCycle = shows[i].getSmallCycle();
      if ((smallCycle <= MESSAGE_REPEAT * MESSAGE_SPACING) && (smallCycle % MESSAGE_SPACING == 0)) {
        sendMessage(i);
      }
    }
  }
  if (ARE_CONNECTED && !IS_SPEAKING) {
    if (last_connection++ > MAX_SILENT_TIME) {
      is_lost = true;
    }
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  set_show_variables(i);  // Look into this
  led[i].push_frame();
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(show_speed);
  shows[i].setColorSpeedMinMax(show_speed);
  shows[i].pickRandomWait();
  shows[i].pickRandomColorSpeeds();
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  current_show[i] = is_other_channel_show_zero(i) ? random(1, NUM_SHOWS) : 0 ;
  pattern[i + 2] = pick_random_mask(i);  // mask
  rotate_amount[i] = random(6);
  symmetry[i] = (pattern[i + 2] == 0) ? pick_random_symmetry(i) : 0;
  shows[i].pickRandomColorSpeeds();
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

uint8_t pick_random_symmetry(uint8_t i) {
  uint8_t random_symmetry = random(10);
  
  if (random_symmetry == 0) {
    return 6;
  } else if (random_symmetry < 3) {
    return 3;
  } else if (random_symmetry < 6) {
    return 2;
  } else {
    return 1;
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

//
// set_show_variables - depends on the show
//
void set_show_variables(uint8_t i) {
  for (uint8_t pixel = 0; pixel < SIZE; pixel++) {
    show_storage[i][pixel] = random8(255);
  }
  
  switch (current_show[i]) {

    case 5:
      show_var[i] = random8(5);
      break;
    case 13:
      show_var[i] = random8(2, 10);
      break;
    case 14:
      show_var[i] = random8(2, 10);
      break;
    case 15:
      show_var[i] = random8(2, 10);
      break;
    case 23:
      show_var[i] = random8(6);
      break;
    default:
      break;
  }
}

//
// Patterns
//
void patterns(uint8_t c) {
  // Reset a lot of variables at the start of the show
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();  // Because we are using a beatsin8
    pattern[c + 0] = random(NUM_PATTERNS);  // Pick a pattern
    pattern[c + 2] = 0;  // Turn off masking
    pattern[c + 4] = random(12);   // Pick a fill algorithm
    pattern[c + 6] = random(6);  // Pick a different wipes
  }
  uint8_t pattern_number = pattern[c];
  uint8_t change_value = pattern[c + 4] % 2;
  uint8_t pattern_type = pattern[c + 4] / 2;
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {
      uint8_t i = pgm_read_byte_near(coords + (y * SIZE) + x);
      if (i != XX) {
        boolean value = get_bit_from_pattern_number(i, pattern_number);
        if (change_value == 1) {
          value = !value;
        }
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
            uint8_t intense = get_wipe_intensity(x, y, pattern[c + 6], (pattern_type < 4));
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
}

//
// Get Wipe Intensity
//
uint8_t get_wipe_intensity(uint8_t x, uint8_t y, uint8_t wipe_number, boolean color_wipe) {
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
      intensity = (255 - get_distance(x, y, 0, 0)) / 2;  // corner wipe
      break;
  }
  uint8_t freq = (color_wipe) ? 1 : 10;
  return beatsin8(freq, 0, 255, 0, intensity);
}




////// Specialized shows

//
// pendulum_wave
//
void pendulum_wave(uint8_t c) {
  uint8_t side_bands = show_var[c] % 6; // how many side bands
  shows[c].fillBlack();
  
  for (uint8_t y = 0; y < SIZE; y++) {
    uint8_t x = beatsin8(30 + y) / 23;
    shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x, y));

    if (side_bands > 0) {
      for (uint8_t i = 0; i < side_bands; i++) {
        CHSV color = led[c].gradient_wheel(shows[c].getForeColor(), (side_bands - i) * (255 / (side_bands + 1)));
        if (x - i - 1 > 0) {
          shows[c].setPixeltoColor(led[c].getLedFromCoord(x - i - 1, y), color);
        }
        if (x + i + 1 < SIZE) {
          shows[c].setPixeltoColor(led[c].getLedFromCoord(x + i + 1, y), color);
        }
      }
    }
  }
}

//
// rain
//
void rain(uint8_t c) {

  shows[c].turnOffMorphing();
  uint8_t side_bands = show_var[c] % 6; // how many side bands
  shows[c].fillForeBlack();
  
  for (uint8_t y = 0; y < SIZE; y++) {
    uint8_t x = beat8(map8(show_storage[c][y], 20, 80)) / 23;
    shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x, y));

    if (side_bands > 0) {
      for (uint8_t i = 0; i < side_bands; i++) {
        CHSV color = led[c].gradient_wheel(shows[c].getForeColor(), (side_bands - i) * (255 / (side_bands + 1)));
        if (x - i - 1 > 0) {
          shows[c].setPixeltoColor(led[c].getLedFromCoord(x - i - 1, y), color);
        }
        if (x + i + 1 < SIZE) {
          shows[c].setPixeltoColor(led[c].getLedFromCoord(x + i + 1, y), color);
        }
      }
    }
  }
}

//
// radar
//
void radar(uint8_t c) {
  uint8_t thickness = map8(show_storage[c][0], 64, 128);
  
  shows[c].fillForeBlack();
  
  for (uint8_t y = 0; y < SIZE; y++) {
    for (uint8_t x = 0; x < SIZE; x++) {
      uint8_t i = led[c].getLedFromCoord(x, y);
      if (i != XX) {
        uint8_t angle_delta = abs((shows[c].getSmallCycle() % 256) - get_angle_to_origin(x, y));
        if (angle_delta < thickness) {
          uint8_t intensity = map(angle_delta, 0, thickness, 255, 0);
          shows[c].setPixeltoColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intensity));
        }
      }
    }
  }
}

//
// wipers
//
void wipers(uint8_t c) {
  uint8_t thickness = map8(show_storage[c][0], 64, 192);
  uint8_t i, angle_delta, intensity;
  
  shows[c].fillForeBlack();
  
  for (uint8_t y = 0; y < SIZE; y++) {
    for (uint8_t x = 0; x < SIZE; x++) {
      i = led[c].getLedFromCoord(x, y);
      
      if (i != XX) {
        
        angle_delta = abs((shows[c].getSmallCycle() % 256) - get_angle(x, y, 10, 5));
        if (angle_delta < thickness) {
          intensity = map(angle_delta, 0, thickness, 255, 0);
          led[c].addPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intensity));
        }
        
        angle_delta = abs((shows[c].getSmallCycle() % 256) - get_angle(x, y, 0, 5));
        if (angle_delta < thickness) {
          intensity = map(angle_delta, 0, thickness, 255, 0);
          led[c].addPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intensity));
        }
      }
    }
  }
}

//
// orbits
//
void orbits(uint8_t c) {
  uint8_t thickness = map8(show_storage[c][0], 48, 192);
  uint8_t i, angle_delta, distance_delta, intensity, intensity2;
  
  shows[c].fillForeBlack();
  
  for (uint8_t y = 0; y < SIZE; y++) {
    for (uint8_t x = 0; x < SIZE; x++) {
      i = led[c].getLedFromCoord(x, y);
      
      if (i != XX) {

        // First orbit
        angle_delta = abs(shows[c].getSmallCycle() % 256 - get_angle_to_origin(x, y));
        if (angle_delta < thickness) {
          intensity = map(angle_delta, 0, thickness, 255, 0);
          
          distance_delta = abs(get_distance_to_origin(x, y) - 24);
          if (distance_delta < thickness) {
            intensity2 = map(distance_delta, 0, thickness, intensity, 0);
            led[c].addPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intensity2));
          }
        }
        
        // Second orbit
        angle_delta = abs(((255 - shows[c].getSmallCycle()) % 256) - get_angle_to_origin(x, y));
        if (angle_delta < thickness) {
          intensity = map(angle_delta, 0, thickness, 255, 0);
          
          distance_delta = abs(get_distance_to_origin(x, y) - 96);
          if (distance_delta < thickness) {
            intensity2 = map(distance_delta, 0, thickness, intensity, 0);
            led[c].addPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intensity2));
          }
        }
      }
    }
  }
}

//
//  get angle - figure out the angle of (y/x) to the center
//
uint8_t get_angle_to_origin(uint8_t x, uint8_t y) {
  return get_angle(x, y, 5, 5);
}

uint8_t get_angle(uint8_t x2, uint8_t y2, uint8_t x1, uint8_t y1) {
  int8_t dx = (2 * (x2 - x1)) + (y2 - y1);
  float dy = -2.309 * (y2 - y1);  // 4 / sqrt(3)  minus for inverted y-coordinates
  
  return get_angle_square_coord(dx, dy);
}

uint8_t get_angle_square_coord(int8_t dx, float dy) {
  if (dy == 0) {
    return (dx >= 0) ? 0 : 128 ;  // Either pointing right (3 o'clock) or left (9 o'clock)
  }
  if (dx == 0) {
    return (dy >= 0) ? 64 : 192 ;  // Either pointing up or down
  }
  
  uint8_t angle = uint8_t(atan2(abs(dy), abs(dx)) * 40.743);  // 128 / pi
  
  if (dy > 0) {
    angle = (dx > 0) ? angle : 128 - angle ;
  } else {
    angle = (dx > 0) ? 256 - angle : angle + 128 ;
  }
  return angle;
}

//// Start Game of Life

void game_of_life_one_color(uint8_t c) {
  game_of_life(c, false);
}

void game_of_life_two_color(uint8_t c) {
  game_of_life(c, true);
}


//
// game_of_life - Conway's Game of Life for Hexes
//
void game_of_life(uint8_t c, boolean two_color) {
  if (shows[c].isShowStart()) {
    seed_game_of_life_map(c);
  }
  
  shows[c].setWait(40);
  if (shows[c].getMorph() == 0) {
    calculate_life(c);
    send_game_of_life_to_leds(c, two_color);
    push_game_of_life_frame(c);
  }
}

void calculate_life(uint8_t c) {
  boolean fate;
  
  for (uint8_t x = 0; x < LIFE_MAP_SIZE; x++) {
    for (uint8_t y = 0; y < LIFE_MAP_SIZE; y++) {
      float life_quantity = get_life_quantity(c, x, y);
      
      if (get_game_of_life_cell(c, x, y)) {
        fate = (life_quantity >= 2.0 && life_quantity <= 3.3) ? true : false;
      } else {
        fate = (life_quantity >= 2.3 && life_quantity <= 2.9) ? true : false;
      }
      set_game_of_next_life_cell(c, x, y, fate);
    }
  }
}

float get_life_quantity(uint8_t c, int8_t x, int8_t y) {
  return num_alive_neighbors(c, x, y) + (num_alive_next_neighbors(c, x, y) * 0.3);
}

uint8_t num_alive_neighbors(uint8_t c, int8_t x, int8_t y) {
  int8_t life_neighbors[] = { 1, 0, 1, -1, 0, -1, -1, 0, -1, 1, 0, 1 };
  float life = 0;
  int8_t dx;
  int8_t dy;
  
  for (uint8_t dir = 0; dir < 6; dir++) {
    dx = life_neighbors[dir * 2];
    dy = life_neighbors[(dir * 2) + 1];
    
    if (out_of_bounds(x + dx, y + dy)) {
      life += 0.5;
    } else {
      if (get_game_of_life_cell(c, x + dx, y + dy)) {
        life += 1.0;
      }
    }
  }
  return life;
}

uint8_t num_alive_next_neighbors(uint8_t c, int8_t x, int8_t y) {
  float life = 0;
  int8_t dx;
  int8_t dy;
  int8_t life_next_neighbors[] = { 1, -2, 2, -1, 1, 1, -1, 2, -2, 1, -1, -1 };
  
  for (uint8_t dir = 0; dir < 6; dir++) {
    dx = life_next_neighbors[dir * 2];
    dy = life_next_neighbors[(dir * 2) + 1];
    
    if (out_of_bounds(x + dx, y + dy)) {
      life += 0.5;
    } else {
      if (get_game_of_life_cell(c, x + dx, y + dy)) {
        life += 1.0;
      }
    }
  }
  return life;
}

void seed_game_of_life_map(uint8_t c) {
  // 20% chance of a life in a starting cell
  for (uint8_t x = 0; x < LIFE_MAP_SIZE; x++) {
    for (uint8_t y = 0; y < LIFE_MAP_SIZE; y++) {
      boolean outcome = (random8(0, 10) < 2) ? true : false;
      set_game_of_curr_life_cell(c, x, y, outcome);
      set_game_of_next_life_cell(c, x, y, false);
    }
  }
}

void push_game_of_life_frame(uint8_t c) {
  for (uint8_t i = 0; i < LIFE_MAP_SIZE * LIFE_MAP_SIZE; i++) {
    game_of_life_curr_map[i + (c * LIFE_NUM_CELLS)] = game_of_life_next_map[i + (c * LIFE_NUM_CELLS)];
  }
}

void send_game_of_life_to_leds(uint8_t c, boolean two_color) {
  uint8_t foreColor = shows[c].getForeColor();
  uint8_t backColor = shows[c].getBackColor();
  uint8_t off_set = (LIFE_MAP_SIZE - SIZE) / 2;
  boolean curr_life, next_life;
  
  for (uint8_t x = 0; x < LIFE_MAP_SIZE; x++) {
    for (uint8_t y = 0; y < LIFE_MAP_SIZE; y++) {
      uint8_t pixel = led[c].getLedFromCoord(x, y);
      if (pixel != XX) {
        curr_life = get_game_of_life_cell(c, x + off_set, y + off_set);
        next_life = get_game_of_life_next_cell(c, x + off_set, y + off_set);
        shows[c].setPixeltoColor(pixel, get_game_of_life_color(curr_life, next_life, two_color, foreColor, backColor));
      }
    }
  }
}

CHSV get_game_of_life_color(boolean curr_life, boolean next_life, boolean two_color, uint8_t foreColor, uint8_t backColor) {
  // One and two color versions depending on two_color boolean
  CHSV color;
  
  if (two_color) {
    if (curr_life) {
      color = (next_life) ? led[CHANNEL_A].wheel(foreColor) : led[CHANNEL_A].gradient_wheel(backColor, 128) ;
    } else {
      color = (next_life) ? led[CHANNEL_A].gradient_wheel(foreColor, 128) : led[CHANNEL_A].wheel(backColor) ;
    }
  
  } else {
    color = (curr_life) ? led[CHANNEL_A].wheel(foreColor) : BLACK ;  // one color version is either foreColor or Black
  }
  
  return color;
}

void set_game_of_curr_life_cell(uint8_t c, int8_t x, int8_t y, boolean value) {
  if (out_of_bounds(x, y) == false) {
    game_of_life_curr_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)] = value;
  }
}

void set_game_of_next_life_cell(uint8_t c, int8_t x, int8_t y, boolean value) {
  if (out_of_bounds(x, y) == false) {
    game_of_life_next_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)] = value;
  }
}

boolean get_game_of_life_cell(uint8_t c, int8_t x, int8_t y) {
  // if coordinates are out of bounds, default to false;
  if (out_of_bounds(x, y)) {
    return false;
  }
  return game_of_life_curr_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)];
}

boolean get_game_of_life_next_cell(uint8_t c, int8_t x, int8_t y) {
  // if coordinates are out of bounds, default to false;
  if (out_of_bounds(x, y)) {
    return false;
  }
  return game_of_life_next_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)];
}

boolean out_of_bounds(int8_t x, int8_t y) {
  // Is (x,y) on the game of life board between (0 ,0) and (LIFE_MAP_SIZE, LIFE_MAP_SIZE)?
  return (x < 0 || x >= LIFE_MAP_SIZE || y < 0 || y >= LIFE_MAP_SIZE);
}

//// End Game of Life

//
// vert back forth dots - vertical dots moving back and forth
//
void vert_back_forth_dots(uint8_t c) {
  uint8_t temp_x;
  uint16_t cycle = shows[c].getCycle();

  shows[c].fillForeBlack();
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {
      temp_x = x + HEX_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      if ((temp_x + cycle) % MAX_XGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
}

//
// vert back forth bands - vertical bands moving back and forth
//
void vert_back_forth_bands(uint8_t c) {
  uint8_t temp_x, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {
      temp_x = x + HEX_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_x = (temp_x + cycle) % MAX_XGRID;
      intensity = sin8(map(temp_x, 0, MAX_XGRID, 0, 255) + (cycle % (255 / MAX_XGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
}

//
// vert back forth colors - vertical colors moving back and forth
//
void vert_back_forth_colors(uint8_t c) {
  uint8_t temp_x, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {
      temp_x = x + HEX_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_x = (temp_x + cycle) % MAX_XGRID;
      hue = sin8(map(temp_x, 0, MAX_XGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_XGRID)) );
      led[c].setPixelHue(led[c].getLedFromCoord(x,y), hue);
    }
  }
}

//
// diag back forth dots - diagonal dots moving back and forth
//
void diag_back_forth_dots(uint8_t c) {
  uint8_t temp_x, temp_y;
  uint16_t cycle = shows[c].getCycle() / 3;  // denominator helps slow

  shows[c].fillForeBlack();
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {
      temp_x = x + HEX_XGRID;
      temp_y = y + HEX_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      if ((temp_x + temp_y + cycle) % (MAX_YGRID / show_var[c]) == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
}

//
// diag back forth bands - diagonal bands moving back and forth
//
void diag_back_forth_bands(uint8_t c) {
  uint8_t temp_x, temp_y, temp, intensity;
  uint16_t cycle = shows[c].getCycle() / 3;  // denominator helps slow
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {  // Don't light the head (row 0)
      temp_x = x + HEX_XGRID;
      temp_y = y + HEX_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp = (temp_x + temp_y + cycle) % (MAX_YGRID / show_var[c]);
      intensity = sin8(map(temp, 0, MAX_YGRID, 0, 255) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
}

//
// diag back forth colors - diagonal colors moving back and forth
//
void diag_back_forth_colors(uint8_t c) {
  uint8_t temp_x, temp_y, temp, hue;
  uint16_t cycle = shows[c].getCycle() / 3;  // denominator helps slow
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE; y++) {
      temp_x = x + HEX_XGRID;
      temp_y = y + HEX_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp = (temp_x + temp_y + cycle) % (MAX_YGRID / show_var[c]);
      hue = sin8(map(temp, 0, MAX_YGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelHue(led[c].getLedFromCoord(x,y), hue);
    }
  }
}

//
// rings
//
void center_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 5, 5, map8(show_speed, 6, 12), 64, 128);
}

void well(uint8_t c) {
  ring(c, shows[c].getForeColor(), led[c].wheel(shows[c].getBackColor()), 5, 5, map8(show_speed, 2, 7), 192, 192);
}

void corner_ring(uint8_t c) {
  uint8_t corner_coords[] = { 5,0, 10,0, 10,5, 5,10, 0,10, 0,5 };
  uint8_t corner = show_var[c] % 6;
  uint8_t x = corner_coords[corner * 2];
  uint8_t y = corner_coords[(corner * 2) + 1];
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), x, y, 3, 64, 255);
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
      uint8_t pixel = led[c].getLedFromCoord(x, y);
      if (pixel != XX) {
        uint8_t delta = abs(get_distance(x, y, center_x, center_y) - value) % ring_freq;
        if (delta < cutoff) {
          uint8_t intensity = map(delta, 0, cutoff, 255, 0);
          shows[c].setPixeltoColor(pixel, led[c].getInterpHSV(background, foreColor, intensity));
        }
      }
    }
  }
}


////// Speaking and Hearing

String getReadings (uint8_t c) {
  JSONVar jsonReadings;

  jsonReadings["type"] = MESH_MESSAGE;
  jsonReadings["c"] = c;
  jsonReadings["morph"] = shows[c].isMorphing();
  jsonReadings["cycle"] = (const double)shows[c].getCycle();
  jsonReadings["smcycle"] = (const double)shows[c].getSmallCycle();
  jsonReadings["f"] = shows[c].getForeColor();
  jsonReadings["fspd"] = shows[c].getForeColorSpeed();
  jsonReadings["b"] = shows[c].getBackColor();
  jsonReadings["bspd"] = shows[c].getBackColorSpeed();
  jsonReadings["s"] = current_show[c];
  jsonReadings["w"] = shows[c].getWait();

  // CHANGE THIS
//  jsonReadings["p"] = show_variables[c];
//  jsonReadings["m"] = show_variables[c + 6];
//  jsonReadings["fill"] = show_variables[c + 8];
//  jsonReadings["wp"] = show_variables[c + 10];

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
  shows[c].setSmallCycle(double(myObject["smcycle"]));
  shows[c].setForeColor(int(myObject["f"]));
  shows[c].setForeColorSpeed(int(myObject["fspd"]));
  shows[c].setBackColor(int(myObject["b"]));
  shows[c].setBackColorSpeed(int(myObject["bspd"]));
  shows[c].setWait(int(myObject["w"]));

  //// CHANGE THIS
//  show_variables[c] = int(myObject["p"]);
//  show_variables[c + 6] = int(myObject["m"]);
//  show_variables[c + 8] = int(myObject["fill"]);
//  show_variables[c + 10] = int(myObject["wp"]);

  if (boolean(myObject["morph"])) {
    shows[c].turnOnMorphing();
  } else {
    shows[c].turnOffMorphing();
  }
  int show_number = int(myObject["s"]);
  if (current_show[c] != show_number) {
    current_show[c] = show_number % NUM_SHOWS;
    next_show(c);
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
    shows[i].setWaitRange(show_speed);
    shows[i].setColorSpeedMinMax(show_speed);
  }
}

void updateShowDuration(uint8_t duration) {
  
  uint32_t new_max_small_cycle = (duration * 2 * (1000 / DELAY_TIME));  // update max_small_cycle
  float cycle_ratio = float(new_max_small_cycle) / max_small_cycle;

  // Need to change smallCycle so it fits within the new duration
  uint32_t newSmallCycle = uint32_t(shows[CHANNEL_A].getSmallCycle() * cycle_ratio) % new_max_small_cycle;
  shows[CHANNEL_A].setSmallCycle(newSmallCycle);
  shows[CHANNEL_B].setSmallCycle((newSmallCycle + (new_max_small_cycle / 2)) % new_max_small_cycle);
  
  show_duration = duration;
  max_small_cycle = new_max_small_cycle;
  
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

////// End Speaking and Hearing


///////// DUAL SHOW LOGIC

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  uint8_t led_number;

  mirror_pixels(CHANNEL_A);
  mirror_pixels(CHANNEL_B);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_number = convert_pixel_to_led(i);
    if (led_number != XX) {
      CHSV color_b = mask(led[CHANNEL_B].getInterpFrameColor(rotate_pixel(i, rotate_amount[CHANNEL_B])), i, CHANNEL_B);
      CHSV color_a = mask(led[CHANNEL_A].getInterpFrameColor(rotate_pixel(i, rotate_amount[CHANNEL_A])), i, CHANNEL_A);
      CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
      
      color = narrow_palette(color);
      
      if (SMOOTHING > 0) {
        color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
      }
  
      leds[led_number] = color;
      led_buffer[i] = color;
    }
  }
  turn_off_spacer_leds();
}

//
// mask
//
CHSV mask(CHSV color, uint8_t i, uint8_t channel) {
  if (pattern[channel + 2] == 0 || current_show[i] == 0) {
    return color;  // no masking
  }
  uint8_t mask_value = pattern[channel + 2] - 1;
  boolean value = get_bit_from_pattern_number(i, pattern[channel]);
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
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();
  uint16_t fade_cycles =  (map8(fade_amount, 0, show_duration) * 1000 / DELAY_TIME);  // cycles to fade in + out

  if (small_cycle < fade_cycles) {
    intensity = map(small_cycle, 0, fade_cycles, 0, 255);  // rise
  } else if (small_cycle <= (max_small_cycle / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((max_small_cycle / 2) + fade_cycles)) {
    intensity = map(small_cycle - (max_small_cycle / 2), 0, fade_cycles, 255, 0);  // decay
  } else {
    intensity = 0;
  }

  return ease8InOutApprox(intensity);
}

//// End DUAL SHOW LOGIC

//
// narrow_palette - confine the color range
//
CHSV narrow_palette(CHSV color) {
  uint8_t h1 = (hue_center - (hue_width / 2)) % MAX_COLOR;
  uint8_t h2 = (hue_center + (hue_width / 2)) % MAX_COLOR;
  color.h = map8(color.h, h1, h2 );
  if (color.s != 0) {
    color.s = saturation;  // Reset saturation
  }
  return color;
}

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t get_distance(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  // distance = sqrt(3 * (x^2 * xy * y^2))  - remove the 3
  // answer should span 0-250
  int16_t dx = (x2 - x1) * 25;
  int16_t dy = (y2 - y1) * 25;
  return sqrt16((dx * dx) + (dx * dy) + (dy * dy));
}

uint8_t get_distance_to_origin(uint8_t x, uint8_t y) {
  return get_distance(x, y, 5, 5);
}

//
// convert pixel to led: account for the spacer pixels
//
uint8_t convert_pixel_to_led(uint8_t i) {
  if (i == XX) {
    return XX;
  }
  uint8_t LED_LOOKUP[] = {
             55, 59, 60, 94, 93, 98,
           54, 53, 61, 62, 92, 91, 99,
         16, 52, 51, 63, 64, 90, 89, 101,
       17, 18, 50, 49, 65, 66, 88, 87, 102,
     12, 19, 20, 48, 47, 67, 68, 86, 85, 104,
   11, 10, 21, 22, 46, 45, 69, 70, 84, 83, 105,
      9,  8, 23, 24, 44, 43, 71, 72, 82, 81,
        7,  6, 25, 26, 42, 41, 73, 74, 80,
          5,  4, 27, 28, 40, 39, 75, 76,
            3,  2, 29, 30, 38, 37, 77,
              1,  0, 31, 32, 36, 35
  }; 
  return LED_LOOKUP[i % NUM_LEDS];
}

//
// mirror_pixels
//
void mirror_pixels(uint8_t channel) {
  // 21 pixels
  uint8_t one_wedge[] = {
    0, 7, 15, 24, 34, 45, 6, 14, 23, 33, 44, 13, 22, 32, 43, 21, 31, 42, 30, 41, 40
  };
  // 36 pixels
  uint8_t two_wedges[] = {
    0, 7, 15, 24, 34, 45, 6, 14, 23, 33, 44, 13, 22, 32, 43, 21, 31, 42, 30, 41, 40,
    1, 8, 16, 25, 35, 2, 9, 17, 26, 3, 10, 18, 4, 11, 5
  };
  // 51 pixels
  uint8_t three_wedges[] = {
    0, 7, 15, 24, 34, 45, 6, 14, 23, 33, 44, 13, 22, 32, 43, 21, 31, 42, 30, 41, 40,
    1, 8, 16, 25, 35, 2, 9, 17, 26, 3, 10, 18, 4, 11, 5,
    12, 19, 27, 36, 46, 20, 28, 37, 47, 29, 38, 48, 39, 49, 50
  };

  if (symmetry[channel] == 6) {
    for (uint8_t i = 0; i < 21; i++) {
      mirror_pixel(channel, one_wedge[i], 1);
      mirror_pixel(channel, one_wedge[i], 2);
      mirror_pixel(channel, one_wedge[i], 3);
      mirror_pixel(channel, one_wedge[i], 4);
      mirror_pixel(channel, one_wedge[i], 5);
    }
  } else if (symmetry[channel] == 2) {
    for (uint8_t i = 0; i < 51; i++) {
      mirror_pixel(channel, three_wedges[i], 3);
    }
  } else if (symmetry[channel] == 3) {
    for (uint8_t i = 0; i < 36; i++) {
      mirror_pixel(channel, two_wedges[i], 2);
      mirror_pixel(channel, two_wedges[i], 4);
    }
  }
}

//
// mirror_pixel
//
void mirror_pixel(uint8_t channel, uint8_t i, uint8_t rotations) {
  led[channel].setInterpFrame(rotate_pixel(i, rotations), led[channel].getInterpFrameColor(i));
}

//
// rotate_pixel
//
uint8_t rotate_pixel(uint8_t i, uint8_t rotations) {
  uint8_t rotate_right[] = {
                5, 12, 20, 29, 39, 50, 
              4, 11, 19, 28, 38, 49, 60,
            3, 10, 18, 27, 37, 48, 59, 69, 
          2,  9, 17, 26, 36, 47, 58, 68, 77,
        1,  8, 16, 25, 35, 46, 57, 67, 76, 84,
      0,  7, 15, 24, 34, 45, 56, 66, 75, 83, 90,
        6, 14, 23, 33, 44, 55, 65, 74, 82, 89,
         13, 22, 32, 43, 54, 64, 73, 81, 88,
           21, 31, 42, 53, 63, 72, 80, 87,
             30, 41, 52, 62, 71, 79, 86,
                40, 51, 61, 70, 78, 85
  };
  uint8_t new_pixel = i;
  for (uint8_t r = 0; r < rotations; r++) {
    new_pixel = rotate_right[new_pixel];
  }
  return new_pixel;
}

//
// turn off spacer leds - blacken the 16 space pixels
//
void turn_off_spacer_leds() {
  uint8_t spacer_leds[] = { 13, 14, 15, 33, 34, 56, 57, 58, 78, 79, 95, 96, 97, 100, 103 };
  for (uint8_t i = 0; i < NUMBER_SPACER_LEDS; i++) {
    leds[spacer_leds[i]] = CRGB(0, 0, 0);
  }
}

//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t PatternMatrix[] = {
    0x1, 0x11, 0x20, 0xa0, 0x30, 0x7b, 0xc1, 0x80, 0xa0, 0x91, 0x10, 0x0,
    0x30, 0xe6, 0xdf, 0x1d, 0x86, 0x0, 0xc, 0x37, 0x1f, 0x6c, 0xe1, 0x80,
    0x30, 0x42, 0x13, 0x19, 0xce, 0x7b, 0xce, 0x73, 0x19, 0x8, 0x41, 0x80,
    0x83, 0x7, 0x6, 0x83, 0x20, 0xff, 0xe0, 0x98, 0x2c, 0x1c, 0x18, 0x20,
    0xbf, 0x15, 0x2c, 0xa4, 0x31, 0xfb, 0xf1, 0x84, 0xa6, 0x95, 0x1f, 0xa0,
    0x1, 0xf2, 0x12, 0xe9, 0x4a, 0x55, 0x4a, 0x52, 0xe9, 0x9, 0xf0, 0x0,
    0xfd, 0xf1, 0xe0, 0xe0, 0x30, 0xff, 0xff, 0xff, 0xbf, 0x9f, 0x1c, 0x20,
    0x0, 0x0, 0x4, 0x47, 0x33, 0xaa, 0xa6, 0x61, 0x10, 0x0, 0x0, 0x0,
    0x0, 0x40, 0xc0, 0xa0, 0x48, 0x91, 0x34, 0x2e, 0x4e, 0x64, 0xa2, 0x40,
    0x30, 0xc0, 0xc0, 0xc0, 0x32, 0xc, 0xc1, 0x90, 0xc0, 0x60, 0xc1, 0x80,
    0x3, 0x7, 0x1b, 0x1c, 0x6, 0x0, 0x3, 0x1, 0xc0, 0xcc, 0x18, 0x20,
    0x0, 0x39, 0x49, 0x45, 0x52, 0x2b, 0x85, 0x1, 0x78, 0x80, 0xf0, 0x0,
    0x2, 0xc, 0xcc, 0xa6, 0x49, 0xff, 0xf2, 0x4c, 0xa6, 0x66, 0x8, 0x0,
    0x3, 0x7, 0x3, 0x4, 0x3, 0x4, 0xe1, 0xba, 0xff, 0xff, 0xff, 0xe0,
    0x0, 0x40, 0xc0, 0xa0, 0x48, 0x11, 0x4, 0x22, 0xb, 0xfc, 0x0, 0x0,
    0x1, 0x82, 0x80, 0x40, 0x14, 0x33, 0xa, 0x0, 0x80, 0x50, 0x60, 0x0,
    0x30, 0xe0, 0xc0, 0x42, 0x1, 0xe0, 0xf0, 0x8, 0x40, 0x60, 0xe1, 0x80,
    0x0, 0x40, 0xc0, 0xe0, 0x30, 0xe, 0x3, 0xc1, 0xf0, 0x60, 0xe3, 0xc0
  };
  
  uint8_t pattern_byte = PatternMatrix[(pattern_number * 12) + (n / 8)];
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
