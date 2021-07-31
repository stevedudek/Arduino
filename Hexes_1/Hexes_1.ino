#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>  // (ArduinoBlue)
//
//  Hexes - 3 Large hexes
//
//  5/30/20 - Time of Covid
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

#define DELAY_TIME 10 // in milliseconds

// Hex Grid - Each Hex is 11 x 11
// MAX_XGRID, MAX_YGRID must be the same for all HEX
#define HEX_XGRID  0  // x-coord (upper left)
#define HEX_YGRID  0  // y-coord (upper left)
#define MAX_XGRID  11  //
#define MAX_YGRID  33  //

#define DATA_PIN 7
#define CLOCK_PIN 8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define ARE_CONNECTED false  // Are the HEX talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

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
#define HEX_HUE 120  // Green?
CHSV BLACK = CHSV(0, 0, 0);

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

// Shows
#define START_SHOW_CHANNEL_A 13  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B  0  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 0 };
#define NUM_SHOWS 20

#define ONLY_RED false

// Clocks and time
#define SHOW_DURATION 20  // Typically 30 seconds. Size problems at 1800+ seconds.
#define FADE_TIME 18   // seconds to fade in. If FADE_TIME = SHOW_DURATION, then Always Be Fading
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// xBee language
#define COMMAND_START      '+'
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_BRIGHTNESS 'I'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_SM_CYCLE   'C'
#define COMMAND_PATTERN    'P'
#define COMMAND_NOISE      'N'
#define COMMAND_CHANNEL_A  'x'
#define COMMAND_CHANNEL_B  'y'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define EMPTY_CHAR         ' '
#define MAX_MESSAGE       100
#define MAX_NUM             5  // To handle 65,535 of small_cycle
char message[MAX_MESSAGE];     // Incoming message buffer
char number_buffer[MAX_NUM];   // Incoming number buffer

// Lookup tables
uint8_t LED_LOOKUP[] PROGMEM = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
                                30, 31, 32, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 
                                59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 80, 81, 82, 83, 84, 
                                85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 98, 99, 101, 102, 104, 105 };

#define NUMBER_SPACER_LEDS 15
uint8_t spacer_leds[] = { 13, 14, 15, 33, 34, 56, 57, 58, 78, 79, 95, 96, 97, 100, 103 };

uint8_t coords[] PROGMEM = {
    XX, XX, XX, XX, XX, 50, 51, 52, 84, 83, 85,
    XX, XX, XX, XX, 49, 48, 53, 54, 82, 81, 86,
    XX, XX, XX, 13, 47, 46, 55, 56, 80, 79, 87,
    XX, XX, 14, 15, 45, 44, 57, 58, 78, 77, 88,
    XX, 12, 16, 17, 43, 42, 59, 60, 76, 75, 89,
    11, 10, 18, 19, 41, 40, 61, 62, 74, 73, 90,
     9, 8, 20, 21, 39, 38, 63, 64, 72, 71, XX,
     7, 6, 22, 23, 37, 36, 65, 66, 70, XX, XX,
     5, 4, 24, 25, 35, 34, 67, 68, XX, XX, XX,
     3, 2, 26, 27, 33, 32, 69, XX, XX, XX, XX,
     1, 0, 28, 29, 31, 30, XX, XX, XX, XX, XX
};

#define PATTERN_BYTES 12

//
// PATTERNS - 8 hex bytes per pattern (see HEX.ipynb)
//
#define NUM_PATTERNS 7   // Total number of patterns

// 11 x 11 = 121 pixel bits = 16 bytes
const uint8_t pattern_matrix[][PATTERN_BYTES] = {
  { 0x20, 0x20, 0x31, 0x80, 0xab, 0x6a, 0x80, 0x36, 0x0, 0x60, 0xc0, 0x0 },
  { 0xf, 0x87, 0xca, 0x7c, 0x50, 0x5, 0xf, 0x80, 0xfa, 0x9f, 0x29, 0x80 },
  { 0xa, 0xa1, 0xfe, 0x0, 0x0, 0x55, 0x0, 0xf, 0xf0, 0xff, 0x0, 0x0 },
  { 0x0, 0x3f, 0xb0, 0x2, 0xaa, 0xeb, 0xe0, 0x6, 0x3f, 0xe0, 0x0, 0x20 },
  { 0x75, 0x3e, 0x31, 0x8f, 0xa8, 0xa, 0xb8, 0x22, 0xb, 0x60, 0xcf, 0xa0 },
  { 0x2a, 0xa1, 0x95, 0x30, 0xcc, 0x99, 0x86, 0x6b, 0x30, 0xd5, 0x60, 0x0 },
  { 0x7f, 0xf0, 0x3f, 0x82, 0xab, 0xea, 0xbf, 0xf7, 0xff, 0xe0, 0xfc, 0x20 },
};

