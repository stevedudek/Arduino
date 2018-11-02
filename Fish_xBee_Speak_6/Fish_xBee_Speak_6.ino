#include <Noise.h>
#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
//
//  Fish
//
//  7/1/18
//
//  FastLED
//
//  xBee - Speaking Mode
//
//  Grid effects
//
#define FISH_NUM   0
#define MAX_FISH   6

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40 // in milliseconds

// Fish Grid - Each fish is 11 wide x 7 tall
//             MAX_XGRID, MAX_YGRID must be the same for all fish
#define FISH_XGRID  0  // x-coord (upper left)
#define FISH_YGRID  0  // y-coord (upper left)
#define MAX_XGRID  7   // 7 has all fish in a line (not stacked)
#define MAX_YGRID  66  // 66 would have all fish in a line

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 61
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number
#define TOTAL_LEDS (MAX_FISH * NUM_LEDS)
#define SIMPLE_TAIL false  // simple tails have 5 segments

#define MAX_XCOORD  7
#define MAX_YCOORD 11

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
#define FISH_HUE (FISH_NUM * (FISH_NUM / MAX_COLOR)) // Main fish color: red (https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors)
CHSV BLACK = CHSV(0, 0, 0);

// Palettes
#define CAN_CHANGE_PALETTES false  // Has strange effects on colors - use with care
#define PALETTE_DURATION 300  // seconds between palettes

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library (will need to be reinitialized)

// Shows
#define NUM_SHOWS 21
uint8_t current_show = 0;
uint8_t current_pattern = 0;

// wait times
#define SHOW_DURATION 20  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 1   // seconds to fade out
#define FADE_IN_TIME 1    // seconds to fade out

// noise
#define HAVE_NOISE true   // set to false to suppress noise
#define MAX_HUE_NOISE 128   // 255 max 
#define MAX_SAT_NOISE 128   // 255 max
Noise noise = Noise(MAX_XCOORD, MAX_YCOORD);

// xBee language
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_BRIGHTNESS 'I'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_CYCLE      'C'
#define COMMAND_PATTERN    'P'
#define COMMAND_NOISE      'N'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define MAX_MESSAGE        40
#define MAX_NUM             5  // To handle 65,535 of small_cycle

// Lookup tables

const int8_t coords[] PROGMEM = {
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
const int8_t neighbors[] PROGMEM = {
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
  //  Serial.println("Speaking");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  if (HAVE_NOISE) {
    noise.turnNoiseOn();
  } else {
    noise.turnNoiseOff();
  }

  if (CAN_CHANGE_PALETTES) {
    led.setPalette();
  }
  
  // Set up the various mappings (1D lists in PROGMEM)
  //led.setLedMap(ConeLookUp);  // turned off mapping
  led.setCoordMap(MAX_YCOORD, coords);  // x,y grid of cones
  led.setNeighborMap(neighbors);  // 6 neighbors for every pixel
  shows = Shows(&led);  // Show library - reinitialized for led mappings
  
  led.fillBlack();
  led.push_frame();
}

//
// loop
//
void loop() { 

  switch (current_show) {
  
    case 0:
      patternsBW();
      break;
    case 1:
      patterns2();
      break;
    case 2:
      patterns_four_color();
      break;
    case 3:
      warp1();
      break;
    case 4:
      warp2();
      break;
    case 5:
      saw_tooth();  // grided
      break;
    case 6:
      morph_chain();  // grided
      break;
    case 7:
      shows.bounce();
      break;
    case 8:
      shows.bounceGlowing();
      break;
    case 9:
      shows.plinko(1);  // 1 = starting pixel
      break;
    case 10:
      shows.bands();  // grided
      break;
    case 11:
      vert_back_forth_dots();  // grided
      break;
    case 12:
      vert_back_forth_bands();  // grided
      break;
    case 13:
      vert_back_forth_colors();  // grided
      break;
    case 14:
      horiz_back_forth_dots();  // grided
      break;
    case 15:
      horiz_back_forth_bands();  // grided
      break;
    case 16:
      horiz_back_forth_colors();  // grided
      break;
    case 17:
      diag_back_forth_dots();  // grided
      break;
    case 18:
      diag_back_forth_bands();  // grided
      break;
    case 19:
      diag_back_forth_colors();  // grided
      break;
    default:
      noisyshow();
      break;
  }

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay

  noise.fillNoise();
  
  shows.advanceClock();
  
  if (shows.getSmallCycle() >= MAX_SMALL_CYCLE) { 
    next_show(); 
  }
  
  EVERY_N_SECONDS(PALETTE_DURATION) { 
    led.randomizePalette();
  }
}

