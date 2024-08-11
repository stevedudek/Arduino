#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Mandala with FastLED
//
//  37 pixels with 64 LEDs
//
//  Modern Software
//
//  4/26/24
//
//
#define NUM_LEDS 37  // Chance of memory shortage for large NUM_LEDS
#define ACTUAL_LEDS 64

#define SIZE 7
#define HALF_SIZE 3

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME 15 // in milliseconds

#define DATA_PIN  0
#define CLOCK_PIN 2

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

// Smoothing constants - lower is slower smoothing
#define SMOOTHING_SHOWS_HUE    10  // Fastest full rainbow = DELAY_TIME * (255 / this value) = 150 ms
#define SMOOTHING_SHOWS_VALUE  30   // Fastest turn off/on = DELAY_TIME * (255 / this value) = 150 ms

#define XX  255  // Empty LED

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
#define BLACK  CHSV(0, 0, 0)

uint8_t colors = 0;  // color scheme: 0 = all, 1 = red, 2 = blue, 3 = yellow

uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[ACTUAL_LEDS];  // Hook for FastLED library

// Shows
#define START_SHOW_CHANNEL_A  1  // Channels A starting show
#define START_SHOW_CHANNEL_B  5  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type
#define NUM_SHOWS 13

// Clocks and time

uint8_t show_duration = 80;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 196;  // 0 = no fading, to 255 = always be fading

//
//  Mesh Network
//
Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

#define ARE_CONNECTED false// Are the pentagons talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = 0;
#define MAX_SILENT_TIME  10000  // Time (in cycles) without communication before marked as is_lost

//#define MESSAGE_REPEAT 3   // send this many duplicate messages
#define MESSAGE_SPACING 4   // wait this many cycles

painlessMesh mesh;
String message;  // String to send to other displays
String getReadings();  // Prototype for reading state of LEDs

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

#define NUM_PATTERNS 16  // Total number of patterns

// Lookup tables

// Convert pixel to type, order
const uint8_t pixel_coord[] PROGMEM = { 
  0, 0,  // 0
  2, 0,  // 1
  3, 0,  // 2
  4, 0,  // 3
  6, 0,  // 4
  0, 2,  // 5
  1, 1,  // 6
  2, 1,  // 7
  4, 1,  // 8
  5, 1,  // 9
  6, 2,  // 10
  1, 2,  // 11
  2, 2,  // 12
  3, 2,  // 13
  4, 2,  // 14
  5, 2,  // 15
  0, 3,  // 16
  2, 3,  // 17
  3, 3,  // 18
  4, 3,  // 19
  6, 3,  // 20
  1, 4,  // 21
  2, 4,  // 22
  3, 4,  // 23
  4, 4,  // 24
  5, 4,  // 25
  0, 4,  // 26
  1, 5,  // 27
  2, 5,  // 28
  4, 5,  // 29
  5, 5,  // 30
  6, 4,  // 31
  0, 6,  // 32
  2, 6,  // 33
  3, 6,  // 34
  4, 6,  // 35
  6, 6   // 36
};

const uint8_t pixel_matrix[] PROGMEM = { 
   0,  0,  1,  2,  3,  4,  4,  // y=0
   0,  6,  7, 13,  8,  9,  4,  //   1
   5, 11, 12, 13, 14, 15, 10,  //   2
  16, 17, 17, 18, 19, 19, 20,  //   3
  26, 21, 22, 23, 24, 25, 31,  //   4
  32, 27, 28, 23, 29, 30, 36,  //   5
  32, 32, 33, 34, 35, 36, 36,  //   6
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
//        test_strand(i);
        shows[i].allOn();
        break;
      case 2:
        shows[i].morphChain();
        break;
      case 3:
        shows[i].twoColor();
        break;
      case 4:
        shows[i].sawTooth();
        break;
      case 5:
        shows[i].sinelon_fastled();
        break;
      case 6:
        shows[i].juggle_fastled();
        break;
      case 7:
        cone_push(i);
        break;
      case 8:
        shows[i].randomFlip();
        break;
      case 9:
        center_ring(i);
        break;
      case 10:
        corner_ring(i);
        break;
      case 11:
        well(i);
        break;
      default:
        shows[i].randomTwoColorBlack();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
    mirror_pixels(i);  // This flickers if done incorrectly
  }
  
  morph_channels();
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
  show_variables[i + 2] = random(4);  // rotate
  
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
  shows[i].fillBlack();