//
// COLORED_PATTERNS - 16 hex bytes per pattern (see HEX.ipynb)
//
#define NUM_COLORED_PATTERNS 5   // Total number of colored patterns
const uint8_t colored_pattern_matrix[][16 * 2] = {
  { 0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0,
    0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0 },
  { 0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0,
    0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0 },
  { 0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0,
    0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0 },
  { 0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0,
    0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0 },
  { 0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0,
    0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0 },
};

#define NUM_RINGS 6   // Total number of rings
// To Do: Write another pattern converter (1 and 2 color)
const uint8_t ring_matrix[][16] = {
  { 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
  { 0x0, 0x0, 0x0, 0x0, 0x3, 0x60, 0x0, 0x14, 0x0, 0x0, 0x0, 0x0 },
  { 0x0, 0x0, 0x15, 0x0, 0xc, 0x18, 0x0, 0x6b, 0x0, 0x0, 0x0, 0x0 },
  { 0x0, 0x0, 0x6a, 0xc0, 0x30, 0x6, 0x1, 0x80, 0xc0, 0x2a, 0x80, 0x0 },
  { 0x2a, 0xa1, 0x80, 0x30, 0xc0, 0x1, 0x86, 0x0, 0x30, 0xd5, 0x60, 0x0 },
  { 0xd5, 0x5e, 0x0, 0xf, 0x0, 0x0, 0x78, 0x0, 0xf, 0x0, 0x1f, 0xe0 },
};

//
// Pixel neighbors
//
uint8_t neighbors[] PROGMEM = {
  XX, XX, 1, 2, 26, 28,
  XX, XX, XX, 3, 2, 0,
  0, 1, 3, 4, 24, 26,
  1, XX, XX, 5, 4, 2,
  2, 3, 5, 6, 22, 24,
  3, XX, XX, 7, 6, 4,
  4, 5, 7, 8, 20, 22,
  5, XX, XX, 9, 8, 6,
  6, 7, 9, 10, 18, 20,
  7, XX, XX, 11, 10, 8,
  8, 9, 11, 12, 16, 18,
  9, XX, XX, XX, 12, 10,
  10, 11, XX, XX, 14, 16,
  15, 14, XX, XX, 49, 47,
  16, 12, XX, XX, 13, 15,
  17, 16, 14, 13, 47, 45,
  18, 10, 12, 14, 15, 17,
  19, 18, 16, 15, 45, 43,
  20, 8, 10, 16, 17, 19,
  21, 20, 18, 17, 43, 41,
  22, 6, 8, 18, 19, 21,
  23, 22, 20, 19, 41, 39,
  24, 4, 6, 20, 21, 23,
  25, 24, 22, 21, 39, 37,
  26, 2, 4, 22, 23, 25,
  27, 26, 24, 23, 37, 35,
  28, 0, 2, 24, 25, 27,
  29, 28, 26, 25, 35, 33,
  XX, XX, 0, 26, 27, 29,
  XX, XX, 28, 27, 33, 31,
  XX, XX, 31, 32, 69, XX,
  XX, XX, 29, 33, 32, 30,
  30, 31, 33, 34, 67, 69,
  31, 29, 27, 35, 34, 32,
  32, 33, 35, 36, 65, 67,
  33, 27, 25, 37, 36, 34,
  34, 35, 37, 38, 63, 65,
  35, 25, 23, 39, 38, 36,
  36, 37, 39, 40, 61, 63,
  37, 23, 21, 41, 40, 38,
  38, 39, 41, 42, 59, 61,
  39, 21, 19, 43, 42, 40,
  40, 41, 43, 44, 57, 59,
  41, 19, 17, 45, 44, 42,
  42, 43, 45, 46, 55, 57,
  43, 17, 15, 47, 46, 44,
  44, 45, 47, 48, 53, 55,
  45, 15, 13, 49, 48, 46,
  46, 47, 49, 50, 51, 53,
  47, 13, XX, XX, 50, 48,
  48, 49, XX, XX, XX, 51,
  53, 48, 50, XX, XX, 52,
  54, 53, 51, XX, XX, 84,
  55, 46, 48, 51, 52, 54,
  56, 55, 53, 52, 84, 82,
  57, 44, 46, 53, 54, 56,
  58, 57, 55, 54, 82, 80,
  59, 42, 44, 55, 56, 58,
  60, 59, 57, 56, 80, 78,
  61, 40, 42, 57, 58, 60,
  62, 61, 59, 58, 78, 76,
  63, 38, 40, 59, 60, 62,
  64, 63, 61, 60, 76, 74,
  65, 36, 38, 61, 62, 64,
  66, 65, 63, 62, 74, 72,
  67, 34, 36, 63, 64, 66,
  68, 67, 65, 64, 72, 70,
  69, 32, 34, 65, 66, 68,
  XX, 69, 67, 66, 70, XX,
  XX, 30, 32, 67, 68, XX,
  XX, 68, 66, 72, 71, XX,
  XX, 70, 72, 73, 90, XX,
  70, 66, 64, 74, 73, 71,
  71, 72, 74, 75, 89, 90,
  72, 64, 62, 76, 75, 73,
  73, 74, 76, 77, 88, 89,
  74, 62, 60, 78, 77, 75,
  75, 76, 78, 79, 87, 88,
  76, 60, 58, 80, 79, 77,
  77, 78, 80, 81, 86, 87,
  78, 58, 56, 82, 81, 79,
  79, 80, 82, 83, 85, 86,
  80, 56, 54, 84, 83, 81,
  81, 82, 84, XX, XX, 85,
  82, 54, 52, XX, XX, 83,
  86, 81, 83, XX, XX, XX,
  87, 79, 81, 85, XX, XX,
  88, 77, 79, 86, XX, XX,
  89, 75, 77, 87, XX, XX,
  90, 73, 75, 88, XX, XX,
  XX, 71, 73, 89, XX, XX,
};


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");  // Serial: For debugging

  Serial1.begin(9600);  // Serial1: xBee port
  
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show

  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setCoordMap(MAX_COORD, coords);  // x, y grid of hexes
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    led[i].fillBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].setColorSpeedMinMax(2, 10); // Speed up color changing
    shows[i].setWaitRange(4, 25, 21);
  }
}