//
// next_show
//
void next_show() {
  // Switch between a patterns show and all the other shows
  current_pattern = random8(NUM_PATTERNS);
  
  if (current_show < 2) {
    current_show++;
  } else if (current_show == 2) {
    current_show = random8(3, NUM_SHOWS);
  } else {
    current_show = 0;
  }

  led.fillBlack();
  led.push_frame();
  shows.resetAllClocks();
  noise.setRandomNoiseParams();
  shows.pickRandomColorSpeeds();
  shows.pickRandomWait();

  speak_all_commands();
}

////// Speaking

void speak_all_commands() {
  // Send all commands at once
  speak_command(COMMAND_SHOW, current_show);
  speak_command(COMMAND_PATTERN, current_pattern);
  speak_command(COMMAND_FORE, shows.getForeColor());
  speak_command(COMMAND_BACK, shows.getBackColor());
  speak_command(COMMAND_BRIGHTNESS, BRIGHTNESS);
  speak_command(COMMAND_WAIT, shows.getWait());
  speak_command(COMMAND_CYCLE, shows.getCycle());
  speak_end_command();
}

void speak_command(char command, int value) {
  Serial.print(command);
  Serial.print(value);
  Serial.print(COMMAND_COMMA);
}

void speak_end_command() {
  Serial.println(COMMAND_PERIOD);  // Speak terminal period
}

void speak_cycle() {
  speak_command(COMMAND_CYCLE, shows.getCycle());
  speak_end_command();
}

////// Specialized shows

//
// saw_tooth - grided! not the standard: shows.sawTooth()
//
void saw_tooth() {
  if (shows.isShowStart()) {
    shows.makeWaitFaster(4);  // Divide wait value by 4 (make much faster)
  }
  uint8_t i, adj_i;
  uint16_t fract;
  
  for (i=0; i < NUM_LEDS; i++) {
    adj_i = i + (FISH_NUM * NUM_LEDS);
    fract = map((adj_i + shows.getCycle()) % TOTAL_LEDS, 0, TOTAL_LEDS, 0, 512);
    if (fract >= 256) {
      fract = 512 - fract;  // the subtraction creates the sawtooth
    }
    led.setPixelColor(i, led.gradient_wheel(shows.getForeColor(), fract));
  }
}

//
// morph_chain - grided! not the standard: shows.morphChain()
//
void morph_chain() {
  if (shows.isShowStart()) {
    shows.makeWaitFaster(4);  // Divide wait value by 4 (make much faster)
  }
  uint8_t i, adj_i, fract;
  for (i=0; i < NUM_LEDS; i++) {
    adj_i = i + (FISH_NUM * NUM_LEDS);
    fract = map((adj_i + shows.getCycle()) % TOTAL_LEDS, 0, TOTAL_LEDS, 0, 255);
    led.setPixelHue(i, led.interpolate_wrap(shows.getForeColor(), shows.getBackColor(), fract));
  }
}

//
// patterns shows
//
void patternsBW() {
  for (int i=0; i < NUM_LEDS; i++) {
    if (get_bit_from_pattern_number(i, current_pattern)) {
      shows.setPixeltoHue(i, FISH_HUE);
    } else {
      shows.setPixeltoBackColor(i);
    }
  }
}

void patterns2() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_pattern_number(i, current_pattern)) {
      shows.setPixeltoForeColor(i);
    } else {
      shows.setPixeltoBlack(i);
    }
  }
}

void patterns_four_color() {
  for (int i = 0; i < NUM_LEDS; i++) {
    switch (get_two_bits_from_pattern_number(i, current_pattern)) {
      case 0:
        shows.setPixeltoBlack(i);
        break;
      case 1:
        shows.setPixeltoForeColor(i);
        break;
      case 2:
        shows.setPixeltoBackColor(i);
        break;
      default:
        shows.setPixeltoHue(i, FISH_HUE);
        break;
    }
  }
}

//
// vert back forth dots - vertical dots moving back and forth
//
void vert_back_forth_dots() {
  uint8_t temp_x;
  uint16_t cycle = shows.getCycle();

  led.fillBlack();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      if ((temp_x + cycle) % MAX_XGRID == 0) {
        shows.setPixeltoForeColor(led.getLedFromCoord(x,y));
      }
    }
  }
  shows.setPixeltoForeColor(1);  // Light up the head
}

//
// vert back forth bands - vertical bands moving back and forth
//
void vert_back_forth_bands() {
  uint8_t temp_x, intensity;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_x = (temp_x + cycle) % MAX_XGRID;
      intensity = sin8_avr(map(temp_x, 0, MAX_XGRID, 0, 255) + (cycle % (255 / MAX_XGRID)) );
      led.setPixelColor(led.getLedFromCoord(x,y), led.gradient_wheel(shows.getBackColor(), intensity));
    }
  }
  shows.setPixeltoBackColor(1);  // Light up the head
}

