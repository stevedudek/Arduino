#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
//
//  Snowflakes — For 2023 Burning Man
//
//  8/9/2023
//
//  Mesh network for the Feather Huzzah
//
#define SNOWFLAKE_NUMBER 2  // 0 = Small Simple, 1 = Medium Pointy, 
                            // 2 = Large Complex, 3 = Medium Inward

#define SPOKE_LENGTH 12  // [8, 10, 12, 10]
#define HUB_LENGTH    1  // [1,  2,  1,  1]

#define ARE_CONNECTED true  // Are the Fish talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0-255)

#define DELAY_TIME 20  // in milliseconds (20ms=50fps, 25ms=40fps)
#define SMOOTHING 1   // 0 = no smoothing, lower the number = more smoothing

#define SPOKE_LEDS  (SPOKE_LENGTH * 6)  // Don't change
#define HUB_LEDS  (HUB_LENGTH * 6)  // Don't change
#define NUM_LEDS  (SPOKE_LEDS + HUB_LEDS + 1)  // Don't change

#define VARIABLE_SPOKE_BRIGHTNESS true
uint8_t spoke_brightness = 96;  // Dim the spokes (not the center or hub) by * X / 256
uint8_t brightness_clock = 0;  // Don't change
// 96: center bright, 128: center a little bright 192: even lighting, 256: center dim

#define DATA_PIN  0
#define CLOCK_PIN 2

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // The Leds themselves
CHSV led_buffer[NUM_LEDS];  // For smoothing

// Shows
#define START_SHOW_CHANNEL_A  1  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B  0  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 1 };
boolean has_black_pattern[] = { false, false };
#define NUM_SHOWS 12
#define SINGLE_SPOKE_SHOWS 4  // Shows here and above can have a single spoke

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
#define BLACK  CHSV(0, 0, 0)

// Clocks and time
uint8_t show_duration = 80;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 48;  // 0 = no fading, to 255 = always be fading
uint32_t max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // *2! 50% = all on, 50% = all off, and rise + decay on edges

// symmetries - 6 arms
#define NUM_SYMMETRIES 5
uint8_t symmetry[] = { 2, 2 };  // 0-4
boolean invert[] = { false, false };  // Whether to invert (0 <-> 1) patterned shows
boolean switch_axis[] = { false, false };
const uint8_t symmetries[] = { 1, 2, 3, 6, 6 };  // Don't change

#define MAX_GROUP_LENGTH 5  // How many pixels in a group (row-width)
#define MAX_GROUPS 5  // How many groups in a Snowflake (column-length)
#define END_GRP 99  // Don't change any of these

#define NUM_PATTERNS 6   // Total number of patterns

//// Communication start

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

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
void updateLeds();
String getReadings();  // Prototype for reading state of LEDs

Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

//// Communication end


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");
  
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( bright );
  
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();
    set_symmetry(i);
    shows[i].setWaitRange(5, 30);  // How fast animation happens
    shows[i].setColorSpeedMinMax(4, 40); // Speed up color changing
    shows[i].turnOnBlur();  // And leave it on
  }
  
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = BLACK;
  }

  if (ONLY_RED) {
    hue_center = 0;
    hue_width = 124;
  }

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
        patternsBW(i);
        break;
      case 1:
        shows[i].twoColor();
        break;
      case 2:
        shows[i].randomTwoColorBlack();
        break;
      case 3:
        shows[i].morphChain();
        break;
      case 4:
        shows[i].lightWave();
        break;
      case 5:
        shows[i].sawTooth();
        break;
      case 6:
        shows[i].lightRunUp();
        break;
      case 7:
        shows[i].sinelon_fastled();
        break;
      case 8:
        shows[i].juggle_fastled();
        break;
      case 9:
        shows[i].randomColors();
        break;
      case 10:
        shows[i].randomFill();
        break;
      default:
        shows[i].bands();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  update_leds();
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
  
  if (VARIABLE_SPOKE_BRIGHTNESS) { 
    EVERY_N_SECONDS(1) { advance_brightness_clock(); }  // in -> out takes 2 minutes
  };
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

// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  led[i].fillBlack();
  led[i].push_frame();
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(show_speed);
  shows[i].setColorSpeedMinMax(show_speed);
  
