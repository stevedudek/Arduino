#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
//
//  Fish
//
//  1/16/18
//
//  FastLED
//
//  FOR TEENSY ONLY
//
//  Speaking & Hearing
//
//  Complex Switch Axis
//
//  Dual shows - Blend together 2 shows running at once
//
//  Removed: Noise, palettes
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
#define FISH_NUM   0
#define MAX_FISH   6

uint8_t BRIGHTNESS = 255;  // (0-255)

#define DELAY_TIME 40 // in milliseconds

// Fish Grid - Each fish is 11 wide x 7 tall
//             MAX_XGRID, MAX_YGRID must be the same for all fish
#define FISH_XGRID  0  // x-coord (upper left)
#define FISH_YGRID  0  // y-coord (upper left)
#define MAX_XGRID  7   // 7 has all fish in a line (not stacked)
#define MAX_YGRID  66  // 66 would have all fish in a line

#define DATA_PIN 9
#define CLOCK_PIN 8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define ARE_CONNECTED true  // Are the Fish talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = millis();
#define MAX_SILENT_TIME  (3 * 1000)  // Time (in sec) without communication before marked as is_lost

#define NUM_LEDS 61
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number
#define TOTAL_LEDS (MAX_FISH * NUM_LEDS)
#define SIMPLE_TAIL true  // simple tails have 5 segments

#define MAX_XCOORD  7
#define MAX_YCOORD 11

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
#define FISH_HUE (FISH_NUM * (FISH_NUM / MAX_COLOR)) // Main fish color: red (https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors)
CHSV BLACK = CHSV(0, 0, 0);

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[NUM_LEDS];  // The Leds themselves

// Shows
#define START_SHOW_CHANNEL_A  0  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B  1  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 1 };
#define NUM_SHOWS 20

#define ONLY_RED false

// Clocks and time
#define SHOW_DURATION 30  // Typically 30 seconds. Size problems at 1800+ seconds.
#define FADE_TIME 28   // seconds to fade in. If FADE_TIME = SHOW_DURATION, then Always Be Fading
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

int8_t coords[] PROGMEM = {
  -1,-1, 1, 0,-1,-1,-1,  // 4 (1) - Head
  -1, 8, 7, 6, 5, 4,-1,  // 5
   9,10,11,12,13,14,-1,  // 6
  21,20,19,18,17,16,15,  // 7
  22,23,24,25,26,27,-1,  // 6
  34,33,32,31,30,29,28,  // 7
  35,36,37,38,39,40,-1,  // 6
  -1,45,44,43,42,41,-1,  // 5
  -1,46,47,48,49,-1,-1,  // 4
  -1,-1,52,51,50,-1,-1,  // 3
  -1,53,54,56,58,59,-1,  // 8 (5) - Tail
};

//
// PATTERNS - 8 hex bytes per pattern (see Fish.ipynb)
//
#define NUM_PATTERNS 19   // Total number of patterns
const uint8_t pattern_matrix[][8] = {
  { 0xa, 0xab, 0x56, 0xa5, 0x4a, 0xa9, 0x55, 0x50 },
  { 0xd, 0x99, 0x8c, 0xcc, 0x66, 0x6d, 0xaa, 0xd0 },
  { 0xf2, 0x18, 0x20, 0xc1, 0x6, 0x11, 0x97, 0xf8 },
  { 0xf2, 0x18, 0x71, 0xe5, 0x56, 0xbb, 0xd1, 0xe0 },
  { 0xf2, 0x18, 0x51, 0x24, 0x50, 0x91, 0xa9, 0x20 },
  { 0xf0, 0x7e, 0x3, 0xf0, 0x1f, 0x83, 0xc4, 0xc8 },
  { 0xf, 0xc3, 0x26, 0xd9, 0x30, 0xc6, 0x7f, 0x38 },
  { 0x2, 0x25, 0x25, 0x29, 0x29, 0x12, 0x54, 0xc8 },
  { 0x8, 0xa4, 0x50, 0xc1, 0x6, 0x2a, 0x42, 0x10 },
  { 0x8, 0x99, 0x4, 0xc8, 0x29, 0x12, 0x43, 0x30 },
  
  { 0xfd, 0xdb, 0x7, 0xf8, 0x36, 0xee, 0x46, 0xd8 },
  { 0xf2, 0x19, 0xdf, 0x3e, 0xf9, 0xee, 0x6a, 0x10 },
  { 0xd, 0xe7, 0xdf, 0x3e, 0xf9, 0xee, 0x68, 0x0 },
  { 0xf0, 0x43, 0x8f, 0x3e, 0xff, 0xc6, 0x6c, 0xc8 },
  { 0xf3, 0xf8, 0xf, 0xc, 0xfc, 0x80, 0xe3, 0x30 },
  { 0x3, 0x9d, 0xc7, 0x11, 0xc7, 0x62, 0x19, 0xf8 },
  { 0xff, 0xe7, 0x8e, 0x1c, 0x79, 0xec, 0x7, 0x38 },
  { 0xf7, 0x5b, 0x8c, 0xc, 0x76, 0xb9, 0xbb, 0xf0 },
  { 0xfd, 0xdb, 0x6d, 0xbb, 0x76, 0xed, 0x9c, 0x88 },
};

