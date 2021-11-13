#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  QBert Large: 111 + 6 = 117 LEDs
//
//  Feather Huzzah
//
//  Adjustable Brightness
//
//  10/16/21
//
//  Listens to the Network and a Phone
//

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 32;  // (0-255)

// 10000 @ 6V is on the edge of flickering
#define MAX_LUMENS 5000  // estimated maximum brightness until flickering. 2A should allow 25500.

uint8_t DELAY_TIME = 20;  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 0  // 13  // 0  // 11
#define CLOCK_PIN 2  // 14  // 2  // 13

#define NUM_LEDS 111
#define NUM_SPACER_LEDS 6
#define TOTAL_LEDS  (NUM_LEDS + NUM_SPACER_LEDS)

#define SIZE 7
#define DIAMOND_NUMBER 3

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define XX  255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[TOTAL_LEDS];  // The Leds themselves
CHSV led_buffer[NUM_LEDS];  // For smoothing

#define MAX_COLOR 256   // Colors are 0-255
#define CAN_CHANGE_PALETTES false
#define ONLY_RED false
#define PALETTE_MINUTES 5   // How long a palette lasts for
uint8_t palette_number = 0;
uint8_t hue_start = 0;
uint8_t hue_width = 255;

// Shows
#define NUM_SHOWS 20
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type

// wait times
uint8_t show_duration = 40;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 148;  // 0 = no fading, to 255 = always be fading
uint32_t max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // *2! 50% = all on, 50% = all off, and rise + decay on edges
uint8_t freq_storage[] = { 60, 80 };  // 1-byte storage for shows

uint8_t LIFE_BOARD[NUM_LEDS];  // Game of Life - This may be too much memory

#define NUM_PATTERNS 11   // Total number of patterns, each 4 bytes wide
#define NUM_COMPLEX_PATTERNS 4  // Total number of complex patterns

//// Mesh parameters

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

#define ARE_CONNECTED true // Are the pentagons talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = 0;
#define MAX_SILENT_TIME  10000  // Time (in cycles) without communication before marked as is_lost

String message;  // String to send to other displays

#define MSG_FREQUENCY  50  // send message every X milliseconds
#define MESSAGE_REPEAT 3   // send this many duplicate messages
#define MESSAGE_SPACING 3   // wait this many cycles
Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

#define PHONE_MESSAGE  0
#define MESH_MESSAGE  1

#define BRIGHTNESS_COMMAND  0  // all of these will be 0-255
#define HUE_COMMAND  1
#define HUE_WIDTH_COMMAND  2
#define SPEED_COMMAND  3
#define SHOW_DURATION_COMMAND  4
#define FADE_COMMAND 5

// User stub
//void sendMessage();
void updateLeds();
void randomize_palette();
String getReadings();  // Prototype for reading state of LEDs

//Task taskSendMessage(MSG_FREQUENCY, TASK_FOREVER, &sendMessage);
Task taskUpdateLeds(DELAY_TIME, TASK_FOREVER, &updateLeds);
Task taskRandomizePalette(1000 * 60 * PALETTE_MINUTES, TASK_FOREVER, &randomize_palette);  // minutes

//// End Mesh parameters

