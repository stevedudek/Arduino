#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Lightbrella with 8 spokes
//
//  6/16/23
//
//  On the Adafruit Feather - Modern Software
//
//  MESH network! No phone connection
//
//  Speaking & Hearing
//
#define SPOKE_LEDS 25
#define NUM_SPOKES 8
#define NUM_LEDS (SPOKE_LEDS * NUM_SPOKES)  

#define DATA_PIN 0  //  11 // 7 // data = 7 (Teensy) or 0 (Feather)
// #define CLOCK_PIN 2  //  13 // 8 // clock = 8 (Teensy) or 2 (Feather)

uint8_t bright = 96;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0-255)

#define MAX_LUMENS 10000  // estimated maximum brightness until flickering

#define DELAY_TIME 15  // in milliseconds (20ms=50fps, 25ms=40fps)

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

// Smoothing constants - lower is slower smoothing
// At 15 ms cycles: 40 = 96 ms, 30 = 127 ms, 20 = 191 ms, 10 = 382 ms, 5 = 765 ms, 2 = 1.9 s, 1 = 3.8 s
#define SMOOTHING_SHOWS_HUE    20   // Fastest full rainbow = DELAY_TIME * (255 / this value)
#define SMOOTHING_SHOWS_VALUE  20   // Fastest turn off/on = DELAY_TIME * (255 / this value)

#define BLACK  CHSV(0, 0, 0)

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // Required for smoothing
CRGB leds[NUM_LEDS];  // The Leds themselves

uint8_t param[] = { 0, 0 };  // 1-byte show memory

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

// Shows
#define START_SHOW_CHANNEL_A   0  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B   0  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 17

// Clocks and time
uint8_t show_duration = 15;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 0;  // 0 = no fading, to 255 = always be fading

#define NUM_SYMMETRIES 7
uint8_t show_symmetry[] = { 0, 0 };
uint8_t spike_remap[] = { 0, 2, 4, 6, 7, 5, 3, 1 };

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

#define ARE_CONNECTED false  // Are the Fish talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = 0;
#define MAX_SILENT_TIME  10000  // Time (in cycles) without communication before marked as is_lost


#define MESSAGE_REPEAT 3   // send this many duplicate messages
#define MESSAGE_SPACING 5   // wait this many cycles

painlessMesh mesh;
String message;  // String to send to other displays
String getReadings();  // Prototype for reading state of LEDs

#define IR_MESSAGE  0
#define MESH_MESSAGE  1

