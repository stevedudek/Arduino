#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Linear Lights with FastLED
//
//  Feather Huzzah
//
//  2/10/24
//
//  Modernized
//
#define NUM_LEDS 32
#define ACTUAL_LEDS (NUM_LEDS / 2)

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 196;  // (0 = fast, 255 = slow)

#define DELAY_TIME 20  // in milliseconds. 10 is on the edge: watch out for jittering
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN  2  // 7
//#define CLOCK_PIN 2  // 8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

// Smoothing constants - lower is slower smoothing
// At 15 ms cycles: 40 = 96 ms, 30 = 127 ms, 20 = 191 ms, 10 = 382 ms, 5 = 765 ms, 2 = 1.9 s, 1 = 3.8 s
#define SMOOTHING_SHOWS_HUE    20   // Fastest full rainbow = DELAY_TIME * (255 / this value)
#define SMOOTHING_SHOWS_VALUE  20   // Fastest turn off/on = DELAY_TIME * (255 / this value)
\

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[ACTUAL_LEDS];  // Hook for FastLED library

#define ONLY_RED true
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
CHSV BLACK = CHSV(0, 0, 0);

// Shows
#define START_SHOW_CHANNEL_A  1  // Channels A starting show
#define START_SHOW_CHANNEL_B  0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 12

// Clocks and time
uint8_t show_duration = 60;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 196; // 148;  // 0 = no fading, to 255 = always be fading

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

#define ARE_CONNECTED false
#define IS_SPEAKING false

boolean is_lost = false;
unsigned long last_connection = 0;
#define MAX_SILENT_TIME  10000  // Time (in cycles) without communication before marked as is_lost

painlessMesh mesh;
String message;  // String to send to other displays
String getReadings();  // Prototype for reading state of LEDs

#define MESSAGE_REPEAT 3   // send this many additional messages
#define MESSAGE_SPACING 5   // wait this many cycles

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

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  // pinMode(CLOCK_PIN, OUTPUT);

  randomSeed(analogRead(0));

  Serial.begin(115200);
  Serial.println(F("Start"));

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );

  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
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
  morph_channels();  // On every cycle
}

//
// update leds - Moved to a task-scheduled event
//
void updateLeds() {
  for (uint8_t i = 0; i < DUAL; i++) {
    run_shows(i);
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }
  advance_clocks();  // advance the cycle clocks and check for next show
}

//
// run shows - Now its own function
//
void run_shows(uint8_t i) {
      
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
      case 8:
        shows[i].sinelon_fastled();
        break;
      case 9:
        shows[i].bpm_fastled();
        break;
      case 10:
        shows[i].juggle_fastled();
        break;
      default:
        shows[i].confetti();
        break;
    }
    
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
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
// next_show
//
void next_show(uint8_t i) {
  shows[i].resetAllClocks();
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  
  shows[i].pickCycleDuration();
  shows[i].pickRandomColorSpeeds();
  shows[i].fillBlack();
//  log_status(i);  // For debugging
}

boolean is_other_channel_show_zero(uint8_t c) {
  if (c == CHANNEL_A) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

//// End specialized shows


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


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  CRGB rgb_color;
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B
  
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getCurrFrameColor(i);   // New! Changed Interp to Next. No more Interp frames.
    CHSV color_a = led[CHANNEL_A].getCurrFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);

    // smoothing backstop. smooth constants should be large.
    color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);

    if (led_buffer[i].h == color.h && led_buffer[i].s == color.s && led_buffer[i].v == color.v) {
      continue;
    } else {
      hsv2rgb_rainbow(color, rgb_color);
      leds[i] = led[CHANNEL_A].smooth_rgb_color(leds[i], rgb_color, 4);
      leds[ACTUAL_LEDS - i - 1] = led[CHANNEL_A].smooth_rgb_color(leds[i], rgb_color, 4);  // symmetrize!
//      leds[i] = color;   // put HSV color on to LEDs
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
    }    
  }

  FastLED.show();
}

//// End DUAL SHOW LOGIC

//
// log status
//
void log_status(uint8_t i) {
  Serial.print(F("Channel: "));
  Serial.print(i);
  Serial.print(F(", Show: "));
  Serial.print(current_show[i]);
  Serial.print(F(", Cycle Duration: "));
  Serial.print(shows[i].getCycleDuration());
  Serial.print(F(", Cycle: "));
  Serial.print(shows[i].getCycle());
  Serial.print(F(", Sm Cycle: "));
  Serial.println(shows[i].getSmallCycle());
}
