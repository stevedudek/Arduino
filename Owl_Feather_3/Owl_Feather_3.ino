#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Owls with FastLED
//
//  MESH network!
//
//  Listens to the Network and an IR Controller
//
//  12/4/22
//
//  The head has its own shows, but the same timing and transitions as the body
//
#define NUM_FEATHERS 70
#define ACTUAL_LEDS 465

#define NUM_HEAD_PARTS 50
#define ACTUAL_HEAD_LEDS 50

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 196;  // (0 = fast, 255 = slow)

#define DELAY_TIME 30  // in milliseconds (20ms=50fps, 25ms=40fps)
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 0  // 7
#define DATA_PIN_HEAD 2  //  8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_FEATHERS), Led(NUM_FEATHERS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_FEATHERS];  // For smoothing
CRGB leds[ACTUAL_LEDS];  // Hook for FastLED library

Led head_led[] = { Led(NUM_HEAD_PARTS), Led(NUM_HEAD_PARTS) };  // Class instantiation of the 2 Led libraries
Shows head_shows[] = { Shows(&head_led[CHANNEL_A]), Shows(&head_led[CHANNEL_B]) };  // 2 Show libraries
CHSV head_led_buffer[NUM_HEAD_PARTS];  // For smoothing
CRGB head_leds[ACTUAL_HEAD_LEDS];  // Hook for FastLED library

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
CHSV BLACK = CHSV(0, 0, 0);

// Shows
#define START_SHOW_CHANNEL_A  0  // Channels A starting show
#define START_SHOW_CHANNEL_B  1  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 10

#define START_HEAD_SHOW_CHANNEL_A  0  // Channels A starting show
#define START_HEAD_SHOW_CHANNEL_B  1  // Channels B starting show
uint8_t head_current_show[] = { START_HEAD_SHOW_CHANNEL_A, START_HEAD_SHOW_CHANNEL_B };
#define NUM_HEAD_SHOWS 2

// wait times apply to both the head and the body
uint8_t show_duration = 30;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 128;  // 0 = no fading, to 255 = always be fading
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

#define ARE_CONNECTED false  // Are the dragons talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = 0;
#define MAX_SILENT_TIME  10000  // Time (in cycles) without communication before marked as is_lost

String message;  // String to send to other displays

#define MESSAGE_REPEAT 2   // send this many additional copy messages
#define MESSAGE_SPACING 5   // wait this many cycles
Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

#define IR_MESSAGE  0
#define MESH_MESSAGE  1
#define PRESET_MESSAGE  2
#define FIRE_MESSAGE  3

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


//// Start Mapping
//
//

//
// column table: pre_column_spacers, y_start, column_height, starting led
//

#define MAX_COLUMNS 10

#define SPACER_FIELD 0  // spacer leds before the column starts
#define Y_START_FIELD 1
#define COLUMN_HEIGHT_FIELD 2
#define COLUMN_PIXEL_START_FIELD 3
#define COLUMN_FULL 4  // Whether a 3-width or 6-width start to the column

// scales = 86, LEDs = 103, spacers = 17 + the end one
uint8_t map_table[] PROGMEM = {
  0, 1, 6,  0, 6,  //  0
  7, 0, 7,  6, 6,  //  1
  6, 0, 8, 13, 3,  //  2
  6, 0, 7, 21, 6,  //  3
  6, 0, 8, 28, 3,  //  4
  6, 0, 7, 36, 6,  //  5
  6, 0, 8, 43, 3,  //  6
  6, 0, 7, 51, 6,  //  7
  7, 1, 6, 58, 6,  //  8
  7, 2, 5, 64, 6,  //  9
};

