#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Fish
//
//  10/1/21
//
//  On the Adafruit Feather
//
//  MESH network!
//
//  Speaking & Hearing
//
#define FISH_NUM   0
#define MAX_FISH   2

uint8_t brightness = 255;  // (0-255)
uint8_t show_speed = 128;  // (0-255)

#define DELAY_TIME 20  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 0  // data = 7 (Teensy) or 0 (Feather)
#define CLOCK_PIN 2  // clock = 8 (Teensy) or 2 (Feather)

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define NUM_LEDS 61
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number
#define TOTAL_LEDS (MAX_FISH * NUM_LEDS)
#define SIMPLE_TAIL false  // simple tails have 5 segments

#define MAX_XCOORD  7
#define MAX_YCOORD 11

#define XX  255

// Fish Grid - Each fish is 11 wide x 7 tall (must be the same for all fish)
//    UNTESTED
#define FISH_XGRID  0  // x-coord (upper left)
#define FISH_YGRID  0  // y-coord (upper left)
#define MAX_XGRID  MAX_XCOORD  // 7 has all fish in a line (not stacked)
#define MAX_YGRID  MAX_YCOORD  // 66 would have all fish in a line

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
#define FISH_HUE (FISH_NUM * (FISH_NUM / MAX_COLOR)) // Main fish color
#define BLACK  CHSV(0, 0, 0)

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // The Leds themselves
CHSV led_buffer[NUM_LEDS];  // For smoothing

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

// Shows
#define START_SHOW_CHANNEL_A  1  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B  0  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 1 };
#define NUM_SHOWS 20

// Clocks and time
uint8_t show_duration = 30;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 255;  // 0 = no fading, to 255 = always be fading
uint32_t max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

#define ARE_CONNECTED false  // Are the Fish talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = 0;
#define MAX_SILENT_TIME  10000  // Time (in cycles) without communication before marked as is_lost

String message;  // String to send to other displays

//#define MSG_FREQUENCY  50  // send message every X milliseconds
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
String getReadings();  // Prototype for reading state of LEDs

//Task taskSendMessage(MSG_FREQUENCY, TASK_FOREVER, &sendMessage);
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

// Lookup tables

uint8_t coords[] PROGMEM = {
  XX,XX, 1, 0,XX,XX,XX,  // 4 (1) - Head
  XX, 8, 7, 6, 5, 4,XX,  // 5
   9,10,11,12,13,14,XX,  // 6
  21,20,19,18,17,16,15,  // 7
  22,23,24,25,26,27,XX,  // 6
  34,33,32,31,30,29,28,  // 7
  35,36,37,38,39,40,XX,  // 6
  XX,45,44,43,42,41,XX,  // 5
  XX,46,47,48,49,XX,XX,  // 4
  XX,XX,52,51,50,XX,XX,  // 3
  XX,53,54,56,58,59,XX,  // 8 (5) - Tail
};

#define NUM_RINGS 6   // Total number of rings
#define NUM_PATTERNS 19   // Total number of patterns

