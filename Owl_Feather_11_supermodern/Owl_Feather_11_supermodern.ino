#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

#define FASTLED_ALLOW_INTERRUPTS 0

//// START HERE IN JULY
//
//  1. Figure out how to x,y coord and map an Owl within this Arduino code
//  2. Look at the Triceratops_Feather_5_supermodern for examples
//  3. Mash up the previous 22 owl/Dragon shows with supermodern shows

//
//  Owl - Huge port off of Dragon library code
//
//  7/17/23
//
//  On the Adafruit Feather - Modern Software
//
//  MESH network! No phone connection
//
//  Speaking & Hearing
//
#define NUM_LEDS 70
#define ACTUAL_LEDS 465

#define NUM_HEAD_PARTS 4
#define ACTUAL_HEAD_LEDS 105

#define DATA_PIN        0  //  11 // 7 // data = 7 (Teensy) or 0 (Feather)
#define DATA_PIN_HEAD   2  //  8
// #define CLOCK_PIN 2  //  13 // 8 // clock = 8 (Teensy) or 2 (Feather)

uint8_t bright = 32;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 196;  // (0-255)

#define MAX_LUMENS 10000  // estimated maximum brightness until flickering

#define DELAY_TIME 15  // in milliseconds (20ms=50fps, 25ms=40fps)

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

// Smoothing constants - lower is slower smoothing
// At 15 ms cycles: 40 = 96 ms, 30 = 127 ms, 20 = 191 ms, 10 = 382 ms, 5 = 765 ms, 2 = 1.9 s, 1 = 3.8 s
#define SMOOTHING_SHOWS_HUE    20   // Fastest full rainbow = DELAY_TIME * (255 / this value)
#define SMOOTHING_SHOWS_VALUE  20   // Fastest turn off/on = DELAY_TIME * (255 / this value)

// body
Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // Required for smoothing
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

// head
Led head_led[] = { Led(NUM_HEAD_PARTS), Led(NUM_HEAD_PARTS) };  // No Shows object for the head
CHSV head_led_buffer[NUM_HEAD_PARTS];  // For smoothing
CRGB head_leds[ACTUAL_HEAD_LEDS];  // Hook for FastLED library

#define ONLY_RED true
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
CHSV BLACK = CHSV(0, 0, 0);

// Shows
#define START_SHOW_CHANNEL_A   14  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B   0  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 18

// Head Shows
#define START_HEAD_SHOW_CHANNEL_A  6  // Channels A starting show
#define START_HEAD_SHOW_CHANNEL_B  8  // Channels B starting show
uint8_t head_current_show[] = { START_HEAD_SHOW_CHANNEL_A, START_HEAD_SHOW_CHANNEL_B };
#define NUM_HEAD_SHOWS 12

#define NUM_PATTERNS 6
uint8_t current_pattern[] = { 0, 0 };

// Clocks and time
uint8_t show_duration = 40;  // Typically 30 seconds. Size problems at 1800+ seconds.
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

#define ARE_CONNECTED false  // Are the Fish talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = 0;
#define MAX_SILENT_TIME  10000  // Time (in cycles) without communication before marked as is_lost

//#define MESSAGE_REPEAT 3   // send this many duplicate messages
#define MESSAGE_SPACING 4   // wait this many DELAY_TIME (7 x 15 ms = every 0.105 s)

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

//// Start Mapping
//
//

//
// column table: pre_column_spacers, y_start, column_height, starting led
//

#define MAX_COLUMNS 10  // min_y = 0, max_y = MAX_COLUMNS
#define MAX_HEIGHT   8  // min_x = 0, max_x = MAX_COLUMNS

#define SPACER_FIELD 0  // spacer leds before the column starts
#define Y_START_FIELD 1
#define COLUMN_HEIGHT_FIELD 2
#define COLUMN_PIXEL_START_FIELD 3

