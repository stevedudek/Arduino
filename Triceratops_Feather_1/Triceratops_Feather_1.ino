#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Triceratops - 45 cones on a coat
//
//  10/9/21
//
//  On the Adafruit Feather
//
//  MESH network! No phone connection
//  
//  Hears — Does not speak

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 32;  // (0-255)

#define MAX_LUMENS 10000  // estimated maximum brightness until flickering

#define DELAY_TIME 20  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 0  // data = 7 (Teensy) or 0 (Feather)
#define CLOCK_PIN 2  // clock = 8 (Teensy) or 2 (Feather)

#define NUM_LEDS 45

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define MAX_XCOORD  4
#define MAX_YCOORD  17

#define XX  255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[NUM_LEDS];  // The Leds themselves

// Shows
#define NUM_SHOWS 23
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

#define ONLY_RED false
uint8_t hue_start = 0;
uint8_t hue_width = 255;

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

#define ARE_CONNECTED true  // Are the Fish talking to each other?
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

uint8_t ConeLookUp[] PROGMEM = {
      44,
    42, 43,
  41, 40, 32,
    39, 33,
  38, 34, 31,
    35, 30,
  36, 29, 20,
37, 28, 21, 19,
  27, 22, 18,
26, 23, 17,  9,
  24, 16,  8,
25, 15,  7, 10,
  14,  6, 11,
     5, 12,
   4, 13,  0,
     3,  1,
       2
};

uint8_t ConeGrid[] PROGMEM = {
 XX,    44,   XX,XX,
 XX,  42, 43,    XX,
    41, 40, 32,  XX,
 XX,  39, 33,    XX,
    38, 34, 31,  XX,
 XX,  35, 30,    XX,
    36, 29, 20,  XX,
  37, 28, 21, 19,  
    27, 22, 18,  XX,
  26, 23, 17,  9,  
    24, 16,  8,  XX,
  25, 15,  7, 10,  
    14,  6, 11,  XX,
 XX,   5, 12,    XX,
     4, 13,  0,  XX,
 XX,   3,  1,    XX,
 XX,     2,   XX,XX,
};

uint8_t neighbors[] PROGMEM = {
  XX,XX,XX,1,13,12, // 0
  0,XX,XX,2,3,13,
  1,XX,XX,XX,XX,3,
  13,1,2,XX,XX,4,
  5,13,3,XX,XX,XX, // 4
  6,12,13,4,XX,14,
  7,11,12,5,14,15,
  8,10,11,6,15,16,
  9,XX,10,7,16,17, // 8
  XX,XX,XX,8,17,18,
  XX,XX,XX,11,7,8,
  10,XX,XX,12,6,7,
  11,XX,0,13,5,6, // 12
  12,0,1,3,4,5,
  15,6,5,XX,XX,25,
  16,7,6,14,25,24,
  17,8,7,15,24,23, // 16
  18,9,8,16,23,22,
  19,XX,9,17,22,21,
  XX,XX,XX,18,21,20,
  XX,XX,19,21,29,30, // 20
  20,19,18,22,28,29,
  21,18,17,23,27,28,
  22,17,16,24,26,27,
  23,16,15,25,XX,26, // 24
  24,15,14,XX,XX,XX,
  27,23,24,XX,XX,XX,
  28,22,23,26,XX,37,
  29,21,22,27,37,36, // 28
  30,20,21,28,36,35,
  31,XX,20,29,35,34,
  XX,XX,XX,30,34,33,
  XX,XX,XX,33,40,43, // 32
  32,XX,31,34,39,40,
  33,31,30,35,38,39,
  34,30,29,36,XX,38,
  35,29,28,37,XX,XX, // 36
  36,28,27,XX,XX,XX,
  39,34,35,XX,XX,XX,
  40,33,34,38,XX,41,
  43,32,33,39,41,42, // 40
  42,40,39,XX,XX,XX,
  44,43,40,41,XX,XX,
  XX,XX,32,40,42,44,
  XX,XX,43,42,XX,XX, // 44
};