//
// COLORED_PATTERNS - 16 hex bytes per pattern (see Fish.ipynb)
//
#define NUM_COLORED_PATTERNS 8   // Total number of colored patterns
const uint8_t colored_pattern_matrix[][16] = {
  { 0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0 },
  { 0xaa, 0x1b, 0xe, 0x40, 0x6, 0xce, 0x40, 0x1, 0xb0, 0xe4, 0x6, 0xc3, 0x9b, 0x0, 0xb9, 0x0 },
  { 0x55, 0xff, 0xb7, 0x7f, 0xbb, 0xf7, 0xff, 0xe7, 0xbe, 0xfb, 0xef, 0x7f, 0x7f, 0xa5, 0xff, 0x80 },
  { 0xaa, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xb9, 0xbe, 0xbf, 0xff, 0x40 },
  { 0x55, 0x0, 0x0, 0x20, 0x0, 0x40, 0xcc, 0x80, 0x3, 0x1, 0x0, 0x2, 0x4, 0x8, 0x53, 0x0 },
  { 0xff, 0xd5, 0xf6, 0x9f, 0x62, 0x76, 0x9, 0x60, 0x26, 0xaa, 0x95, 0x5b, 0xeb, 0x9c, 0xa3, 0x40 },
  { 0xff, 0x55, 0x5f, 0xf7, 0xea, 0xfa, 0xa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1a, 0xa, 0x40 },
  { 0xff, 0xd1, 0xf4, 0x1f, 0x48, 0x7d, 0x7, 0xd2, 0x1f, 0x41, 0xf4, 0x7d, 0x7e, 0xf5, 0xa5, 0x80 },
};

#define NUM_RINGS 6   // Total number of rings
const uint8_t ring_matrix[][8] = {
  { 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0 },
  { 0x0, 0x0, 0x0, 0xc2, 0x86, 0x0, 0x0, 0x0 },
  { 0x0, 0x0, 0x71, 0x24, 0x49, 0x38, 0x0, 0x0 },
  { 0x0, 0x3c, 0x8a, 0x18, 0x30, 0xc7, 0xc0, 0x0 },
  { 0xf, 0xc3, 0x4, 0x0, 0x0, 0x0, 0x38, 0x0 },
  { 0xf0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7, 0xf8 },
};

