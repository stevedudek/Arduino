#include <FastLED.h>
#include <Led_Dragon.h>
#include <Shows_Dragon.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Dragon - several sections
//
//  6/26/22
//
//  FastLED - On the Adafruit Feather
//
//  MESH network!
//
//  Listens to the Network and an IR Controller
//
//  For Shows & LED Libraries: Look in Documents/Arduino/libraries/Shows
//
//  Check the library code for anything that stores arrays
//  Additional smoothing function for RGB / RGB smoothing
//  No rotating nor symmetry easily possible
//  Masks and wipes should be possible
//
//

// Sections

#define SECTION_HEAD  0
#define SECTION_A     1
#define SECTION_B     2
#define SECTION_TAIL  3

#define NUM_SECTIONS 2
//#define NUM_SECTIONS 4

/// SHOWS
// sides in
// edges with a pulse down the middle
// double moving sine waves (normalize height + offset)  NEEDS TESTING
// gradient ribbon sign wave
// wave pendulum (fast near the head - offset so 0 is at y=0)  NEEDS TESTING
// lots of tubes shows
// verticle moving bars (up and down)


uint8_t section_list[] = { SECTION_HEAD, SECTION_TAIL };
//uint8_t section_list[] = { SECTION_HEAD, SECTION_B, SECTION_A, SECTION_TAIL };

// SCALES  { head: 86, a: 101, b: 97, tail: 46 }   // Include trailing LED in spacers
// SPACERS { head:  17, a:  21, b: 18, tail: 13 }

#define NUM_SCALES   (86 + 46)  // set this manually
#define NUM_SPACERS  (17 + 13)  // set this manually
//#define NUM_SCALES   (86 + 101 + 97 + 46)  // set this manually
//#define NUM_SPACERS  (17 + 21 + 18 + 13)  // set this manually
#define ACTUAL_LEDS  (NUM_SCALES + NUM_SPACERS)

// 1 Dragon has 330 scales and 990 LEDs x 0.04A / LED = 40A = 480W? @ 12V Power: 0.04 A/LED

#define SECT_NUM_SCALES_FIELD      0
#define SECT_NUM_SPACERS_FIELD     1
#define SECT_NUM_LEDS_FIELD        2
#define SECT_NUM_COLUMNS_FIELD     3
#define SECT_Y_OFFSET_FIELD        4
#define SECT_dX_UP_OFFSET_FIELD    5
#define SECT_dX_DOWN_OFFSET_FIELD  6
#define SECT_TABLE_NUM_FIELDS      7

// num_scales, num_spacers, num_leds (includes last spacer), num_columns, 
// y-offset, +dx, -dx (5 values)
int8_t section_table[] PROGMEM = {
  86, 17, 104, 12, -6,  6, -7,  //  head
 101, 21, 123, 14,  6, 14,  0,  //   A
  97, 18, 116, 14, -4,  7, -5,  //   B
  46, 13,  59,  9,  8, 11,  0,  //  tail
};
// scales = 100, LEDs = 121, spacers = 20

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 25;  // (0 = fast, 255 = slow)

#define DELAY_TIME 20  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN  0  // 3-pin, so no clock

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define XX  9999

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
CHSV BLACK = CHSV(0, 0, 0);

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

Led led[] = { Led(NUM_SCALES), Led(NUM_SCALES) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], NUM_SECTIONS), 
                  Shows(&led[CHANNEL_B], NUM_SECTIONS) };  // 2 Show libraries
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

// Shows
#define START_SHOW_CHANNEL_A  12
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 18

// wait times
uint8_t show_duration = 200;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 0;  // 0 = no fading, to 255 = always be fading
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

#define ARE_CONNECTED false // Are the dragons talking to each other?
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


//// Start Mapping
//
//

// These globals to be set in setup()
int8_t min_x = 0;  
int8_t max_x = 0;  
int8_t min_y = 0;  
int8_t max_y = 0;

//
// column table: pre_column_spacers, y_start, column_height, starting led
//

#define SPACER_FIELD 0
#define Y_START_FIELD 1
#define COLUMN_HEIGHT_FIELD 2
#define COLUMN_PIXEL_START_FIELD 3