//  log_status(i);
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
  uint8_t random_symmetry = random(9);
  
  if (random_symmetry <= 6) {
    return random_symmetry;
  }
  return 0;  // No symmetrizing
}


////// Specialized shows

//
// Cone Push
//
void cone_push(uint8_t channel) {
  if (shows[channel].isShowStart()) {
    shows[channel].turnOffMorphing();
  }
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t dist = get_distance_to_origin(get_led_x(i), get_led_y(i));
    uint8_t value = beatsin8(10, 64, 255, 0, dist);
    shows[channel].setPixeltoColor(i, led[channel].gradient_wheel(shows[channel].getForeColor(), value));
  }
}

//
// rings
//
void center_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 128, 128, 1, 64, 128);
}

void well(uint8_t c) {
  ring(c, shows[c].getForeColor(), led[c].wheel(shows[c].getBackColor()), 128, 128, 2, 192, 192);
}

void corner_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 0, 0, 3, 96, 255);
}

void ring(uint8_t c, uint8_t color, CHSV background, uint8_t center_x, uint8_t center_y, uint8_t ring_speed, uint8_t cutoff, uint8_t ring_freq) {
  // center_x, center_y = ring center coordinates; (128,128) is hex centers
  // ring_speed = 1+. Higher = faster.
  // cutoff = ring thickness with higher values = thicker
  // ring_freq = (255: 1 ring at a time; 128: 2 rings at a time)
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();
  }
  CHSV foreColor = led[c].wheel(color);
  shows[c].fill(background);
  uint8_t value = shows[c].getCycle() / ring_speed;


  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t delta = abs(get_distance_to_coord(center_x, center_y, get_led_x(i), get_led_y(i)) - value) % ring_freq;
    if (delta < cutoff) {
      uint8_t intensity = map(delta, 0, cutoff, 255, 0);
      shows[c].setPixeltoColor(i, led[c].getInterpHSV(background, foreColor, intensity));
    }
  }
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
        uint8_t intense = get_wipe_intensity(i, show_variables[c + 10], (pattern_type < 4));
        uint8_t back_hue = shows[c].getBackColor();
        if (pattern_type < 4) {
          shows[c].setPixeltoHue(i, shows[c].IncColor(back_hue, intense / 8));
        } else {
          shows[c].setPixeltoColor(i, led[c].gradient_wheel(back_hue, map8(intense, 128, 255)));
        }
      }
    }
  }
}

//
// Get Wipe Intensity
//
uint8_t get_wipe_intensity(uint8_t i, uint8_t wipe_number, boolean color_wipe) {
  uint8_t intensity;
  uint8_t x = get_led_x(i);  // 0-255
  uint8_t y = get_led_y(i);  // 0-255

  wipe_number = 0;
  
  switch (wipe_number) {
    case 0:
      intensity = x;  // coefficient = thickness
      break;
    case 1: 
      intensity = y;
      break;
    case 2:
      intensity = (x + y) / 2;
      break;
    case 3:
      intensity = (x + (255 - y)) / 2;
      break;
    case 4:
      intensity = 255 - (get_distance_to_origin(x, y) * 2);  // center wipe
      break;
    default:
      intensity = 255 - get_distance_to_coord(x, y, 0, 0);  // corner wipe
      break;
  }
  uint8_t beat_freq = (color_wipe) ? 2 : 10;
  return beatsin8(beat_freq, 0, 255, 0, intensity);
}

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t get_distance_to_origin(uint8_t x, uint8_t y) {
  return get_distance_to_coord(x, y, 128, 128);
}