//
// Pixel neighbors
//
// DIRECTIONS - If looking from head to tail
//
// 0 = Up, 1 = up-right, 2 = down-right, 3 = Down, 4 = down-left, 5 = up-left
//
int8_t neighbors[] PROGMEM = {
  -1,6,7,-1,-1,-1, // 0 - Same as 1
  -1,6,7,-1,-1,-1, // 1
  -1,6,7,-1,-1,-1, // 2 - Same as 1
  -1,6,7,-1,-1,-1, // 3 - Same as 1
  -1,14,13,5,-1,-1, // 4
  4,13,12,6,-1,-1, // 5
  5,12,11,7,1,-1, // 6
  6,11,10,8,-1,1, // 7
  7,10,9,-1,-1,-1, // 8
  10,20,21,-1,-1,8, // 9
  11,19,20,9,8,7, // 10
  12,18,19,10,7,6, // 11
  13,17,18,11,6,5, // 12
  14,16,17,12,5,4, // 13
  -1,15,16,13,4,-1, // 14
  -1,-1,27,16,14,-1, // 15
  15,27,26,17,13,14, // 16
  16,26,25,18,12,13, // 17
  17,25,24,19,11,12, // 18
  18,24,23,20,10,11, // 19
  19,23,22,21,9,10, // 20
  20,22,-1,-1,-1,9, // 21
  23,33,34,-1,21,20, // 22
  24,32,33,22,20,19, // 23
  25,31,32,23,19,18, // 24
  26,30,31,24,18,17, // 25
  27,29,30,25,17,16, // 26
  -1,28,29,26,16,15, // 27
  -1,-1,40,29,27,-1, // 28
  28,40,39,30,26,27, // 29
  29,39,38,31,25,26, // 30
  30,38,37,32,24,25, // 31
  31,37,36,33,23,24, // 32
  32,36,35,34,22,23, // 33
  33,35,-1,-1,-1,22, // 34
  36,45,-1,-1,34,33, // 35
  37,44,45,35,33,32, // 36
  38,43,44,36,32,31, // 37
  39,42,43,37,31,30, // 38
  40,41,42,38,30,29, // 39
  -1,-1,41,39,29,28, // 40
  -1,-1,49,42,39,40, // 41
  41,49,48,43,38,39, // 42
  42,48,47,44,37,38, // 43
  43,47,46,45,36,37, // 44
  44,46,-1,-1,35,36, // 45
  47,52,-1,-1,45,44, // 46
  48,51,52,46,44,43, // 47
  49,50,51,47,43,42, // 48
  -1,-1,50,48,42,41, // 49
  -1,58,56,51,48,49, // 50
  50,56,54,52,47,48, // 51
  51,54,-1,-1,46,47, // 52
  60,-1,-1,-1,54,56, // 53
  56,57,55,-1,52,51, // 54
  57,-1,-1,53,-1,54, // 55
  58,59,57,54,51,50, // 56
  59,-1,-1,55,54,56, // 57
  -1,60,59,56,50,-1, // 58
  60,-1,-1,57,56,58, // 59
  -1,-1,-1,59,58,-1, // 60
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
  
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show

  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setCoordMap(MAX_YCOORD, coords);  // x,y grid of cones
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    led[i].fillBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].setColorSpeedMinMax(2, 10); // Speed up color changing
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
          patterns2(i);
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
          saw_tooth(i);  // grided
          break;
        case 6:
          morph_chain(i);  // grided
          break;
        case 7:
          shows[i].bounce();
          break;
        case 8:
          shows[i].bounceGlowing();
          break;
        case 9:
          shows[i].plinko(1);  // 1 = starting pixel
          break;
        case 10:
          shows[i].bands();  // grided
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
  unify_head_tail();
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
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE && !is_listening()) { 
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
  // Switch between a patterns show and all the other shows
  current_pattern[i] = random8(NUM_PATTERNS);
  
  if (current_show[i] < 2) {
    current_show[i]++;
  } else if (current_show[i] == 2) {
    current_show[i] = random8(3, NUM_SHOWS);
  } else {
    current_show[i] = 0;
  }
  
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();

  speak_all_commands();
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
// saw_tooth - grided! not the standard: shows[i]sawTooth()
//
void saw_tooth(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].makeWaitFaster(4);  // Divide wait value by 4 (make much faster)
  }
  uint8_t i, adj_i;
  uint16_t fract;
  
  for (i = 0; i < NUM_LEDS; i++) {
    adj_i = i + (FISH_NUM * NUM_LEDS);
    fract = map((adj_i + shows[c].getCycle()) % TOTAL_LEDS, 0, TOTAL_LEDS, 0, 512);
    if (fract >= 256) {
      fract = 512 - fract;  // the subtraction creates the sawtooth
    }
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), fract));
  }
}

//
// morph_chain - grided! not the standard: shows[i].morphChain()
//
void morph_chain(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].makeWaitFaster(4);  // Divide wait value by 4 (make much faster)
  }
  uint8_t i, adj_i, fract;
  for (i = 0; i < NUM_LEDS; i++) {
    adj_i = i + (FISH_NUM * NUM_LEDS);
    fract = map((adj_i + shows[c].getCycle()) % TOTAL_LEDS, 0, TOTAL_LEDS, 0, 255);
    led[c].setPixelHue(i, led[c].interpolate_wrap(shows[c].getForeColor(), shows[c].getBackColor(), fract));
  }
}

