#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Hedgehog - 50 Lights
//
//  10/29/22
//
//  On the Adafruit Feather
//
//  MESH network! No phone connection
//

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 32;  // (0-255)

#define DELAY_TIME 20  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 0 // 7
#define CLOCK_PIN 2  // 8

#define NUM_LEDS 50

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // Hook for FastLED library
CHSV led_buffer[NUM_LEDS];  // For smoothing

// Shows
#define NUM_SHOWS 14
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
#define WHITE  CHSV(0, 0, 255)
#define BLACK  CHSV(0, 0, 0)

// Clocks and time
uint8_t show_duration = 40;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 196;  // 0 = no fading, to 255 = always be fading
uint32_t max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // *2! 50% = all on, 50% = all off, and rise + decay on edges

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

#define ARE_CONNECTED false  // Are the Fish talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

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

// Lookup tables

uint8_t ConeLookUp[50] = {
      8,
    9,13,7,
   10,12,14,6,
    11,15,5,
   17,16,3,4,
    18,2,0,
     19,1,
    20,22,24,
     21,23,
    40,25,27,
   41,39,26,28,
    42,38,29,
   43,37,31,30,
    44,36,32,
   45,47,35,33,
    46,48,34,
      49
};


//
// Setup
//
void setup() {
  
  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  randomSeed(analogRead(0));
  
  // Serial.begin(115200);
  // Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );

  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setLedMap(ConeLookUp);  // turned off mapping - it's handled explicitly here in update_leds()
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].setWaitRange(4, 20, 16);
  }

  if (ONLY_RED) {
    hue_center = 0;
    hue_width = 124;
  }
  
  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);  // Start Channel B offset at halfway through show

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = BLACK;
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
        shows[i].allOn();
        break;
      case 1:
        shows[i].twoColor();
        break;
      case 2:
        shows[i].lightRunUp();
        break;
      case 3:
        chevrons(i);
        break;
      case 4:
        hogshell(i);
        break;
      case 5:
        bullseye(i);
        break;
      case 6:
        chevronrainbow(i);
        break;
      case 7:
        stripe(i);
        break;
      case 8:
        chevronfill(i);
        break;
      case 9:
        starburst(i);
        break;
      case 10:
        shows[i].morphChain();
        break;
      case 11:
        shows[i].lightWave();
        break;
      case 12:
        shows[i].juggle_fastled();
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
  current_show[i] = random8(NUM_SHOWS);
  // current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();

  // log_status(i);  // For debugging
}

//// Start specialized shows

//
// calcIntensity - use sine wave + cycle + variable to calculate intensity
//
// x ranges from 0 to max_x-1
// output is 0 to max_y
// speed spd (1, 2, 3, etc.) determines the rate of color change
//
uint8_t calcIntensity(uint8_t channel, uint8_t x, uint8_t max_x, uint8_t max_y, uint8_t spd) {
  uint8_t intense = map8(sin8(map(x, 0, max_x, 0, 255) + (shows[channel].getCycle() * spd)), 0, max_y);
  return intense;
}

//
// stripe
//
void stripe(uint8_t c) {  // c = channel
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (get_centerstripe(i)) {
      shows[c].setPixeltoForeBlack(i);
    } else {
      shows[c].setPixeltoBlack(i);
    }
  }
}

//
// bullseye
//
void bullseye(uint8_t c) {  // c = channel
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), sin8((shows[c].getBackColor() / 5) * get_centering(i))));
  }
}

//
// starburst
//
void starburst(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, get_starburstpattern(i), 10, 255, shows[c].getForeColorSpeed() * 3);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intense));
  }
}

//
// chevrons
//
void chevrons(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, get_chevpattern(i), 10, 255, shows[c].getForeColorSpeed() * 3);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intense));
  }
}

