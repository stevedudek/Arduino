#include <FastLED.h>
#include <Led_Dragon_Modern.h>
#include <Shows_Dragon_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Dragon - several sections
//
//  10/7/23
//
//  Super modern clocking
//
//  For Shows & LED Libraries: Look in Documents/Arduino/libraries/Shows_Dragon_Modern
//

// Sections

#define SECTION_HEAD  0
#define SECTION_A     1
#define SECTION_B     2
#define SECTION_TAIL  3

#define NUM_SECTIONS 4

/// SHOWS
//
// symmetry: flip top and bottom  - CHECK THIS
// drops shows: make a function that draws hexagons - CHECK THIS
// packets - CHECK THIS
// patterns - CHECK THIS
// verticle moving bars (up and down) diagonal moving bars (make general) - CHECK THIS
// import Hex shows: orbits, wipers, well, rain, vert_back_forth_colors, diag_back_forth_dots

uint8_t section_list[] = { SECTION_HEAD, SECTION_A, SECTION_B, SECTION_TAIL };

// SCALES  { head: 86, a: 101, b: 97, tail: 47 }   // Include trailing LED in spacers
// SPACERS { head:  17, a:  21, b: 18, tail: 13 }

#define NUM_SCALES   (86 + 101 + 97 + 47)  // set this manually
#define NUM_SPACERS  (17 + 21 + 18 + 13)  // set this manually
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
  86, 17, 104, 12, -6,  6, -8,  //  head - changed last value from -7 to -8
 101, 21, 123, 14,  6, 14,  0,  //   A
  97, 18, 116, 14, -4,  7, -5,  //   B
  47, 13,  59,  9,  8, 11,  0,  //  tail
};
// scales = 100, LEDs = 121, spacers = 20

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 25;  // (0 = fast, 255 = slow)

#define DELAY_TIME 6  // in milliseconds (20ms=50fps, 25ms=40fps)

#define DATA_PIN  0  // 3-pin, so no clock

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define XX  9999

// Smoothing constants - lower is slower smoothing
// At 15 ms cycles: 40 = 96 ms, 30 = 127 ms, 20 = 191 ms, 10 = 382 ms, 5 = 765 ms, 2 = 1.9 s, 1 = 3.8 s
#define SMOOTHING_SHOWS_HUE    4   // Fastest full rainbow = DELAY_TIME * (255 / this value)
#define SMOOTHING_SHOWS_VALUE  40   // Fastest turn off/on = DELAY_TIME * (255 / this value)

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
CHSV BLACK = CHSV(0, 0, 0);

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

Led led[] = { Led(NUM_SCALES), Led(NUM_SCALES) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], NUM_SECTIONS, CHANNEL_A), 
                  Shows(&led[CHANNEL_B], NUM_SECTIONS, CHANNEL_B) };  // 2 Show libraries
CHSV led_buffer[NUM_SCALES];  // Required for smoothing
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

// Shows
#define NUM_SHOWS 24
#define SLOW_SHOWS 8
#define START_SHOW_CHANNEL_A  0
#define START_SHOW_CHANNEL_B  SLOW_SHOWS
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

// wait times
uint8_t show_duration = 80;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 192;  // 0 = no fading, to 255 = always be fading

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
#define IS_SPEAKING false  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = 0;
#define MAX_SILENT_TIME  10000  // Time (in cycles) without communication before marked as is_lost

String message;  // String to send to other displays
#define MESSAGE_SPACING 4   // wait this many DELAY_TIME (7 x 15 ms = every 0.105 s)

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
void updateLeds();
String getReadings();  // Prototype for reading state of LEDs

Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

//// End Mesh parameters


//// Start Mapping
//
//

//
// column table: pre_column_spacers, y_start, column_height, starting led
//

#define SPACER_FIELD 0
#define Y_START_FIELD 1
#define COLUMN_HEIGHT_FIELD 2
#define COLUMN_PIXEL_START_FIELD 3