//
// patterns shows
//
void patternsBW(uint8_t c) {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_pattern_number(i, current_pattern[c])) {
      shows[c].setPixeltoHue(i, FISH_HUE);
    } else {
      shows[c].setPixeltoBackColor(i);
    }
  }
}

void patterns2(uint8_t c) {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_pattern_number(i, current_pattern[c])) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoBlack(i);
    }
  }
}

void patterns_four_color(uint8_t c) {
  for (int i = 0; i < NUM_LEDS; i++) {
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
        shows[c].setPixeltoHue(i, FISH_HUE);
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
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      if ((temp_x + cycle) % MAX_XGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
  shows[c].setPixeltoForeColor(1);  // Light up the head
}

//
// vert back forth bands - vertical bands moving back and forth
//
void vert_back_forth_bands(uint8_t c) {
  uint8_t temp_x, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_x = (temp_x + cycle) % MAX_XGRID;
      intensity = sin8(map(temp_x, 0, MAX_XGRID, 0, 255) + (cycle % (255 / MAX_XGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
  shows[c].setPixeltoBackColor(1);  // Light up the head
}

//
// vert back forth colors - vertical colors moving back and forth
//
void vert_back_forth_colors(uint8_t c) {
  uint8_t temp_x, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_x = x + FISH_XGRID;
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
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_y = y + FISH_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      if ((temp_y + cycle) % MAX_XGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
  shows[c].setPixeltoForeColor(1);  // Light up the head
}

//
// horiz back forth bands - horizontal bands moving back and forth
//
void horiz_back_forth_bands(uint8_t c) {
  uint8_t temp_y, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_y = y + FISH_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp_y = (temp_y + cycle) % MAX_YGRID;
      intensity = sin8(map(temp_y, 0, MAX_YGRID, 0, 255) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
  shows[c].setPixeltoForeColor(1);  // Light up the head
}

//
// horiz back forth colors - horizontal colors moving back and forth
//
void horiz_back_forth_colors(uint8_t c) {
  uint8_t temp_y, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_y = y + FISH_YGRID;
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
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_y = y + FISH_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      if ((temp_x + temp_y + cycle) % MAX_YGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
  shows[c].setPixeltoForeColor(1);  // Light up the head
}

//
// diag back forth bands - diagonal bands moving back and forth
//
void diag_back_forth_bands(uint8_t c) {
  uint8_t temp_x, temp_y, temp, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_y = y + FISH_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp = (temp_x + temp_y + cycle) % MAX_YGRID;
      intensity = sin8(map(temp, 0, MAX_YGRID, 0, 255) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
  shows[c].setPixeltoBackColor(1);  // Light up the head
}

//
// diag back forth colors - diagonal colors moving back and forth
//
void diag_back_forth_colors(uint8_t c) {
  uint8_t temp_x, temp_y, temp, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_x = x + FISH_XGRID;
      temp_y = y + FISH_YGRID;
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
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][i], 
                                          leds_buffer[CHANNEL_A][i], 
                                          fract);  // interpolate a + b channels
  }
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
// redden - if ONLY_RED, turn hues to red, yellow, blue
//
CHSV redden(CHSV color) {  
  if (ONLY_RED) { color.h = map8( sin8(color.h), 192, 60); }
  return color;
}

// 
//  unify head tail - Make the same color all pixels of the head
//
void unify_head_tail() {
  leds[0] = leds[1];  // head
  leds[2] = leds[1];
  leds[3] = leds[1];
  
  if (SIMPLE_TAIL) {
    leds[55] = leds[54]; // tail
    leds[57] = leds[56];
    leds[58] = leds[58];
  }
}

//
// Unpack hex into bits
boolean get_bit_from_ring(uint8_t n, uint8_t ring_number) {
  uint8_t pattern_byte = ring_matrix[ring_number][(n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern_byte = pattern_matrix[pattern_number][(n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

uint8_t get_two_bits_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern = pattern_number % NUM_COLORED_PATTERNS;
  uint8_t pattern_byte = colored_pattern_matrix[pattern][(n / 4)];
  return ( (pattern_byte >> (2 * (3 - (n % 4)))) & B00000011 );
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


