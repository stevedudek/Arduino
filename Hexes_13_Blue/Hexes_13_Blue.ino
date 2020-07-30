#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>  // (ArduinoBlue)
#include <math.h>
//
//  Hexes - 3 Large hexes
//
//  7/19/20 - Time of Covid
//
//  FastLED
//
//  Large chips - dual shows - xBee - Arduino Blue
//
//  Most elaborate shows possible - hold on to your hats
//
#define HEX_NUM   0
#define MAX_HEX   1

uint8_t BRIGHTNESS = 255;  // (0-255)

uint8_t DELAY_TIME = 15;  // in milliseconds. A cycle takes 3-5 milliseconds, so make this 6+.
long last_time;

// Hex Grid - Each Hex is 11 x 11
// MAX_XGRID, MAX_YGRID must be the same for all HEX
#define HEX_XGRID  0  // x-coord (upper left)
#define HEX_YGRID  0  // y-coord (upper left)
#define MAX_XGRID  11  //
#define MAX_YGRID  33  //

#define DATA_PIN 7
#define CLOCK_PIN 8

#define CHANNEL_A  1  // Don't change these
#define CHANNEL_B  0
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define ARE_CONNECTED false  // Are the HEX talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = millis();
#define MAX_SILENT_TIME  (3 * 1000)  // Time (in sec) without communication before marked as is_lost

#define NUM_LEDS 91
#define ACTUAL_LEDS 106
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number

#define MAX_COORD 11

#define XX  255

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
CHSV BLACK = CHSV(0, 0, 0);

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

// Shows
#define START_SHOW_CHANNEL_A  0  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B 25  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 27

uint8_t show_var[] = { 0, 0 };  // generic show variable - use carefully
uint8_t *show_storage[2];  // Array of MAX_COORD length
uint8_t rotate_amount[] = { 0, 0 };  // 0-5 how much to rotate the hexes
uint8_t symmetry[] = { 0, 0 };  // 1 = no reflection, 2 = two halves, 3 = three pieces, 6 = six pieces

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

// Clocks and time
#define SHOW_DURATION 60  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t FADE_TIME = 40;  // seconds to fade in. If FADE_TIME = SHOW_DURATION, then Always Be Fading
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// ArduinoBlue
ArduinoBlue phone(Serial2); // Blue Tx = pin 9; Blue Rx = pin 10
#define HUE_SLIDER        0
#define HUE_WIDTH_SLIDER  1
#define SPEED_SLIDER      2
#define BRIGHTNESS_SLIDER 3
#define FADE_TIME_SLIDER  4
#define BAM_BUTTON        0
#define BOLT_TIME        80

// xBee language
#define COMMAND_START      '+'
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_BRIGHTNESS 'I'
#define COMMAND_PALETTE    'H'
#define COMMAND_PAL_WIDTH  'J'
#define COMMAND_SPEED      'Y'
#define COMMAND_FADE       'R'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_SM_CYCLE   'C'
#define COMMAND_PATTERN    'P'
#define COMMAND_NOISE      'N'
#define COMMAND_BOLT       'X'
#define COMMAND_CHANNEL_A  'x'
#define COMMAND_CHANNEL_B  'y'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define EMPTY_CHAR         ' '
#define MAX_MESSAGE       100
#define MAX_NUM             6  // To handle 65,535 of small_cycle
char message[MAX_MESSAGE];     // Incoming message buffer
char number_buffer[MAX_NUM];   // Incoming number buffer

#define NUMBER_SPACER_LEDS 15
uint8_t spacer_leds[] = { 13, 14, 15, 33, 34, 56, 57, 58, 78, 79, 95, 96, 97, 100, 103 };

const uint8_t coords[] PROGMEM = {
  XX, XX, XX, XX, XX,  0,  1,  2,  3,  4,  5, 
  XX, XX, XX, XX,  6,  7,  8,  9, 10, 11, 12, 
  XX, XX, XX, 13, 14, 15, 16, 17, 18, 19, 20, 
  XX, XX, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
  XX, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 
  40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 
  51, 52, 53, 54, 55, 56, 57, 58, 59, 60, XX, 
  61, 62, 63, 64, 65, 66, 67, 68, 69, XX, XX, 
  70, 71, 72, 73, 74, 75, 76, 77, XX, XX, XX, 
  78, 79, 80, 81, 82, 83, 84, XX, XX, XX, XX, 
  85, 86, 87, 88, 89, 90, XX, XX, XX, XX, XX
};


const uint8_t corner_coords[] PROGMEM = {
  5,0, 10,0, 10,5, 5,10, 0,10, 0,5
};

const uint8_t LED_LOOKUP[] PROGMEM = {
             55, 59, 60, 94, 93, 98,
           54, 53, 61, 62, 92, 91, 99,
         16, 52, 51, 63, 64, 90, 89, 101,
       17, 18, 50, 49, 65, 66, 88, 87, 102,
     12, 19, 20, 48, 47, 67, 68, 86, 85, 104,
   11, 10, 21, 22, 46, 45, 69, 70, 84, 83, 105,
      9,  8, 23, 24, 44, 43, 71, 72, 82, 81,
        7,  6, 25, 26, 42, 41, 73, 74, 80,
          5,  4, 27, 28, 40, 39, 75, 76,
            3,  2, 29, 30, 38, 37, 77,
              1,  0, 31, 32, 36, 35
}; 

const uint8_t rotate_right[] PROGMEM = {
                5, 12, 20, 29, 39, 50, 
              4, 11, 19, 28, 38, 49, 60,
            3, 10, 18, 27, 37, 48, 59, 69, 
          2,  9, 17, 26, 36, 47, 58, 68, 77,
        1,  8, 16, 25, 35, 46, 57, 67, 76, 84,
      0,  7, 15, 24, 34, 45, 56, 66, 75, 83, 90,
        6, 14, 23, 33, 44, 55, 65, 74, 82, 89,
         13, 22, 32, 43, 54, 64, 73, 81, 88,
           21, 31, 42, 53, 63, 72, 80, 87,
             30, 41, 52, 62, 71, 79, 86,
                40, 51, 61, 70, 78, 85
};