//
// vert back forth colors - vertical colors moving back and forth
//
void vert_back_forth_colors() {
  uint8_t temp_x, hue;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_x = x + FISH_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_x = (temp_x + cycle) % MAX_XGRID;
      hue = sin8_avr(map(temp_x, 0, MAX_XGRID, shows.getForeColor(), shows.getBackColor()) + (cycle % (255 / MAX_XGRID)) );
      led.setPixelHue(led.getLedFromCoord(x,y), hue);
    }
  }
}

//
// horiz back forth dots - horizontal dots moving back and forth
//
void horiz_back_forth_dots() {
  uint8_t temp_y;
  uint16_t cycle = shows.getCycle();

  led.fillBlack();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_y = y + FISH_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      if ((temp_y + cycle) % MAX_XGRID == 0) {
        shows.setPixeltoForeColor(led.getLedFromCoord(x,y));
      }
    }
  }
  shows.setPixeltoForeColor(1);  // Light up the head
}

//
// horiz back forth bands - horizontal bands moving back and forth
//
void horiz_back_forth_bands() {
  uint8_t temp_y, intensity;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_y = y + FISH_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp_y = (temp_y + cycle) % MAX_YGRID;
      intensity = sin8_avr(map(temp_y, 0, MAX_YGRID, 0, 255) + (cycle % (255 / MAX_YGRID)) );
      led.setPixelColor(led.getLedFromCoord(x,y), led.gradient_wheel(shows.getBackColor(), intensity));
    }
  }
  shows.setPixeltoForeColor(1);  // Light up the head
}

//
// horiz back forth colors - horizontal colors moving back and forth
//
void horiz_back_forth_colors() {
  uint8_t temp_y, hue;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_y = y + FISH_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp_y = (temp_y + cycle) % MAX_YGRID;
      hue = sin8_avr(map(temp_y, 0, MAX_YGRID, shows.getForeColor(), shows.getBackColor()) + (cycle % (255 / MAX_YGRID)) );
      led.setPixelHue(led.getLedFromCoord(x,y), hue);
    }
  }
}

//
// diag back forth dots - diagonal dots moving back and forth
//
void diag_back_forth_dots() {
  uint8_t temp_x, temp_y;
  uint16_t cycle = shows.getCycle();

  led.fillBlack();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_y = y + FISH_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      if ((temp_x + temp_y + cycle) % MAX_YGRID == 0) {
        shows.setPixeltoForeColor(led.getLedFromCoord(x,y));
      }
    }
  }
  shows.setPixeltoForeColor(1);  // Light up the head
}

//
// diag back forth bands - diagonal bands moving back and forth
//
void diag_back_forth_bands() {
  uint8_t temp_x, temp_y, temp, intensity;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = x + FISH_XGRID;
      temp_y = y + FISH_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp = (temp_x + temp_y + cycle) % MAX_YGRID;
      intensity = sin8_avr(map(temp, 0, MAX_YGRID, 0, 255) + (cycle % (255 / MAX_YGRID)) );
      led.setPixelColor(led.getLedFromCoord(x,y), led.gradient_wheel(shows.getBackColor(), intensity));
    }
  }
  shows.setPixeltoBackColor(1);  // Light up the head
}

//
// diag back forth colors - diagonal colors moving back and forth
//
void diag_back_forth_colors() {
  uint8_t temp_x, temp_y, temp, hue;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_x = x + FISH_XGRID;
      temp_y = y + FISH_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp = (temp_x + temp_y + cycle) % MAX_YGRID;
      hue = sin8_avr(map(temp, 0, MAX_YGRID, shows.getForeColor(), shows.getBackColor()) + (cycle % (255 / MAX_YGRID)) );
      led.setPixelHue(led.getLedFromCoord(x,y), hue);
    }
  }
}