//
// loop
//
void loop() { 

  for (uint8_t i = 0; i < DUAL; i++) {
    
    if (get_intensity(i) > 0) {
      
      switch (current_show[i]) {
  
        case 0:
          patternsBW(i);
          break;
        case 1:
          shows[i].juggle_fastled();
//          patterns2(i);
          break;
        case 2:
          patterns_four_color(i);
          break;
        case 3:
          warp1(i);  // Nope
          break;
        case 4:
          warp2(i);
          break;
        case 5:
          shows[i].sawTooth();
          break;
        case 6:
          shows[i].morphChain();
          break;
        case 7:
          shows[i].packets();
          // shows[i].bounce();
          break;
        case 8:
          shows[i].packets_two();
          //shows[i].bounceGlowing();
          break;
        case 9:
          shows[i].sinelon_fastled();
          //shows[i].plinko(1);  // 1 = starting pixel !! To Do: check this
          break;
        case 10:
          shows[i].bands();
          break;
        case 11:
          vert_back_forth_dots(i);  // grided
          break;
        case 12:
          vert_back_forth_bands(i);  // grided
          break;
        case 13:
          vert_back_forth_colors(i);  // grided
          break;
        case 14:
          horiz_back_forth_dots(i);  // grided
          break;
        case 15:
          horiz_back_forth_bands(i);  // grided
          break;
        case 16:
          horiz_back_forth_colors(i);  // grided
          break;
        case 17:
          diag_back_forth_dots(i);  // grided
          break;
        case 18:
          diag_back_forth_bands(i);  // grided
          break;
        default:
          diag_back_forth_colors(i);  // grided
          break;
      }
 
      shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
    }
  }

  update_leds();  // push the interp_frames on to the leds
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
  speak_and_hear();  // speak out or hear in signals
  
  delay(DELAY_TIME); // The only delay
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
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  led[i].fillBlack();
  led[i].push_frame();
  shows[i].resetAllClocks();

//  // Set new show's colors to those of the other channel
//  uint8_t other_channel = (i == 0) ? 1 : 0 ;
//  shows[i].setForeColor(shows[other_channel].getForeColor());
//  shows[i].setBackColor(shows[other_channel].getBackColor());
  
//  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  current_show[i] = is_other_channel_show_zero(i) ? random8(1, NUM_SHOWS) : 0 ;
  current_pattern[i] = random8(NUM_PATTERNS);
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
//  speak_all_commands();

//  log_status(i);  // For debugging
}