uint8_t get_distance_to_coord(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  // distance = sqrt(x^2 + y^2)
  // input: 0-255 coordinates
  // output: 0-255 distance
  uint8_t dx = abs(x2 - x1);  // 0-255
  uint8_t dy = abs(y2 - y1);  // 0-255
  return sqrt16(((dx * dx) + (dy * dy)) / 2);
}

//
// get led x, y - lookup table for pixel x and y coordinate (0-255)
//
uint8_t get_led_x(uint8_t i) {
  uint8_t led_lookup_x_table[NUM_LEDS] = {
     0, 73, 128, 182, 255,
     0, 47, 78, 177, 208,
     255, 31, 91, 128, 164,
     224, 0, 55, 128, 200,
     255, 31, 91, 128, 164,
     224, 0, 47, 78, 177,
     208, 255, 0, 73, 128,
     182, 255
  };
  return led_lookup_x_table[i];
}

uint8_t get_led_y(uint8_t i) {
  uint8_t led_lookup_y_table[NUM_LEDS] = {
     0, 0, 0, 0, 0,
     73, 47, 31, 31, 47,
     73, 78, 91, 55, 91,
     78, 128, 128, 128, 128,
     128, 177, 164, 200, 164,
     177, 182, 208, 224, 224,
     208, 182, 255, 255, 255,
     255, 255
  };
  return led_lookup_y_table[i];
}