const uint8_t PatternLookUp[45] = { 41,43,44,42,39,37,35,32,29,26,
                           33,36,38,40,34,31,28,25,22,19,
                           15,18,21,24,27,30,23,20,17,14,
                           12,10,5,7,9,11,13,16,8,6,
                           4,3,2,1,0 };

const uint8_t Stripe_Pattern[45] PROGMEM = {
       0,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
       0
};

const uint8_t Section_Pattern[45] PROGMEM = {
       2,
     2,  2,
   0,  0,  0,
     0,  0,
   0,  0,  0,
     0,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
     0,  0,
   2,  0,  2,
     2,  2,
       2
};

const uint8_t Explode_Pattern[45] PROGMEM = {
       5,
     5,  5,
   5,  4,  5,
     4,  4,
   4,  3,  4,
     3,  3,
   3,  2,  3,
 3,  1,  1,  3,
   2,  0,  2,
 3,  1,  1,  3,
   3,  2,  3,
 4,  3,  3,  4,
   4,  3,  4,
     4,  4,
   5,  4,  5,
     5,  5,
       5
};

const uint8_t Alternate_Pattern[45] PROGMEM = {
       0,
     1,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
 0,  0,  1,  0,
   1,  0,  0,
 1,  0,  1,  0,
   1,  0,  0,
 0,  0,  0,  1,
   0,  1,  0,
     0,  0,
   1,  0,  1,
     0,  0,
       1
};

const uint8_t SideSide_Pattern[45] PROGMEM = {
       3,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
       3
};

const uint8_t Diag_Pattern[45] PROGMEM = {
       0,
     1,  0,
   2,  1,  0,
     2,  1,
   3,  2,  1,
     3,  2,
   4,  3,  2,
 5,  4,  3,  2,
   5,  4,  3,
 6,  5,  4,  3,
   6,  5,  4,
 7,  6,  5,  4,
   7,  6,  5,
     7,  6,
   8,  7,  6,
     8,  7,
       8
};

const uint8_t ConeSize[45]  = { 5,5,5,5,5,3,5,1,3,5,1,1,5,4,3,4,5,5,6,3,6,2,1,6,6,4,2,6,2,1,3,4,2,4,4,5,3,2,2,3,1,6,3,3,1 };

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
  FastLED.setBrightness( bright );

  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setLedMap(ConeLookUp);
    led[i].setCoordMap(MAX_YCOORD, ConeGrid);  // x,y grid of cones
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
        colorsize(i);
        break;
      case 4:
        brightsize(i);
        break;
      case 5:
        stripe(i);
        break;
      case 6:
        alternate(i);
        break;
      case 7:
        diagcolor(i);
        break;
      case 8:
        sidesidecolor(i);
        break;
      case 9:
        explodecolor(i);
        break;
      case 10:
        diagbright(i);
        break;
      case 11:
        sidesidebright(i);
        break;
      case 12:
        explodebright(i);
        break;
      case 13:
        sectioncolor(i);
        break;
      case 14:
        shows[i].bands();
        break;
      case 15:
        shows[i].packets();
        break;
      case 16:
        shows[i].packets_two();
        break;
      case 17:
        shows[i].morphChain();
        break;
      case 18:
        shows[i].bpm_fastled();
        break;
      case 19:
        shows[i].juggle_fastled();
        break;
      case 20:
        shows[i].randomColors();
        break;
      case 21:
        shows[i].randomFill();
        break;
      default:
        shows[i].lightWave();
        break;
    }
  
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  // adj_brightess();  // Limit brightness to conserve power - hope 6V power supplies fix the flickering
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
}