// scales = 86, LEDs = 103, spacers = 17
int8_t head_column_table[] PROGMEM = {
  1, 0, 5, 0,   //  0
  1, 0, 6, 5,   //  1
  1, 0, 6, 11,  //  2
  2, 1, 7, 17,  //  3
  1, 0, 7, 23,  //  4
  2, 1, 8, 30,  //  5
  1, 1, 8, 38,  //  6
  2, 3, 9, 46,  //  7
  1, 3, 8, 55,  //  8
  2, 4, 8, 63,  //  9
  2, 4, 7, 71,  // 10
  1, 5, 7, 78,  // 11
};
// Note: 1 trailing spacer LED that will need to be accounted for

// scales = 108, LEDs = 127
int8_t a_column_table[] PROGMEM = {
  1, 0, 6, 0,   //  0
  1, 0, 7, 6,   //  1
  2, 0, 8, 13,  //  2
  1, 1, 8, 21,  //  3
  2, 1, 9, 29,  //  4
  1, 3, 8, 38,  //  5
  2, 3, 8, 46,  //  6
  1, 5, 7, 54,  //  7
  2, 5, 7, 61,  //  8
  1, 7, 6, 68,  //  9
  2, 7, 6, 74,  // 10
  1, 7, 7, 80,  // 11
  1, 6, 7, 87,  // 12
  2, 6, 7, 94,  // 13  There's a trailing LED
};

// scales = 97, LEDs = 116
int8_t b_column_table[] PROGMEM = {
  1,  0, 6, 0,   //  1
  1,  0, 7, 6,   //  2
  1, -1, 7, 13,  //  3
  2, -2, 8, 20,  //  4
  1, -3, 8, 28,  //  5
  2, -3, 8, 36,  //  6
  1, -4, 7, 44,  //  7
  2, -4, 7, 51,  //  8
  1, -5, 6, 58,  //  9
  2, -5, 6, 64,  // 10
  2, -5, 6, 70,  // 11
  1, -5, 7, 76,  // 12
  0, -5, 7, 83,  // 13
  1, -4, 7, 90,  // 14  There's a trailing LED
};

// scales = 46, LEDs = 59
int8_t tail_column_table[] PROGMEM = {
  1, 0, 6, 0,   //  1
  1, 0, 7, 6,   //  2
  2, 0, 7, 13,  //  3
  1, 2, 6, 20,  //  4
  2, 2, 7, 26,  //  5
  1, 6, 4, 34,  //  6
  2, 6, 4, 38,  //  7
  1, 8, 3, 42,  //  8
  2, 8, 2, 45,  //  9
};

//// Hex geometry
#define PIXEL_HEIGHT 1
#define PIXEL_WIDTH 0.866
#define CART_WIDTH  (256.0 / (DRAGON_SECTIONS * MAX_COLUMNS))
#define CART_HEIGHT (CART_WIDTH / 0.866)

//
//
//// End Mapping


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  
  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");
  
  FastLED.addLeds<WS2811, DATA_PIN, BRG>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );  // ws2811 oddly is BRG

  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].setPointers(NUM_SECTIONS, section_list, section_table, 
                         head_column_table, a_column_table, b_column_table, tail_column_table);
    shows[i].fillForeBlack();
    led[i].push_frame();
    
    shows[i].setColorSpeedMinMax(show_speed);
    shows[i].setWaitRange(show_speed);
  }
  
  if (ONLY_RED) {  // (ArduinoBlue)
    hue_center = 0;
    hue_width = 124;
  }

  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);  // Start Channel B offset at halfway through show

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskUpdateLeds);
  taskUpdateLeds.enable();

  
  calc_coordinate_frame();
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
        shows[i].patterns();
        break;
      case 1:
        shows[i].juggle_fastled();
        break;
      case 2:
        shows[i].sinelon_fastled();
        break;
      case 3:
        shows[i].randomFill();
        break;
      case 4:
        shows[i].twoColor();
        break;
      case 5:
        shows[i].tubes();
        break;
      case 6:
        shows[i].plinko();
        break;
      case 7:
        shows[i].randomTwoColorBlack();
        break;
      case 8:
        shows[i].morphChain();
        break;
      case 9:
        shows[i].sawTooth();
        break;
      case 10:
        shows[i].bands();
        break;
      case 11:
        shows[i].bounce();
        break;
      case 12:
        shows[i].bounceGlowing();
        break;
      case 13:
        shows[i].pendulum_wave(false);  // broken - REWRITE BY SECTION
        break;
      case 14:
        shows[i].pendulum_wave(true);  // broken - REWRITE BY SECTION
        break;
      case 15:
        shows[i].twoWaves();  // broken
        break;
      case 16:
        shows[i].fire();  // works
        break;
      default:
        shows[i].confetti();  // works
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
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();

  // Optional: Set new show's colors to those of the other channel
  uint8_t other_channel = (i == 0) ? 1 : 0 ;
  shows[i].setForeColor(shows[other_channel].getForeColor());
  shows[i].setBackColor(shows[other_channel].getBackColor());
  