//
// mirror_pixels
//
void mirror_pixels(uint8_t channel) {  
  uint8_t symmetry = show_variables[4 + channel];
  
  if (symmetry == 1 || symmetry == 3) {  // Vertical mirroring
    for (uint8_t y = 0 ; y < HALF_SIZE; y++) {
      for (uint8_t x = 0 ; x < SIZE; x++) {
        led[channel].setCurrentFrame(get_pixel_from_coord(x, SIZE - y - 1), led[channel].getCurrFrameColor(get_pixel_from_coord(x,y)));
      }
    }
  }
  if (symmetry == 2 || symmetry == 3) {  // Horizontal mirroring
    for (uint8_t x = 0 ; x < HALF_SIZE; x++) {
      for (uint8_t y = 0 ; y < SIZE; y++) {
        led[channel].setCurrentFrame(get_pixel_from_coord(SIZE - x - 1, y), led[channel].getCurrFrameColor(get_pixel_from_coord(x,y)));
      }
    }
  }
  if (symmetry == 4 || symmetry == 6) {  // Diagonal 1 mirroring
    for (uint8_t x = 0 ; x < SIZE; x++) {
      for (uint8_t y = 0 ; y < SIZE; y++) {
        if (y <= x) {
          led[channel].setCurrentFrame(get_pixel_from_coord(y,x), led[channel].getCurrFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }
  if (symmetry == 5 || symmetry == 6) {  // Diagonal 2 mirroring
    for (uint8_t x = 0 ; x < SIZE; x++) {
      for (uint8_t y = 0 ; y < SIZE; y++) {
        if (x + y < SIZE) {
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
  uint8_t x = get_pixel_x(i);
  uint8_t y = get_pixel_y(i);
  uint8_t rotation = show_variables[2 + channel];

  for (uint8_t r = rotation; r > 0; r--) {
    new_x = SIZE - y - 1;
    new_y = x;
    x = new_x;
    y = new_y;
  }

  return get_pixel_from_coord(x, y);
}

uint8_t get_pixel_from_coord(uint8_t x, uint8_t y) {
  return pgm_read_byte_near(pixel_matrix + (y * 7) + x);
}

uint8_t get_pixel_x(uint8_t i) {
  return pgm_read_byte_near(pixel_coord + (i * 2));
}

uint8_t get_pixel_y(uint8_t i) {
  return pgm_read_byte_near(pixel_coord + (i * 2) + 1);
}

//
// Test Strand
//
void test_strand(uint8_t channel) {
  shows[channel].fillBlack();
  uint8_t i = shows[channel].getCycle() % NUM_LEDS;
  led[channel].setPixelHue(i, shows[channel].getForeColor());
}


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
  shows[c].setBackColor(int(myObject["f"]));
  shows[c].setForeColorSpeed(int(myObject["fspd"]));
  shows[c].setBackColor(int(myObject["b"]));
  shows[c].setBackColorSpeed(int(myObject["bspd"]));
  show_variables[c] = int(myObject["p"]);

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
      remap_leds(i, color);   // put HSV color on to LEDs
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
      update_leds = true;
    }
  }
  
  turn_off_spacers();

  if (update_leds) { 
    FastLED.show();
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
// remap_leds - send color to the correct leds
//
void remap_leds(uint8_t i, CHSV color) {
  uint8_t led_lookup_table[NUM_LEDS * 3] = {
     0,  2,  3,  // 0
     6, XX, XX,
     8, XX, XX,
    10, XX, XX,
    13, 14, 16,
     1, XX, XX,  // 5
     4, XX, XX,
     5,  7, XX,
     9, 11, XX,
    12, XX, XX,
    15, XX, XX,  // 10
    29, 31, XX,
    27, XX, XX,
    25, XX, XX,
    23, XX, XX,
    19, 22, XX,  // 15
    32, XX, XX,
    30, XX, XX,
    24, 26, 28,
    21, XX, XX,
    20, XX, XX,  // 20
    33, 35, XX,
    36, XX, XX,
    38, XX, XX,
    40, XX, XX,
    42, 44, XX,  // 25
    62, XX, XX,
    59, XX, XX,
    56, 58, XX,
    52, 54, XX,
    51, XX, XX,  // 30
    48, XX, XX,
    60, 61, 63,
    57, XX, XX,
    55, XX, XX,
    53, XX, XX,  // 35
    47, 49, 50
  };

  if (i == 18) {  // Center star
    light_center(color);
  } else {
    for (uint8_t j = 0; j < 3; j++) {
      uint8_t led_position = led_lookup_table[((i % NUM_LEDS) * 3) + j];
      if (led_position != XX) {
        leds[led_position % ACTUAL_LEDS] = color;
      }
    }
  }
}

//
// light_center - light the center 6 leds with the same color
//
void light_center(CHSV color) {
  uint8_t center_pixels[] = { 24, 26, 28, 37, 39, 41 };
  for (uint8_t i = 0; i < 6; i++) {
    leds[center_pixels[i]] = color;
  }
}

//
// turn_off_spacers - turn off the 4 spacer pixels
//
void turn_off_spacers() {
  uint8_t spacer_pixels[] = { 17, 18, 45, 46 };
  for (uint8_t i = 0; i < 4; i++) {
    leds[spacer_pixels[i]] = CHSV(0, 0, 0);
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
  uint8_t PatternMatrix[] = {
    0xfe, 0x6a, 0xaa, 0xb3, 0xf8,
    0x46, 0x4, 0x71, 0x3, 0x10,
    0x21, 0x8a, 0xaa, 0x8c, 0x20,
    0x20, 0xe, 0x53, 0x80, 0x88,
    0x1a, 0x28, 0x20, 0xa2, 0xc0,
    0x21, 0xcb, 0x8e, 0x9c, 0x20,
    0x24, 0xd3, 0x4c, 0xe6, 0x20,
    0x18, 0x28, 0x32, 0x1, 0x18,
    0x88, 0x0, 0x20, 0x0, 0x88,
    0x1, 0x91, 0x4, 0x4c, 0x0,
    0x56, 0x6e, 0x53, 0xb3, 0x50,
    0x70, 0xe, 0xdb, 0x80, 0x70,
    0x92, 0x8c, 0x21, 0x8a, 0x48,
    0xf8, 0x1b, 0xa6, 0xc0, 0xf8,
    0x18, 0xa5, 0x75, 0x28, 0xc0,
    0x21, 0x8a, 0x22, 0x8c, 0x20,
    0x21, 0x8a, 0xaa, 0x8c, 0x20,
    0x23, 0xdb, 0x8e, 0xde, 0x70,
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
