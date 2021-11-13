#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
//
//  QBert: 10 Hexes x 3 = 30 LEDs in on a triangle
//
//  11/9/21
//
//  On the Adafruit Feather
//
//  MESH network!
//
//  Listens to the Network and an IR Controller
//

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME 12  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN  0
#define CLOCK_PIN 2

#define NUM_LEDS 30

#define SIZE 4
#define DIAMOND_NUMBER 3

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define XX  255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // The Leds themselves
CHSV led_buffer[NUM_LEDS];  // For smoothing

#define MAX_COLOR 256   // Colors are 0-255
#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

// Shows
#define NUM_SHOWS 19
#define START_SHOW_CHANNEL_A  1  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  0
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type

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

uint8_t neighbors[] = {
  1, 2, XX, XX,
  3, 2, 0, 14,  // 3, XX, XX, 14,
  XX, XX, 0, 1,
  4, 5, 1, 14,
  6, 5, 3, 17,
  XX, XX, 3, 4,
  7, 8, 4, 17,
  9, 8, 6, 20,
  XX, XX, 6, 7,
  10, 11, 7, 20,  // XX, XX, 7, 20,
  XX, 11, 9, XX,
  XX, XX, 9, 10,
  13, 14, XX, XX,
  15, 14, 12, 23,
  3, 1, 12, 13,
  16, 17, 13, 23,
  18, 17, 15, 26,
  6, 4, 15, 16,
  19, 20, 16, 26,
  XX, 20, 18, XX,
  9, 7, 18, 19,
  22, 23, XX, XX,
  24, 23, 21, 29,
  15, 13, 21, 22,
  25, 26, 22, 29,
  XX, 26, 24, XX,
  18, 16, 24, 25,
  28, 29, XX, XX,
  XX, 29, 27, XX,
  24, 22, 27, 28  // 24, 22, XX, XX
};

#define NUM_PATTERNS 10   // Total number of patterns, each 4 bytes wide
#define NUM_COMPLEX_PATTERNS 6  // Total number of complex patterns

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
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 4 neighbors for every pixel
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    
    shows[i].setColorSpeedMinMax(show_speed);
    shows[i].setWaitRange(show_speed);

    shows[i].setAsSquare();
  }
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }
  
  if (ONLY_RED) {  // (ArduinoBlue)
    hue_center = 0;
    hue_width = 124;
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
        shows[i].morphChain();
        break;
      case 2:
        shows[i].bounce();
        break;
      case 3:
        shows[i].bounceGlowing();
        break;
      case 4:
        shows[i].plinko(12);
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
//  shows[i].fillForeBlack();
  led[i].push_frame();  // This might not work well with speaking & hearing
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
  show_variables[i + 2] = random(3);  // rotation
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

uint8_t pick_random_symmetry() {
  uint8_t random_symmetry = random(12);
  
  if (random_symmetry <= 3) {
    return random_symmetry;  // 2-fold (1), 3-fold (2), 6-fold (3), None
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
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE - x; y++) {
      for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
        uint8_t i = get_pixel_from_coord(x,y,z);
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
            uint8_t cartesian_x = get_cartesian_x(x, y, z);
            uint8_t cartesian_y = get_cartesian_y(x, y, z);
            uint8_t intense = get_wipe_intensity(cartesian_x, cartesian_y, show_variables[c + 10], (pattern_type < 4));
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
      uint8_t intense = sin8_C((cycle + map(value, 1, pattern_max, 0, 255)) % 255);
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
  uint8_t beat = beatsin8(map8(show_speed, 2, 6), 0, 128, 0, 0);
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE - x; y++) {
      for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
        uint8_t value = 255 - (abs(beat - get_coord_distance_to_origin(x,y,z)) * 4);
        led[i].setPixelColor(get_pixel_from_coord(x,y,z), led[i].gradient_wheel(shows[i].getForeColor(), value));
      }
    }
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
      for (uint8_t y = 0; y < SIZE - x; y++) {
        led[i].setPixelColor(get_pixel_from_coord(x,y,z), color);
      }
    }
  }
}

//
// Windmill
//
void windmill(uint8_t i) {
  uint8_t beat_freq = map8(show_speed, 30, 60);
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE - x; y++) {
      for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
        uint8_t cart_x = get_cartesian_x(x, y, z);
        uint8_t cart_y = get_cartesian_y(x, y, z);
        uint8_t value = beatsin8(beat_freq - (cart_y / 10), 0, 255, 0, cart_x);
        value = (value > 200) ? value : 0 ;
        led[i].setPixelColor(get_pixel_from_coord(x,y,z), CHSV(shows[i].getForeColor(), 255, value) );
      }
    }
  }
}

