#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Lightbrella with 8 spokes
//
//  11/8/22
//
//  On the Adafruit Feather
//
//  MESH network! No phone connection
//
//  Speaking & Hearing
//
#define SPOKE_LEDS 25
#define NUM_SPOKES 8
#define NUM_LEDS (SPOKE_LEDS * NUM_SPOKES)  

uint8_t bright = 128;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0-255)

#define MAX_LUMENS 10000  // estimated maximum brightness until flickering

#define DELAY_TIME 20  // in milliseconds (20ms=50fps, 25ms=40fps)
#define SMOOTHING 1   // 0 = no smoothing, lower the number = more smoothing

#define DATA_PIN 0  // data = 7 (Teensy) or 0 (Feather)
#define POT_PIN  17  // ADC  clock = 8 (Teensy) or 2 (Feather)

#define MIN_POT   169  // minimum pot reading
#define MAX_POT   212  // maximum pot reading
#define MAX_CHANGE  5  // fastest update to reading

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define XX  255

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
#define BLACK  CHSV(0, 0, 0)

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[NUM_LEDS];  // The Leds themselves

uint8_t param[] = { 0, 0 };

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

// Shows
#define START_SHOW_CHANNEL_A   9  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B   0  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 11

// Clocks and time
uint8_t show_duration = 80;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 48;  // 0 = no fading, to 255 = always be fading
uint32_t max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // *2! 50% = all on, 50% = all off, and rise + decay on edges

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

#define ARE_CONNECTED true  // Are the Fish talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

// POTS
#define HAS_POT false  // Knob attached
#define IS_BRIGHT_POT true  // true = brightness, false = hue width
uint8_t pot_value = 255;

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

  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);  // Start Channel B offset at halfway through show
  
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();
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
        dim_fall(i, false);
        break;
      case 1:
        dim_fall(i, true);
        break;
      case 2:
        pendulum_wave(i, false);
        break;
      case 3:
        morphChain(i, false);
        break;
      case 4:
        morphChain(i, true);
        break;
      case 5:
        confetti(i);
        break;
      case 6:
        juggle_fastled(i);
        break;
      case 7:
        windmill(i);
        break;
      case 8:
        circle_wave(i);
        break;
      case 9:
        verticle_wipe(i);
        break;
      default:
        rotating_blade(i);
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
//  check_pot();  // check the knob
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

  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  // current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
}

void check_pot(void) {
  if (HAS_POT) {

    uint8_t curr_value = map(analogRead(POT_PIN), MIN_POT, MAX_POT, 0, 255);
        
    if (abs(curr_value - pot_value) > 5) {
      // Serial.printf("Pot %d, stored %d, diff %d\n", curr_value, pot_value, abs(curr_value - pot_value));
      // Serial.println(curr_value);
      if (curr_value > pot_value) {
        if (255 - pot_value < MAX_CHANGE) {
          pot_value = 255;
        } else {
          pot_value += MAX_CHANGE;
        }
      } else {
        if (pot_value < MAX_CHANGE) {
          pot_value = 0;
        } else {
          pot_value -= MAX_CHANGE;
        }
      }
      if (IS_BRIGHT_POT) {
        bright = pot_value;
        FastLED.setBrightness( bright );
        sendParameterMessage(BRIGHTNESS_COMMAND, pot_value);
      } else {
        hue_width = pot_value;
        sendParameterMessage(HUE_WIDTH_COMMAND, pot_value);
      }
    }
  }
}

void sendParameterMessage (uint8_t param, uint8_t value) {
  JSONVar jsonReadings;

  jsonReadings["type"] = IR_MESSAGE;
  jsonReadings["param1"] = param;
  jsonReadings["value1"] = value;
  jsonReadings["param2"] = EMPTY_COMMAND;
  jsonReadings["value2"] = 0;

  message = JSON.stringify(jsonReadings);
  mesh.sendBroadcast(message);
  Serial.println(message);
}


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
    param[c] = random(1, 10);
  }
  led->dimAllPixels(param[c]);  // Try different amounts
  CHSV color = led[c].wheel(shows[c].getForeColor());
  
  mirror_spokes(shows[c].getCycle() % SPOKE_LEDS, color, reversed, c);
}

void rotating_blade(uint8_t c) {
  if (shows[c].isShowStart()) {
    param[c] = random(20, 50);
  }
  led->dimAllPixels(param[c]);  // Try different amounts
  CHSV color = led[c].wheel(shows[c].getForeColor());  
  color_spoke(shows[c].getCycle() % 8, color, c);
}

void juggle_fastled(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();
  }
  // eight colored dots, weaving in and out of sync with each other
  uint8_t pixel;
  uint8_t dot_hue = 0;
  CHSV curr_color, new_color;

  led->dimAllPixels(20); // replaces: fadeToBlackBy( leds, NUM_LEDS, 20);

  for(uint8_t i=0; i < 8; i++) {
    pixel = beatsin16(i+3, 0, SPOKE_LEDS);
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

void confetti(uint8_t c) {
  led->dimAllPixels(2);  // Try different amounts
  if (random8(10) == 0) {
    shows[c].setPixeltoForeColor(random16(NUM_LEDS - 1));
  }
}

void circle_wave(uint8_t c) {
  uint8_t value;
  
  for (uint8_t s = 0; s < 5; s++) {
    for (uint8_t i = 0; i < SPOKE_LEDS; i++) {
      value = beatsin8(10, 0, 255, 0, (i * 255 / SPOKE_LEDS) + (s * 255 / 5));
      mirror_circle(i, s, led[c].gradient_wheel(shows[c].getForeColor(), smooth_value(value)), c);
    }
  }
}

void morphChain(uint8_t c, boolean reversed) {
  uint8_t color;
  for (uint8_t i = 0; i < SPOKE_LEDS; i++) {
    color = map8(sin8_C((i + shows[c].getCycle()) * shows[c].getForeColorSpeed() * 0.5), 0, sin8_C(shows[c].getBackColor()));
    mirror_spokes(i, led[c].wheel(color), reversed, c);
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
      shows[c].setPixeltoColor((s * SPOKE_LEDS) + (SPOKE_LEDS - i - 1), color);
    } else {
      shows[c].setPixeltoColor((s * SPOKE_LEDS) + i, color);
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
    shows[c].setPixeltoColor((y * SPOKE_LEDS) + x, color);
    if (y > 0 || y < 4) {
      shows[c].setPixeltoColor(((NUM_SPOKES - y) * SPOKE_LEDS) + x, color);
    }
  }
}

//
// verticle_wipe
//
void verticle_wipe(uint8_t c) {
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
  adj_brightess();
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
// log status
//
void log_status(uint8_t i) {
  Serial.print("Channel: ");
  Serial.print(i);
  Serial.print(", Show: ");
  Serial.print(current_show[i]);
  Serial.println(".");
}