// 21 pixels
const uint8_t one_wedge[] PROGMEM = {
  0, 7, 15, 24, 34, 45, 6, 14, 23, 33, 44, 13, 22, 32, 43, 21, 31, 42, 30, 41, 40
};
// 36 pixels
const uint8_t two_wedges[] PROGMEM = {
  0, 7, 15, 24, 34, 45, 6, 14, 23, 33, 44, 13, 22, 32, 43, 21, 31, 42, 30, 41, 40,
  1, 8, 16, 25, 35, 2, 9, 17, 26, 3, 10, 18, 4, 11, 5
};
// 51 pixels
const uint8_t three_wedges[] PROGMEM = {
  0, 7, 15, 24, 34, 45, 6, 14, 23, 33, 44, 13, 22, 32, 43, 21, 31, 42, 30, 41, 40,
  1, 8, 16, 25, 35, 2, 9, 17, 26, 3, 10, 18, 4, 11, 5,
  12, 19, 27, 36, 46, 20, 28, 37, 47, 29, 38, 48, 39, 49, 50
};

#define LIFE_MAP_SIZE 13
#define LIFE_NUM_CELLS  (LIFE_MAP_SIZE * LIFE_MAP_SIZE)
boolean *game_of_life_curr_map = (boolean *)calloc(LIFE_NUM_CELLS * DUAL, sizeof(boolean));
boolean *game_of_life_next_map = (boolean *)calloc(LIFE_NUM_CELLS * DUAL, sizeof(boolean));

const int8_t life_neighbors[] PROGMEM = { 1, 0, 1, -1, 0, -1, -1, 0, -1, 1, 0, 1 };
const int8_t life_next_neighbors[] PROGMEM = { 1, -2, 2, -1, 1, 1, -1, 2, -2, 1, -1, -1 };

//
// Pixel neighbors
//
const uint8_t neighbors[] PROGMEM = {
7, 6, XX, XX, XX, 1,
8, 7, 0, XX, XX, 2,
9, 8, 1, XX, XX, 3,
10, 9, 2, XX, XX, 4,
11, 10, 3, XX, XX, 5,
12, 11, 4, XX, XX, XX,
14, 13, XX, XX, 0, 7,
15, 14, 6, 0, 1, 8,
16, 15, 7, 1, 2, 9,
17, 16, 8, 2, 3, 10,
18, 17, 9, 3, 4, 11,
19, 18, 10, 4, 5, 12,
20, 19, 11, 5, XX, XX,
22, 21, XX, XX, 6, 14,
23, 22, 13, 6, 7, 15,
24, 23, 14, 7, 8, 16,
25, 24, 15, 8, 9, 17,
26, 25, 16, 9, 10, 18,
27, 26, 17, 10, 11, 19,
28, 27, 18, 11, 12, 20,
29, 28, 19, 12, XX, XX,
31, 30, XX, XX, 13, 22,
32, 31, 21, 13, 14, 23,
33, 32, 22, 14, 15, 24,
34, 33, 23, 15, 16, 25,
35, 34, 24, 16, 17, 26,
36, 35, 25, 17, 18, 27,
37, 36, 26, 18, 19, 28,
38, 37, 27, 19, 20, 29,
39, 38, 28, 20, XX, XX,
41, 40, XX, XX, 21, 31,
42, 41, 30, 21, 22, 32,
43, 42, 31, 22, 23, 33,
44, 43, 32, 23, 24, 34,
45, 44, 33, 24, 25, 35,
46, 45, 34, 25, 26, 36,
47, 46, 35, 26, 27, 37,
48, 47, 36, 27, 28, 38,
49, 48, 37, 28, 29, 39,
50, 49, 38, 29, XX, XX,
51, XX, XX, XX, 30, 41,
52, 51, 40, 30, 31, 42,
53, 52, 41, 31, 32, 43,
54, 53, 42, 32, 33, 44,
55, 54, 43, 33, 34, 45,
56, 55, 44, 34, 35, 46,
57, 56, 45, 35, 36, 47,
58, 57, 46, 36, 37, 48,
59, 58, 47, 37, 38, 49,
60, 59, 48, 38, 39, 50,
XX, 60, 49, 39, XX, XX,
61, XX, XX, 40, 41, 52,
62, 61, 51, 41, 42, 53,
63, 62, 52, 42, 43, 54,
64, 63, 53, 43, 44, 55,
65, 64, 54, 44, 45, 56,
66, 65, 55, 45, 46, 57,
67, 66, 56, 46, 47, 58,
68, 67, 57, 47, 48, 59,
69, 68, 58, 48, 49, 60,
XX, 69, 59, 49, 50, XX,
70, XX, XX, 51, 52, 62,
71, 70, 61, 52, 53, 63,
72, 71, 62, 53, 54, 64,
73, 72, 63, 54, 55, 65,
74, 73, 64, 55, 56, 66,
75, 74, 65, 56, 57, 67,
76, 75, 66, 57, 58, 68,
77, 76, 67, 58, 59, 69,
XX, 77, 68, 59, 60, XX,
78, XX, XX, 61, 62, 71,
79, 78, 70, 62, 63, 72,
80, 79, 71, 63, 64, 73,
81, 80, 72, 64, 65, 74,
82, 81, 73, 65, 66, 75,
83, 82, 74, 66, 67, 76,
84, 83, 75, 67, 68, 77,
XX, 84, 76, 68, 69, XX,
85, XX, XX, 70, 71, 79,
86, 85, 78, 71, 72, 80,
87, 86, 79, 72, 73, 81,
88, 87, 80, 73, 74, 82,
89, 88, 81, 74, 75, 83,
90, 89, 82, 75, 76, 84,
XX, 90, 83, 76, 77, XX,
XX, XX, XX, 78, 79, 86,
XX, XX, 85, 79, 80, 87,
XX, XX, 86, 80, 81, 88,
XX, XX, 87, 81, 82, 89,
XX, XX, 88, 82, 83, 90,
XX, XX, 89, 83, 84, XX,
};


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial1.begin(9600);  // Serial1: xBee port
  Serial2.begin(9600);  // Serial2: Bluetooth serial (ArduinoBlue)
  Serial.println("Start");  // Serial: For debugging
  
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setCoordMap(MAX_COORD, coords);  // x, y grid of hexes
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    led[i].fillBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].setWaitRange(15, 165, 150);
    show_storage[i] = (uint8_t *)calloc(MAX_COORD, sizeof(uint8_t));
    set_show_variables(i);
  }

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_start = 192;
    hue_width = 124;
  }

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show
  last_time = millis();
}