//  log_status(i);  // For debugging
}

////// Shows
//
//

//
//
////// End Shows



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
//  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
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


///////// DUAL SHOW LOGIC

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  uint8_t column_height;
  uint16_t scale_number = 0;  // which pixel we are on
  uint16_t led_number = 0;    // which led we are on (include spacers)
  uint16_t start_column_led_number, adj_led_number;

  /// Iterate one SECTION at a time
  for (uint8_t section = 0; section < NUM_SECTIONS; section++) {
    
    /// Iterate one COLUMN at a time over the number of columns for the section
    for (uint8_t column = 0; column < get_section_table_value(section, SECT_NUM_COLUMNS_FIELD); column++) {
  
      // Turn off the space LEDS
      for (uint8_t spacer = 0; spacer < get_column_table_value(column, SPACER_FIELD, section); spacer++) {
        leds[led_number] = BLACK;
        led_number++;
      }
  
      start_column_led_number = led_number;
      column_height = get_column_table_value(column, COLUMN_HEIGHT_FIELD, section);
      
      for (uint8_t scale = 0; scale < column_height; scale++) {
        adj_led_number = (column % 2 == 0) ? led_number : (start_column_led_number + column_height - scale - 1) ;  // serpentine reverse column
        morph_channels_for_pixel(scale_number, adj_led_number, fract);
        led_number++;
        scale_number++;
      }
    }
    
    led_number++;  // all sections end with a trailing pixel except the tial
  }
}