uint8_t neighbors[] PROGMEM = {
2, 1, XX, 13,
XX, XX, 0, 2,
3, 1, 0, 16,
5, 4, 2, 16,
XX, XX, 3, 5,
6, 4, 3, 19,
8, 7, 5, 19,
XX, XX, 6, 8,
9, 7, 6, 22,
11, 10, 8, 22,
XX, XX, 9, 11,
XX, 10, 9, 25,
14, 13, XX, 28,
0, XX, 12, 14,
15, 13, 12, 31,
17, 16, 14, 31,
3, 2, 15, 17,
18, 16, 15, 34,
20, 19, 17, 34,
6, 5, 18, 20,
21, 19, 18, 37,
23, 22, 20, 37,
9, 8, 21, 23,
24, 22, 21, 40,
26, 25, 23, 40,
XX, 11, 24, 26,
XX, 25, 24, 43,
29, 28, XX, 46,
12, XX, 27, 29,
30, 28, 27, 49,
32, 31, 29, 49,
15, 14, 30, 32,
33, 31, 30, 52,
35, 34, 32, 52,
18, 17, 33, 35,
36, 34, 33, 55,
38, 37, 35, 55,
21, 20, 36, 38,
39, 37, 36, 58,
41, 40, 38, 58,
24, 23, 39, 41,
42, 40, 39, 61,
44, 43, 41, 61,
XX, 26, 42, 44,
XX, 43, 42, 64,
47, 46, XX, XX,
27, XX, 45, 47,
48, 46, 45, 67,
50, 49, 47, 67,
30, 29, 48, 50,
51, 49, 48, 70,
53, 52, 50, 70,
33, 32, 51, 53,
54, 52, 51, 73,
56, 55, 53, 73,
36, 35, 54, 56,
57, 55, 54, 76,
59, 58, 56, 76,
39, 38, 57, 59,
60, 58, 57, 79,
62, 61, 59, 79,
42, 41, 60, 62,
63, 61, 60, 82,
65, 64, 62, 82,
XX, 44, 63, 65,
XX, 64, 63, XX,
68, 67, XX, XX,
48, 47, 66, 68,
69, 67, 66, 85,
71, 70, 68, 85,
51, 50, 69, 71,
72, 70, 69, 88,
74, 73, 71, 88,
54, 53, 72, 74,
75, 73, 72, 91,
77, 76, 74, 91,
57, 56, 75, 77,
78, 76, 75, 94,
80, 79, 77, 94,
60, 59, 78, 80,
81, 79, 78, 97,
83, 82, 80, 97,
63, 62, 81, 83,
XX, 82, 81, XX,
86, 85, XX, XX,
69, 68, 84, 86,
87, 85, 84, 100,
89, 88, 86, 100,
72, 71, 87, 89,
90, 88, 87, 103,
92, 91, 89, 103,
75, 74, 90, 92,
93, 91, 90, 106,
95, 94, 92, 106,
78, 77, 93, 95,
96, 94, 93, 109,
98, 97, 95, 109,
81, 80, 96, 98,
XX, 97, 96, XX,
101, 100, XX, XX,
87, 86, 99, 101,
102, 100, 99, XX,
104, 103, 101, XX,
90, 89, 102, 104,
105, 103, 102, XX,
107, 106, 104, XX,
93, 92, 105, 107,
108, 106, 105, XX,
110, 109, 107, XX,
96, 95, 108, 110,
XX, 109, 108, XX
};

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  randomSeed(analogRead(0));
  