//
// loop
//
void loop() { 

  for (uint8_t i = 0; i < DUAL; i++) {
    
    switch (current_show[i]) {

      case 0:
        shows[i].randomFill();  // Vetted
        break;
      case 1:
        shows[i].juggle_fastled();
        break;
      case 2:
        shows[i].randomTwoColorBlack();
        break;
      case 3:
        shows[i].randomOneColorBlack();
        break;
      case 4:
        shows[i].sawTooth();  // Vetted
        break;
      case 5:
        pendulum_wave(i);  // Need work
        break;
      case 6:
        shows[i].bounce();  // Vetted
        break;
      case 7:
        shows[i].bounceGlowing();  // Vetted
        break;
      case 8:
        shows[i].plinko(90);  // Vetted
        break;
      case 9:
        shows[i].bands();  // Vetted
        break;
      case 10:
        vert_back_forth_dots(i);  // grided
        break;
      case 11:
        vert_back_forth_bands(i);  // grided
        break;
      case 12:
        vert_back_forth_colors(i);  // grided
        break;
      case 13:
        diag_back_forth_dots(i);  // grided
        break;
      case 14:
        diag_back_forth_bands(i);  // grided
        break;
      case 15:
        diag_back_forth_colors(i);  // grided
        break;
      case 16:
        shows[i].packets();  // Vetted
        break;
      case 17:
        shows[i].sinelon_fastled();  // Vetted
        break;
      case 18:
        center_ring(i);
        break;
      case 19:
        corner_ring(i);
        break;
      case 20:
        game_of_life_one_color(i);
        break;
      case 21:
        rain(i);
        break;
      case 22:
        well(i);
        break;
      case 23:
        radar(i);
        break;
      case 24:
        wipers(i);
        break;
      case 25:
        orbits(i);
        break;
      default:
        game_of_life_two_color(i);
        break;
    }

    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  check_phone();  // Check the phone settings (ArduinoBlue)
  update_leds();  // push the interp_frames on to the leds
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  fixed_delay();
  speak_and_hear();  // speak out or hear in signals
  advance_clocks();  // advance the cycle clocks and check for next show  
}

//
// fixed_delay - make every cycle the same time
//
void fixed_delay() {
  long new_time = millis();
  long time_delta = new_time - last_time;  // how much time has elapsed? Usually 3-5 milliseconds
  last_time = new_time;  // update the counter
  if (time_delta < DELAY_TIME) {  // if we have excess time,
    delay(DELAY_TIME - time_delta);  // delay for the excess
  }
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { // && !is_listening()) { 
      next_show(i);
      pick_next_show(i);
    }
  }
  if (curr_lightning > 0 ) {  // (ArduinoBlue)
    curr_lightning--; // Reduce the current bolt
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  led[i].fillBlack();
  led[i].push_frame();
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();

  // Set new show's colors to those of the other channel
  uint8_t other_channel = (i == 0) ? 1 : 0 ;
  shows[i].setForeColor(shows[other_channel].getForeColor());
  shows[i].setBackColor(shows[other_channel].getBackColor());

//  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  //  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  current_show[i] = random8(NUM_SHOWS);
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
  set_show_variables(i);
  rotate_amount[i] = random8(0, 6);
  symmetry[i] = pick_random_symmetry(i);
  speak_and_hear();

  log_status(i);  // For debugging
}

boolean is_other_channel_show_zero(uint8_t channel) {
  if (channel == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

uint8_t pick_random_symmetry(uint8_t i) {
  uint8_t random_symmetry = random(10);
  
  if (random_symmetry == 0) {
    return 6;
  } else if (random_symmetry < 3) {
    return 3;
  } else if (random_symmetry < 6) {
    return 2;
  } else {
    return 1;
  }
}

//
// set_show_variables - depends on the show
//
void set_show_variables(uint8_t i) {
  for (uint8_t pixel = 0; pixel < MAX_COORD; pixel++) {
    show_storage[i][pixel] = random8(255);
  }
  shows[i].setWaitRange(15, 165, 150);  // default
  
  switch (current_show[i]) {

    case 0:  // randomFill
      shows[i].setWaitRange(1, 6, 7);
      break;
    case 5:
      show_var[i] = random8(5);
      break;
    case 13:
      show_var[i] = random8(2, 10);
      break;
    case 14:
      show_var[i] = random8(2, 10);
      break;
    case 15:
      show_var[i] = random8(2, 10);
      break;
    case 19:
      shows[i].setWaitRange(10, 50, 7);
      break;
    case 23:
      show_var[i] = random8(6);
      break;
    default:
      break;
  }
}


////// Speaking

void speak_and_hear() {
  if (ARE_CONNECTED && IS_SPEAKING) { speak_commands(); }
  if (ARE_CONNECTED && !IS_SPEAKING) { hear_signal(); }  
}

boolean is_listening() {
  return (ARE_CONNECTED && !IS_SPEAKING && !is_lost);
}

void speak_commands() {
  for (uint8_t i = 0; i < DUAL; i++) {  // Send one channel, then the next
    if (shows[i].getMorph() == 0) {
      speak_channel_commands(i);
    }
  }
}

void speak_channel_commands(uint8_t i) {
  // Speak all the commands for a particular channel
  uint8_t m = 0;  // where we are in the send-out message string

  m = speak_start(m);
  m = speak_channel(i, m);  // Send the channel A or B prompt
  
  m = speak_command(COMMAND_SM_CYCLE, shows[CHANNEL_A].getSmallCycle(), m);
  m = speak_command(COMMAND_FORE, shows[i].getForeColor(), m);
  m = speak_command(COMMAND_BACK, shows[i].getBackColor(), m);
  m = speak_command(COMMAND_SHOW, current_show[i], m);
  m = speak_command(COMMAND_WAIT, shows[i].getWait(), m);
  
  message[m++] = COMMAND_PERIOD;  // Terminate message with period character
  message[m++] = '\0';  // Terminate message with null (new-line) character
  Serial1.print(message);  // Only done once
  // Serial.println(message);  // For debugging
}

void speak_arduinoblue_commands() {
  uint8_t m = 0;  // where we are in the send-out message string

  m = speak_start(m);
  m = speak_command(COMMAND_BRIGHTNESS, BRIGHTNESS, m);
  m = speak_command(COMMAND_PALETTE, hue_start, m);
  m = speak_command(COMMAND_PAL_WIDTH, hue_width, m);
  m = speak_command(COMMAND_SPEED, DELAY_TIME, m);
  m = speak_command(COMMAND_FADE, FADE_TIME, m);
  m = speak_command(COMMAND_BOLT, curr_lightning, m);
  
  message[m++] = COMMAND_PERIOD;  // Terminate message with null character
  message[m++] = '\0';  // Terminate message with null character
  Serial1.print(message);  // Only done once
//  Serial.println(message);  // For debugging
}

uint8_t speak_start(uint8_t m) {
  message[m++] = COMMAND_START;
  return m;
}

uint8_t speak_command(char cmd, int value, uint8_t m) {
  message[m++] = cmd;

  String value_str = String(value);
  for (uint8_t i = 0; i < value_str.length(); i++) {
    message[m++] = value_str[i];
  }

  message[m++] = COMMAND_COMMA;
  return m;
}

uint8_t speak_channel(int i, uint8_t m) {
  char command_channel = (i == 0) ? COMMAND_CHANNEL_A : COMMAND_CHANNEL_B ;
  message[m++] = command_channel;
  return m;
}

////// Hearing

void hear_signal() {
  if (Serial1.available()) {  // Heard a signal!
    ResetLostCounter();
    boolean msg_received = GetMessage(message);  // Load global message buffer by pointer
    if (msg_received) {
      // Serial.println(message);  // For debugging
      ProcessMessage(message);
    }
  } else {
    if (!is_lost && (millis() - last_connection > MAX_SILENT_TIME) ) {
      is_lost = true;
    }
  }
}

//
// ResetLostCounter - board is not lost
//
void ResetLostCounter() {
  is_lost = false;
  last_connection = millis();
}

//
// GetMessage - pulls the whole serial buffer until a period
//
boolean GetMessage(char* message) {
  char tmp;
  boolean have_start_signal = false;
  uint8_t tries = 0;
  uint16_t MAX_TRIES = 2000;
  uint8_t i = 0;

  while (tries++ < MAX_TRIES) {
    if (Serial1.available()) {
      tries = 0;
      tmp = Serial1.read();
//      Serial.print(tmp);  // For debugging

      if (!have_start_signal) {
        if (tmp == COMMAND_START) { have_start_signal = true; }  // Start of message
        else { } // discard prefix garbage
      } else {
        if (tmp == COMMAND_PERIOD) {
          message[i++] = tmp;  // End of message
          message[i++] = '\0';  // End of string
          return true;
        }  
        if (isAscii(tmp)) {
          message[i++] = tmp;
          if (i >= MAX_MESSAGE) {
            Serial.println("Message too long!");  // Ran out of message space
            return false;
          }
        }
      }
    }
  }
  Serial.println("Ran out of tries");
  return false;  // discard message
}

//
// ProcessMessage
//
void ProcessMessage(char* message) {
  uint8_t i = 0;
  uint8_t channel = 0;  // Default to channel 0
  char cmd;
  
  while (i < MAX_MESSAGE) {
    
    cmd = message[i];  // Get one-letter command

    if (cmd == COMMAND_PERIOD) {
      return;
    } else if (!isAscii(cmd)) {
      return;
    } else if (cmd == COMMAND_CHANNEL_A) {
      i++;
      channel = 0;
    } else if (cmd == COMMAND_CHANNEL_B) {
      i++;
      channel = 1;
    } else if (isAlpha(cmd)) {
      for (uint8_t j = 0; j < MAX_NUM; j++) { 
        number_buffer[j] = EMPTY_CHAR;  // Clear number buffer
      }
      
      int numsiz = ReadNum(message, i+1, number_buffer);  // Read in the number
      i = i + 2 + numsiz; // 2 = leap beginning command and trailing comma
      
      ExecuteOrder(cmd, atoi(number_buffer), channel);
    } else {
      return;  // Garbage
    }
  }
}

//
// ReadNum - reads numbers in a string
//
uint8_t ReadNum(char* msg, uint8_t place, char* number) {
  uint8_t i = 0; // Start of number
  char tmp;
  
  while (i < MAX_NUM) {
    tmp = msg[place];
    if (tmp == COMMAND_COMMA) {
      return (i);
    } else {
      number[i] = msg[place];
      i++;
      place++;
      if (place >= MAX_MESSAGE) { 
        break;  
      }
    }
  }
  Serial.println("Number too long");
  return (0); // Number too long
}

//
// Execute Order - execute a letter command followed by a number
//
void ExecuteOrder(char command, uint16_t value, uint8_t i) {
      
  switch (command) {
    
    case COMMAND_FORE:
      shows[i].setForeColor(value);
      break;
    
    case COMMAND_BACK:
      shows[i].setBackColor(value);
      break;

    case COMMAND_BRIGHTNESS:
      if (BRIGHTNESS != value) {
        BRIGHTNESS = value;
        FastLED.setBrightness( BRIGHTNESS );
      }
      break;
    
    case COMMAND_WAIT:
      if (shows[i].getWait() != value) { 
        shows[i].setWait(value);
      }
      break;
    
    case COMMAND_SHOW:  // Show
      if (current_show[i] != value) {
        current_show[i] = value;
        next_show(i);
      }
      break;

    case COMMAND_SM_CYCLE: // small_cycle
      shows[i].setSmallCycle(value);
      break;
  }
}

////// End Speaking & Hearing



////// Arduino Blue Start

//
// check_phone - poll the phone for updated values  (ArduinoBlue)
//
void check_phone() {
  int8_t sliderId = phone.getSliderId();  // ID of the slider moved
  int8_t buttonId = phone.getButton();  // ID of the button

  if (sliderId != -1) {
    int16_t sliderVal = phone.getSliderVal();  // Slider value goes from 0 to 200
    sliderVal = map(sliderVal, 0, 200, 0, 255);  // Recast to 0-255

    switch (sliderId) {
      case BRIGHTNESS_SLIDER:
        BRIGHTNESS = sliderVal;
        FastLED.setBrightness( BRIGHTNESS );
        break;
      case HUE_SLIDER:
        hue_start = sliderVal;
        break;
      case HUE_WIDTH_SLIDER:
        hue_width = sliderVal;
        break;
      case SPEED_SLIDER:
        DELAY_TIME = map8(sliderVal, 6, 100);
        break;
      case FADE_TIME_SLIDER:
        FADE_TIME = map8(sliderVal, 0, SHOW_DURATION);
        break;
    }
    speak_arduinoblue_commands();
  }
  
  if (buttonId == BAM_BUTTON) { 
    curr_lightning = BOLT_TIME;
    speak_arduinoblue_commands();
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
// lightning - ramp all pixels quickly up to white (down sat & up value) and back down
//
CHSV lightning(CHSV color) {  // (ArduinoBlue)
  if (curr_lightning > 0) {
    uint8_t increase = 255 - cos8( map(curr_lightning, 0, BOLT_TIME, 0, 255));
    color.s -= increase;
    color.v += increase;
  }
  return color;
}

////// Arduino Blue End


////// Specialized shows

//
// pendulum_wave
//
void pendulum_wave(uint8_t c) {
  uint8_t side_bands = show_var[c] % 6; // how many side bands
  shows[c].fillBlack();
  
  for (uint8_t y = 0; y < MAX_COORD; y++) {
    uint8_t x = beatsin8(30 + y) / 23;
    shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x, y));

    if (side_bands > 0) {
      for (uint8_t i = 0; i < side_bands; i++) {
        CHSV color = led[c].gradient_wheel(shows[c].getForeColor(), (side_bands - i) * (255 / (side_bands + 1)));
        if (x - i - 1 > 0) {
          shows[c].setPixeltoColor(led[c].getLedFromCoord(x - i - 1, y), color);
        }
        if (x + i + 1 < MAX_COORD) {
          shows[c].setPixeltoColor(led[c].getLedFromCoord(x + i + 1, y), color);
        }
      }
    }
  }
}

//
// rain
//
void rain(uint8_t c) {

  // Randomly rotate
  if (random8(50) == 1) {
    symmetry[c] = (symmetry[c] + random8(3) - 1) % 6;
  }
  
  uint8_t side_bands = show_var[c] % 6; // how many side bands
  shows[c].fillBlack();
  
  for (uint8_t y = 0; y < MAX_COORD; y++) {
    uint8_t x = beat8(map8(show_storage[c][y], 10, 40)) / 23;
    shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x, y));

    if (side_bands > 0) {
      for (uint8_t i = 0; i < side_bands; i++) {
        CHSV color = led[c].gradient_wheel(shows[c].getForeColor(), (side_bands - i) * (255 / (side_bands + 1)));
        if (x - i - 1 > 0) {
          shows[c].setPixeltoColor(led[c].getLedFromCoord(x - i - 1, y), color);
        }
        if (x + i + 1 < MAX_COORD) {
          shows[c].setPixeltoColor(led[c].getLedFromCoord(x + i + 1, y), color);
        }
      }
    }
  }
}

//
// radar
//
void radar(uint8_t c) {
  uint8_t thickness = map8(show_storage[c][0], 64, 128);
  
  shows[c].fillBlack();
  
  for (uint8_t y = 0; y < MAX_COORD; y++) {
    for (uint8_t x = 0; x < MAX_COORD; x++) {
      uint8_t i = led[c].getLedFromCoord(x, y);
      if (i != XX) {
        uint8_t angle_delta = get_difference(shows[c].getSmallCycle() % 256, get_angle_to_origin(x, y));
        if (angle_delta < thickness) {
          uint8_t intensity = dim8_raw(map(angle_delta, 0, thickness, 255, 0));
          shows[c].setPixeltoColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intensity));
        }
      }
    }
  }
}