//
// Pixel neighbors
//
// DIRECTIONS - If looking from head to tail
//
// 0 = Up, 1 = up-right, 2 = down-right, 3 = Down, 4 = down-left, 5 = up-left
//
uint8_t neighbors[] PROGMEM = {
  XX,6,7,XX,XX,XX, // 0 - Same as 1
  XX,6,7,XX,XX,XX, // 1
  XX,6,7,XX,XX,XX, // 2 - Same as 1
  XX,6,7,XX,XX,XX, // 3 - Same as 1
  XX,14,13,5,XX,XX, // 4
  4,13,12,6,XX,XX, // 5
  5,12,11,7,1,XX, // 6
  6,11,10,8,XX,1, // 7
  7,10,9,XX,XX,XX, // 8
  10,20,21,XX,XX,8, // 9
  11,19,20,9,8,7, // 10
  12,18,19,10,7,6, // 11
  13,17,18,11,6,5, // 12
  14,16,17,12,5,4, // 13
  XX,15,16,13,4,XX, // 14
  XX,XX,27,16,14,XX, // 15
  15,27,26,17,13,14, // 16
  16,26,25,18,12,13, // 17
  17,25,24,19,11,12, // 18
  18,24,23,20,10,11, // 19
  19,23,22,21,9,10, // 20
  20,22,XX,XX,XX,9, // 21
  23,33,34,XX,21,20, // 22
  24,32,33,22,20,19, // 23
  25,31,32,23,19,18, // 24
  26,30,31,24,18,17, // 25
  27,29,30,25,17,16, // 26
  XX,28,29,26,16,15, // 27
  XX,XX,40,29,27,XX, // 28
  28,40,39,30,26,27, // 29
  29,39,38,31,25,26, // 30
  30,38,37,32,24,25, // 31
  31,37,36,33,23,24, // 32
  32,36,35,34,22,23, // 33
  33,35,XX,XX,XX,22, // 34
  36,45,XX,XX,34,33, // 35
  37,44,45,35,33,32, // 36
  38,43,44,36,32,31, // 37
  39,42,43,37,31,30, // 38
  40,41,42,38,30,29, // 39
  XX,XX,41,39,29,28, // 40
  XX,XX,49,42,39,40, // 41
  41,49,48,43,38,39, // 42
  42,48,47,44,37,38, // 43
  43,47,46,45,36,37, // 44
  44,46,XX,XX,35,36, // 45
  47,52,XX,XX,45,44, // 46
  48,51,52,46,44,43, // 47
  49,50,51,47,43,42, // 48
  XX,XX,50,48,42,41, // 49
  XX,58,56,51,48,49, // 50
  50,56,54,52,47,48, // 51
  51,54,XX,XX,46,47, // 52
  60,XX,XX,XX,54,56, // 53
  56,57,55,XX,52,51, // 54
  57,XX,XX,53,XX,54, // 55
  58,59,57,54,51,50, // 56
  59,XX,XX,55,54,56, // 57
  XX,60,59,56,50,XX, // 58
  60,XX,XX,57,56,58, // 59
  XX,XX,XX,59,58,XX, // 60
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
  
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( brightness );

  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setCoordMap(MAX_YCOORD, coords);  // x,y grid of cones
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].setColorSpeedMinMax(show_speed);
    shows[i].setWaitRange(show_speed);
  }
  
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_start = 192;
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
    mesh.update() ;
  }
}

//
// update leds - Moved to a task-scheduled event
//
void updateLeds() {
  
  for (uint8_t i = 0; i < DUAL; i++) {
      
    switch (current_show[i]) {

      case 0:
        patternsBW(i);
        break;
      case 1:
        patterns2(i);
        break;
      case 2:
        patterns_four_color(i);
        break;
      case 3:
        warp1(i);
        break;
      case 4:
        warp2(i);
        break;
      case 5:
        saw_tooth(i);
        break;
      case 6:
        morph_chain(i);
        break;
      case 7:
        shows[i].bounce();
        break;
      case 8:
        shows[i].bounceGlowing();
        break;
      case 9:
        shows[i].plinko(1);  // 1 = starting pixel
        break;
      case 10:
        shows[i].bands();
        break;
      case 11:
        vert_back_forth_dots(i);
        break;
      case 12:
        vert_back_forth_bands(i);
        break;
      case 13:
        vert_back_forth_colors(i);
        break;
      case 14:
        horiz_back_forth_dots(i);
        break;
      case 15:
        horiz_back_forth_bands(i);
        break;
      case 16:
        horiz_back_forth_colors(i);
        break;
      case 17:
        diag_back_forth_dots(i);
        break;
      case 18:
        diag_back_forth_bands(i);
        break;
      default:
        diag_back_forth_colors(i);
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  unify_head_tail();
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
      if (!ARE_CONNECTED || is_lost || (ARE_CONNECTED && IS_SPEAKING)) {
        next_show(i);
        pick_next_show(i);
      }
    }
    if (ARE_CONNECTED) {
      if (IS_SPEAKING) {
        uint32_t smallCycle = shows[i].getSmallCycle();
        if ((smallCycle <= MESSAGE_REPEAT * MESSAGE_SPACING) && (smallCycle % MESSAGE_SPACING == 0)) {
          sendMessage(i);  // send message to mess if connected and speaking on particular small cycles
        }
      } else {
        if (last_connection++ > MAX_SILENT_TIME) {
          is_lost = true;
        }
      }
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
  // Switch between a patterns show and all the other shows
  current_pattern[i] = random8(NUM_PATTERNS);
  
  if (current_show[i] < 2) {
    current_show[i]++;
  } else if (current_show[i] == 2) {
    current_show[i] = random8(3, NUM_SHOWS);
  } else {
    current_show[i] = 0;
  }
  
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();

  // Set new show's colors to those of the other channel
  uint8_t other_channel = (i == 0) ? 1 : 0 ;
  shows[i].setForeColor(shows[other_channel].getForeColor());
  shows[i].setBackColor(shows[other_channel].getBackColor());
}


////// Speaking and Hearing

String getReadings (uint8_t c) {
  JSONVar jsonReadings;

  jsonReadings["type"] = MESH_MESSAGE;
  jsonReadings["c"] = c;
  jsonReadings["cycle"] = (const double)shows[c].getSmallCycle();
  jsonReadings["f"] = shows[c].getForeColor();
  jsonReadings["fspd"] = shows[c].getForeColorSpeed();
  jsonReadings["b"] = shows[c].getBackColor();
  jsonReadings["bspd"] = shows[c].getBackColorSpeed();
  jsonReadings["s"] = current_show[c];
  jsonReadings["w"] = shows[c].getWait();
  jsonReadings["p"] = current_pattern[c];

  message = JSON.stringify(jsonReadings);
  return message;
}

void sendMessage (uint8_t c) {
  String msg = getReadings(c);
  mesh.sendBroadcast(msg);
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
    current_pattern[c] = int(myObject["p"]);
    
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
      brightness = value;
      FastLED.setBrightness( brightness );
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

////// End Speaking & Hearing


////// Specialized shows

//
// saw_tooth - grided! not the standard: shows[i]sawTooth()
//
void saw_tooth(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].makeWaitFaster(4);  // Divide wait value by 4 (make much faster)
  }
  uint8_t i, adj_i;
  uint16_t fract;
  
  for (i = 0; i < NUM_LEDS; i++) {
    adj_i = i + (FISH_NUM * NUM_LEDS);
    fract = map((adj_i + shows[c].getCycle()) % TOTAL_LEDS, 0, TOTAL_LEDS, 0, 512);
    if (fract >= 256) {
      fract = 512 - fract;  // the subtraction creates the sawtooth
    }
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), fract));
  }
}