//  Serial.begin(9600);
  Serial.begin(115200);
  Serial.println(F("Start"));

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, TOTAL_LEDS);
  FastLED.setBrightness( bright );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 4 neighbors for every pixel
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    
    shows[i].setColorSpeedMinMax(show_speed);
    shows[i].setWaitRange(show_speed);

    shows[i].setAsSquare();  // Each pixel has 4 neighbors
  }
  
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }
  
  if (ONLY_RED) {  // (ArduinoBlue)
    palette_number = 1;  // red palette
  }

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskUpdateLeds);
  userScheduler.addTask(taskRandomizePalette);
  taskUpdateLeds.enable();
  taskRandomizePalette.enable();
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
        shows[i].morphChain();
        break;
      case 2:
        shows[i].bounce();
        break;
      case 3:
        shows[i].bounceGlowing();
        break;
      case 4:
        shows[i].plinko(66);
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
        shows[i].randomFill();
        break;
      case 9:
        shows[i].juggle_fastled();
        break;
      case 10:
        shows[i].randomTwoColorBlack();
        break;
      case 11:
        shows[i].randomOneColorBlack();
        break;
      case 12:
        shows[i].sawTooth();
        break;
      case 13:
        center_ring(i);
        break;
      case 14:
        corner_ring(i);
        break;
      case 15:
        qbert(i);
        break;
      case 16:
        complex_patterns(i, true);
        break;
      case 17:
        complex_patterns(i, false);
        break;
      case 18:
        shows[i].confetti();
        break;
      default:
        shows[i].bands();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  adj_brightess();  // Limit brightness to conserve power
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
      if (!ARE_CONNECTED || (ARE_CONNECTED && IS_SPEAKING)) {
        pick_next_show(i);
      }
    }
    if (ARE_CONNECTED && IS_SPEAKING) {
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
//  shows[i].fillForeBlack();
  led[i].push_frame();  // This might not work well with speaking & hearing
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(show_speed);
  shows[i].setColorSpeedMinMax(show_speed);
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  uint8_t mask = 0;
  uint8_t symmetry = 0;
  
  current_show[i] = is_other_channel_show_zero(i) ? random8(1, NUM_SHOWS) : 0 ;
  show_variables[i] = random8(NUM_PATTERNS);
  mask = pick_random_mask(i);
  
  if (mask == 0) {
    symmetry = pick_random_symmetry();
  }
  show_variables[i + 6] = mask;
  show_variables[i + 4] = symmetry;
  show_variables[i + 2] = random(3);  // rotation (0-1)
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickRandomColorSpeeds();
//  shows[i].tweakColorSpeeds();
  shows[i].pickRandomWait();
  log_status(i);  // For debugging
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

uint8_t pick_random_symmetry() {
  uint8_t random_symmetry = random(12);
  
  if (random_symmetry <= 6) {
    return random_symmetry / 2;  // 2-fold (1), 3-fold (2), 6-fold (3), None
  }
  return 0;  // No symmetrizing
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

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    boolean value = get_bit_from_pattern_number(i, pattern);
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
        uint8_t intense = get_wipe_intensity(get_xcoord(i), get_ycoord(i), show_variables[c + 10], (pattern_type < 4));
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

//
// complex patterns shows
//
void complex_patterns(uint8_t c, boolean smooth) {
  // Reset a lot of variables at the start of the show
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();  // Because we are using a beatsin8
    show_variables[c] = random(NUM_COMPLEX_PATTERNS);  // Pick a pattern
    show_variables[c + 8] = random(3);   // 0 = gradient/black, 1 = color/black, 2 = color/color
  }
  uint8_t pattern = show_variables[c];
  uint8_t pattern_max = get_complex_max(pattern);
  uint8_t pattern_fill = show_variables[c + 8];
  uint8_t foreColor = shows[c].getForeColor();
  uint8_t cycle = shows[c].getCycle() % 255;
  
  if (pattern_fill == 2) {
    shows[c].fillBackColor();
  } else {
    shows[c].fillForeBlack();
  }
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t value = lookup_complex_pattern_table(i, pattern);
    if (value > 0) {
      uint8_t intense = sin8_C((cycle + map(value, 1, pattern_max + 1, 0, 255)) % 255);
      if (intense > 200 || smooth) {
        if (pattern_fill > 0) {
          shows[c].setPixeltoHue(i, shows[c].IncColor(foreColor, map8(intense, 0, 64)));
        } else {
          shows[c].setPixeltoColor(i, led[c].gradient_wheel(foreColor, map8(intense, 96, 255)));
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
  shows[channel].setForeColor(0);
  shows[channel].setBackColor(120);
  uint8_t i = shows[channel].getCycle() % NUM_LEDS;
  led[channel].setPixelHue(i, shows[channel].getForeColor());
  
  for (uint8_t j = 0; j < 4; j++) {
    uint8_t n = pgm_read_byte_near(neighbors + (i * 4) + j);
    if (n != XX) {
      led[channel].setPixelHue(n, shows[channel].getBackColor());
    }
  }
}

boolean inbounds(uint8_t x, uint8_t y) {
  return (y >= 0) && (y < SIZE) && (x >= min(0, 3 - y)) && (x <= max(6, 9 - y));
}

//
//  Cartessian Coordinates
//
uint8_t get_xcoord(uint8_t i) {
  const uint8_t XCOORDS[] = {
    10, 0, 20, 29, 20, 39, 49, 39, 59, 69, 59, 79, 29, 20, 39, 49, 39, 59, 
    69, 59, 79, 88, 79, 98, 108, 98, 118, 49, 39, 59, 69, 59, 79, 88, 79, 
    98, 108, 98, 118, 128, 118, 138, 147, 138, 157, 69, 59, 79, 88, 79, 98, 
    108, 98, 118, 128, 118, 138, 147, 138, 157, 167, 157, 177, 187, 177, 196, 
    108, 98, 118, 128, 118, 138, 147, 138, 157, 167, 157, 177, 187, 177, 196, 
    206, 196, 216, 147, 138, 157, 167, 157, 177, 187, 177, 196, 206, 196, 216, 
    226, 216, 236, 187, 177, 196, 206, 196, 216, 226, 216, 236, 246, 236, 255
  };
  return XCOORDS[i];
}

uint8_t get_ycoord(uint8_t i) {
  const uint8_t YCOORDS[] = {
    119, 136, 136, 158, 175, 175, 197, 214, 214, 236, 253, 253, 79, 96, 96, 
    119, 136, 136, 158, 175, 175, 197, 214, 214, 236, 253, 253, 40, 57, 57, 79, 
    96, 96, 119, 136, 136, 158, 175, 175, 197, 214, 214, 236, 253, 253, 1, 18, 
    18, 40, 57, 57, 79, 96, 96, 119, 136, 136, 158, 175, 175, 197, 214, 214, 
    236, 253, 253, 1, 18, 18, 40, 57, 57, 79, 96, 96, 119, 136, 136, 158, 175, 
    175, 197, 214, 214, 1, 18, 18, 40, 57, 57, 79, 96, 96, 119, 136, 136, 158, 
    175, 175, 1, 18, 18, 40, 57, 57, 79, 96, 96, 119, 136, 136
  };
  return YCOORDS[i];
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
  uint8_t beat = beatsin8(4, 0, 128, 0, 0);

  for (uint8_t n = 0; n < NUM_LEDS; n++) {
    uint8_t value = 255 - (abs(beat - get_pixel_distance_to_origin(n)) * 4);
    led[i].setPixelColor(n, led[i].gradient_wheel(shows[i].getForeColor(), value));
  }
}

//
// Q*bert
//
void qbert(uint8_t i) {
  CHSV color;
  
  for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
    switch (z) {
      case 0: 
        color = led[i].gradient_wheel(shows[i].getForeColor(), 255);
        break;
      case 1:
        color = led[i].gradient_wheel(shows[i].getBackColor(), 255);
        break;
      case 2:
        color = led[i].gradient_wheel(shows[i].getForeColor(), 128);
        break;
    }
    for (uint8_t x = 0; x < SIZE; x++) {
      for (uint8_t y = 0; y < SIZE; y++) {
        if (inbounds(x, y)) {
          led[i].setPixelColor(get_pixel_from_coord(x,y,z), color);
        }
      }
    }
  }
}

//
// Windmill
//
void windmill(uint8_t i) {
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(30, 50);
    shows[i].turnOffMorphing();
  }

  for (uint8_t n = 0; n < NUM_LEDS; n++) {
    uint8_t value = beatsin8(freq_storage[i] - (get_ycoord(n) / 10), 0, 255, 0, get_xcoord(n));
    value = (value > 200) ? value : 0 ;
    led[i].setPixelColor(n, CHSV(shows[i].getForeColor(), 255, value) );
  }
}

void windmill_smoothed(uint8_t i) {
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(30, 50);
    shows[i].turnOffMorphing();
  }

  for (uint8_t n = 0; n < NUM_LEDS; n++) {
    uint8_t value = beatsin8(freq_storage[i] - (get_ycoord(n) / 10), 0, 255, 0, get_xcoord(n));
    led[i].setPixelColor(n, CHSV(shows[i].getForeColor(), 255, value) );
    led[i].setPixelColor(n, CHSV(shows[i].getForeColor(), 255, value) );
  }
}

//
// rings
//
void center_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 128, 128, 1, 64, 64);
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


  for (uint8_t n = 0; n < NUM_LEDS; n++) {
    uint8_t delta = abs(get_pixel_distance_to_point(n, center_x, center_y) - value) % ring_freq;
    if (delta < cutoff) {
      uint8_t intensity = map(delta, 0, cutoff, 255, 0);
      shows[c].setPixeltoColor(n, led[c].getInterpHSV(background, foreColor, intensity));
    }
  }
}