//
// wipers
//
void wipers(uint8_t c) {
  uint8_t thickness = map8(show_storage[c][0], 64, 192);
  uint8_t i, angle_delta, intensity;
  
  shows[c].fillBlack();
  
  for (uint8_t y = 0; y < MAX_COORD; y++) {
    for (uint8_t x = 0; x < MAX_COORD; x++) {
      i = led[c].getLedFromCoord(x, y);
      
      if (i != XX) {
        
        angle_delta = get_difference(shows[c].getSmallCycle() % 256, get_angle(x, y, 10, 5));
        if (angle_delta < thickness) {
          intensity = dim8_raw(map(angle_delta, 0, thickness, 255, 0));
          led[c].addPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intensity));
        }
        
        angle_delta = get_difference(shows[c].getSmallCycle() % 256, get_angle(x, y, 0, 5));
        if (angle_delta < thickness) {
          intensity = dim8_raw(map(angle_delta, 0, thickness, 255, 0));
          led[c].addPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intensity));
        }
      }
    }
  }
}

//
// orbits
//
void orbits(uint8_t c) {
  uint8_t thickness = map8(show_storage[c][0], 32, 128);
  uint8_t i, angle_delta, distance_delta, intensity, intensity2;
  
  shows[c].fillBlack();
  
  for (uint8_t y = 0; y < MAX_COORD; y++) {
    for (uint8_t x = 0; x < MAX_COORD; x++) {
      i = led[c].getLedFromCoord(x, y);
      
      if (i != XX) {

        // First orbit
        angle_delta = get_difference(shows[c].getSmallCycle() % 256, get_angle_to_origin(x, y));
        if (angle_delta < thickness) {
          intensity = dim8_raw(map(angle_delta, 0, thickness, 255, 0));
          
          distance_delta = abs(get_distance_to_origin(x, y) - 24);
          if (distance_delta < thickness) {
            intensity2 = dim8_raw(map(distance_delta, 0, thickness, intensity, 0));
            led[c].addPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intensity2));
          }
        }
        
        // Second orbit
        angle_delta = get_difference((255 - shows[c].getSmallCycle()) % 256, get_angle_to_origin(x, y));
        if (angle_delta < thickness) {
          intensity = dim8_raw(map(angle_delta, 0, thickness, 255, 0));
          
          distance_delta = abs(get_distance_to_origin(x, y) - 96);
          if (distance_delta < thickness) {
            intensity2 = dim8_raw(map(distance_delta, 0, thickness, intensity, 0));
            led[c].addPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intensity2));
          }
        }
      }
    }
  }
}

