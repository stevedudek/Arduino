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
//  11/28/21 - On the Adafruit Feather
//
//  MESH network!
//
//  Listens to the Network and an IR Controller
//

uint8_t BRIGHTNESS = 255;  // (0-255)

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME  12  // in milliseconds
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

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 13
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 0 };

// wait times
uint8_t show_duration = 40;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 148;  // 0 = no fading, to 255 = always be fading
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

// Lookup tables

const uint8_t rewire[] PROGMEM = {
  30, 26, 27,  0,  1,
   3,  2, 25, 24, 32,
  33, 22, 23,  4,  5,
   7,  6, 21, 20, 35,
  36, 18, 19,  8,  9,
  10, 17, 16, 15
};

const uint8_t PatternMatrix[] PROGMEM = {
    1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,
    1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,1,
    1,1,1,1,1,1,2,2,2,1,1,2,1,2,1,1,2,2,2,1,1,2,1,2,1,1,2,1,1,
    1,1,1,1,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,1,1,
    1,1,1,1,1,2,2,2,2,2,2,1,1,1,2,2,1,1,1,2,1,2,1,2,1,1,2,1,1,
    2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,2,
    1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,1,1,1,2,1,2,1,2,1,1,2,1,1,
    1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,2,1,2,1 
};

const uint8_t neighbors[] PROGMEM = {
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
  FastLED.setBrightness( bright );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].setColorSpeedMinMax(show_speed);
    shows[i].setWaitRange(show_speed);
  }
  
  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);  // Start Channel B offset at halfway through show

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_center = 0;
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
  // Moved to a task-scheduled event

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
  shows[i].turnOnMorphing();
  shows[i].resetAllClocks();
  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  current_show[i] = is_other_channel_show_zero(i) ? random8(1, NUM_SHOWS) : 0 ;

  shows[i].setWaitRange(show_speed);
  shows[i].pickRandomWait();
  shows[i].setColorSpeedMinMax(show_speed);  
  shows[i].pickRandomColorSpeeds();
  // current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
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

  if (shows[c].isShowStart()) {
    current_pattern[c] = random8(NUM_PATTERNS);
  }
   
  uint8_t pattern_number = current_pattern[c] % NUM_PATTERNS;
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t pattern = pgm_read_byte_near(PatternMatrix + (pattern_number * NUM_LEDS) + i);
 
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
// color1 is the primary color, color2 is a trail color
// background is the background color
//
void tunnelvision(CHSV color1, CHSV color2, CHSV background, uint8_t c) {
  led[c].fill(background);  
  uint8_t i = shows[c].getCycle() % 5;
  if (i < 4) { 
    draw_ring(i, color1, c); 
  }      
  if (i != 0) { 
    draw_ring(i - 1, color2, c); 
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

  jsonReadings["type"] = MESH_MESSAGE;
  jsonReadings["c"] = c;
  // jsonReadings["morph"] = shows[c].isMorphing();
  jsonReadings["cycle"] = (const double)shows[c].getCycle();
  jsonReadings["smcycle"] = (const double)shows[c].getSmallCycle();
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
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
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

  current_pattern[c] = int(myObject["p"]) % NUM_PATTERNS;

  int show_number = int(myObject["s"]) % NUM_SHOWS;
  if (current_show[c] != show_number) {
    current_show[c] = show_number;
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
    shows[i].pickRandomColorSpeeds();
    shows[i].pickRandomWait();
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
  // leds[10] = CRGB(0,0,0);  // led 10 (orig 25) is the problem
  turn_off_spacer_leds();
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