int8_t map_table[] PROGMEM = {
  0, 1, 6,  0,  //  0
  7, 1, 7,  6,  //  1
  6, 0, 8, 13,  //  2
  6, 1, 7, 21,  //  3
  6, 0, 8, 28,  //  4
  6, 1, 7, 36,  //  5
  6, 0, 8, 43,  //  6
  6, 1, 7, 51,  //  7
  7, 1, 6, 58,  //  8
  7, 2, 5, 64,  //  9
};

// edge, beak, eye1, eye2
// 0 = black, 1 = forecolor, 2 = backcolor
uint8_t head_show_table[] PROGMEM = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 2, 1, 1,
  1, 2, 1, 1,
  0, 0, 1, 2,
  1, 2, 2, 2,
  1, 2, 0, 2,
  0, 0, 1, 1,
};

uint8_t column_full_lookup[] PROGMEM = { 6, 6, 3, 6, 3, 6, 3, 6, 6, 6 };

//// Hex geometry
#define PIXEL_HEIGHT 1
#define PIXEL_WIDTH 0.866
#define CART_WIDTH  (256.0 / MAX_COLUMNS)
#define CART_HEIGHT (CART_WIDTH / PIXEL_WIDTH)

#define XX  255

uint8_t neighbors[] PROGMEM = {
  XX,1,7,6,XX,XX, // 0
  XX,2,8,7,0,XX,
  XX,3,9,8,1,XX,
  XX,4,10,9,2,XX,
  XX,5,11,10,3,XX, // 4
  XX,XX,12,11,4,XX,
  0,7,14,13,XX,XX,
  1,8,15,14,6,0,
  2,9,16,15,7,1, // 8
  3,10,17,16,8,2,
  4,11,18,17,9,3,
  5,12,19,18,10,4,
  XX,XX,20,19,11,5, // 12
  6,14,21,XX,XX,XX,
  7,15,22,21,13,6,
  8,16,23,22,14,7,
  9,17,24,23,15,8, // 16
  10,18,25,24,16,9,
  11,19,26,25,17,10,
  12,20,27,26,18,11,
  XX,XX,XX,27,19,12, // 20
  14,22,29,28,XX,13,
  15,23,30,29,21,14,
  16,24,31,30,22,15,
  17,25,32,31,23,16, // 24
  18,26,33,32,24,17,
  19,27,34,33,25,18,
  20,XX,35,34,26,19,
  21,29,36,XX,XX,XX, // 28
  22,30,37,36,28,21,
  23,31,38,37,29,22,
  24,32,39,38,30,23,
  25,33,40,39,31,24, // 32
  26,34,41,40,32,25,
  27,35,42,41,33,26,
  XX,XX,XX,42,34,27,
  29,37,44,43,XX,28, // 36
  30,38,45,44,36,29,
  31,39,46,45,37,30,
  32,40,47,46,38,31,
  33,41,48,47,39,32, // 40
  34,42,49,48,40,33,
  35,XX,50,49,41,34,
  36,44,51,XX,XX,XX,
  37,45,52,51,43,36, // 44
  38,46,53,52,44,37,
  39,47,54,53,45,38,
  40,48,55,54,46,39,
  41,49,56,55,47,40, // 48
  42,50,57,56,48,41,
  XX,XX,XX,57,49,42,
  44,52,58,XX,XX,43,
  45,53,59,58,51,44, // 52
  46,54,60,59,52,45,
  47,55,61,60,53,46,
  48,56,62,61,54,47,
  49,57,63,62,55,48, // 56
  50,XX,XX,63,56,49,
  52,59,64,XX,XX,51,
  53,60,65,64,58,52,
  54,61,66,65,59,53, // 60
  55,62,67,66,60,54,
  56,63,68,67,61,55,
  57,XX,XX,68,62,56,
  59,65,XX,XX,XX,58, // 64
  60,66,XX,XX,64,59,
  61,67,XX,XX,65,60,
  62,68,XX,XX,66,61,
  63,XX,XX,XX,67,62, // 68
};