///// Distance functions

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t get_pixel_distance_to_origin(uint8_t i) {
  return get_pixel_distance_to_point(i, 128, 128);
}

uint8_t get_pixel_distance_to_point(uint8_t i, uint8_t point_x, uint8_t point_y) {
  return get_distance_to_coord(get_xcoord(i), get_ycoord(i), point_x, point_y);
}

uint8_t get_distance_to_origin(uint8_t x, uint8_t y) {
  return get_distance_to_coord(x, y, 128, 128);
}

uint8_t get_distance_to_coord(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  // distance = sqrt(x^2 + y^2)
  // input: 0-255 coordinates
  // output: 0-255 distance
  // dx and dy should be no larger than 180 each
  uint8_t dx = map8(abs(x2 - x1), 0, 100);
  uint8_t dy = map8(abs(y2 - y1), 0, 100);
  return sqrt16((dx * dx) + (dy * dy));
}

////////// Game of Life

//
// Game of Life
//
void game_of_life(uint8_t i, boolean faster) {
  if (shows[i].getMorph() != 0) {
    return;
  }
  if (shows[i].isShowStart()) {
    if (faster) { 
      shows[i].makeWaitFaster(2);
    }
    else { 
      shows[i].makeWaitSlower(2);
    }
  } 
  if (shows[i].isShowStart() || count_life() < 10) {
    randomly_populate_life(random(20, 50));
  } else {
//    print_life_board();
    grow_life(i);
  }
}