//
// morph_chain - grided! not the standard: shows[i].morphChain()
//
void morph_chain(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].makeWaitFaster(4);  // Divide wait value by 4 (make much faster)
  }
  uint8_t i, adj_i, fract;
  for (i = 0; i < NUM_LEDS; i++) {
    adj_i = i + (FISH_NUM * NUM_LEDS);
    fract = map((adj_i + shows[c].getCycle()) % TOTAL_LEDS, 0, TOTAL_LEDS, 0, 255);
    led[c].setPixelHue(i, led[c].interpolate_wrap(shows[c].getForeColor(), shows[c].getBackColor(), fract));
  }
}

//
// patterns shows
//
void patternsBW(uint8_t c) {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_pattern_number(i, current_pattern[c])) {
      shows[c].setPixeltoHue(i, FISH_HUE);
    } else {
      shows[c].setPixeltoForeBlack(i);
    }
  }
}

void patterns2(uint8_t c) {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_pattern_number(i, current_pattern[c])) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoForeBlack(i);
    }
  }
}

void patterns_four_color(uint8_t c) {
  for (int i = 0; i < NUM_LEDS; i++) {
    switch (get_two_bits_from_pattern_number(i, current_pattern[c])) {
      case 0:
        shows[c].setPixeltoBlack(i);
        break;
      case 1:
        shows[c].setPixeltoForeColor(i);
        break;
      case 2:
        shows[c].setPixeltoBackColor(i);
        break;
      default:
        shows[c].setPixeltoHue(i, FISH_HUE);
        break;
    }
  }
}