//
//  get angle - figure out the angle of (y/x) to the center
//
uint8_t get_angle_to_origin(uint8_t x, uint8_t y) {
  return get_angle(x, y, 5, 5);
}

uint8_t get_angle(uint8_t x2, uint8_t y2, uint8_t x1, uint8_t y1) {
  int8_t dx = (2 * (x2 - x1)) + (y2 - y1);
  float dy = -2.309 * (y2 - y1);  // 4 / sqrt(3)  minus for inverted y-coordinates
  
  return get_angle_square_coord(dx, dy);
}

uint8_t get_angle_square_coord(int8_t dx, float dy) {
  if (dy == 0) {
    return (dx >= 0) ? 0 : 128 ;  // Either pointing right (3 o'clock) or left (9 o'clock)
  }
  if (dx == 0) {
    return (dy >= 0) ? 64 : 192 ;  // Either pointing up or down
  }
  
  uint8_t angle = uint8_t(atan2(abs(dy), abs(dx)) * 40.743);  // 128 / pi
  
  if (dy > 0) {
    angle = (dx > 0) ? angle : 128 - angle ;
  } else {
    angle = (dx > 0) ? 256 - angle : angle + 128 ;
  }
  return angle;
}

//// Start Game of Life

void game_of_life_one_color(uint8_t c) {
  game_of_life(c, false);
}