void grow_life(uint8_t i) {
  shows[i].fillBlack();

  for (uint8_t n = 0; n < NUM_LEDS; n++) {
    uint8_t neighbors = count_life_neighbors(n);

    if (has_life(n) == 0) {  // Pixel is off
      if (neighbors == 3) {
        set_life_coord(n, true, 180, i);  // spontaneously grow a pixel
      } else {
        set_life_coord(n, false, 0, i);  // Turn pixel off
      }
    } else {
      
      switch(neighbors) {  // Pixel is on
        case 2:
          set_life_coord(n, true, 160, i);
          break;
        case 3:
          set_life_coord(n, true, 160, i);
          break;
        default:
          set_life_coord(n, false, 200, i);
          break;
      }
    }
  }
}

uint8_t count_life() {
  uint8_t life = 0;
  for (uint8_t n = 0; n < NUM_LEDS; n++) {
    life += has_life(n);
  }
  return life;
}

uint8_t count_life_neighbors(uint8_t n) {
  uint8_t neigh_count = 0;

  for (uint8_t dir = 0; dir < 4; dir++) {
    uint8_t neighbor = get_neighbor(n, dir);
    neigh_count += has_life(neighbor);
    neigh_count += has_life(get_neighbor(neighbor, dir));
  }
  return neigh_count;
}

uint8_t has_life(uint8_t n) {
  if (n == XX) {
    return 0;  // off the board
  }
  return LIFE_BOARD[n];
}

uint8_t get_neighbor(uint8_t pos, uint8_t dir)
{
  if (pos != XX) {
    return neighbors[((pos % NUM_LEDS) * 4) + (dir % 4)];
  } else {
    return XX;
  }
}

void randomly_populate_life(uint8_t seeds) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) { 
    set_life(i, false);  // Clear the board
  }
  for (uint8_t i = 0; i < seeds; i++) { 
    set_life(random8(NUM_LEDS), true);  // Populate some life
  }
}

void set_life(uint8_t n, boolean value) {
  LIFE_BOARD[n] = (value) ? 1 : 0;
}

void set_life_coord(uint8_t n, boolean value, uint8_t hue, uint8_t i) {
  set_life(n, value);
  
  shows[i].setPixeltoBlack(n);
  
  if (value == false && hue == 0) {
    shows[i].setPixeltoBlack(n);
  } else {
    shows[i].setPixeltoHue(n, hue);
  }
}

void print_life_board() {
  uint8_t neighs[] = { 1, 0, 16, 3 };
  Serial.print(has_life(2));
  Serial.print(": ");
  for (uint8_t n = 0; n < 4; n++) {
    Serial.print(has_life(neighs[n]));
  }
  Serial.print(" = ");
  Serial.println(count_life_neighbors(2));
}


//// End specialized shows


////// Speaking and Hearing

String getReadings (uint8_t c) {
  JSONVar jsonReadings;

  jsonReadings["c"] = c;
  jsonReadings["cycle"] = (const double)shows[c].getSmallCycle();
  jsonReadings["f"] = shows[c].getForeColor();
  jsonReadings["fspd"] = shows[c].getForeColorSpeed();
  jsonReadings["b"] = shows[c].getBackColor();
  jsonReadings["bspd"] = shows[c].getBackColorSpeed();
  jsonReadings["s"] = current_show[c];
  jsonReadings["w"] = shows[c].getWait();
  jsonReadings["m"] = show_variables[c + 6];
  jsonReadings["p"] = show_variables[c + 8];
  jsonReadings["w"] = show_variables[c + 10];

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

  // Check to see whether a phone or mesh command
  if (int(myObject["type"]) == PHONE_MESSAGE) {
    processPhoneMessage(myObject);
  
  } else {
    int c = int(myObject["c"]);
    shows[c].setSmallCycle(double(myObject["cycle"]));
    shows[c].setForeColor(int(myObject["f"]));
    shows[c].setForeColorSpeed(int(myObject["fspd"]));
    shows[c].setBackColor(int(myObject["b"]));
    shows[c].setBackColorSpeed(int(myObject["bspd"]));
    shows[c].setWait(int(myObject["w"]));
    show_variables[c + 6] = int(myObject["m"]);
    show_variables[c + 8] = int(myObject["p"]);
    show_variables[c + 10] = int(myObject["w"]);
    
    int show_number = int(myObject["s"]);
    if (current_show[c] != show_number) {
      current_show[c] = show_number % NUM_SHOWS;
      next_show(c);
    }
  }
  
  last_connection = 0;
  is_lost = false;
}

