#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Turtle! - 29 Lights in a hexagonal grid
//
//  3 Turtle Shells with a different wiring layout
//
//  9/20/21 - Feather WiFi
//
//  MESH network!
//
//  Speaking & Hearing
//

uint8_t BRIGHTNESS = 255;  // (0-255)

#define DELAY_TIME  20  // in milliseconds
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 0  //  11 // 7 // data = 7 (Teensy) or 0 (Feather)
#define CLOCK_PIN 2  //  13 // 8 // clock = 8 (Teensy) or 2 (Feather)

#define NUM_LEDS 29
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number

#define ACTUAL_LEDS 37  // There are 8 dummy spacer LEDs

#define XX  255

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define ARE_CONNECTED true  // Are the HEX talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

#define ONLY_RED false
uint8_t hue_start = 0;
uint8_t hue_width = 255;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 13
#define START_SHOW_CHANNEL_A  1  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  0
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 0 };

// wait times
#define SHOW_DURATION 25  // seconds
uint8_t FADE_TIME = 20;  // seconds to fade in + out (Arduino Blue)
#define MAX_SMALL_CYCLE  (SHOW_DURATION * 2 * (1000 / DELAY_TIME))  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

long elasped_time = 0;
boolean is_lost = false;
unsigned long last_connection = 0;
#define MAX_SILENT_TIME  10000  // Time (in cycles) without communication before marked as is_lost

String message;  // String to send to other displays

//#define MSG_FREQUENCY  50  // send message every X milliseconds
#define MESSAGE_REPEAT 3   // send this many duplicate messages
#define MESSAGE_SPACING 3   // wait this many cycles
Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

// User stub
//void sendMessage();
void updateLeds();
String getReadings();  // Prototype for reading state of LEDs

//Task taskSendMessage(TASK_MILLISECOND * MSG_FREQUENCY, TASK_FOREVER, &sendMessage);  // send message happens in updateLeds
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

// Lookup tables

const uint8_t rewire[] PROGMEM = {
  30, 26, 27,  0,  1,
   3,  2, 25, 24, 32,
  33, 22, 23,  4,  5,
   7,  6, 21, 20, 35,
  36, 18, 19,  8,  9,
  10, 17, 16, 15
};

uint8_t neighbors[] PROGMEM = {
  XX,1,8,9,XX,XX,
  XX,2,7,8,0,XX,
  XX,XX,3,7,1,XX,
  XX,XX,4,6,7,2,
  XX,XX,XX,5,6,3, // 4
  4,XX,XX,14,13,6,
  3,4,5,13,12,7,
  2,3,6,12,8,1,
  1,7,12,11,9,0,  // 8
  0,8,11,10,XX,XX,
  9,11,18,19,XX,XX,
  8,12,17,18,10,9,
  7,6,13,17,11,8, // 12
  6,5,14,16,17,12,
  5,XX,XX,15,16,13,
  14,XX,XX,24,23,16,
  13,14,15,23,22,17, // 16
  12,13,16,22,18,11,
  11,17,22,21,19,10,
  10,18,21,20,XX,XX,
  19,21,27,XX,XX,XX,
  18,22,26,27,20,19,
  17,16,23,26,21,18, // 22
  16,15,24,25,26,22,
  15,XX,XX,XX,25,23,
  23,24,XX,XX,28,26,
  22,23,25,28,27,21,
  21,26,28,XX,XX,20,
  26,25,XX,XX,XX,27 // 28
};

#define NUM_PATTERNS 8   // Total number of patterns 

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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);  // only use of ACTUAL_LEDS
  FastLED.setBrightness( BRIGHTNESS );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].setColorSpeedMinMax(4, 20);  // Make colors change faster (lower = faster) Usually 8, 30
    shows[i].setWaitRange(30, 120, 90);
    shows[i].setBandsBpm(10, 30);
  }
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_start = 192;
    hue_width = 124;
  }

  // mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
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
        patterns(i);
        break;
      case 1:
        warp1(i);
        break;
      case 2:
        warp2(i);
        break;
      case 3:
        rainbow_show(i);
        break;
      case 4:
        shows[i].morphChain();
        break;
      case 5:
        shows[i].bounce();
        break;
      case 6:
        shows[i].bounceGlowing();
        break;
      case 7:
        shows[i].plinko(2);
        break;
      case 8:
        shows[i].randomFill();  // Vetted
        break;
      case 10:
        shows[i].sawTooth();  // Vetted
        break;
      case 11:
        shows[i].sinelon_fastled();  // Vetted
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
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { 
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
  shows[i].turnOnMorphing();
  shows[i].resetAllClocks();
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  current_show[i] = is_other_channel_show_zero(i) ? random8(1, NUM_SHOWS) : 0 ;
  current_pattern[i] = random8(NUM_PATTERNS);
  // current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].setWaitRange(30, 120, 90);
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
  // log_status(i);  // For debugging
}