//
// vert back forth dots - vertical dots moving back and forth
//
void vert_back_forth_dots(uint8_t c) {
  uint8_t temp_x;
  uint16_t cycle = shows[c].getCycle();

  shows[c].fillForeBlack();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      if ((temp_x + cycle) % MAX_XGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
  shows[c].setPixeltoForeColor(1);  // Light up the head
}

//
// vert back forth bands - vertical bands moving back and forth
//
void vert_back_forth_bands(uint8_t c) {
  uint8_t temp_x, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_x = (temp_x + cycle) % MAX_XGRID;
      intensity = sin8(map(temp_x, 0, MAX_XGRID, 0, 255) + (cycle % (255 / MAX_XGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
  shows[c].setPixeltoBackColor(1);  // Light up the head
}

//
// vert back forth colors - vertical colors moving back and forth
//
void vert_back_forth_colors(uint8_t c) {
  uint8_t temp_x, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_x = x + FISH_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_x = (temp_x + cycle) % MAX_XGRID;
      hue = sin8(map(temp_x, 0, MAX_XGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_XGRID)) );
      led[c].setPixelHue(led[c].getLedFromCoord(x,y), hue);
    }
  }
}

//
// horiz back forth dots - horizontal dots moving back and forth
//
void horiz_back_forth_dots(uint8_t c) {
  uint8_t temp_y;
  uint16_t cycle = shows[c].getCycle();

  shows[c].fillForeBlack();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_y = y + FISH_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      if ((temp_y + cycle) % MAX_XGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
  shows[c].setPixeltoForeColor(1);  // Light up the head
}

//
// horiz back forth bands - horizontal bands moving back and forth
//
void horiz_back_forth_bands(uint8_t c) {
  uint8_t temp_y, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_y = y + FISH_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp_y = (temp_y + cycle) % MAX_YGRID;
      intensity = sin8(map(temp_y, 0, MAX_YGRID, 0, 255) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
  shows[c].setPixeltoForeColor(1);  // Light up the head
}

//
// horiz back forth colors - horizontal colors moving back and forth
//
void horiz_back_forth_colors(uint8_t c) {
  uint8_t temp_y, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_y = y + FISH_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp_y = (temp_y + cycle) % MAX_YGRID;
      hue = sin8(map(temp_y, 0, MAX_YGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelHue(led[c].getLedFromCoord(x,y), hue);
    }
  }
}

//
// diag back forth dots - diagonal dots moving back and forth
//
void diag_back_forth_dots(uint8_t c) {
  uint8_t temp_x, temp_y;
  uint16_t cycle = shows[c].getCycle();

  shows[c].fillForeBlack();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_y = y + FISH_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      if ((temp_x + temp_y + cycle) % MAX_YGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
  shows[c].setPixeltoForeColor(1);  // Light up the head
}

//
// diag back forth bands - diagonal bands moving back and forth
//
void diag_back_forth_bands(uint8_t c) {
  uint8_t temp_x, temp_y, temp, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_y = y + FISH_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp = (temp_x + temp_y + cycle) % MAX_YGRID;
      intensity = sin8(map(temp, 0, MAX_YGRID, 0, 255) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
  shows[c].setPixeltoBackColor(1);  // Light up the head
}

//
// diag back forth colors - diagonal colors moving back and forth
//
void diag_back_forth_colors(uint8_t c) {
  uint8_t temp_x, temp_y, temp, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_x = x + FISH_XGRID;
      temp_y = y + FISH_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp = (temp_x + temp_y + cycle) % MAX_YGRID;
      hue = sin8(map(temp, 0, MAX_YGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelHue(led[c].getLedFromCoord(x,y), hue);
    }
  }
}

//
// draw_ring
//
void draw_ring(uint8_t ring_n, CHSV color, uint8_t c) {  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_ring(i, ring_n % NUM_RINGS) == true) {
      led[c].setPixelColor(i, color);
    }
  }
}

//
// tunnel vision
//
// Colored ring animating outwards
// color1 is the primary color, color2 is a trail color
// background is the background color
//
void tunnelvision(CHSV color1, CHSV color2, CHSV background, uint8_t i) {  
  led[i].fill(background);
  uint8_t ring = shows[i].getCycle() % NUM_RINGS;
  draw_ring(ring, color1, i);
  if (ring != 0) {
    draw_ring(ring - 1, color2, i);
  }
}

//
// warp1 - colors on a black field
// 
void warp1(uint8_t i) {
  
  switch ((shows[i].getCycle() / NUM_RINGS) % 6) {
    case 0:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0, 255, 0)), led[i].rgb_to_hsv(CRGB(0,100,0)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    case 1:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,0,255)), led[i].rgb_to_hsv(CRGB(0,0,100)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    case 2:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,255,255)), led[i].rgb_to_hsv(CRGB(0,100,100)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    case 3:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,255,0)), led[i].rgb_to_hsv(CRGB(100,100,0)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);  
      break;
    case 4:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,0,0)), led[i].rgb_to_hsv(CRGB(100,0,0)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    default:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,0,255)), led[i].rgb_to_hsv(CRGB(100,0,100)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);  
      break;
  }
}