void windmill_smoothed(uint8_t i) {
  uint8_t beat_freq = map8(show_speed, 30, 60);
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE - x; y++) {
      for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
        uint8_t cart_x = get_cartesian_x(x, y, z);
        uint8_t cart_y = get_cartesian_y(x, y, z);
        uint8_t value = beatsin8(beat_freq - (cart_y / 10), 0, 255, 0, cart_x);
        led[i].setPixelColor(get_pixel_from_coord(x,y,z), CHSV(shows[i].getForeColor(), 255, value) );
      }
    }
  }
}

//
// rings
//
void center_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 80, 128, map8(show_speed, 1, 4), 64, 64);
}

void corner_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 0, 0, map8(show_speed, 2, 7), 32, 128);
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

  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE - x; y++) {
      for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
        uint8_t delta = abs(get_coord_distance_to_point(x, y, z, center_x, center_y) - value) % ring_freq;
        if (delta < cutoff) {
          uint8_t intensity = map(delta, 0, cutoff, 255, 0);
          shows[c].setPixeltoColor(get_pixel_from_coord(x, y, z), led[c].getInterpHSV(background, foreColor, intensity));
        }
      }
    }
  }
}

///// Distance functions

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t get_coord_distance_to_origin(uint8_t x, uint8_t y, uint8_t z) {
  return get_coord_distance_to_point(x, y, z, 80, 128);
}

uint8_t get_coord_distance_to_point(uint8_t x, uint8_t y, uint8_t z, uint8_t point_x, uint8_t point_y) {
  return get_distance_to_coord(get_cartesian_x(x, y, z), get_cartesian_y(x, y, z), point_x, point_y);
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

//
// Get Cartesian X
//
uint8_t get_cartesian_x(uint8_t x, uint8_t y, uint8_t z) {
  float z_lookup_delta_x[] = { -17, 17, 0 };
  float cartesian_x = ((70 * x) + (35 * y) + 17);
  cartesian_x += z_lookup_delta_x[z];
  return uint8_t(cartesian_x);
}

//
// Get Cartesian Y
//
uint8_t get_cartesian_y(uint8_t x, uint8_t y, uint8_t z) {
  float z_lookup_delta_y[] = { 10, 10, -20.2 };
  float cartesian_y = ((60.6 * float(y)) + 35);
  cartesian_y += z_lookup_delta_y[z];
  return uint8_t(cartesian_y);
}

//// End specialized shows


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
  jsonReadings["p"] = show_variables[c];
  jsonReadings["m"] = show_variables[c + 6];
  jsonReadings["fill"] = show_variables[c + 8];
  jsonReadings["wp"] = show_variables[c + 10];

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
  
  show_variables[c] = int(myObject["p"]);
  show_variables[c + 6] = int(myObject["m"]);
  show_variables[c + 8] = int(myObject["fill"]);
  show_variables[c + 10] = int(myObject["wp"]);

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


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  // Lookup table to convert pixels to LEDs
  const uint8_t LED_LOOKUP[NUM_LEDS] = {
    27, 28, 29, 23, 22, 21, 18, 19, 20, 2, 1, 0,
    26, 25, 24, 15, 16, 17, 5, 4, 3,
    12, 13, 14, 8, 7, 6,
    11, 10, 9
  };

  mirror_pixels(CHANNEL_A);
  mirror_pixels(CHANNEL_B);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = mask(led[CHANNEL_B].getInterpFrameColor(rotate_pixel(i, show_variables[2 + CHANNEL_B])), i, CHANNEL_B);
    CHSV color_a = mask(led[CHANNEL_A].getInterpFrameColor(rotate_pixel(i, show_variables[2 + CHANNEL_A])), i, CHANNEL_A);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    
    color = narrow_palette(color);
    
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }

    leds[LED_LOOKUP[i]] = color;
    led_buffer[i] = color;
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
  
  // 2-fold (1), 3-fold (2), 6-fold (3), None
  if (symmetry == 1 || symmetry == 3) {  // Vertical mirroring
    for (uint8_t x = 0; x < SIZE; x++) {
      for (uint8_t y = 0; y < SIZE - x; y++) {
        for (uint8_t z = 0 ; z < DIAMOND_NUMBER; z++) {
          if (y >= x) {
            uint8_t new_z = z;
            if (z != 1) {
              new_z = (z + 2) % 4;
            }
            led[channel].setInterpFrame(get_pixel_from_coord(y, x, new_z), led[channel].getInterpFrameColor(get_pixel_from_coord(x, y, z)));
          }
        }
      }
    }
  }
  
  if (symmetry == 2 || symmetry == 3) {  // Rotational mirroring
    for (uint8_t x = 0; x < SIZE; x++) {
      for (uint8_t y = 0; y < SIZE - x; y++) {
        for (uint8_t z = 0 ; z < DIAMOND_NUMBER; z++) {
          if ((x + y <= 1) || (x == 1 && y == 1 && z == 0)) {
            uint8_t i = get_pixel_from_coord(x, y, z);
            CHSV color = led[channel].getInterpFrameColor(i);
            led[channel].setInterpFrame(rotate_pixel(i, 2), color);  // Does not work
            led[channel].setInterpFrame(rotate_pixel(i, 1), color);  // Does not work
          }
        }
      }
    }
  }
}