boolean is_other_channel_show_zero(uint8_t channel) {
  if (channel == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

//
// patterns shows
//
void patterns(uint8_t c) {
  uint8_t PatternMatrix[] = {
    1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,
    1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,1,
    1,1,1,1,1,1,2,2,2,1,1,2,1,2,1,1,2,2,2,1,1,2,1,2,1,1,2,1,1,
    1,1,1,1,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,1,1,
    1,1,1,1,1,2,2,2,2,2,2,1,1,1,2,2,1,1,1,2,1,2,1,2,1,1,2,1,1,
    2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,2,
    1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,1,1,1,2,1,2,1,2,1,1,2,1,1,
    1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,2,1,2,1 
  }; 

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t pattern = PatternMatrix[(current_pattern[c] * 29) + i];
 
    switch (pattern) {
      case 0: {        // Off
        led[c].setPixelBlack(i);
        break;
      }
      case 1: {        // always green
        led[c].setPixelColor(i, CHSV(96, 255, 255));
        break;
      }
      case 2: {        // the other color
        led[c].setPixelHue(i, shows[c].getForeColor());
        break;
      }
    }
  }
}

//
// draw_ring
//
void draw_ring(uint8_t i, CHSV color, uint8_t c) {
  uint8_t rings[] = {
    17, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX,
    12, 13, 16, 22, 18, 11, XX, XX, XX, XX, XX, XX,
     7,  6,  5, 14, 15, 23, 26, 21, 19, 10,  9,  8,
     0,  1,  2,  3,  4, 24, 25, 28, 27, 20, XX, XX 
  };

  for (uint8_t j = 0; j < 12; j++) {
    uint8_t r = rings[(i * 12) + j];
    if (r != XX) {
      led[c].setPixelColor(r, color);
    }
  }
}

//
// tunnel vision
//
// Colored ring animating outwards
// color1 is the primary color, colo22 is a trail color
// background is the background color
//
void tunnelvision(CHSV color1, CHSV color2, CHSV background, uint8_t c) {
  led[c].fill(background);  
  uint8_t i = shows[c].getCycle() % 5;
  if (i < 4) { 
    draw_ring(i, color1, c); 
  }      
  if (i != 0) { 
    draw_ring(i-1, color2, c); 
  }
}

//
// warp1 - colors on a black field
// 
void warp1(uint8_t c) {
  switch ((shows[c].getCycle() / 5) % 6) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0, 255, 0)), rgb_to_hsv(CRGB(0,40,0)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,0,255)), rgb_to_hsv(CRGB(0,0,40)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,255,255)), rgb_to_hsv(CRGB(0,40,40)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(40,40,0)), rgb_to_hsv(CRGB(0,0,0)), c);  
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,0,0)), rgb_to_hsv(CRGB(40,0,0)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(255,0,255)), rgb_to_hsv(CRGB(40,0,40)), rgb_to_hsv(CRGB(0,0,0)), c);  
      break;
  }
}

//
// warp2 - colors on a green field
//
void warp2(uint8_t c) {
  switch ((shows[c].getCycle() / 5) % 8) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0,255,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,200,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,150,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(0,100,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 5:
      tunnelvision(rgb_to_hsv(CRGB(200,200,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 6:
      tunnelvision(rgb_to_hsv(CRGB(150,150,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(100,100,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);  
      break;
  }
}

//
// rainbow show - distribute a rainbow wheel equally distributed along the chain
//
void rainbow_show(uint8_t c) {
  for (uint8_t i = 0; i < HALF_LEDS; i++) {
    uint8_t hue = ((shows[c].getForeColorSpeed() * i) + shows[c].getBackColorSpeed() + (10 * shows[c].getCycle())) % MAX_COLOR;
    led[c].setPixelHue(i, hue);
    led[c].setPixelHue(NUM_LEDS - i - 1, hue);
  }  
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
  jsonReadings["p"] = current_pattern[c];

  message = JSON.stringify(jsonReadings);
  return message;
}

void sendMessage (uint8_t c) {
  String msg = getReadings(c);
  mesh.sendBroadcast(msg);
  // Serial.println(msg);
}

void receivedCallback( uint32_t from, String &msg ) {
  // Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  JSONVar myObject = JSON.parse(msg.c_str());

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
  
  last_connection = 0;
  is_lost = false;
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


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getInterpFrameColor(i);
    CHSV color_a = led[CHANNEL_A].getInterpFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = narrow_palette(color);
    
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }
    
    leds[pgm_read_byte_near(rewire + i)] = color;
    led_buffer[i] = color;
  }
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();

  if (small_cycle <= FADE_CYCLES) {
    intensity = map(small_cycle, 0, FADE_CYCLES, 0, 255);  // rise
  } else if (small_cycle <= (MAX_SMALL_CYCLE / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((MAX_SMALL_CYCLE / 2) + FADE_CYCLES)) {
    intensity = map(small_cycle - (MAX_SMALL_CYCLE / 2), 0, FADE_CYCLES, 255, 0);  // decay
  } else {
    intensity = 0;
  }
  return intensity;  // do not use easing
}

//// End DUAL SHOW LOGIC

//
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  color.h = map8(color.h, hue_start, (hue_start + hue_width) % MAX_COLOR );
  return color;
}

void turn_off_spacer_leds() {
  uint8_t spacer_leds[] = { 11, 12, 13, 14, 28, 29, 31, 34 };
  
  for (uint8_t i = 0; i < 8; i++) {
    leds[spacer_leds[i]] = CRGB(0, 0, 0);
  }
}

//
// RGB to HSV - save having to translate RGB colors by hand
//
CHSV rgb_to_hsv( CRGB color) {
  return led[0].rgb_to_hsv(color);  // static method
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
  Serial.print(F(", Delay: "));
  Serial.print(DELAY_TIME);
  Serial.print(F(", Sm Cycle: "));
  Serial.println(shows[i].getSmallCycle());
}