//
// chevronrainbow
//
void chevronrainbow(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, get_chevpattern(i), 10, 255, shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

//
// chevronfill
//
void chevronfill(uint8_t c) {  // c = channel
  uint8_t pos = shows[c].getCycle() % (11 * 2);  // Where we are in the show
  if (pos >= 11) {
    pos = (11 * 2) - pos;  // For a sawtooth effect
  }
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (get_chevpattern(i) < pos) {
      shows[c].setPixeltoForeBlack(i);
    } else {
      shows[c].setPixeltoForeColor(i);
    }
  }
}

//
// hogshell
//
void hogshell(uint8_t c) {  // c = channel
  uint8_t s[4] = { 2, 10, 50, 255 };
  
  for (int i=0; i < NUM_LEDS; i++) {
    led[c].setPixelColor(i, CHSV(shows[c].getForeColor(), 255, s[get_centering(i)]));
  }
}


// centerstripe: a center stripe
uint8_t get_centerstripe(uint8_t i) {
  uint8_t centerstripe[50] = {
      1,
    0,1,0,
   0,0,1,0,
    0,1,0,
   0,1,0,0,
    0,1,0,
     0,1,
    0,1,0,
     1,0,
    0,1,0,
   0,0,1,0,
    0,1,0,
   0,1,0,0,
    0,1,0,
   0,0,1,0,
    0,1,0,
      1,
  };
  return centerstripe[i % 50];
}

// starburstpattern: from center to edge
uint8_t get_starburstpattern(uint8_t i) {
  uint8_t starburstpattern[50] = {
      9,
    9,8,9,
   8,7,7,8,
    6,5,6,
   5,4,4,5,
    3,2,3,
     1,1,
    2,0,2,
     1,1,
    2,1,2,
   4,3,3,4,
    5,4,5,
   6,5,5,6,
    7,6,7,
   8,7,7,8,
    9,8,9,
      9,
  };
  return starburstpattern[i % 50];
}

uint8_t get_chevpattern(uint8_t i) {
  uint8_t chevpattern[50] = {
      9,
    9,8,9,
   9,8,8,9,
    8,7,8,
   8,7,7,8,
    7,6,7,
     6,6,
    6,5,6,
     5,5,
    5,4,5,
   5,4,4,5,
    4,3,4,
   4,3,3,4,
    3,2,3,
   3,2,2,3,
    2,1,2,
      0,
  };
  return chevpattern[i % 50];
}

// Centering: creates a center bright ring
uint8_t get_centering(uint8_t i) {
  uint8_t centering[50] = {
      0,
    0,1,0,
   0,1,1,0,
    0,2,0,
   0,2,2,0,
    1,2,1,
     2,2,
    2,3,2,
     3,3,
    2,3,2,
   2,3,3,2,
    2,3,2,
   1,2,2,1,
    0,2,0,
   0,1,1,0,
    0,1,0,
      0,
  };
  return centering[i % 50];
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

void processIrMessage(JSONVar myObject) {
  processIrParam(int(myObject["param1"]), int(myObject["value1"]));
//  processIrParam(int(myObject["param2"]), int(myObject["value2"]));
}

void processIrParam(uint8_t param, uint8_t value) {
  
  switch (param) {
    case BRIGHTNESS_COMMAND:
      bright = value;
//      FastLED.setBrightness( bright );
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
void morph_channels(uint8_t fract) {

  for (int i = 0; i < NUM_LEDS; i++) {
        
    CHSV color_b = led[CHANNEL_B].getInterpFrameColor(i);
    CHSV color_a = led[CHANNEL_A].getInterpFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    
    // color = narrow_palette(color);
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }

    leds[i] = color;
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
// narrow_palette - confine the color range
//
CHSV narrow_palette(CHSV color) {
  if (hue_width == 255) {
    return color;
  }
  uint8_t h1 = (hue_center - (hue_width / 2)) % MAX_COLOR;
  uint8_t h2 = (hue_center + (hue_width / 2)) % MAX_COLOR;
  color.h = map8(color.h, h1, h2);
  if (color.s != 0) {
    color.s = saturation;  // Reset saturation
  }
  return color;
}
   
//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}
