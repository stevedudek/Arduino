#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Gold Dinosaur with 8 spikes
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  10/26/2021
//
#define NUM_LEDS 52
#define ACTUAL_LEDS 66

#define BRIGHTNESS  255 // (0-255)

uint8_t show_speed = 128;  // (0-255)

#define DELAY_TIME 30  // in milliseconds. FastLED demo has 8.3 ms delay!

#define DATA_PIN 0  // Feather  // 7

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

CHSV morph_buffer[DUAL][NUM_LEDS];

#define ONLY_RED true  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define START_SHOW_CHANNEL_A  10  // Channels A starting show
#define START_SHOW_CHANNEL_B  0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 11

// Clocks and time
uint8_t show_duration = 40;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 255;  // 0 = no fading, to 255 = always be fading
uint32_t max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // *2! 50% = all on, 50% = all off, and rise + decay on edges

boolean simplespikes[] = { false, false };

#define XX 99

//// Mesh parameters

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

#define ARE_CONNECTED false// Are the pentagons talking to each other?
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

// User stub
//void sendMessage();
void updateLeds();
String getReadings();  // Prototype for reading state of LEDs

//Task taskSendMessage(MSG_FREQUENCY, TASK_FOREVER, &sendMessage);
Task taskUpdateLeds(DELAY_TIME, TASK_FOREVER, &updateLeds);

// LED Lookup table
const uint8_t spine_lookup_table[] PROGMEM = {
   0, 1, 2, 3,XX,XX,XX,XX,
   4, 5, 6, 7, 8, 9,XX,XX,
  10,11,12,13,14,15,16,17,
  18,19,20,21,22,23,24,25,
  26,27,28,29,30,31,32,33,
  34,35,36,37,38,39,40,41,
  42,43,44,45,46,47,XX,XX,
  48,49,50,51,XX,XX,XX,XX
};

const uint8_t led_lookup_table[] PROGMEM = {
   0, 1, 2, 3, 6, 7, 8, 9,10,11,14,15,16,17,18,19,20,21,
  24,25,26,27,28,29,30,31,34,35,36,37,38,39,40,41,44,45,
  46,47,48,49,50,51,54,55,56,57,58,59,62,63,64,65
};

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( BRIGHTNESS );

  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();
    
    shows[i].setColorSpeedMinMax(show_speed);
    shows[i].setWaitRange(show_speed);
  }

  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);  // Start Channel B offset at halfway through show
  
  for (uint8_t c = 0; c < DUAL; c++) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      leds_buffer[c][i] = CHSV(0, 0, 0);
      morph_buffer[c][i] = CHSV(0, 0, 0);
    }
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
    mesh.update();
  }
}

void updateLeds() {
  // Moved to a task-scheduled event
  for (uint8_t i = 0; i < DUAL; i++) {

    switch (current_show[i]) {
      
        case 0:
          shows[i].allOn();
          break;
        case 1:
          shows[i].morphChain();
          break;
        case 2:
          shows[i].twoColor();
          break;
        case 3:
          shows[i].lightWave();
          break;
        case 4:
          shows[i].sawTooth();
          break;
        case 5:
          shows[i].lightRunUp();
          break;
        case 6:
          shows[i].packets();
          break;
        case 7:
          shows[i].sinelon_fastled();
          break;
        case 8:
          shows[i].bpm_fastled();
          break;
        case 9:
          shows[i].juggle_fastled();
          break;
        default:
          shows[i].bands();
          break;
      }
      shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }
  
  update_leds();  // morph together the 2 chanels & push the interp_frame on to the leds
  unpack_leds();
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
    }
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  
  simplespikes[i] = (random8(2) == 0) ? true : false ;
  set_led_number(i);
  
  led[i].push_frame();
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].pickRandomWait();
  shows[i].pickRandomColorSpeeds();

  log_status(i);
}

//
// set_led_number
//
void set_led_number(uint8_t i) {
  led[i].fillBlack();  // clear leds before symmetry change
  uint8_t numSymLeds = (simplespikes[i]) ? 8 : 54;
  shows[i].resetNumLeds(numSymLeds);  // resets the virtual number of LEDs
}


//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (uint8_t channel = 0; channel < DUAL; channel++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds_buffer[channel][i] = led[channel].getInterpFrameColor(i);
    }
  }
}

//
// unpack_leds - if simplespines, need to unpack into component leds
//
void unpack_leds() {
  for (uint8_t channel = 0; channel < DUAL; channel++) {
    if (simplespikes[channel]) {
      for (uint8_t spike = 0; spike < 8; spike++) {
        for (uint8_t n = 0; n < 8; n++) {
          uint8_t i = pgm_read_byte_near(spine_lookup_table + (spike * 8) + n);
          if (i != XX) {
             morph_buffer[channel][i] = leds_buffer[channel][spike];
          }
        }
      }
    } else {
      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        morph_buffer[channel][i] = leds_buffer[channel][i];
      }
    }
  }
}


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color = led[CHANNEL_A].getInterpHSV(morph_buffer[CHANNEL_B][i], 
                                             morph_buffer[CHANNEL_A][i], 
                                             fract);  // interpolate a + b channels
    color = narrow_palette(color);
    
    leds[pgm_read_byte_near(led_lookup_table + i)] = color;
  }
}

//
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  color.h = map8(color.h, hue_start, (hue_start + hue_width) % MAX_COLOR );
  return color;
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
    shows[c].setForeColor(int(myObject["f"]));
    shows[c].setForeColorSpeed(int(myObject["fspd"]));
    shows[c].setBackColor(int(myObject["b"]));
    shows[c].setBackColorSpeed(int(myObject["bspd"]));
    shows[c].setWait(int(myObject["w"]));
  }
  
  last_connection = 0;
  is_lost = false;
}

void processPhoneMessage(JSONVar myObject) {
  int value = int(myObject["value"]);
  Serial.printf("Received control value of %d\n", value);
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