//
// warp2 - colors on a green field
//
void warp2(uint8_t i) {
  
  switch ((shows[i].getCycle() / NUM_RINGS) % 8) {
    case 0:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,255,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 1:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,200,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 2:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,150,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 3:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,100,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 4:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,255,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 5:
      tunnelvision(led[i].rgb_to_hsv(CRGB(200,200,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 6:
      tunnelvision(led[i].rgb_to_hsv(CRGB(150,150,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    default:
      tunnelvision(led[i].rgb_to_hsv(CRGB(100,100,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);  
      break;
  }
}

///////// DUAL SHOW LOGIC

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {

  // Good hues: 0, 96, 160
  // Bad hues: 64, 136, 208

  // Power issue. Good value = 128, bad = ~175+
  // Is this power or clock/data?
  
  uint8_t hue = 136;
  CHSV forced_color = CHSV(hue, 255, 255);

  
  for (int i = 0; i < NUM_LEDS; i++) {

    // Consider masking
    CHSV color = led[CHANNEL_A].getInterpHSV(led[CHANNEL_B].getInterpFrameColor(i),
                                             led[CHANNEL_A].getInterpFrameColor(i),
                                             fract);  // interpolate a + b channels
    color = narrow_palette(color);

    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }

    leds[i] = forced_color; //color;
    led_buffer[i] = color;
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
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  color.h = map8(color.h, hue_start, (hue_start + hue_width) % MAX_COLOR );
  return color;
}

//
// RGB to HSV - save having to translate RGB colors by hand
//
CHSV rgb_to_hsv( CRGB color) {
  return led[0].rgb_to_hsv(color);  // static method
}

//
// HSV to RGB - Ignore saturation
//
CRGB hsv_to_rgb( CHSV color) {
  if (color.v == 0) {
    return CRGB(0,0,0); // Black
  }
  CRGB rgb = wheel(color.h);
  if (color.v != 255) {  // gradient value part of HSV
    rgb.r = map8(color.v, 0, rgb.r);
    rgb.g = map8(color.v, 0, rgb.g);
    rgb.b = map8(color.v, 0, rgb.b);
  }
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
CRGB wheel(uint8_t hue) {
  
  if (hue < 85) {
    return CRGB(hue * 3, 255 - hue * 3, 0);
  } else if (hue < 170) {
    hue -= 85;
    return CRGB(255 - hue * 3, 0, hue * 3);
  } else {
    hue -= 170; 
    return CRGB(0, hue * 3, 255 - hue * 3);
  }
}

// 
//  unify head tail - Make the same color all pixels of the head
//
void unify_head_tail() {
  leds[0] = leds[1];  // head
  leds[2] = leds[1];
  leds[3] = leds[1];
  
  if (SIMPLE_TAIL) {
    leds[55] = leds[54]; // tail
    leds[57] = leds[56];
    leds[58] = leds[58];
  }
}

//
// Unpack hex into bits
boolean get_bit_from_ring(uint8_t n, uint8_t ring_number) {
  uint8_t ring_matrix[][8] = {
    { 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x0, 0xc2, 0x86, 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x71, 0x24, 0x49, 0x38, 0x0, 0x0 },
    { 0x0, 0x3c, 0x8a, 0x18, 0x30, 0xc7, 0xc0, 0x0 },
    { 0xf, 0xc3, 0x4, 0x0, 0x0, 0x0, 0x38, 0x0 },
    { 0xf0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7, 0xf8 },
  };
  uint8_t pattern_byte = ring_matrix[ring_number][(n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  // PATTERNS - 8 hex bytes per pattern (see Fish.ipynb)
  uint8_t pattern_matrix[][8] = {
    { 0xa, 0xab, 0x56, 0xa5, 0x4a, 0xa9, 0x55, 0x50 },
    { 0xd, 0x99, 0x8c, 0xcc, 0x66, 0x6d, 0xaa, 0xd0 },
    { 0xf2, 0x18, 0x20, 0xc1, 0x6, 0x11, 0x97, 0xf8 },
    { 0xf2, 0x18, 0x71, 0xe5, 0x56, 0xbb, 0xd1, 0xe0 },
    { 0xf2, 0x18, 0x51, 0x24, 0x50, 0x91, 0xa9, 0x20 },
    { 0xf0, 0x7e, 0x3, 0xf0, 0x1f, 0x83, 0xc4, 0xc8 },
    { 0xf, 0xc3, 0x26, 0xd9, 0x30, 0xc6, 0x7f, 0x38 },
    { 0x2, 0x25, 0x25, 0x29, 0x29, 0x12, 0x54, 0xc8 },
    { 0x8, 0xa4, 0x50, 0xc1, 0x6, 0x2a, 0x42, 0x10 },
    { 0x8, 0x99, 0x4, 0xc8, 0x29, 0x12, 0x43, 0x30 },
    
    { 0xfd, 0xdb, 0x7, 0xf8, 0x36, 0xee, 0x46, 0xd8 },
    { 0xf2, 0x19, 0xdf, 0x3e, 0xf9, 0xee, 0x6a, 0x10 },
    { 0xd, 0xe7, 0xdf, 0x3e, 0xf9, 0xee, 0x68, 0x0 },
    { 0xf0, 0x43, 0x8f, 0x3e, 0xff, 0xc6, 0x6c, 0xc8 },
    { 0xf3, 0xf8, 0xf, 0xc, 0xfc, 0x80, 0xe3, 0x30 },
    { 0x3, 0x9d, 0xc7, 0x11, 0xc7, 0x62, 0x19, 0xf8 },
    { 0xff, 0xe7, 0x8e, 0x1c, 0x79, 0xec, 0x7, 0x38 },
    { 0xf7, 0x5b, 0x8c, 0xc, 0x76, 0xb9, 0xbb, 0xf0 },
    { 0xfd, 0xdb, 0x6d, 0xbb, 0x76, 0xed, 0x9c, 0x88 },
  };
  uint8_t pattern_byte = pattern_matrix[pattern_number][(n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

uint8_t get_two_bits_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  // COLORED_PATTERNS - 16 hex bytes per pattern (see Fish.ipynb)
  #define NUM_COLORED_PATTERNS 8   // Total number of colored patterns
  uint8_t colored_pattern_matrix[][16] = {
    { 0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0 },
    { 0xaa, 0x1b, 0xe, 0x40, 0x6, 0xce, 0x40, 0x1, 0xb0, 0xe4, 0x6, 0xc3, 0x9b, 0x0, 0xb9, 0x0 },
    { 0x55, 0xff, 0xb7, 0x7f, 0xbb, 0xf7, 0xff, 0xe7, 0xbe, 0xfb, 0xef, 0x7f, 0x7f, 0xa5, 0xff, 0x80 },
    { 0xaa, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xb9, 0xbe, 0xbf, 0xff, 0x40 },
    { 0x55, 0x0, 0x0, 0x20, 0x0, 0x40, 0xcc, 0x80, 0x3, 0x1, 0x0, 0x2, 0x4, 0x8, 0x53, 0x0 },
    { 0xff, 0xd5, 0xf6, 0x9f, 0x62, 0x76, 0x9, 0x60, 0x26, 0xaa, 0x95, 0x5b, 0xeb, 0x9c, 0xa3, 0x40 },
    { 0xff, 0x55, 0x5f, 0xf7, 0xea, 0xfa, 0xa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1a, 0xa, 0x40 },
    { 0xff, 0xd1, 0xf4, 0x1f, 0x48, 0x7d, 0x7, 0xd2, 0x1f, 0x41, 0xf4, 0x7d, 0x7e, 0xf5, 0xa5, 0x80 },
  };
  uint8_t pattern = pattern_number % NUM_COLORED_PATTERNS;
  uint8_t pattern_byte = colored_pattern_matrix[pattern][(n / 4)];
  return ( (pattern_byte >> (2 * (3 - (n % 4)))) & B00000011 );
}

//
// log status
//
void log_status(uint8_t i) {
  Serial.print("Channel: ");
  Serial.print(i);
  Serial.print(", Show: ");
  Serial.print(current_show[i]);
  Serial.print(", Pattern: ");
  Serial.print(current_pattern[i]);
  Serial.println(".");
}