//  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  // Switch between a patterns show and all the other shows
  current_pattern[i] = random8(NUM_PATTERNS);
  has_black_pattern[i] = (random8(2) == 0) ? true : false ;
  current_show[i] = is_other_channel_show_zero(i) ? random8(2, NUM_SHOWS) : 0 ;
  //  current_show = (current_show + 1) % NUM_SHOWS;  // For debugging
  
  pick_symmetry(i);
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
}

boolean is_other_channel_show_zero(uint8_t channel) {
  if (channel == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

////// Speaking and Hearing

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
  jsonReadings["sym"] = symmetry[c];
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

  int sym = int(myObject["sym"]);
  if (symmetry[c] != sym) {
    symmetry[c] != sym;
    set_symmetry(c);
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

//
// advance_brightness_clock
//
void advance_brightness_clock() {
  // Clock goes up by evens to 254
  if (brightness_clock % 2 == 0) {
    brightness_clock += 2;
    if (brightness_clock == 254) {
      brightness_clock = 255;
    }
  } else {
    // Clock goes down by odds to 1
    brightness_clock -= 2;
    if (brightness_clock == 1) {
      brightness_clock = 0;
    }
  }
  spoke_brightness =  map8(sin8_C(brightness_clock), 96, 255);
}

////// Specialized shows

//
// patterns shows
//
void patternsBW(uint8_t channel) {
  map_pattern(get_bit_from_pattern_number(0, current_pattern[channel]), 0, channel);  // Center

  for (int i = 0; i < HUB_LENGTH; i++) {
    map_pattern(get_bit_from_pattern_number(i+1, current_pattern[channel]), i+1, channel);  // Hub
  }
  
  for (int i = 0; i < SPOKE_LENGTH; i++) {
    map_pattern(get_bit_from_pattern_number(i+1+HUB_LENGTH, current_pattern[channel]), i + get_hub_pixels(symmetry[channel]), channel);
    if (symmetry[channel] < 3 && invert[channel] == 0 && i == SPOKE_LENGTH - 1 && SNOWFLAKE_NUMBER != 1) {
      shows[channel].setPixeltoForeBlack(i + get_hub_pixels(symmetry[channel]));  // Hack: strip off the last odd pixel
    } else {
      map_pattern(get_bit_from_pattern_number(i+1+HUB_LENGTH, current_pattern[channel]), i + get_hub_pixels(symmetry[channel]), channel);
    }
  }
}

void map_pattern(boolean isOn, uint8_t i, uint8_t channel) {
  if (invert[channel]) { 
    isOn = (isOn == 1) ? 0 : 1 ;  // invert pattern (0 <-> 1)
  }
  
  if (isOn) {  // For pattern table
    if (has_black_pattern[channel]) {
      shows[channel].setPixeltoHue(i, shows[channel].getForeColor() + i);  // 1 = foreColor
    } else {
      shows[channel].setPixeltoForeColor(i);  // 1 = foreColor
    }
  } else {
    if (has_black_pattern[channel]) {
      shows[channel].setPixeltoBackBlack(i);  // 0 = Black or backColor
    } else {
      shows[channel].setPixeltoBackColor(i);  // 0 = backColor
    }
  }
}

///////// DUAL SHOW LOGIC

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  
  for (int i = 0; i < NUM_LEDS; i++) {
    
    CHSV color = led[CHANNEL_A].getInterpHSV(led[CHANNEL_B].getInterpFrameColor(i),
                                             led[CHANNEL_A].getInterpFrameColor(i),
                                             fract);  // interpolate a + b channels
    color = narrow_palette(color);

    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }
    
    leds[i] = color;  // these are the leds themselves
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

  if (small_cycle <= fade_cycles) {
    intensity = map(small_cycle, 0, fade_cycles, 0, 255);  // rise
  } else if (small_cycle <= (max_small_cycle / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((max_small_cycle / 2) + fade_cycles)) {
    intensity = map(small_cycle - (max_small_cycle / 2), 0, fade_cycles, 255, 0);  // decay
  } else {
    intensity = 0;
  }
  return ease8InOutQuad(intensity);
}

//// End DUAL SHOW LOGIC

//
// pick_symmetry  - pick a random symmetry for the duration of the show
//
void pick_symmetry(uint8_t i) {
  symmetry[i] = random8(NUM_SYMMETRIES); // 0-4
  if (current_show[i] < SINGLE_SPOKE_SHOWS) {
    symmetry[i] = random8(1, 4);  // don't allow symmetry 0 or 4 for shows < SINGLE_SPOKE_SHOWS
  }
  invert[i] = (random8(3) == 0) ? true : false;
  switch_axis[i] = (random8(3) == 0 && current_show[i] != 0) ? true : false;
  set_symmetry(i);
}

//
// set_symmetry
//
void set_symmetry(uint8_t i) {
  led[i].fillBlack();  // clear leds before symmetry change
  uint8_t numSymLeds = ((6 / symmetries[symmetry[i]]) * SPOKE_LENGTH) + get_hub_pixels(symmetry[i]);
  if (symmetry[i] == 4) {
    numSymLeds = (SPOKE_LENGTH / 2) + 1 + get_hub_pixels(symmetry[i]);  // Hack for Symmetry 4
  }
  shows[i].resetNumLeds(numSymLeds);  // resets the virtual number of LEDs
  led[i].fillBlack();  // and clear leds after symmetry change
  led[i].push_frame();
}

//
// update_leds - push the interp_frames on to the leds - adjust for SYMMETRY!
//
void update_leds() {
  for (uint8_t i = 0; i < DUAL; i++) {
    draw_center(i);
    draw_hub(i);
    if (symmetry[i] == 4) {
      draw_spokes_symm_4(i);  // Hack for Symmetry 4
    } else {
      draw_spokes(i);
    }
  }
}

//
// draw_center - fill in the center circle - Virtual led 0 maps to the last pixel
//
void draw_center(uint8_t channel) {
  //// An intermediate buffer might be necessary. Showing older code below.
  // leds_buffer[channel][NUM_LEDS-1] = led[channel].getInterpFrameColor(0);
  led[channel].setInterpFrame(NUM_LEDS-1, led[channel].getInterpFrameColor(0));
}

//
// draw_hub - fill in the 6 petals around the center with virtual leds (1+)
//            Virtual leds 1+ map to end -6:-1 pixels 
//
void draw_hub(uint8_t channel) {
  uint8_t trans_i, pixel;
  uint8_t pitch = HUB_LEDS / symmetries[symmetry[channel]];
  uint8_t num_arms = 6 / symmetries[symmetry[channel]];
  CHSV color;
  
  for (uint8_t i = 0; i < pitch; i++) {
    for (uint8_t j = 0; j < symmetries[symmetry[channel]]; j++) {
      trans_i = i;
      
      if (SNOWFLAKE_NUMBER == 1 && switch_axis[channel]) {
        trans_i = ((i % num_arms) * HUB_LENGTH) + (i / num_arms);  // Hack for SNOWFLAKE 1
      }

      pixel = SPOKE_LEDS + trans_i + (j * pitch);

      if (SNOWFLAKE_NUMBER == 1 || SNOWFLAKE_NUMBER == 3) { pixel = shift_one_hub(pixel); }
      
      color = led[channel].getInterpFrameColor(1 + i);

      //// An intermediate buffer might be necessary. Showing older code below.
//      leds_buffer[channel][pixel] = color;
      led[channel].setInterpFrame(pixel, color);
    }
  }
}

//
// draw_spokes - draw the six spokes
//
// spoke virtual leds (2-7:SPOKE_LEDS) map to (0:SPOKE_LEDS) pixels
//
void draw_spokes(uint8_t channel) {
  uint8_t symm_type = symmetry[channel];  // 0-4
  uint8_t symm = symmetries[symm_type];  // Number of repeated arms: 1, 2, 3, 6, 6
  uint8_t num_arms = 6 / symm;  // How many spokes per repeat
  uint8_t arm_length = SPOKE_LENGTH * num_arms;  // unique pixels in an arm
  uint8_t hub_pixels = get_hub_pixels(symm_type);
  uint8_t i, trans_i, pixel, switch_pixel;
  
  // For switch_axis
  uint8_t switch_arm = 0;  
  uint8_t switch_group_num = 0;
  uint8_t switch_item = 0;
  uint8_t switch_group[MAX_GROUP_LENGTH];
  
  CHSV color;

  if (switch_axis[channel]) { 
    fill_group(switch_group, switch_group_num);  // Seed first group
  }

  for (i = 0; i < arm_length; i++) {
    trans_i = i; // default is easy for no switch_axis

    // SWITCH AXIS algorithm
    if (switch_axis[channel]) {
      // Headache here to parse an array of variable-length arrays (switch_matrix)
      switch_pixel = switch_group[switch_item++];  // Pull the pixel from the 5-item array
      
      if (switch_pixel == END_GRP) {
        switch_item = 0;
        switch_arm++;
        
        if (switch_arm >= num_arms) {
          switch_arm = 0;
          switch_group_num++;  // Danger: not checking if this goes over 5 groups
          fill_group(switch_group, switch_group_num);
        }
        switch_pixel = switch_group[switch_item++];  // Try again
      }
      trans_i = switch_pixel + (switch_arm * SPOKE_LENGTH);
    }
    // End SWITCH AXIS algorithm
    
    color = led[channel].getInterpFrameColor(hub_pixels + i);
    color = dim_spoke(color);
    
    // Repeat the pixel on to each symmetric arm
    for (uint8_t j = 0; j < symm; j++) {  // 1, 2, 3, 6, 6
      pixel = trans_i + (j * arm_length);  // repeat
      pixel = shift_pixel(pixel, SNOWFLAKE_NUMBER, channel);  // shift pixel by snowflake type
      
      //// An intermediate buffer might be necessary. Showing older code below.
//       leds_buffer[channel][pixel] = color;
      led[channel].setInterpFrame(pixel, color);
    }
  }
}

//
// fill_group - load switch_group[5] with the 5 pixel row from the switch_matrix
//
void fill_group(uint8_t *switch_group, uint8_t switch_group_num) {
  
  uint8_t switch_matrix[] = {
    // Small Simple Snowflake (8-spoke): Start at left-most of 3-set pixel
    0, 1, 2, END_GRP, END_GRP,
    3, 7, END_GRP, END_GRP, END_GRP,
    4, 6, END_GRP, END_GRP, END_GRP,
    5, END_GRP, END_GRP, END_GRP, END_GRP,
    END_GRP, END_GRP, END_GRP, END_GRP, END_GRP,
  
    // Medium Pointy Snowflake (10-spoke): Start at left-most of middle 4-set pixel
    1, 7, END_GRP, END_GRP, END_GRP,
    0, 2, 6, 8, END_GRP,
    3, 5, 9, END_GRP, END_GRP,
    4, END_GRP, END_GRP, END_GRP, END_GRP,
    END_GRP, END_GRP, END_GRP, END_GRP, END_GRP,
  
    // Large Complex Snowflake (12-spoke): Start at left-most of 3-set pixel
     0, 1, 2, END_GRP, END_GRP,
     3,11, END_GRP, END_GRP, END_GRP,
     4,10, END_GRP, END_GRP, END_GRP,
     5, 6, 8, 9, END_GRP,
     7, END_GRP, END_GRP, END_GRP, END_GRP,
  
    // Medium Inward Snowflake (10-spoke): Start at left-most inward pixel
     0, 2, 4, END_GRP, END_GRP,
     1, 3, 5, 9, END_GRP,
     6, 8, END_GRP, END_GRP, END_GRP,
     7, END_GRP, END_GRP, END_GRP, END_GRP,
     END_GRP, END_GRP, END_GRP, END_GRP, END_GRP,
  };
  
  for (uint8_t i = 0; i < MAX_GROUP_LENGTH; i++) {
    switch_group[i] = switch_matrix[(SNOWFLAKE_NUMBER * (MAX_GROUPS * MAX_GROUP_LENGTH)) +
                                    (switch_group_num * MAX_GROUP_LENGTH) + i];
  }
}

//
// draw_spokes_symm_4 - special case for 12-fold symmetry. No switch_axis possible
//
void draw_spokes_symm_4(uint8_t channel) {
  uint8_t symm_type = 4;
  uint8_t hub_pixels = get_hub_pixels(symm_type);
  uint8_t i, pixel;
  
  CHSV color;

  for (i = 0; i < (SPOKE_LENGTH / 2) + 1; i++) {
    
    color = led[channel].getInterpFrameColor(hub_pixels + i);
    color = dim_spoke(color);
    
    // Repeat the pixel on to each symmetric arm
    for (uint8_t j = 0; j < 6; j++) {
      pixel = i + (j * SPOKE_LENGTH);  // left arm
      pixel = shift_pixel(pixel, SNOWFLAKE_NUMBER, channel);  // shift pixel by snowflake type
      
      //// An intermediate buffer might be necessary. Showing older code below.
      // leds_buffer[channel][pixel] = color;
      led[channel].setInterpFrame(pixel, color);

      if (i > 0 && i < SPOKE_LENGTH / 2) {  // Do not double-up first and last pixels
        pixel = SPOKE_LENGTH - i + (j * SPOKE_LENGTH);  // right arm
        pixel = shift_pixel(pixel, SNOWFLAKE_NUMBER, channel);  // shift pixel by snowflake type

        //// An intermediate buffer might be necessary. Showing older code below.
        // leds_buffer[channel][pixel] = color;
        led[channel].setInterpFrame(pixel, color);
      }
    }
  }
}

//
// dim_spoke - dim the value part of the CHSV color
//
CHSV dim_spoke(CHSV color) {
  color.v = scale8(color.v, spoke_brightness);
  return color;
}

//
// shift_pixel - shift the pixel in software to fix the hardware layout
//
uint8_t shift_pixel(uint8_t i, uint8_t snowflake_type, uint8_t channel) {
  int8_t shift_amount;  // Three kinds of shift matrices. Pick the right one

  int8_t shift_matrix_regular[] =  { -1, -1, -1, -2 };
  int8_t shift_matrix_sym4[] =     { -1, -5, -1, -3 };
  int8_t shift_matrix_switched[] = { -2, -2, -2,  5 };
  
  if (symmetry[channel] == 4) {
    shift_amount = shift_matrix_sym4[snowflake_type];  // For Sym 4
  } else if (switch_axis[channel]) {
    shift_amount = shift_matrix_switched[snowflake_type];  // For switch_axis
  } else {
    shift_amount = shift_matrix_regular[snowflake_type];  // Regular
  }
  return (i + SPOKE_LEDS + shift_amount) % SPOKE_LEDS;
}

uint8_t shift_one_hub(uint8_t i) {
  return ((i - SPOKE_LEDS + 1) % HUB_LEDS) + SPOKE_LEDS;
}

//
// get_hub_pixels - Get the number of symmetric hub pixels
//
uint8_t get_hub_pixels(uint8_t n) {
  return (1 + (HUB_LENGTH * (6 / symmetries[n])));
}

//
// narrow_palette - confine the color range (ArduinoBlue)
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
//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern_matrix[] = {  // A pattern is 2-byte of binary
    // Small Simple Snowflake (8-spoke)               
    B01010111, B01000000,
    B10101101, B10000000,
    B01001111, B01000000,
    B10110111, B00000000,
    B11001101, B10000000,
    B11001010, B10000000,
  
    // Medium Pointy Snowflake (10-spoke)
    // C=center, I=inner hub, O=outer hub, 1=next outer, 2=next outer, S=squashed Tip, T=Pointy tip,
    // BCIO1xxSx, Bx2xTx000,
    B01000010, B00101000,
    B10010111, B01010000,
    B01101010, B10000000,
    B10110000, B11100000,
    B01101010, B10101000,
    B11010101, B01111000,
    
    // Large Complex Snowflake (12-spoke)
    B01010101, B11010100,
    B10001011, B01101000,
    B01101010, B11001100,
    B10110000, B11110000,
    B01110010, B10101100,
    B11000111, B01110000,
  
    // Medium Inward Snowflake (10-spoke)
    B01001010, B10010000,
    B11100010, B10000000,
    B10110000, B01100000,
    B01110011, B01000000,
    B10101010, B10100000,
    B10110101, B01100000,
  };

  uint8_t pattern_byte = pattern_matrix[(SNOWFLAKE_NUMBER * (NUM_PATTERNS * 2)) + 
                                        (pattern_number * 2) + (n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

//
// log status
//
void log_status(uint8_t i) {
  Serial.print("Channel: ");
  Serial.print(i);
  Serial.print(", Show: ");
  Serial.print(current_show[i]);
  Serial.print(", Symmetry: ");
  Serial.print(symmetry[i]);
  Serial.print(", Invert: ");
  Serial.print(invert[i]);
  Serial.print(", Switch Axis: ");
  Serial.print(switch_axis[i]);
  Serial.println(".");
}