boolean is_other_channel_show_zero(uint8_t channel) {
  if (channel == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

////// Speaking

void speak_and_hear() {
  if (ARE_CONNECTED && IS_SPEAKING) { speak_all_commands(); }
  if (ARE_CONNECTED && !IS_SPEAKING) { hear_signal(); }  
}

boolean is_listening() {
  return (ARE_CONNECTED && !IS_SPEAKING && !is_lost);
}

void speak_all_commands() {
  uint8_t m = 0;  // where we are in the send-out message string

  if (shows[0].getMorph() == 0 || shows[1].getMorph() == 0) {
    m = speak_start(m);
    m = speak_command(COMMAND_BRIGHTNESS, BRIGHTNESS, m);
    
    for (uint8_t i = 0; i < DUAL; i++) {  // Send one channel, then the next
      if (shows[i].getMorph() == 0) {
        m = speak_channel(i, m);  // Send the channel A or B prompt
        m = speak_command(COMMAND_SM_CYCLE, shows[i].getSmallCycle(), m);

        if (shows[i].getCycle() < 3) {
          m = speak_command(COMMAND_SHOW, current_show[i], m);
          m = speak_command(COMMAND_PATTERN, current_pattern[i], m);
          m = speak_command(COMMAND_WAIT, shows[i].getWait(), m);
          m = speak_command(COMMAND_FORE, shows[i].getForeColor(), m);
          m = speak_command(COMMAND_BACK, shows[i].getBackColor(), m);
        }
      }
    }
    
    message[m++] = COMMAND_PERIOD;  // Terminate message with null character
    message[m++] = '\0';  // Terminate message with null character
    Serial1.print(message);  // Only done once
//    Serial.println(message);  // For debugging
  }
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

    case COMMAND_PATTERN:  // Pattern
      current_pattern[i] = value;
      break;
  }
}

////// End Speaking & Hearing


////// Specialized shows

//
// morph_chain - grided! not the standard: shows[i].morphChain()
//
void morph_chain(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].makeWaitFaster(4);  // Divide wait value by 4 (make much faster)
  }
  uint8_t i, adj_i, fract;
  for (i = 0; i < NUM_LEDS; i++) {
    adj_i = i + (HEX_NUM * NUM_LEDS);
    fract = map((adj_i + shows[c].getCycle()) % NUM_LEDS, 0, NUM_LEDS, 0, 255);
    led[c].setPixelHue(i, led[c].interpolate_wrap(shows[c].getForeColor(), shows[c].getBackColor(), fract));
  }
}

//
// patterns shows
//
void patternsBW(uint8_t c) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_pattern_number(i, current_pattern[c])) {
      shows[c].setPixeltoHue(i, HEX_HUE);
    } else {
      shows[c].setPixeltoBackColor(i);
    }
  }
}

void patterns2(uint8_t c) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_pattern_number(i, current_pattern[c])) {
        shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoBlack(i);
    }
  }
}