void game_of_life_two_color(uint8_t c) {
  game_of_life(c, true);
}


//
// game_of_life - Conway's Game of Life for Hexes
//
void game_of_life(uint8_t c, boolean two_color) {
  if (shows[c].isShowStart()) {
    shows[c].setWait(40);
    seed_game_of_life_map(c);
  }
  
  if (shows[c].getMorph() == 0) {
    calculate_life(c);
    send_game_of_life_to_leds(c, two_color);
    push_game_of_life_frame(c);
  }
}

void calculate_life(uint8_t c) {
  boolean fate;
  
  for (uint8_t x = 0; x < LIFE_MAP_SIZE; x++) {
    for (uint8_t y = 0; y < LIFE_MAP_SIZE; y++) {
      float life_quantity = get_life_quantity(c, x, y);
      
      if (get_game_of_life_cell(c, x, y)) {
        fate = (life_quantity >= 2.0 && life_quantity <= 3.3) ? true : false;
      } else {
        fate = (life_quantity >= 2.3 && life_quantity <= 2.9) ? true : false;
      }
      set_game_of_next_life_cell(c, x, y, fate);
    }
  }
}

float get_life_quantity(uint8_t c, int8_t x, int8_t y) {
  return num_alive_neighbors(c, x, y) + (num_alive_next_neighbors(c, x, y) * 0.3);
}

uint8_t num_alive_neighbors(uint8_t c, int8_t x, int8_t y) {
  float life = 0;
  int8_t dx;
  int8_t dy;
  
  for (uint8_t dir = 0; dir < 6; dir++) {
    dx = pgm_read_byte_near(life_neighbors + (dir * 2));
    dy = pgm_read_byte_near(life_neighbors + (dir * 2) + 1);
    
    if (out_of_bounds(x + dx, y + dy)) {
      life += 0.5;
    } else {
      if (get_game_of_life_cell(c, x + dx, y + dy)) {
        life += 1.0;
      }
    }
  }
  return life;
}

uint8_t num_alive_next_neighbors(uint8_t c, int8_t x, int8_t y) {
  float life = 0;
  int8_t dx;
  int8_t dy;
  
  for (uint8_t dir = 0; dir < 6; dir++) {
    dx = pgm_read_byte_near(life_next_neighbors + (dir * 2));
    dy = pgm_read_byte_near(life_next_neighbors + (dir * 2) + 1);
    
    if (out_of_bounds(x + dx, y + dy)) {
      life += 0.5;
    } else {
      if (get_game_of_life_cell(c, x + dx, y + dy)) {
        life += 1.0;
      }
    }
  }
  return life;
}

void seed_game_of_life_map(uint8_t c) {
  // 20% chance of a life in a starting cell
  for (uint8_t x = 0; x < LIFE_MAP_SIZE; x++) {
    for (uint8_t y = 0; y < LIFE_MAP_SIZE; y++) {
      boolean outcome = (random8(0, 10) < 2) ? true : false;
      set_game_of_curr_life_cell(c, x, y, outcome);
      set_game_of_next_life_cell(c, x, y, false);
    }
  }
}

void push_game_of_life_frame(uint8_t c) {
  for (uint8_t i = 0; i < LIFE_MAP_SIZE * LIFE_MAP_SIZE; i++) {
    game_of_life_curr_map[i + (c * LIFE_NUM_CELLS)] = game_of_life_next_map[i + (c * LIFE_NUM_CELLS)];
  }
}