//
//
//// End Mapping


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  pinMode(DATA_PIN_HEAD, OUTPUT);

  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");

  FastLED.addLeds<WS2812B, DATA_PIN, GBR>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<WS2812B, DATA_PIN_HEAD, GBR>(head_leds, ACTUAL_HEAD_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );
  
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    shows[i] = Shows(&led[i], i);  // Show library - reinitialized for led mappings
    shows[i].setAsHexagon();
    shows[i].setShowDuration(show_duration);
    shows[i].setShowSpeed(show_speed);
    shows[i].setDelayTime(DELAY_TIME);
    shows[i].setFadeAmount(fade_amount);
    shows[i].setColorSpeedMinMax();
    shows[i].pickCycleDuration();
    shows[i].fillForeBlack();
    led[i].push_frame();

    head_led[i].fillBlack();
    head_led[i].push_frame();
  }

  shows[CHANNEL_B].setStartShowTime(millis() - shows[CHANNEL_A].getElapsedShowTime() - (show_duration * 1000 / 2));

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }
  
  if (ONLY_RED) {
    hue_center = 0;
    hue_width = 128;
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
    run_shows(i);
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette

    head_run_show(i);  // similar to run_shows()
    head_led[i].smooth_frame(255 / shows[i].getCyclesPerFrame());  // similar to shows[i].morphFrame()
  }
  morph_channels();  // morph together the 2 leds channels and deposit on to Channel_A
  morph_head_channels();
  advance_clocks();  // advance the cycle clocks and check for next show
}

//
// run shows - Now its own function
//
void run_shows(uint8_t i) {
      
  switch (current_show[i]) {
  
    case 0:
      shows[i].allOn();  // Check why these changes so much
      break;
    case 1:
      shows[i].randomFlip();
      break;
    case 2:
      shows[i].twoColor();
      break;
    case 3:
      shows[i].stripes();
      break;
    case 4:
      shows[i].randomColors();
      break;
    case 5:
      shows[i].randomTwoColorBlack();
      break;
    case 6:
      shows[i].randomOneColorBlack();
      break;
    case 7:
      shows[i].morphChain();  // flashy
      break;
    case 8:
      shows[i].sawTooth(); // weird color interpolation
      break;
    case 9:
     shows[i].confetti();
      break;
    case 10:
      shows[i].lightWave();
      break;
    case 11:
      shows[i].lightRunUp();
      break;
    case 12:
      shows[i].bounce();
      break;
    case 13:
      shows[i].bounceGlowing();
      break;
    case 14:
      shows[i].plinko(3);
      break;
    case 15:
      shows[i].sinelon_fastled();
      break;
    case 16:
      shows[i].bpm_fastled();
      break;
    default:
      shows[i].juggle_fastled();
      break;
  }
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    
    if (shows[i].getElapsedCycleTime() > shows[i].getCycleDuration()) {
      head_led[i].push_frame();
    }
    
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
  current_pattern[i] = random8(NUM_PATTERNS);
  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
//  current_show[i] = random8(NUM_SHOWS);
  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  head_current_show[i] = random8(NUM_HEAD_SHOWS);
  
  shows[i].pickCycleDuration();
  shows[i].pickRandomColorSpeeds();
  shows[i].fillBlack();
}

boolean is_other_channel_show_zero(uint8_t c) {
  if (c == CHANNEL_A) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}