//// Hex geometry
#define PIXEL_HEIGHT 1
#define PIXEL_WIDTH 0.866
#define CART_WIDTH  (256.0 / MAX_COLUMNS)
#define CART_HEIGHT (CART_WIDTH / PIXEL_WIDTH)

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

  FastLED.addLeds<WS2812B, DATA_PIN, BRG>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<WS2812B, DATA_PIN_HEAD, BRG>(head_leds, ACTUAL_HEAD_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );
   
  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);  // Start Channel B offset at halfway through show
  head_shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);  // Start Channel B offset at halfway through show
  
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i].setColorSpeedMinMax(show_speed);
    shows[i].setWaitRange(show_speed);

    head_shows[i] = Shows(&head_led[i]);  // Show library - reinitialized for led mappings
    head_shows[i].fillForeBlack();
    head_led[i].push_frame();
    head_shows[i].setColorSpeedMinMax(show_speed);
    head_shows[i].setWaitRange(show_speed);
  }

  for (uint8_t i = 0; i < NUM_FEATHERS; i++) {
    led_buffer[i] = BLACK;
  }

  for (uint8_t i = 0; i < NUM_HEAD_PARTS; i++) {
    head_led_buffer[i] = BLACK;
  }

  if (ONLY_RED) {  // (ArduinoBlue)
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

void updateLeds() {

  for (uint8_t i = 0; i < DUAL; i++) {
      
    switch (current_show[i]) {
    
      case 0:
        test_pattern(i);
        break;
      case 1:
        shows[i].morphChain();
        break;
      case 2:
        shows[i].twoColor();
        break;
      case 3:
        shows[i].sawTooth();
        break;
      case 4:
        shows[i].packets();
        break;
      case 5:
        shows[i].packets_two();
        break;
      case 6:
        shows[i].sinelon_fastled();
        break;
      case 7:
        shows[i].bpm_fastled();
        break;
      case 8:
        shows[i].juggle_fastled();
        break;
      default:
        shows[i].bands();
        break;
    }
  
    switch (head_current_show[i]) {
    
      case 0:
        head_shows[i].allOn();
        break;
      default:
        head_shows[i].sinelon_fastled();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
    head_shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
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
    head_shows[i].advanceClock();
    
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
  head_led[i].push_frame();
  
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(show_speed);
  shows[i].setColorSpeedMinMax(show_speed);

  head_shows[i].resetAllClocks();
  head_shows[i].turnOnMorphing();
  head_shows[i].setWaitRange(show_speed);
  head_shows[i].setColorSpeedMinMax(show_speed);
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  head_current_show[i] = random8(NUM_HEAD_SHOWS);
  
  // current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();

  head_shows[i].pickRandomColorSpeeds();
  head_shows[i].pickRandomWait();
  
  // log_status(i);  // For debugging
}

//
// test pattern - map the pitch of 6
//
void test_pattern(uint8_t c) {
  for (uint8_t i = 0; i < NUM_FEATHERS; i++) {
    if (i % 2) {
      shows[c].setPixeltoHue(i, 0);
    } else {
      shows[c].setPixeltoBlack(i);
    }
  }
}


////// Speaking and Hearing
//
//

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
  jsonReadings["hs"] = head_current_show[c];
  jsonReadings["w"] = shows[c].getWait();
  
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

  switch (int(myObject["type"])) {
    case IR_MESSAGE:
      processIrMessage(myObject);
      break;
    case PRESET_MESSAGE:
      processPresetMessage(myObject);
      break;
    case MESH_MESSAGE:
      if (!IS_SPEAKING) {
        processMeshMessage(myObject);
      }
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

  head_shows[c].setCycle(double(myObject["cycle"]));
  head_shows[c].setSmallCycle(double(myObject["smcycle"]));
  head_shows[c].setForeColor(int(myObject["f"]));
  head_shows[c].setForeColorSpeed(int(myObject["fspd"]));
  head_shows[c].setBackColor(int(myObject["b"]));
  head_shows[c].setBackColorSpeed(int(myObject["bspd"]));
  head_shows[c].setWait(int(myObject["w"]));

  if (boolean(myObject["morph"])) {
    head_shows[c].turnOnMorphing();
    head_shows[c].turnOnMorphing();
  } else {
    head_shows[c].turnOffMorphing();
    head_shows[c].turnOffMorphing();
  }

  int show_number = int(myObject["s"]);
  int head_show_number = int(myObject["hs"]);
  if (current_show[c] != show_number) {
    current_show[c] = show_number % NUM_SHOWS;
    head_current_show[c] = head_show_number % NUM_HEAD_SHOWS;
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
    head_shows[i].setWaitRange(show_speed);
    head_shows[i].setColorSpeedMinMax(show_speed);
  }
}

void updateShowDuration(uint8_t duration) {
  
  uint32_t new_max_small_cycle = (duration * 2 * (1000 / DELAY_TIME));  // update max_small_cycle
  float cycle_ratio = float(new_max_small_cycle) / max_small_cycle;

  // Need to change smallCycle so it fits within the new duration
  uint32_t newSmallCycle = uint32_t(shows[CHANNEL_A].getSmallCycle() * cycle_ratio) % new_max_small_cycle;
  shows[CHANNEL_A].setSmallCycle(newSmallCycle);
  shows[CHANNEL_B].setSmallCycle((newSmallCycle + (new_max_small_cycle / 2)) % new_max_small_cycle);
  head_shows[CHANNEL_A].setSmallCycle(newSmallCycle);
  head_shows[CHANNEL_B].setSmallCycle((newSmallCycle + (new_max_small_cycle / 2)) % new_max_small_cycle);
  
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

////// End Speaking and Hearing

/*
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

    light_feather(i, color);
    led_buffer[i] = color;
  }

  for (uint8_t i = 0; i < NUM_HEAD_LEDS; i++) {
    CHSV color_b = head_led[CHANNEL_B].getInterpFrameColor(i);
    CHSV color_a = head_led[CHANNEL_A].getInterpFrameColor(i);
    CHSV color = head_led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = narrow_palette(color);
    if (SMOOTHING > 0) {
      color = head_led[CHANNEL_A].smooth_color(head_led_buffer[i], color, SMOOTHING);  // smoothing
    }

    head_leds[i] = color;  // Need a conversion function here
    head_led_buffer[i] = color;
  }
}

void light_feather(uint8_t feather, CHSV color) {
  uint16_t pixel = feather * 6;
  if (feather >= 12) {
    pixel++;
  }
  for (uint8_t i = 0; i < 6; i++) {
    leds[pixel + i] = color;
  }
}
*/

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  uint8_t column_height, edge_width, pixel_width;
  uint8_t feather_number = 0;  // which pixel we are on
  uint16_t led_number = 0;    // which led we are on (include spacers)
  uint8_t adj_feather_number;
    
  /// Iterate one COLUMN at a time over the number of columns for the section
  for (uint8_t column = 0; column < MAX_COLUMNS; column++) {

    // Turn off the spacer LEDS
    for (uint8_t spacer = 0; spacer < get_column_table_value(column, SPACER_FIELD); spacer++) {
      leds[led_number] = BLACK;
      led_number++;
    }

    column_height = get_column_table_value(column, COLUMN_HEIGHT_FIELD);
    edge_width = get_column_table_value(column, COLUMN_FULL);
    
    for (uint8_t feather = 0; feather < column_height; feather++) {
      // serpentine reverse column
      adj_feather_number = (column % 2 == 0) ? feather_number : (feather_number + column_height - feather - 1) ;
      pixel_width = 6;  // default
      if (edge_width == 3 && (feather == 0 || feather == column_height - 1)) {
        pixel_width = 3;
      }
      morph_channels_for_pixel(adj_feather_number, led_number, pixel_width, fract);
      feather_number++;
      led_number += pixel_width;
    }
  }
}

void morph_channels_for_pixel(uint8_t feather_number, uint16_t led_number, uint8_t feather_size, uint8_t fract) {

  CHSV color = led[CHANNEL_A].getInterpHSV(led[CHANNEL_B].getInterpFrameColor(feather_number), 
                                           led[CHANNEL_A].getInterpFrameColor(feather_number), 
                                           fract);  // interpolate a + b channels
  color = narrow_palette(color);
  if (SMOOTHING > 0) {
    color = led[CHANNEL_A].smooth_color(led_buffer[feather_number], color, SMOOTHING);  // smoothing
  }
  led_buffer[feather_number] = color;
  distribute_color(led_number, feather_size, color);

  /*
  for (uint16_t i = led_number; i < led_number + feather_size ; i++) {
    // TODO: function here for distributive lighting
    leds[i] = color;
  }
  */
}

void distribute_color(uint16_t led_number, uint8_t feather_size, CHSV color) {
  uint8_t fill_order[6] = { 2, 3, 1, 4, 0, 5 };
  
  if (feather_size == 3) {
    for (uint16_t i = led_number; i < led_number + feather_size ; i++) {
      // TODO: function here for distributive lighting for 3 pixels
      leds[i] = color;
    }
  } else {
    uint8_t middle_led = color.h / 42;
    for (uint8_t i = 0; i < 6 ; i++) {
      if (i < middle_led) {
        leds[led_number + fill_order[i]] = CHSV(color.h, color.s, 255);
      } else if (i == middle_led) {
        leds[led_number + fill_order[i]] = CHSV(color.h, color.s, 5.93 * (color.h % 43));
      } else {
        leds[led_number + fill_order[i]] = CHSV(color.h, color.s, 0);
      }
    }
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
  uint8_t h1 = (hue_center - (hue_width / 2)) % MAX_COLOR;
  uint8_t h2 = (hue_center + (hue_width / 2)) % MAX_COLOR;
  color.h = map8(color.h, h1, h2 );
  if (color.s != 0) {
    color.s = saturation;  // Reset saturation
  }
  return color;
}

///////// Mapping
//
//

uint8_t get_column_table_value(uint8_t column, uint8_t field_index) {
  return pgm_read_byte_near(map_table + ((column % MAX_COLUMNS) * 5) + field_index);  // 5 fields per column
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