void send_game_of_life_to_leds(uint8_t c, boolean two_color) {
  uint8_t foreColor = shows[c].getForeColor();
  uint8_t backColor = shows[c].getBackColor();
  uint8_t off_set = (LIFE_MAP_SIZE - MAX_COORD) / 2;
  boolean curr_life, next_life;
  
  for (uint8_t x = 0; x < LIFE_MAP_SIZE; x++) {
    for (uint8_t y = 0; y < LIFE_MAP_SIZE; y++) {
      uint8_t pixel = led[c].getLedFromCoord(x, y);
      if (pixel != XX) {
        curr_life = get_game_of_life_cell(c, x + off_set, y + off_set);
        next_life = get_game_of_life_next_cell(c, x + off_set, y + off_set);
        shows[c].setPixeltoColor(pixel, get_game_of_life_color(curr_life, next_life, two_color, foreColor, backColor));
      }
    }
  }
}

CHSV get_game_of_life_color(boolean curr_life, boolean next_life, boolean two_color, uint8_t foreColor, uint8_t backColor) {
  // One and two color versions depending on two_color boolean
  CHSV color;
  
  if (two_color) {
    if (curr_life) {
      color = (next_life) ? led[CHANNEL_A].wheel(foreColor) : led[CHANNEL_A].gradient_wheel(backColor, 128) ;
    } else {
      color = (next_life) ? led[CHANNEL_A].gradient_wheel(foreColor, 128) : led[CHANNEL_A].wheel(backColor) ;
    }
  
  } else {
    color = (curr_life) ? led[CHANNEL_A].wheel(foreColor) : BLACK ;  // one color version is either foreColor or Black
  }
  
  return color;
}

void set_game_of_curr_life_cell(uint8_t c, int8_t x, int8_t y, boolean value) {
  if (out_of_bounds(x, y) == false) {
    game_of_life_curr_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)] = value;
  }
}

void set_game_of_next_life_cell(uint8_t c, int8_t x, int8_t y, boolean value) {
  if (out_of_bounds(x, y) == false) {
    game_of_life_next_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)] = value;
  }
}

boolean get_game_of_life_cell(uint8_t c, int8_t x, int8_t y) {
  // if coordinates are out of bounds, default to false;
  if (out_of_bounds(x, y)) {
    return false;
  }
  return game_of_life_curr_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)];
}

boolean get_game_of_life_next_cell(uint8_t c, int8_t x, int8_t y) {
  // if coordinates are out of bounds, default to false;
  if (out_of_bounds(x, y)) {
    return false;
  }
  return game_of_life_next_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)];
}

boolean out_of_bounds(int8_t x, int8_t y) {
  // Is (x,y) on the game of life board between (0 ,0) and (LIFE_MAP_SIZE, LIFE_MAP_SIZE)?
  return (x < 0 || x >= LIFE_MAP_SIZE || y < 0 || y >= LIFE_MAP_SIZE);
}

//// End Game of Life

//
// vert back forth dots - vertical dots moving back and forth
//
void vert_back_forth_dots(uint8_t c) {
  uint8_t temp_x;
  uint16_t cycle = shows[c].getCycle();

  led[c].fillBlack();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_x = x + HEX_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      if ((temp_x + cycle) % MAX_XGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
}

//
// vert back forth bands - vertical bands moving back and forth
//
void vert_back_forth_bands(uint8_t c) {
  uint8_t temp_x, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_x = x + HEX_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_x = (temp_x + cycle) % MAX_XGRID;
      intensity = sin8(map(temp_x, 0, MAX_XGRID, 0, 255) + (cycle % (255 / MAX_XGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
}

//
// vert back forth colors - vertical colors moving back and forth
//
void vert_back_forth_colors(uint8_t c) {
  uint8_t temp_x, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_x = x + HEX_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_x = (temp_x + cycle) % MAX_XGRID;
      hue = sin8(map(temp_x, 0, MAX_XGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_XGRID)) );
      led[c].setPixelHue(led[c].getLedFromCoord(x,y), hue);
    }
  }
}

//
// diag back forth dots - diagonal dots moving back and forth
//
void diag_back_forth_dots(uint8_t c) {
  uint8_t temp_x, temp_y;
  uint16_t cycle = shows[c].getCycle();

  led[c].fillBlack();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_x = x + HEX_XGRID;
      temp_y = y + HEX_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      if ((temp_x + temp_y + cycle) % (MAX_YGRID / show_var[c]) == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
}

//
// diag back forth bands - diagonal bands moving back and forth
//
void diag_back_forth_bands(uint8_t c) {
  uint8_t temp_x, temp_y, temp, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {  // Don't light the head (row 0)
      temp_x = x + HEX_XGRID;
      temp_y = y + HEX_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp = (temp_x + temp_y + cycle) % (MAX_YGRID / show_var[c]);
      intensity = sin8(map(temp, 0, MAX_YGRID, 0, 255) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
}

//
// diag back forth colors - diagonal colors moving back and forth
//
void diag_back_forth_colors(uint8_t c) {
  uint8_t temp_x, temp_y, temp, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_x = x + HEX_XGRID;
      temp_y = y + HEX_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp = (temp_x + temp_y + cycle) % (MAX_YGRID / show_var[c]);
      hue = sin8(map(temp, 0, MAX_YGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelHue(led[c].getLedFromCoord(x,y), hue);
    }
  }
}

//
// rings
//
void center_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), BLACK, 5, 5, 8, 64, 128);
}

void well(uint8_t c) {
  ring(c, shows[c].getForeColor(), led[c].wheel(shows[c].getBackColor()), 5, 5, 16, 192, 192);
}

void corner_ring(uint8_t c) {
  uint8_t corner = show_var[c] % 6;
  uint8_t x = pgm_read_byte_near(corner_coords + (corner * 2));
  uint8_t y = pgm_read_byte_near(corner_coords + (corner * 2) + 1);
  ring(c, shows[c].getForeColor(), BLACK, x, y, 8, 64, 255);
}

void ring(uint8_t c, uint8_t color, CHSV background, uint8_t center_x, uint8_t center_y, uint8_t ring_speed, uint8_t cutoff, uint8_t ring_freq) {
  // center_x, center_y = ring center coordinates; (5,5) is hex centers
  // ring_speed = 1+. Higher = faster.
  // cutoff = ring thickness with higher values = thicker
  // ring_freq = (255: 1 ring at a time; 128: 2 rings at a time)
  CHSV foreColor = led[c].wheel(color);
  led[c].fill(background);
  uint8_t value = (shows[c].getCycle() * ring_speed) % 256;

  for (uint8_t y = 0; y < MAX_COORD; y++) {
    for (uint8_t x = 0; x < MAX_COORD; x++) {
      uint8_t pixel = led[c].getLedFromCoord(x, y);
      if (pixel != XX) {
        uint8_t delta = abs(get_distance(x, y, center_x, center_y) - value) % ring_freq;
        if (delta < cutoff) {
          uint8_t intensity = map(delta, 0, cutoff, 255, 0);
          shows[c].setPixeltoColor(pixel, led[c].getInterpHSV(background, foreColor, intensity));
        }
      }
    }
  }
}

///////// DUAL SHOW LOGIC

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  uint8_t led_number;
  CHSV color;

  mirror_pixels(CHANNEL_A);
  mirror_pixels(CHANNEL_B);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    led_number = convert_pixel_to_led(i);
    if (led_number != XX) {
      color = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][rotate_pixel(i, CHANNEL_B)], 
                                          leds_buffer[CHANNEL_A][rotate_pixel(i, CHANNEL_A)], 
                                          fract);  // interpolate a + b channels
      leds[led_number] = narrow_palette(color);
    }                                                 
  }
  turn_off_spacer_leds();
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();

  // Similar logic to check_fades (deprecated)
  if (small_cycle <= FADE_CYCLES) {
    intensity = map(small_cycle, 0, FADE_CYCLES, 0, 255);  // rise
  } else if (small_cycle <= (MAX_SMALL_CYCLE / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((MAX_SMALL_CYCLE / 2) + FADE_CYCLES)) {
    intensity = map(small_cycle - (MAX_SMALL_CYCLE / 2), 0, FADE_CYCLES, 255, 0);  // decay
  } else {
    intensity = 0;
  }
  return intensity;  // do not use easing
}