#define EMPTY_COMMAND  0
#define BRIGHTNESS_COMMAND  1
#define HUE_COMMAND  2
#define HUE_WIDTH_COMMAND  3
#define SPEED_COMMAND  4
#define SHOW_DURATION_COMMAND  5
#define FADE_COMMAND 6
#define SATURATION_COMMAND 7


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
//  pinMode(POT_PIN, INPUT);

  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");
  
  FastLED.addLeds<WS2812B, DATA_PIN, BRG>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
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

  shows[CHANNEL_B].setSmallCycleHalfway();  // Start Channel B offset at halfway through show

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }
  
  if (ONLY_RED) {
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

//
// update leds - Moved to a task-scheduled event
//
void updateLeds() {
  
  for (uint8_t i = 0; i < DUAL; i++) {
      
    switch (current_show[i]) {

      case 0:
        dim_fall(i, false);  // good - jerky
        break;
      case 1:
        dim_fall(i, true);  // good - jerky
        break;
      case 2:
        pendulum_wave(i, false);  // good
        break;
      case 3:
        morphChain(i, false);  // way too many colors. Sux
        break;
      case 4:
        morphChain(i, true);  // way too many colors. Sux
        break;
      case 5:
        shows[i].confetti();  // good
        break;
      case 6:
        juggle_fastled(i);  // good
        break;
      case 7:
        windmill(i);  // good
        break;
      case 8:
        circle_wave(i);  // good
        break;
      case 9:
        verticle_wipe(i);  // good
        break;
      case 10:
        shows[i].randomFlip();  // not enough flips
        break;
      case 11:
        shows[i].twoColor();
        break;
      case 12:
        shows[i].stripes();
        break;
      case 13:
        shows[i].sawTooth();  // okay
        break;
      case 14:
        shows[i].sinelon_fastled();  // good - too many colors
        break;
      case 15:
        shows[i].bpm_fastled();  // good
        break;
      default:
        rotating_blade(i);  // goodd
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
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
    if (shows[i].hasShowFinished()) {
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
  shows[i].resetAllClocks();
  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
//  current_show[i] = random8(NUM_SHOWS);
  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
  shows[i].fillBlack();
}


////// Speaking and Hearing

void sendParameterMessage (uint8_t param, uint8_t value) {
  JSONVar jsonReadings;

  jsonReadings["type"] = IR_MESSAGE;
  jsonReadings["param1"] = param;
  jsonReadings["value1"] = value;
  jsonReadings["param2"] = EMPTY_COMMAND;
  jsonReadings["value2"] = 0;

  message = JSON.stringify(jsonReadings);
  mesh.sendBroadcast(message);
//  Serial.println(message);
}

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
  
  message = JSON.stringify(jsonReadings);
  return message;
}

void sendMessage (uint8_t c) {
  String msg = getReadings(c);
  mesh.sendBroadcast(msg);
//  Serial.println(message);
}

void receivedCallback( uint32_t from, String &msg ) {
//  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  JSONVar myObject = JSON.parse(msg.c_str());

  switch (int(myObject["type"])) {
    case IR_MESSAGE:
      processIrMessage(myObject);
      break;
    default:
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

  int show_number = int(myObject["s"]);
  if (current_show[c] != show_number) {
    current_show[c] = show_number % NUM_SHOWS;
    next_show(c);
  }
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
  // Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  // Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  // Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

////// End Speaking & Hearing


////// Specialized shows

//
// dim_fall - single pixel moving down the chain
//
void dim_fall(uint8_t c, boolean reversed) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    param[c] = random(1, 10);
    shows[c].pickRandomCycleDuration(30, 500);
  }
  shows[c].dimAllPixels(param[c]);  // Try different amounts
  
  if (shows[c].isCycleStart()) {
    CHSV color = led[c].wheel(shows[c].getForeColor());
    mirror_spokes(shows[c].getCycle() % SPOKE_LEDS, color, reversed, c);
  }
}

void spoke_test(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }
  for (uint8_t i=0; i < NUM_SPOKES; i++) {
    if (i % 2) {
      color_spoke(i, CHSV(0, 255, 255), c);
    } else {
      color_spoke(i, BLACK, c);
    }
  }
}

void rotating_blade(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    param[c] = random(1, 10);
    shows[c].pickRandomCycleDuration(30, 500);
  }
  shows[c].dimAllPixels(param[c]);  // Try different amounts
  
  if (shows[c].isCycleStart()) {
    CHSV color = led[c].wheel(shows[c].getForeColor());  
    color_spoke(shows[c].getCycle() % 8, color, c);
  }
}

void juggle_fastled(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();
  }
  // eight colored dots, weaving in and out of sync with each other
  uint8_t pixel;
  uint8_t dot_hue = 0;
  CHSV curr_color, new_color;

  shows[c].dimAllPixels(4); // replaces: fadeToBlackBy( leds, NUM_LEDS, 20);

  for(uint8_t i = 0; i < NUM_SPOKES; i++) {
    pixel = beatsin16(i, 0, SPOKE_LEDS);
    curr_color = led[c].getCurrFrameColor(i);
    new_color = CHSV(curr_color.h | dot_hue, curr_color.s | 200, curr_color.v | 255);
    mirror_spokes(pixel, new_color, false, c);
    dot_hue += 32;
  }
}

//
// Pendulum Wave
//
void pendulum_wave(uint8_t i, boolean smoothed) {
  uint8_t value, pos, dist;
  
  if (shows[i].isShowStart()) {
    param[i] = random(40, 60);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t y = 0; y < 5; y++) {
    value = beatsin8(param[i] - y);
    for (uint8_t x = 0; x < SPOKE_LEDS; x++) {
      pos = (x * 256 / SPOKE_LEDS);
      dist = (pos >= value) ? 255 - (pos - value) : 255 - (value - pos) ;
      if (smoothed == false) {
        dist = smooth_value(dist);
      }
      mirror_circle(x, y, CHSV(shows[i].getForeColor(), 255, dist), i);
    }
  }
}

//
// Windmill
//
void windmill(uint8_t i) {
  if (shows[i].isShowStart()) {
    param[i] = random(40, 60);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < SPOKE_LEDS; x++) {
    for (uint8_t y = 0; y < 5; y++) {
      uint8_t value = beatsin8(param[i] - (4 * y), 0, 255, 0, x * 128 / SPOKE_LEDS );
      mirror_circle(x, y, CHSV(shows[i].getForeColor(), 255, smooth_value(value)), i);
    }
  }
}

void circle_wave(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();
  }
  
  for (uint8_t s = 0; s < 5; s++) {
    for (uint8_t i = 0; i < SPOKE_LEDS; i++) {
      uint8_t value = beatsin8(10, 0, 255, 0, (i * 255 / SPOKE_LEDS) + (s * 255 / 5));
      mirror_circle(i, s, led[c].gradient_wheel(shows[c].getForeColor(), smooth_value(value)), c);
    }
  }
}