void patterns_four_color(uint8_t c) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
     
    switch (get_two_bits_from_pattern_number(i, current_pattern[c])) {
      case 0:
        shows[c].setPixeltoBlack(i);
        break;
      case 1:
        shows[c].setPixeltoForeColor(i);
        break;
      case 2:
        shows[c].setPixeltoBackColor(i);
        break;
      default:
        shows[c].setPixeltoHue(i, HEX_HUE);
        break;
    }
  }
}

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
// horiz back forth dots - horizontal dots moving back and forth
//
void horiz_back_forth_dots(uint8_t c) {
  uint8_t temp_y;
  uint16_t cycle = shows[c].getCycle();

  led[c].fillBlack();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_y = y + HEX_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      if ((temp_y + cycle) % MAX_XGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
}

//
// horiz back forth bands - horizontal bands moving back and forth
//
void horiz_back_forth_bands(uint8_t c) {
  uint8_t temp_y, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_y = y + HEX_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp_y = (temp_y + cycle) % MAX_YGRID;
      intensity = sin8(map(temp_y, 0, MAX_YGRID, 0, 255) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
}

//
// horiz back forth colors - horizontal colors moving back and forth
//
void horiz_back_forth_colors(uint8_t c) {
  uint8_t temp_y, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_y = y + HEX_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp_y = (temp_y + cycle) % MAX_YGRID;
      hue = sin8(map(temp_y, 0, MAX_YGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_YGRID)) );
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
      if ((temp_x + temp_y + cycle) % MAX_YGRID == 0) {
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
      temp = (temp_x + temp_y + cycle) % MAX_YGRID;
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
      temp = (temp_x + temp_y + cycle) % MAX_YGRID;
      hue = sin8(map(temp, 0, MAX_YGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelHue(led[c].getLedFromCoord(x,y), hue);
    }
  }
}

//
// draw_ring
//
void draw_ring(uint8_t ring_n, CHSV color, uint8_t c) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_ring(i, ring_n % NUM_RINGS) == true) {
        led[c].setPixelColor(i, color);
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
void tunnelvision(CHSV color1, CHSV color2, CHSV background, uint8_t i) {  
  led[i].fill(background);
  uint8_t ring = shows[i].getCycle() % NUM_RINGS;
  draw_ring(ring, color1, i);
  if (ring != 0) {
    draw_ring(ring - 1, color2, i);
  }
}

//
// warp1 - colors on a black field
// 
void warp1(uint8_t i) {
  
  switch ((shows[i].getCycle() / NUM_RINGS) % 6) {
    case 0:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0, 255, 0)), led[i].rgb_to_hsv(CRGB(0,100,0)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    case 1:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,0,255)), led[i].rgb_to_hsv(CRGB(0,0,100)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    case 2:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,255,255)), led[i].rgb_to_hsv(CRGB(0,100,100)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    case 3:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,255,0)), led[i].rgb_to_hsv(CRGB(100,100,0)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);  
      break;
    case 4:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,0,0)), led[i].rgb_to_hsv(CRGB(100,0,0)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    default:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,0,255)), led[i].rgb_to_hsv(CRGB(100,0,100)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);  
      break;
  }
}

//
// warp2 - colors on a green field
//
void warp2(uint8_t i) {
  
  switch ((shows[i].getCycle() / NUM_RINGS) % 8) {
    case 0:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,255,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 1:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,200,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 2:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,150,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 3:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,100,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 4:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,255,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 5:
      tunnelvision(led[i].rgb_to_hsv(CRGB(200,200,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 6:
      tunnelvision(led[i].rgb_to_hsv(CRGB(150,150,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    default:
      tunnelvision(led[i].rgb_to_hsv(CRGB(100,100,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);  
      break;
  }
}

///////// DUAL SHOW LOGIC

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  uint8_t led_number;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    led_number = convert_pixel_to_led(i);
    if (led_number != XX) {
      leds[led_number] = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][i], 
                                                     leds_buffer[CHANNEL_A][i], 
                                                     fract);  // interpolate a + b channels
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
  
  return intensity;
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
// convert pixel to led: account for the spacer pixels
//
uint8_t convert_pixel_to_led(uint8_t i) {
  if (i == XX) {
    return XX;
  }
  return pgm_read_byte_near(LED_LOOKUP + (i % NUM_LEDS));
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
// Unpack hex into bits
boolean get_bit_from_ring(uint8_t i, uint8_t ring_number) {
  uint8_t pattern_byte = ring_matrix[ring_number][(i / 8)];
  return (pattern_byte & (1 << (7 - (i % 8)) ));
}

boolean get_bit_from_pattern_number(uint8_t i, uint8_t pattern_number) {
  uint8_t pattern_byte = pattern_matrix[pattern_number][(i / 8)];
  return (pattern_byte & (1 << (7 - (i % 8)) ));
}

uint8_t get_two_bits_from_pattern_number(uint8_t i, uint8_t pattern_number) {
  uint8_t pattern_byte = colored_pattern_matrix[pattern_number][(i / 4)];
  return ( (pattern_byte >> (2 * (3 - (i % 4)))) & B00000011 );
}

//
// log status
//
void log_status(uint8_t i) {
  Serial.print("Channel: ");
  Serial.print(i);
  Serial.print(", Show: ");
  Serial.print(current_show[i]);
  Serial.print(", Pattern: ");
  Serial.print(current_pattern[i]);
  Serial.println(".");
}