//
// rotate_pixel - rotate triangle grid 120-degrees for each "r"
//
uint8_t rotate_pixel(uint8_t i, uint8_t rotation) {
  if (rotation == 0) {
    return i;
  }
  uint8_t new_x, new_y;
  uint8_t x = get_x_from_pixel(i);
  uint8_t y = get_y_from_pixel(i);
  uint8_t z = get_z_from_pixel(i);
  
  for (uint8_t r = rotation; r > 0; r--) {
    new_y = x;
    new_x = SIZE - 1 - (x + y);
    x = new_x;
    y = new_y;
    z = (z - 1 + DIAMOND_NUMBER) % DIAMOND_NUMBER;
  }
  return get_pixel_from_coord(x, y, z);
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
// Get pixel from coordinate
//
uint8_t get_pixel_from_coord(uint8_t x, uint8_t y, uint8_t z) {
  uint8_t SPACING[] = { 0, 4, 7, 9 };
  return (((x + SPACING[y % 4]) * DIAMOND_NUMBER) + z) % NUM_LEDS;
}

uint8_t get_x_from_pixel(uint8_t i) {
  uint8_t SPACING[] = { 0, 4, 7, 9 };
  i /= DIAMOND_NUMBER;
  for (int8_t y = SIZE - 1; y >= 0; y--) {
    if (i >=  SPACING[y]) {
      return i - SPACING[y];
    }
  }
  return 0;
}

uint8_t get_y_from_pixel(uint8_t i) {
  uint8_t SPACING[] = { 0, 4, 7, 9 };
  i /= DIAMOND_NUMBER;
  for (int8_t y = SIZE - 1; y >= 0; y--) {
    if (i >=  SPACING[y]) {
      return y;
    }
  }
  return 0;
}

uint8_t get_z_from_pixel(uint8_t i) {
  return i % DIAMOND_NUMBER;
}

//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t PatternMatrix[] = {
    0x24, 0x92, 0x49, 0x24,
    0xa4, 0xb9, 0xd4, 0x58,
    0x51, 0x43, 0xca, 0x84,
    0xaa, 0x35, 0xe1, 0x38,
    0xe, 0x81, 0xc3, 0xbc,
    0xd, 0x1, 0x2b, 0x40,
    0xf1, 0x73, 0x8b, 0xa0,
    0x9c, 0xc9, 0x14, 0xf0,
    0xb0, 0x53, 0x60, 0x34,
    0x51, 0x4c, 0x32, 0x98
  }; 
  pattern_number = pattern_number % NUM_PATTERNS;
  uint8_t pattern_byte = PatternMatrix[(pattern_number * 4) + (n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

uint8_t lookup_complex_pattern_table(uint8_t n, uint8_t complex_number) {
  // See iPython Notebook: Pattern Maker QBert
  
  uint8_t ComplexMatrix[] = {
    9,0,8,3,4,7,0,0,6,0,4,5,10,1,2,6,11,5,0,3,0,11,8,7,9,2,10,12,1,0,
    1,0,1,0,0,1,0,0,1,0,1,1,1,0,0,1,1,1,0,1,0,1,0,0,0,1,0,1,1,0,
    0,1,0,1,0,0,0,1,0,1,0,0,0,0,1,1,1,1,0,0,1,0,1,0,1,0,0,0,0,1,
    1,0,1,0,1,0,1,0,0,0,1,1,0,1,0,1,1,1,1,0,0,0,0,1,0,0,1,1,1,0,
    0,0,0,0,1,1,1,0,1,0,0,0,0,0,0,1,1,1,0,0,0,0,1,1,1,0,1,1,1,1,
    0,0,0,0,1,1,0,1,0,0,0,0,0,0,0,1,0,0,1,0,1,0,1,1,0,1,0,0,0,0,
    1,1,1,1,0,0,0,1,0,1,1,1,0,0,1,1,1,0,0,0,1,0,1,1,1,0,1,0,0,0,
    1,0,0,1,1,1,0,0,1,1,0,0,1,0,0,1,0,0,0,1,0,1,0,0,1,1,1,1,0,0,
    1,0,1,1,0,0,0,0,0,1,0,1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,1,1,0,1,
    0,1,0,1,0,0,0,1,0,1,0,0,1,1,0,0,0,0,1,1,0,0,1,0,1,0,0,1,1,0
  };
  return ComplexMatrix[((complex_number % NUM_COMPLEX_PATTERNS) * NUM_LEDS) + n];
}

uint8_t get_complex_max(uint8_t complex_number) {
  uint8_t complex_max[] = { 12, 8, 12, 3, 15, 9 };
  return complex_max[complex_number % NUM_COMPLEX_PATTERNS];
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
  Serial.print(", Mask: ");
  Serial.print(show_variables[i + 6]);
  Serial.print(", Symmetry: ");
  Serial.print(show_variables[i + 4]);
  Serial.println(".");
}