void processPhoneMessage(JSONVar myObject) {
  int value = int(myObject["value"]);
  Serial.printf("Received control value of %d\n", value);

  switch (int(myObject["param"])) {
    case BRIGHTNESS_COMMAND:
      bright = value;
      FastLED.setBrightness( bright );
      break;
    case HUE_COMMAND:
      hue_start = value;
      break;
    case HUE_WIDTH_COMMAND:
      hue_width = value;
      break;
    case SPEED_COMMAND:
      show_speed = value;
      break;
    case SHOW_DURATION_COMMAND:
      show_duration = map8(value, 10, 180);  // min = 10 seconds; max = 3 minutes
      max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // update max_small_cycle
      break;
    case FADE_COMMAND:
      fade_amount = value;  // 0-255
      break;
  }
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


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
   mirror_pixels(CHANNEL_A);
   mirror_pixels(CHANNEL_B);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = mask(led[CHANNEL_B].getInterpFrameColor(rotate_pixel(i, show_variables[2 + CHANNEL_B])), i, CHANNEL_B);
    CHSV color_a = mask(led[CHANNEL_A].getInterpFrameColor(rotate_pixel(i, show_variables[2 + CHANNEL_A])), i, CHANNEL_A);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }

    leds[lookup_led(i)] = (palette_number == 0) ? color : narrow_palette(color);
    led_buffer[i] = color;
  }
}

//
// lookup led: convert pixel to LED
//
uint8_t lookup_led(uint8_t i) {
  if (i <= 11) {  // Row 0
    return i;
  } else if (i <= 26) {  // Row 1
    return (26 - i + 14);
  } else if (i <= 44) {  // Row 2
    return i + 4;
  } else if (i <= 65) {  // Row 3
    return (65 - i + 51);
  } else if (i <= 83) {  // Row 4
    return i + 6;
  } else if (i <= 98) {  // Row 5
    return (98 - i + 90);
  } else {  // Row 6
    return i + 6;
  }
}

void turn_off_spacer_leds() {
  const uint8_t SPACER_LEDS[] = { 12, 13, 29, 30, 49, 50 };
  for (uint8_t i = 0; i < NUM_SPACER_LEDS; i++) {
    leds[SPACER_LEDS[i]] = CHSV(0, 0, 0);
  }
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
// mirror_pixels
//
void mirror_pixels(uint8_t channel) {

  uint8_t symmetry = show_variables[4 + channel];
  
  if (symmetry == 0) {
    return;  // no mirroring
  }
  
  // 2-fold (1), 3-fold (2), 6-fold (3)
  if (symmetry == 1 || symmetry == 3) {  // 180-degree 2-fold mirroring
    // vertical 2-fold mirroring
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      uint8_t new_i = get_vert_symmetric_pixel(i);
      if (new_i != i) {
        led[channel].setInterpFrame(new_i, led[channel].getInterpFrameColor(i));
      }
    }
  }
  
  if (symmetry == 2 || symmetry == 3) {  // 120-degree 3-fold rotation
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      uint8_t pixel = i / 3;
      if ((pixel < 13 && pixel != 8) || (i == 55)) {
        CHSV color = led[channel].getInterpFrameColor(i);
        led[channel].setInterpFrame(rotate_pixel(i, 1), color);
        led[channel].setInterpFrame(rotate_pixel(i, 2), color);
      }
    }
  }
}

//
// rotate_pixel - rotate triangle grid 120-degrees for each "r"
//
uint8_t rotate_pixel(uint8_t i, uint8_t rotation) {
  
  const uint8_t ROTATE[] = {
    33, 28, 22, 15,
    34, 29, 23, 16, 9,
    35, 30, 24, 17, 10, 4,
    36, 31, 25, 18, 11, 5, 0,
    32, 26, 19, 12, 6, 1,
    27, 20, 13, 7, 2,
    21, 14, 8, 3
  };
  if (rotation == 0) {
    return i;
  }
  uint8_t z = get_z_from_pixel(i);
  uint8_t hex = i / 3;
  
  for (uint8_t r = rotation; r > 0; r--) {
    hex = ROTATE[hex];
    z = (z + 2) % DIAMOND_NUMBER;
  }
  return (hex * 3) + z;
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


//
// adjust brightness - put a cap on maximum brightness to prevent flickering
//
void adj_brightess() {
  uint8_t brightness_step_size = 5;
  uint32_t lumens = 0;

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    lumens += leds[i].r;
    lumens += leds[i].g;
    lumens += leds[i].b;
  }
  lumens = curr_bright * lumens / 255;  // total lumens adjusted by brightess setting
  
  if (lumens > MAX_LUMENS) {  // we are too bright
    curr_bright -= brightness_step_size;  // Turn down brightness
    FastLED.setBrightness( curr_bright );
  } else {
    if (bright > curr_bright) {
      curr_bright = min(bright, qadd8(curr_bright, brightness_step_size));  // Turn up brightness
      FastLED.setBrightness( curr_bright );
    }
  }
}