// scales = 86, LEDs = 103, spacers = 17 + the end one
int8_t head_column_table[] PROGMEM = {
  1, 0, 5, 0,   //  0  // CHECKED mostly
  1, 0, 6, 5,   //  1
  1, -1, 6, 11,  //  2
  2, -2, 7, 17,  //  3
  1, -4, 7, 24,  //  4
  2, -5, 8, 31,  //  5
  1, -6, 8, 39,  //  6
  2, -7, 9, 47,  //  7
  1, -8, 8, 56,  //  8
  2, -8, 8, 64,  //  9
  2, -8, 7, 72,  // 10
  1, -7, 7, 79,  // 11
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

// scales = 46, LEDs = 59, spacers = 13
int8_t tail_column_table[] PROGMEM = {
  1, 0, 6, 0,   //  1  // CHECKED mostly
  1, 0, 7, 6,   //  2
  2, 0, 7, 13,  //  3
  1, 2, 6, 20,  //  4
  2, 2, 7, 26,  //  5
  1, 6, 4, 33,  //  6
  2, 6, 4, 37,  //  7
  1, 8, 3, 41,  //  8
  2, 8, 2, 44,  //  9
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
//  FastLED.addLeds<WS2812B, DATA_PIN, GBR>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);  - Owl lights
  FastLED.setBrightness( bright );  // ws2811 oddly is BRG

  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].setPointers(NUM_SECTIONS, section_list, section_table, 
                         head_column_table, a_column_table, b_column_table, tail_column_table);
    shows[i].setDelayTime(DELAY_TIME);
    shows[i].setShowDuration(show_duration);
    shows[i].setShowSpeed(show_speed);
    shows[i].setFadeAmount(fade_amount);
    shows[i].setColorSpeedMinMax();
    shows[i].pickCycleDuration();
    shows[i].resetAllClocks();
    shows[i].fillForeBlack();
    led[i].push_frame();
  }
  
  if (ONLY_RED) {  // (ArduinoBlue)
    hue_center = 0;
    hue_width = 124;
  }

  shows[CHANNEL_B].setStartShowTime(millis() - shows[CHANNEL_A].getElapsedShowTime() - (show_duration * 1000 / 2));

  for (uint16_t i = 0; i < NUM_SCALES; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
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

void updateLeds() {

  for (uint8_t i = 0; i < DUAL; i++) {
    run_shows(i);
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
    shows[i].symmetrize();  // if symmetric, mirror top and bottom
  }
  // morph_channels();  // morph together the 2 leds channels and deposit on to Channel_A
  advance_clocks();  // advance the cycle clocks and check for next show
}

void run_shows(uint8_t i) {
      
  switch (current_show[i]) {

    case 0:
      shows[i].patterns();  // works, flickers
      break;
    case 1:
      shows[i].tubes();  // works, flickers
      break;
    case 2:
      shows[i].randomTwoColorBlack();  // works, flickers
      break;
    case 3:
      shows[i].twoColor();  // works, flickers
      break;
    case 4:
      shows[i].randomOneColorBlack();  // works, flickers
      break;
    case 5:
      shows[i].morphChain();  // works, flickers a little, kinda boring
      break;
    case 6:
      shows[i].fire();  // okay
      break;
    case 7:
      shows[i].juggle_fastled();  // great
      break;
    case 8:
      shows[i].sinelon_fastled();  // great
      break;
    case 9:
      shows[i].plinko();  // great
      break;
    case 10:
      shows[i].morphChain();  // works, flickers a little, kinda boring
      break;
    case 11:
      shows[i].sawTooth();  // great
      break;
    case 12:
      shows[i].bounce();  // great
      break;
    case 13:
      shows[i].bounceGlowing();  // great
      break;
    case 14:
     shows[i].horiz_back_forth_dots();  // great
      break;
    case 15:
      shows[i].diag_back_forth_dots();  // great
      break;
    case 16:
      shows[i].pendulum_wave(false);  // great
      break;
    case 17:
      shows[i].pendulum_wave(true);  // great
      break;
    case 18:
      shows[i].twoWaves();  // great
      break;
    case 19:
      shows[i].moving_bars(false);  // great
      break;
    case 20:
      shows[i].moving_bars(true);  // great
      break;
    case 21:
      shows[i].expanding_drops(false);  // good enough
      break;
    case 22:
      shows[i].expanding_drops(true);  // good enough
      break;
    default:
      shows[i].confetti();  // great
      break;
  }
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
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  shows[i].resetAllClocks();
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  uint8_t other_channel = (i == CHANNEL_A) ? CHANNEL_B : CHANNEL_A;
  if (current_show[other_channel] < SLOW_SHOWS) {
    current_show[i] = random8(SLOW_SHOWS, NUM_SHOWS);  // do a fast show
  } else {
    current_show[i] = random8(SLOW_SHOWS);  // do a slowshow
  }
  
//  current_show[i] = random8(NUM_SHOWS);
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging

  shows[i].pickRandomColorSpeeds();
  shows[i].pickCycleDuration();
  
   log_status(i);  // For debugging
}



////// Speaking and Hearing
//
//

String getReadings(uint8_t c) {
  JSONVar jsonReadings;

  jsonReadings["type"] = MESH_MESSAGE;
  jsonReadings["c"] = c;
  jsonReadings["morph"] = shows[c].isMorphing();
  jsonReadings["cycle"] = (const double)shows[c].getCycle();
  jsonReadings["time"] = (const double)shows[c].getElapsedShowTime();
  jsonReadings["cd"] = shows[c].getCycleDuration();
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
    case FIRE_MESSAGE:
      break;  // do nothing
  }
}

void processMeshMessage(JSONVar myObject) {
  last_connection = 0;
  is_lost = false;
  
  int c = int(myObject["c"]);
  shows[c].setCycle(double(myObject["cycle"]));
  shows[c].setCycleDuration(int(myObject["cd"]));
  shows[c].setElapsedShowTime(double(myObject["time"]));
  shows[c].setForeColor(int(myObject["f"]));
  shows[c].setForeColorSpeed(int(myObject["fspd"]));
  shows[c].setBackColor(int(myObject["b"]));
  shows[c].setBackColorSpeed(int(myObject["bspd"]));

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
    shows[i].setShowSpeed(show_speed);
  }
}

void updateShowDuration(uint8_t duration) {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].setShowDuration(duration);
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  // Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

////// End Speaking and Hearing


///////// DUAL SHOW LOGIC

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B
  
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
  
  FastLED.show();
}

void morph_channels_for_pixel(uint16_t scale_number, uint16_t led_number, uint8_t fract) {
  CRGB rgb_color;
  
  // Prevent overflows
  scale_number = scale_number % NUM_SCALES;
  led_number = led_number % ACTUAL_LEDS;
  CHSV color_b = led[CHANNEL_B].getCurrFrameColor(scale_number);   // New! Changed Interp to Next. No more Interp frames.
  CHSV color_a = led[CHANNEL_A].getCurrFrameColor(scale_number);
  CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
  color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);

  // Removed smoothing. smoothing backstop. smooth constants should be large.
//  color = led[CHANNEL_A].smooth_color(led_buffer[scale_number], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);    
  led_buffer[scale_number] = color;
  // Smooth the rgb color on the led
  hsv2rgb_rainbow(color, rgb_color);
  rgb_color = led[CHANNEL_A].smooth_rgb_color(leds[led_number], rgb_color, 4);
  leds[led_number] = rgb_color;  // Set the led
}

//// End DUAL SHOW LOGIC


///////// Mapping
//
//

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