//// End DUAL SHOW LOGIC


//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (uint8_t c = 0; c < DUAL; c++) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      leds_buffer[c][i] = redden(led[c].getInterpFrameColor(i));
    }
  }
}

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t get_distance(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  // distance = sqrt(3 * (x^2 * xy * y^2))  - remove the 3
  // answer should span 0-250
  int16_t dx = (x2 - x1) * 25;
  int16_t dy = (y2 - y1) * 25;
  return sqrt16((dx * dx) + (dx * dy) + (dy * dy));
}

uint8_t get_distance_to_origin(uint8_t x, uint8_t y) {
  return get_distance(x, y, 5, 5);
}

//
// convert pixel to led: account for the spacer pixels
//
uint8_t convert_pixel_to_led(uint8_t i) {
  if (i == XX) {
    return XX;
  }
  return pgm_read_byte_near(LED_LOOKUP + (i % NUM_LEDS));
}

//
// mirror_pixels
//
void mirror_pixels(uint8_t channel) {
  if (symmetry[channel] == 6 || curr_lightning > 0) {
    for (uint8_t i = 0; i < 21; i++) {
      mirror_pixel(channel, pgm_read_byte_near(one_wedge + i), 1);
      mirror_pixel(channel, pgm_read_byte_near(one_wedge + i), 2);
      mirror_pixel(channel, pgm_read_byte_near(one_wedge + i), 3);
      mirror_pixel(channel, pgm_read_byte_near(one_wedge + i), 4);
      mirror_pixel(channel, pgm_read_byte_near(one_wedge + i), 5);
    }
  } else if (symmetry[channel] == 2) {
    for (uint8_t i = 0; i < 51; i++) {
      mirror_pixel(channel, pgm_read_byte_near(three_wedges + i), 3);
    }
  } else if (symmetry[channel] == 3) {
    for (uint8_t i = 0; i < 36; i++) {
      mirror_pixel(channel, pgm_read_byte_near(two_wedges + i), 2);
      mirror_pixel(channel, pgm_read_byte_near(two_wedges + i), 4);
    }
  }
}

//
// mirror_pixel
//
void mirror_pixel(uint8_t channel, uint8_t i, uint8_t rotations) {
  uint8_t new_pixel = i;
  for (uint8_t r = 0; r < rotations; r++) {
    new_pixel = pgm_read_byte_near(rotate_right + new_pixel);
  }
  leds_buffer[channel][new_pixel] = leds_buffer[channel][i];
}

//
// rotate_pixel
//
uint8_t rotate_pixel(uint8_t i, uint8_t channel) {
  uint8_t new_pixel = i;
  for (uint8_t r = 0; r < rotate_amount[channel]; r++) {
    new_pixel = pgm_read_byte_near(rotate_right + new_pixel);
  }
  return new_pixel;
}

//
// turn off spacer leds - blacken the 16 space pixels
//
void turn_off_spacer_leds() {
  for (uint8_t i = 0; i < NUMBER_SPACER_LEDS; i++) {
    leds[spacer_leds[i]] = CRGB(0, 0, 0);
  }
}

//
// redden - if ONLY_RED, turn hues to red, yellow, blue
//
CHSV redden(CHSV color) {  
  if (ONLY_RED) { color.h = map8( sin8(color.h), 192, 60); }
  return color;
}

//
// get_difference
//
uint8_t get_difference(uint8_t a, uint8_t b) {
  uint16_t distCCW, distCW;
  if (a >= b) {
    distCW = 256 + b - a;
    distCCW = a - b;
  } else {
    distCW = b - a;
    distCCW = 256 + a - b;
  }
  return min(distCCW, distCW)
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