//
// draw_ring
//
void draw_ring(uint8_t ring_n, CHSV color) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_ring(i, ring_n % NUM_RINGS) == true) {
      led.setPixelColor(i, color);
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
void tunnelvision(CHSV color1, CHSV color2, CHSV background) {  
  led.fill(background);
  uint8_t ring = shows.getCycle() % NUM_RINGS;
  draw_ring(ring, color1);
  if (ring != 0) {
    draw_ring(ring - 1, color2);
  }
}

//
// warp1 - colors on a black field
// 
void warp1() {
  
  switch ((shows.getCycle() / NUM_RINGS) % 6) {
    case 0:
      tunnelvision(led.rgb_to_hsv(CRGB(0, 255, 0)), led.rgb_to_hsv(CRGB(0,100,0)), led.rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 1:
      tunnelvision(led.rgb_to_hsv(CRGB(0,0,255)), led.rgb_to_hsv(CRGB(0,0,100)), led.rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 2:
      tunnelvision(led.rgb_to_hsv(CRGB(0,255,255)), led.rgb_to_hsv(CRGB(0,100,100)), led.rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 3:
      tunnelvision(led.rgb_to_hsv(CRGB(255,255,0)), led.rgb_to_hsv(CRGB(100,100,0)), led.rgb_to_hsv(CRGB(0,0,0)));  
      break;
    case 4:
      tunnelvision(led.rgb_to_hsv(CRGB(255,0,0)), led.rgb_to_hsv(CRGB(100,0,0)), led.rgb_to_hsv(CRGB(0,0,0)));
      break;
    default:
      tunnelvision(led.rgb_to_hsv(CRGB(255,0,255)), led.rgb_to_hsv(CRGB(100,0,100)), led.rgb_to_hsv(CRGB(0,0,0)));  
      break;
  }
}

//
// warp2 - colors on a green field
//
void warp2() {
  
  switch ((shows.getCycle() / NUM_RINGS) % 8) {
    case 0:
      tunnelvision(led.rgb_to_hsv(CRGB(0,255,100)), led.rgb_to_hsv(CRGB(0,160,100)), led.rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 1:
      tunnelvision(led.rgb_to_hsv(CRGB(0,200,100)), led.rgb_to_hsv(CRGB(0,160,100)), led.rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 2:
      tunnelvision(led.rgb_to_hsv(CRGB(0,150,100)), led.rgb_to_hsv(CRGB(0,160,100)), led.rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 3:
      tunnelvision(led.rgb_to_hsv(CRGB(0,100,100)), led.rgb_to_hsv(CRGB(0,160,100)), led.rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 4:
      tunnelvision(led.rgb_to_hsv(CRGB(255,255,0)), led.rgb_to_hsv(CRGB(160,160,0)), led.rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 5:
      tunnelvision(led.rgb_to_hsv(CRGB(200,200,0)), led.rgb_to_hsv(CRGB(160,160,0)), led.rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 6:
      tunnelvision(led.rgb_to_hsv(CRGB(150,150,0)), led.rgb_to_hsv(CRGB(160,160,0)), led.rgb_to_hsv(CRGB(0,100,0)));
      break;
    default:
      tunnelvision(led.rgb_to_hsv(CRGB(100,100,0)), led.rgb_to_hsv(CRGB(160,160,0)), led.rgb_to_hsv(CRGB(0,100,0)));  
      break;
  }
}

//
// noisyshow
//
void noisyshow() {
  if (shows.isShowStart()) {
    noise.makeVeryNoisy();
  }
  shows.allOn();
}

//
// morph_frame
//
void morph_frame() {
  shows.morphFrame();  // 1. calculate interp_frame 2. adjust palette
  add_noise();   // Use the noise library
  update_leds();  // push the interp_frame on to the leds
  check_fades();  // Fade start and end of shows
  unify_head_tail(); // Make all 4 head pixels the same color
  FastLED.show();  // Update the display 
}

//
// add_noise - from library - uses led. getters and setters
//
void add_noise() {
  if (HAVE_NOISE) {
    for (uint8_t x = 0; x < MAX_XCOORD; x++) {
      for (uint8_t y = 0; y < MAX_YCOORD; y++) {
        int8_t i = led.getLedFromCoord(x, y);
        if (i != -1) {
          led.setInterpFrameHue(i, noise.addNoiseAtoValue(x, y, led.getInterpFrameHue(i))); 
          led.setInterpFrameSat(i, noise.addNoiseBtoValueNoWrap(x, y, led.getInterpFrameSat(i)));
        } 
      }
    }
  }
}

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = led.getInterpFrameColor(i);
  }
}

//
// check_fades - check the fade-to-blacks at beginning and end of show
//
void check_fades() {
  uint8_t fade_amount = 0;
  uint32_t small_cycle = shows.getSmallCycle();
  
  if (small_cycle <= (FADE_IN_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map(small_cycle, 0, (FADE_IN_TIME * 1000 / DELAY_TIME), 256, 0);
  } else if ((MAX_SMALL_CYCLE - small_cycle) <= (FADE_OUT_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map((MAX_SMALL_CYCLE - small_cycle), 0, (FADE_OUT_TIME * 1000 / DELAY_TIME), 256, 0);
  }
  
  // Fade lights
  if (fade_amount > 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(fade_amount);
    }
  }

  // Speak
  if (shows.getMorph()== 0) {
    if (fade_amount > 0) {
      speak_all_commands();
    } else {
      speak_cycle();
    }
  }
  
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