////// Speaking and Hearing
//
//

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
  jsonReadings["p"] = current_pattern[c];
  jsonReadings["hs"] = head_current_show[c];
  
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
  current_pattern[c] = int(myObject["p"]);

  int show_number = int(myObject["s"]);
  int head_show_number = int(myObject["hs"]);
  if (current_show[c] != show_number) {
    current_show[c] = show_number % NUM_SHOWS;
    head_current_show[c] = head_show_number % NUM_HEAD_SHOWS;
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


///////// DUAL SHOW LOGIC


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  boolean update_leds = false;
  boolean did_led_update;
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B
  uint8_t column_height, edge_width, pixel_width, start_column_feather;
  uint8_t feather_number = 0;  // which pixel we are on
  uint16_t led_number = 0;    // which led we are on (include spacers)
  uint8_t adj_feather_number;

  for (uint8_t column = 0; column < MAX_COLUMNS; column++) {  /// Iterate one column at a time
    
    // Turn off the spacer LEDS
    for (uint8_t spacer = 0; spacer < get_column_table_value(column, SPACER_FIELD); spacer++) {
      leds[led_number] = BLACK;
      led_number++;
    }

    column_height = get_column_table_value(column, COLUMN_HEIGHT_FIELD);
    edge_width = pgm_read_byte_near(column_full_lookup + column);
    start_column_feather = feather_number;

    for (uint8_t feather = 0; feather < column_height; feather++) {
      // serpentine reverse column
      adj_feather_number = (column % 2 == 0) ? feather_number : (start_column_feather + column_height - feather - 1) ;
      pixel_width = 6;  // default
      if (edge_width == 3 && (feather == 0 || feather == column_height - 1)) {
        pixel_width = 3;
      }
      did_led_update = morph_channels_for_pixel(adj_feather_number, led_number, pixel_width, fract);
      if (did_led_update) {
        update_leds = true;
      }
      feather_number++;
      led_number += pixel_width;
    }
  }
  
  if (update_leds) { 
    FastLED.show();
  }
}

bool morph_channels_for_pixel(uint8_t f, uint16_t led_number, uint8_t feather_size, uint8_t fract) {
  // New! Changed Interp to Next. No more Interp frames
  CHSV color = led[CHANNEL_A].getInterpHSV(led[CHANNEL_B].getCurrFrameColor(f), 
                                           led[CHANNEL_A].getCurrFrameColor(f), 
                                           fract);  // interpolate a + b channels
  color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
    
  // smoothing backstop. smooth constants should be large.
  color = led[CHANNEL_A].smooth_color(led_buffer[f], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);

  if (led_buffer[f].h == color.h && led_buffer[f].s == color.s && led_buffer[f].v == color.v) {
    return false;
  } else {
    led_buffer[f] = color;
    for (uint16_t i = led_number; i < led_number + feather_size ; i++) {
      leds[i] = color;
    }
    return true;
  }
}

//
// head_run_show - roll-you-own shows object for owl head to call the head_led canned object
//
void head_run_show(uint8_t c) {
  for (uint8_t i = 0; i < NUM_HEAD_PARTS; i++) {
    switch(pgm_read_byte_near(head_show_table + (head_current_show[c] * NUM_HEAD_PARTS) + i)) {
      case 0:
        head_led[c].setPixelBlack(i);
        break;
      case 1:
        head_led[c].setPixelHue(i, shows[c].getForeColor());
        break;
      case 2:
        head_led[c].setPixelHue(i, shows[c].getBackColor());
        break;
    }
  }
}

//
// morph_head_channels - morph together the 2 chanels and update the LEDs
//
void morph_head_channels() {
  uint8_t head_section_led_start[NUM_HEAD_PARTS] = { 0, 84, 65, 92 };  // edge, beak, eye1, eye2
  uint8_t head_section_led_size[NUM_HEAD_PARTS] = { 61, 6, 13, 13 };

  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B
  
  for (uint8_t i = 0; i < NUM_HEAD_PARTS; i++) {
    CHSV color = head_led[CHANNEL_A].getInterpHSV(head_led[CHANNEL_B].getCurrFrameColor(i), 
                                                  head_led[CHANNEL_A].getCurrFrameColor(i), 
                                                  fract);  // interpolate a + b channels
    color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
    color = led[CHANNEL_A].smooth_color(head_led_buffer[i], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);
    head_led_buffer[i] = color;

    for (uint8_t j = head_section_led_start[i]; j < head_section_led_start[i] + head_section_led_size[i]; j++) {
      head_leds[j] = color;
    }
    if (i == 0) {  // head has two spacer regions on either side of the beak
      for (uint8_t j = 78; j < 84; j++) {
        head_leds[j] = color;
      }
      for (uint8_t j = 90; j < 92; j++) {
        head_leds[j] = color;
      }
    }
  }
}

//// End DUAL SHOW LOGIC


//// Mapping

int8_t get_column_table_value(uint8_t column, uint8_t field_index) {
  return pgm_read_byte_near(map_table + (column * 4) + field_index);  // 4 fields per column
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