void morph_channels_for_pixel(uint16_t scale_number, uint16_t led_number, uint8_t fract) {

  // Prevent overflows
  scale_number = scale_number % NUM_SCALES;
  led_number = led_number % ACTUAL_LEDS;
  
  CRGB color = led[CHANNEL_A].getInterpHSV(led[CHANNEL_B].getInterpFrameColor(scale_number), 
                                           led[CHANNEL_A].getInterpFrameColor(scale_number), 
                                           fract);  // interpolate a + b channels
  
  if (SMOOTHING > 0) {
    // Smooth the LED's new color against the LED's current color
    color = led[CHANNEL_A].smooth_rgb_color(leds[led_number], color, SMOOTHING);
  }
  leds[led_number] = color;
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


///////// Mapping
//
//

void test_neighs(uint8_t c) {

  shows[c].fillForeBlack();
  
  for (int8_t y = min_y; y < max_y; y++) {
    for (int8_t x = min_x; x < max_x; x++) {
      uint16_t scale = get_scale_for_coord(get_coord_from_xy(x, y));
      if (scale != XX && scale % 20 == 0) {
        shows[c].setPixeltoBackColor(scale);
        for (int dir = 0; dir < 6; dir++) {
          shows[c].setPixeltoForeColor(get_scale_for_coord(get_coord_in_dir(dir, get_coord_from_xy(x, y))));
        }
      }
    }
  }
}


//
// get scale for xy (column, row) - return the scale number (or XX if out of bounds)
//
uint16_t get_scale_for_coord(uint16_t coord) {
  if (is_coord_inbounds(coord)) {
    int8_t dx = 0;
    int8_t dy = 0;
    uint16_t dscale = 0;  
    int8_t section_width;
    int8_t x = get_x_from_coord(coord);
    int8_t y = get_y_from_coord(coord);
    
    for (uint8_t section = 0; section < NUM_SECTIONS; section++) {
      section_width = get_section_table_value(section, SECT_NUM_COLUMNS_FIELD);
      if (x < dx + section_width) {
        // Found the correct section
        int8_t column = x - dx;
        int8_t row = y - dy;
//        Serial.printf("section, column, row (%d, %d, %d)\n", section, column, row);
        int8_t y_start = get_column_table_value(column, Y_START_FIELD, section);
        if (row >= y_start && row < y_start + get_column_table_value(column, COLUMN_HEIGHT_FIELD, section)) {
          uint16_t scale = dscale + (row - y_start) + get_column_table_value(column, COLUMN_PIXEL_START_FIELD, section);
//          Serial.printf("x,y (%d, %d) = (%d, %d) = %d\n", x, y, column, row, scale);
          return scale;
        } else {
          return XX;
        }
      }
      dx += section_width;
      dy += get_section_table_value(section, SECT_Y_OFFSET_FIELD);
      dscale += get_section_table_value(section, SECT_NUM_SCALES_FIELD);
    }
  }
  return XX;  // out of bounds
}

int8_t get_column_table_value(uint8_t column, uint8_t field_index, uint8_t section) {
  // 4 fields per column
  uint8_t max_columns = get_section_table_value(section, SECT_NUM_COLUMNS_FIELD);
  
  switch (section_list[section]) {
    case SECTION_HEAD:
      return pgm_read_byte_near(head_column_table + ((column % max_columns) * 4) + field_index);
    case SECTION_A:
      return pgm_read_byte_near(a_column_table + ((column % max_columns) * 4) + field_index);
    case SECTION_B:
      return pgm_read_byte_near(b_column_table + ((column % max_columns) * 4) + field_index);
    case SECTION_TAIL:
      return pgm_read_byte_near(tail_column_table + ((column % max_columns) * 4) + field_index);
    default:
      Serial.println("Out of bounds for section type!");
      return 0;
  }
}

//
// Get Coord in Dir - directions go 0 to 5 - does not say whether in bounds!
//
uint16_t get_coord_in_dir(uint8_t dir, uint16_t coord) {
  const int8_t dx[] = { 0, 1, 1, 0, -1, -1 };
  const int8_t dy[] = { 1, 1, 0, -1, 1, 0 };
  
  int8_t x = get_x_from_coord(coord);
  int8_t y = get_y_from_coord(coord);
  
  if (x % 2 && dx[dir % 6]) {  // odd
    y -= 1;
  }
  
  x += dx[dir % 6];
  y += dy[dir % 6];

  return get_coord_from_xy(x, y);
}

bool is_coord_inbounds(uint16_t coord) {
  int8_t x = get_x_from_coord(coord);
  int8_t y = get_y_from_coord(coord);
  return (x >= min_x && x < max_x && y >= min_y && y < max_y);
}

int8_t get_x_from_coord(uint16_t coord) {
  return lowByte(coord>>8);
}

int8_t get_y_from_coord(uint16_t coord) {
  return lowByte(coord);
}

uint16_t get_coord_from_xy(int8_t x, int8_t y) {
  return (lowByte(x)<<8) | lowByte(y);
}

//
// Calc Coordinate Frame - set once the global bounds to the cartesian space
//
void calc_coordinate_frame() {
  int8_t dy = 0;
  int8_t temp;  // max is not playing nice with this temp indirection
  
  for (uint8_t section = 0; section < NUM_SECTIONS; section++) {
    max_x += get_section_table_value(section, SECT_NUM_COLUMNS_FIELD);
    temp = dy + get_section_table_value(section, SECT_dX_UP_OFFSET_FIELD);
    max_y = max(max_y, temp);
    temp = dy + get_section_table_value(section, SECT_dX_DOWN_OFFSET_FIELD);
    min_y = min(min_y, temp);

    dy += get_section_table_value(section, SECT_Y_OFFSET_FIELD);
  }
  Serial.printf("Dragon coords: x = (%d, %d), y = (%d, %d)\n", min_x, max_x, min_y, max_y);
}

//
// Section Manipulation
//
int8_t get_section_table_value(uint8_t section, uint8_t field_index) {
  return pgm_read_byte_near(section_table + (section_list[section] * SECT_TABLE_NUM_FIELDS) + field_index);
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