//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();

    // Mesh network will not control shows; not enough shows are similar
    if (shows[i].getSmallCycle() >= max_small_cycle) { 
      next_show(i);
      pick_next_show(i);
    }
    
    if (ARE_CONNECTED) {
      if (IS_SPEAKING) {
        uint32_t smallCycle = shows[i].getSmallCycle();
        if ((smallCycle <= MESSAGE_REPEAT * MESSAGE_SPACING) && (smallCycle % MESSAGE_SPACING == 0)) {
          sendMessage(i);  // send message to mesh if connected and speaking on particular small cycles
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
  current_show[i] = random8(NUM_SHOWS);
  //  current_show = (current_show + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();

  // Set new show's colors to those of the other channel
  uint8_t other_channel = (i == 0) ? 1 : 0 ;
  shows[i].setForeColor(shows[other_channel].getForeColor());
  shows[i].setBackColor(shows[other_channel].getBackColor());
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
// colorsize - light each cone according to its cone size
//
void colorsize(uint8_t c) {  // c = channel
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, ConeSize[i]-1, 5, sin8(shows[c].getBackColor()), shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

//
// brightsize - light just one cone size
//
void brightsize(uint8_t c) {  // c = channel
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, ConeSize[i]-1, 5, 255, shows[c].getForeColorSpeed());
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

//
// stripe
//
void stripe(uint8_t c) {  // c = channel
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Stripe_Pattern + PatternLookUp[i]) == 0) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoBackColor(i);
    }
  }
}

//
// alternate
//
void alternate(uint8_t c) {  // c = channel
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Alternate_Pattern + PatternLookUp[i])) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoBackColor(i);
    }
  }
}

void diagcolor(uint8_t c) {  // c = channel
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, pgm_read_byte_near(Diag_Pattern + i), 9, 200, shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void sectioncolor(uint8_t c) {  // c = channel
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, pgm_read_byte_near(Section_Pattern + i), 3, 128, shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void sidesidecolor(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, pgm_read_byte_near(SideSide_Pattern + i), 7, 255, shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void explodecolor(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, pgm_read_byte_near(Explode_Pattern + i), 6, 255, shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void diagbright(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, pgm_read_byte_near(Diag_Pattern + i), 9, 255, shows[c].getForeColorSpeed() * 3);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

void sidesidebright(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, pgm_read_byte_near(SideSide_Pattern + i), 7, 255, shows[c].getForeColorSpeed() * 2);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

void explodebright(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, pgm_read_byte_near(Explode_Pattern + i), 6, 255, shows[c].getForeColorSpeed() * 2);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

//// End specialized shows

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
  // jsonReadings["p"] = current_pattern[c];  // no pattern capability

  message = JSON.stringify(jsonReadings);
  return message;
}

void sendMessage (uint8_t c) {
  String msg = getReadings(c);
  mesh.sendBroadcast(msg);
}

void receivedCallback( uint32_t from, String &msg ) {
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
    // current_pattern[c] = int(myObject["p"]);  // no pattern capability  

    /*  Do not change shows
    int show_number = int(myObject["s"]);
    if (current_show[c] != show_number) {
      current_show[c] = show_number % NUM_SHOWS;
      next_show(c);
    }
    */
  }
  
  last_connection = 0;
  is_lost = false;
}

void processPhoneMessage(JSONVar myObject) {
  return;  // Not implemented yet

  /*
  int value = int(myObject["value"]);
  Serial.printf("Received control value of %d\n", value);

  switch (int(myObject["param"])) {
    case BRIGHTNESS_COMMAND:
      bright = value;
      FastLED.setBrightness( bright );
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
  */
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
    CHSV color = narrow_palette(led[CHANNEL_A].getInterpHSV(color_b, color_a, fract));  // interpolate a + b channels

    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }

    leds[i] = color;
    led_buffer[i] = color;
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

//
// adjust brightness - put a cap on maximum brightness to prevent flickering
//
void adj_brightess() {
  uint8_t brightness_step_size = 5;
  uint32_t lumens = 0;

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
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

void setHead(uint8_t channel, uint8_t hue) {
  led[channel].setPixelHue(0, hue);
  led[channel].setPixelHue(1, hue);
  led[channel].setPixelHue(2, hue);
}
 
//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}