//// End DUAL SHOW LOGIC

//
// palettes
//
void randomize_palette() {
  if (CAN_CHANGE_PALETTES) {
    palette_number = random8(6);  // global
  }
}

//
// narrow palette 
//
CRGB narrow_palette(CHSV color) {  
  if( palette_number ==  2)  { return ColorFromPalette( CloudColors_p, color.h, color.v, LINEARBLEND); }
  if( palette_number ==  3)  { return ColorFromPalette( OceanColors_p, color.h, color.v, LINEARBLEND); }
  if( palette_number ==  4)  { return ColorFromPalette( LavaColors_p, color.h, color.v, LINEARBLEND); }
  if( palette_number ==  5)  { return ColorFromPalette( ForestColors_p, color.h, color.v, LINEARBLEND); }
  if( palette_number ==  6)  { return ColorFromPalette( PartyColors_p, color.h, color.v, LINEARBLEND); }
  return ColorFromPalette( RainbowColors_p, color.h, color.v, LINEARBLEND);  // default
}

//
// Get pixel from coordinate
//
uint8_t get_pixel_from_coord(uint8_t x, uint8_t y, uint8_t z) {
  const uint8_t SPACING[] = { 0, 4, 9, 15, 22, 28, 33 };
  return (((x + SPACING[y % SIZE]) * DIAMOND_NUMBER) + z) % NUM_LEDS;
}

uint8_t get_x_from_pixel(uint8_t i) {
  const uint8_t SPACING[] = { 0, 4, 9, 15, 22, 28, 33 };
  i /= DIAMOND_NUMBER;
  for (uint8_t y = SIZE - 1; y >= 0; y--) {
    if (i >=  SPACING[y]) {
      return i - SPACING[y];
    }
  }
}

uint8_t get_y_from_pixel(uint8_t i) {
  const uint8_t SPACING[] = { 0, 4, 9, 15, 22, 28, 33 };
  i /= DIAMOND_NUMBER;
  for (uint8_t y = SIZE - 1; y >= 0; y--) {
    if (i >=  SPACING[y]) {
      return y;
    }
  }
}

uint8_t get_z_from_pixel(uint8_t i) {
  return i % DIAMOND_NUMBER;
}

uint8_t get_vert_symmetric_pixel(uint8_t i) {
  // Return the vertically symmetric pixel or i, if not mirrored
  uint8_t x = get_x_from_pixel(i);
  uint8_t y = get_y_from_pixel(i);
  uint8_t z = get_z_from_pixel(i);

  const uint8_t WIDTHS[] = { 8, 10, 12, 14, 12, 10, 8 };
  if ((x * 2) + (z / 2) < WIDTHS[y]) {
    // y stays constant, x flips around the center axis, y jumps
    z = (z == 1) ? 1 : (z + 2) % 4;  // 0 -> 2, 1 -> 1, 2 -> 0;
    x = (WIDTHS[y] / 2) - x - 1;  // flip
    return get_pixel_from_coord(x, y, z);
  } else {
    return i;  // no mirroring on the right side
  }
}