void morphChain(uint8_t c, boolean reversed) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(40, 400);
  }
  if (shows[c].isCycleStart()) {
    for (uint8_t i = 0; i < SPOKE_LEDS; i++) {
      uint8_t color = map8(sin8_C(((i / 10) + (shows[c].getCycle()) * 20)), 0, sin8_C(shows[c].getBackColor()));
      mirror_spokes(i, led[c].wheel(color), reversed, c);
    }
  }
}

uint8_t smooth_value(uint8_t value) {
  uint8_t smooth_min = 230;
  value = (value > smooth_min) ? map(value, smooth_min, 255, 0, 255) : 0 ;  // keep the top of the wave
  return value;
}

///////// DUAL SHOW LOGIC

//
// mirror_spokes
//
void mirror_spokes(uint8_t i, CHSV color, boolean reversed, uint8_t c) {
  i = i % SPOKE_LEDS;
  for (uint8_t s = 0; s < 8; s++) {
    if (s % 2 && reversed) {
      shows[c].setPixeltoColor(((s * SPOKE_LEDS) + (SPOKE_LEDS - i - 1)) % NUM_LEDS, color);
    } else {
      shows[c].setPixeltoColor(((s * SPOKE_LEDS) + i) % NUM_LEDS, color);
    }
  }
}

void color_spoke(uint8_t s, CHSV color, uint8_t c) {
  for (uint16_t i = s * SPOKE_LEDS; i < (s+1) * SPOKE_LEDS; i++) {
    shows[c].setPixeltoColor(i, color);
  }
}

void set_led_color(uint8_t s, uint8_t r, CHSV color, uint8_t c) {
  shows[c].setPixeltoColor((s * SPOKE_LEDS) + r, color);
}

//
// mirror_circle
//
void mirror_circle(uint8_t x, uint8_t y, CHSV color, uint8_t c) {
  if (y < 5) {
    shows[c].setPixeltoColor(((y * SPOKE_LEDS) + x) % NUM_LEDS, color);
    if (y > 0 || y < 4) {
       shows[c].setPixeltoColor((((NUM_SPOKES - y) * SPOKE_LEDS) + x) % NUM_LEDS, color);
    }
  }
}

//
// verticle_wipe
//
void verticle_wipe(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();
  }
  uint8_t value = beatsin8(10);
  for (uint8_t s = 0; s < NUM_SPOKES; s++) {
    for (uint8_t r = 0; r < SPOKE_LEDS; r++) {
      set_led_color(s, r, led[c].gradient_wheel(shows[c].getForeColor(), smooth_value(255 - abs(value - get_x_coord(s, r)))), c);
    }
  }
}

//
// get_cartesian_coord
//
uint8_t get_x_coord(uint8_t s, uint8_t r) {
  return ((r * 128 / SPOKE_LEDS) * sin(s * 6.28 / NUM_SPOKES)) + 127;  // Not sure this works!
}

uint8_t get_y_coord(uint8_t s, uint8_t r) {
  return ((r * 128 / SPOKE_LEDS) * cos(s * 6.28 / NUM_SPOKES)) + 127;  // Not sure this works!
}

uint8_t get_x_coord_from_i(uint8_t i) {
  uint8_t r = i % SPOKE_LEDS;
  uint8_t s = i / SPOKE_LEDS;
  return ((r * 128 / SPOKE_LEDS) * sin(s * 6.28 / NUM_SPOKES)) + 127;  // Not sure this works!
}

uint8_t get_y_coord_from_i(uint8_t i) {
  uint8_t r = i % SPOKE_LEDS;
  uint8_t s = i / SPOKE_LEDS;
  return ((r * 128 / SPOKE_LEDS) * cos(s * 6.28 / NUM_SPOKES)) + 127;  // Not sure this works!
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  boolean update_leds = false;
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // New! Changed Interp to Next. No more Interp frames
    CHSV color_b = led[CHANNEL_B].getCurrFrameColor(i);   
    CHSV color_a = led[CHANNEL_A].getCurrFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
    
    // smoothing backstop. smooth constants should be large.
    color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);

    if (led_buffer[i].h == color.h && led_buffer[i].s == color.s && led_buffer[i].v == color.v) {
      continue;
    } else {
      leds[i] = color;   // put HSV color on to LEDs
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
      update_leds = true;
    }
  }

  if (update_leds) { 
    adj_brightess();
    FastLED.show();
  }
}

//
// adjust brightness - put a cap on maximum brightness to prevent flickering
//
void adj_brightess() {
  uint8_t brightness_step_size = 2;
  uint32_t lumens = 0;

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
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
// log status
//
void log_status(uint8_t i) {
  Serial.print("Channel: ");
  Serial.print(i);
  Serial.print(", Show: ");
  Serial.print(current_show[i]);
  Serial.println(".");
}
