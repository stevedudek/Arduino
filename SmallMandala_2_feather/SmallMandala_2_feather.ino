#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Mandala with FastLED
//
//  37 pixels with 64 LEDs
//
//  FastLED - On the Adafruit Feather
//
//  MESH network!
//
//  Listens to the Network and an IR Controller
//
//  2/28/22
//
//
#define NUM_LEDS 37  // Chance of memory shortage for large NUM_LEDS
#define ACTUAL_LEDS 64

#define SIZE 7
#define HALF_SIZE 3

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME 12  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN  0
#define CLOCK_PIN 2

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define XX  255  // Empty LED

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
#define BLACK  CHSV(0, 0, 0)

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[ACTUAL_LEDS];  // Hook for FastLED library

// Shows
#define START_SHOW_CHANNEL_A 13  // Channels A starting show
#define START_SHOW_CHANNEL_B  0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type
#define NUM_SHOWS 18

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
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i].setColorSpeedMinMax(6, 30);  // Make colors change faster (lower = faster)
    shows[i].setBandsBpm(10, 30);

    shows[i].setColorSpeedMinMax(show_speed);
    shows[i].setWaitRange(show_speed);
  }

  if (ONLY_RED) {
    hue_center = 0;
    hue_width = 124;
  }

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = BLACK;
  }

  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);  // Start Channel B offset at halfway through show

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
        shows[i].randomFill();
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
        shows[i].bands();
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
  
  led[i].push_frame();
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
  show_variables[i + 2] = random(4);  // rotate
  
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
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
    led[channel].setPixelColor(i, led[channel].gradient_wheel(shows[channel].getForeColor(), value));
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
  led[c].fill(background);
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
        led[channel].setInterpFrame(get_pixel_from_coord(x, SIZE - y - 1), led[channel].getInterpFrameColor(get_pixel_from_coord(x,y)));
      }
    }
  }
  if (symmetry == 2 || symmetry == 3) {  // Horizontal mirroring
    for (uint8_t x = 0 ; x < HALF_SIZE; x++) {
      for (uint8_t y = 0 ; y < SIZE; y++) {
        led[channel].setInterpFrame(get_pixel_from_coord(SIZE - x - 1, y), led[channel].getInterpFrameColor(get_pixel_from_coord(x,y)));
      }
    }
  }
  if (symmetry == 4 || symmetry == 6) {  // Diagonal 1 mirroring
    for (uint8_t x = 0 ; x < SIZE; x++) {
      for (uint8_t y = 0 ; y < SIZE; y++) {
        if (y <= x) {
          led[channel].setInterpFrame(get_pixel_from_coord(y,x), led[channel].getInterpFrameColor(get_pixel_from_coord(x,y)));
        }
      }
    }
  }
  if (symmetry == 5 || symmetry == 6) {  // Diagonal 2 mirroring
    for (uint8_t x = 0 ; x < SIZE; x++) {
      for (uint8_t y = 0 ; y < SIZE; y++) {
        if (x + y < SIZE) {
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
  jsonReadings["morph"] = shows[c].isMorphing();
  jsonReadings["cycle"] = (const double)shows[c].getCycle();
  jsonReadings["smcycle"] = (const double)shows[c].getSmallCycle();
  jsonReadings["f"] = shows[c].getForeColor();
  jsonReadings["fspd"] = shows[c].getForeColorSpeed();
  jsonReadings["b"] = shows[c].getBackColor();
  jsonReadings["bspd"] = shows[c].getBackColorSpeed();
  jsonReadings["s"] = current_show[c];
  jsonReadings["w"] = shows[c].getWait();

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
  mirror_pixels(CHANNEL_A);
  mirror_pixels(CHANNEL_B);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = mask(led[CHANNEL_B].getInterpFrameColor(rotate_pixel(i, CHANNEL_B)), i, CHANNEL_B);
    CHSV color_a = mask(led[CHANNEL_A].getInterpFrameColor(rotate_pixel(i, CHANNEL_A)), i, CHANNEL_A);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    
    color = narrow_palette(color);
    
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }
    remap_leds(i, color);
    led_buffer[i] = color;
  }
  turn_off_spacers();
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
  Serial.print(", Wait: ");
  Serial.print(shows[i].getNumFrames());
  Serial.println(".");
}