//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  const uint8_t PatternMatrix[] = {
    0xa0, 0x57, 0x83, 0xc0, 0xa, 0x0, 0xa1, 0xe0, 0x1e, 0x2, 0x81, 0x43, 0xcf, 0x0,
    0xb6, 0xd0, 0x0, 0x3, 0x24, 0xc3, 0x13, 0x91, 0x98, 0x84, 0x66, 0x28, 0xcc, 0xc,
    0x0, 0xce, 0x3a, 0xe0, 0x18, 0x60, 0x31, 0x74, 0x3a, 0xe0, 0x8, 0x61, 0x9d, 0x4,
    0x6, 0x0, 0x10, 0x0, 0xdb, 0x3f, 0xc, 0x60, 0x0, 0x78, 0x3, 0x11, 0x88, 0x4,
    0x6, 0x0, 0x7c, 0x0, 0xe7, 0x0, 0x38, 0x38, 0xf, 0xb7, 0xc4, 0x92, 0x40, 0x0,
    0x20, 0x4b, 0xc7, 0xa8, 0x3c, 0x10, 0xb7, 0xde, 0x1, 0xcc, 0x1, 0xc7, 0x9, 0x80,
    0x20, 0x40, 0xc6, 0x2, 0xbd, 0x40, 0x7b, 0xbc, 0x0, 0xcc, 0x1, 0x83, 0x8, 0x4,
    0x60, 0x60, 0xee, 0x1c, 0x3c, 0x38, 0xe3, 0x8e, 0x38, 0xfc, 0x71, 0xc7, 0x1f, 0xfe,
    0x3, 0x3e, 0x5, 0x3, 0x80, 0x0, 0x1f, 0xf0, 0x0, 0x3, 0x81, 0x98, 0xe2, 0x80,
    0x0, 0xc0, 0x68, 0x80, 0x8c, 0x80, 0x39, 0x66, 0x8, 0x82, 0x84, 0xb3, 0x0, 0x50,
    0x0, 0xe, 0x0, 0x3, 0x80, 0x0, 0x1f, 0xff, 0xc7, 0x0, 0xe, 0x33, 0x0, 0x50
  }; 
  pattern_number = pattern_number % NUM_PATTERNS;
  uint8_t pattern_byte = PatternMatrix[(pattern_number * 14) + (n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

uint8_t lookup_complex_pattern_table(uint8_t n, uint8_t complex_number) {
  // See iPython Notebook: Pattern Maker Large QBert
  
  const uint8_t ComplexMatrix[] = {
    13,13,13,12,12,12,11,11,11,10,10,10,14,14,14,5,4,4,3,3,2,2,1,1,9,9,9,15,15,15,6,5,6,6,7,8,9,10,
    11,18,18,17,8,8,8,16,16,16,7,7,8,3,5,4,0,0,0,14,12,13,16,17,16,7,7,7,17,17,17,9,8,9,1,2,18,17,15,
    16,15,15,14,6,6,6,18,18,18,10,10,11,11,12,12,13,14,13,5,5,5,1,1,1,2,2,2,3,3,3,4,4,4,1,1,1,5,5,5,
    1,1,1,7,7,7,2,2,2,4,4,4,2,2,2,6,6,6,1,1,1,3,3,3,3,3,3,3,3,3,5,5,5,2,2,2,5,5,5,4,4,4,2,2,2,3,3,3,
    4,4,4,3,3,3,4,4,4,1,1,1,1,1,1,4,4,4,3,3,3,4,4,4,3,3,3,2,2,2,5,5,5,2,2,2,5,5,5,2,2,2,3,3,3,1,1,1,
    6,6,6,1,1,1,4,4,4,0,0,0,4,0,5,0,0,0,4,0,3,3,0,4,0,3,2,1,6,2,0,5,6,1,2,6,6,2,1,6,5,0,0,6,5,4,3,0,
    0,2,3,4,5,0,0,5,4,3,2,0,3,0,2,6,0,5,0,0,0,3,0,4,0,0,0,0,0,0,0,4,5,6,1,2,3,4,0,6,2,1,6,5,0,0,0,0,
    5,0,6,2,0,3,0,5,4,3,2,0,0,4,3,2,1,6,5,4,0,0,0,0,0,0,6,7,0,8,9,0,10,11,0,0,8,7,6,0,5,4,0,0,0,13,12,
    0,3,4,5,1,9,2,3,5,4,0,3,2,15,14,0,1,2,9,8,6,7,0,0,11,12,0,13,14,0,15,1,1,1,2,0,3,4,0,5,6,0,0,0,10,
    9,0,0,0,3,2,0,0,15,14,0,0,0,8,7,0,0,8,7,5,4,0,9,1,2,0,13,12,10,9,0,0,6,0,7,8,6,5,3,4,0,11,0
  };
  return ComplexMatrix[((complex_number % NUM_COMPLEX_PATTERNS) * NUM_LEDS) + n];
}

uint8_t get_complex_max(uint8_t complex_number) {
  uint8_t complex_max[] = { 18, 7, 6, 15 };
  return complex_max[complex_number % NUM_COMPLEX_PATTERNS];
}

//
// log status
//
void log_status(uint8_t i) {
  Serial.print(F("Channel: "));
  Serial.print(i);
  Serial.print(F(", Show: "));
  Serial.print(current_show[i]);
  Serial.print(F(", Wait: "));
  Serial.print(shows[i].getNumFrames());
  Serial.print(F(", Mask: "));
  Serial.print(show_variables[i + 6]);
  Serial.print(F(", Symmetry: "));
  Serial.print(show_variables[i + 4]);
  Serial.println(".");
}
