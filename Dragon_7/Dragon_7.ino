#include <FastLED.h>
#include <Led_Dragon.h>
#include <Shows_Dragon.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
//#include <math.h>  # Do I need this?

//
//  Dragon - several sections
//
//  6/7/22
//
//  FastLED - On the Adafruit Feather
//
//  MESH network!
//
//  Listens to the Network and an IR Controller
//
//  For Shows & LED Libraries: Look in Documents/Arduino/libraries/Shows
//
//  No Patterns: too expensive
//  Calculate neighbors: don't use a lookup table
//      Need to pull out of the library shows that use neighbors:
//          plinko, bounce, bounceGlowing
//  Check the library code for anything that stores arrays
//  Additional smoothing function for RGB / RGB smoothing
//  No rotating nor symmetry easily possible
//  Game of life unlikely, unless the life table can be used for other applications
//  Masks and wipes should be possible
//
//  Functions
//  get_x_cart(x_hex, y_hex); get_y_cart(x_hex, y_hex);
//  get_x_cart(scale); get_y_cart(scale);
//

// Sections

#define SECTION_HEAD  0
#define SECTION_A     1
#define SECTION_B     2
#define SECTION_TAIL  3

#define NUM_SECTIONS 1

uint8_t section_list[] = { SECTION_B };
//uint8_t section_list[] = { SECTION_HEAD, SECTION_A, SECTION_B, SECTION_TAIL };

// SCALES  { head: 86, a: 101, b: 97, tail: 46 }   // Include trailing LED in spacers
// SPACERS { head:  17, a:  21, b: 18, tail: 13 }

#define NUM_SCALES   (97)  // set this manually
#define NUM_SPACERS  (18)  // set this manually
//#define NUM_SCALES   (86 + 101 + 97 + 46)  // set this manually
//#define NUM_SPACERS  (17 + 21 + 18 + 13)  // set this manually
#define ACTUAL_LEDS  (NUM_SCALES + NUM_SPACERS)

// 1 Dragon has 330 scales and 990 LEDs x 0.04A / LED = 40A = 480W? @ 12V
// Power: 0.04A/LED

#define SECT_NUM_SCALES_FIELD  0
#define SECT_NUM_SPACERS_FIELD 1
#define SECT_NUM_LEDS_FIELD    2
#define SECT_NUM_COLUMNS_FIELD 3
#define SECT_Y_OFFSET_FIELD    4

// num_scales, num_spacers, num_leds, num_columns, y-offset (5 values)
const int8_t section_table[] PROGMEM = {
  86, 17, 103, 12, -5,   //  head  (ToDo: Check that -5)
  46, 13,  59,  9,  9,   //   A
  97, 18, 115, 14, -4,   //   B
  46, 13,  59,  9,  9,   //  tail
};
// scales = 100, LEDs = 121, spacers = 20

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 25;  // (0 = fast, 255 = slow)

#define DELAY_TIME 15  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
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
#define START_SHOW_CHANNEL_A  0  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B  1  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 12

// wait times
uint8_t show_duration = 20;  // Typically 30 seconds. Size problems at 1800+ seconds.
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

#define ARE_CONNECTED false // Are the dragons talking to each other?
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

//
// column table: pre_column_spacers, y_start, column_height, starting led
//

// scales = 86, LEDs = 103, spacers = 17
const uint8_t head_column_table[] PROGMEM = {
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
const int8_t a_column_table[] PROGMEM = {
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
const int8_t b_column_table[] PROGMEM = {
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
const int8_t tail_column_table[] PROGMEM = {
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

#define SPACER_FIELD 0
#define Y_START_FIELD 1
#define COLUMN_HEIGHT_FIELD 2
#define COLUMN_PIXEL_START_FIELD 3

//// Hex geometry
#define PIXEL_HEIGHT 1
#define PIXEL_WIDTH 0.866
#define CART_WIDTH  (256.0 / (DRAGON_SECTIONS * MAX_COLUMNS))
#define CART_HEIGHT (CART_WIDTH / 0.866)

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  
  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");
  
  FastLED.addLeds<WS2811, DATA_PIN>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );

  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();

//    shows[i].setColumnTableUp(tail_column_table);
    
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
        shows[i].lightRunUp();
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
        shows[i].randomColors();
        break;
      case 6:
        shows[i].randomOneColorBlack();
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
      default:
        shows[i].confetti();
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
        adj_led_number = (column % 2) ? led_number : (start_column_led_number + column_height - scale - 1) ;  // serpentine reverse column
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
// get scale for xy (column, row) - return the scale number (or XX if out of bounds)
//
/*
uint16_t get_scale_for_xy(int8_t x, int8_t y) {
  if (x < 0 || y < 0 || x >= MAX_COLUMNS || y >= MAX_ROWS) {
    return XX;  // out of bounds
  }
  uint8_t col_pixel_start = get_column_table_value(x, Y_START_FIELD);
  if (y < col_pixel_start || y >= col_pixel_start + get_column_table_value(x, COLUMN_HEIGHT_FIELD)) {
    return XX;  // out of bounds
  }
  return get_column_table_value(x, COLUMN_PIXEL_START_FIELD) + (y - col_pixel_start);
}

//
// color by cartesian coord - iterate gently over all scales
//
void color_by_cart_coord(uint8_t c) {
  uint8_t intensity;
  uint16_t scale = 0;
  float x_cart = 0;
  float y_cart = 0;
  uint8_t hue;
  uint8_t back_hue = shows[c].getBackColor();
  
  for (uint8_t x = 0; x < MAX_COLUMNS; x++) {
    uint8_t y_start = get_column_table_value(x, Y_START_FIELD);
    uint8_t y_end = y_start + get_column_table_value(x, COLUMN_HEIGHT_FIELD);
    y_cart = get_ycart_for_y(x, y_start) * CART_HEIGHT;
    
    for (uint8_t y = y_start; y < y_end; y++) {
      // Do something and store
      // Check wipes in previous software versions
      intensity = uint8_t(x_cart + y_cart);
      hue = beatsin8(1, 0, 255, 0, intensity);
      // shows[c].setPixeltoHue(scale, shows[c].IncColor(back_hue, hue));  // FIX!
      shows[c].setPixeltoHue(scale, shows[c].IncColor(hue, (x * 5) + (y * 2)));
      
      y_cart += CART_HEIGHT;
      scale += 1;
    }
    x_cart += CART_WIDTH;
  }
}

float get_xcart_for_x(int8_t x) {
  return x * 0.866;
}

float get_ycart_for_y(int8_t x, int8_t y) {
  return y - ((x % 2) * 0.5);
}
*/

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
  return pgm_read_byte_near(section_table + (section_list[section] * 5) + field_index);
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
