#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
//
//  Feather Cape - 38 Petals on a cape
//
//  6/14/23
//
//  FastLED
//
/*****************************************************************************/

uint8_t bright = 192;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 192;  // (0 = fast, 255 = slow)

#define DELAY_TIME  15  // in milliseconds

// Smoothing constants - lower is slower smoothing
// At 15 ms cycles: 40 = 96 ms, 30 = 127 ms, 20 = 191 ms, 10 = 382 ms, 5 = 765 ms, 2 = 1.9 s, 1 = 3.8 s
#define SMOOTHING_SHOWS_HUE   20   // Fastest full rainbow = DELAY_TIME * (255 / this value)
#define SMOOTHING_SHOWS_VALUE 20   // Fastest turn off/on = DELAY_TIME * (255 / this value)

#define DATA_PIN 0  // 9
#define CLOCK_PIN 2  // 8

#define NUM_LEDS 38
#define NUM_SPACERS 10  // Should always be turned off
#define TOTAL_LEDS 48

#define XX  255

#define MAX_XCOORD 11
#define MAX_YCOORD 4

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), Shows(&led[CHANNEL_B], CHANNEL_B)  };
CHSV led_buffer[NUM_LEDS];  // Required for smoothing
CRGB leds[TOTAL_LEDS];  // The Leds themselves

// Shows
#define NUM_SHOWS 20
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 1 };

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
#define MAIN_COLOR 120
#define BLACK CHSV(0, 0, 0)

// wait times
uint8_t show_duration = 90;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 128;  // 0 = no fading, to 255 = always be fading

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

// Lookup tables

uint8_t coords[] PROGMEM = {
     0,  1,  2, XX,
   3,  4,  5,  6,
     7,  8,  9, XX,
  10, 11, 12, 13,
    14, 15, 16, XX,
  17, 18, 19, 20,
    21, 22, 23, XX,
  24, 25, 26, 27,
    28, 29, 30, XX,
  31, 32, 33, 34,
    35, 36, 37, XX
};

uint8_t ConeLookUp[] PROGMEM = {
     0,  1,  2,
   7,  6,  5,  4,
     9, 10, 11,
  16, 15, 14, 13,
    18, 19, 20,
  25, 24, 23, 22,
    27, 28, 29,
  34, 33, 32, 31,
    36, 37, 38,
  43, 42, 41, 40,
    45, 46, 47
};

uint8_t SpacerPixels[NUM_SPACERS] = { 3, 8, 12, 17, 21, 26, 30, 35, 39, 44 };

uint8_t neighbors[] PROGMEM = {
  XX,1,4,3,XX,XX, // 0
  XX,2,5,4,0,XX,
  XX,XX,6,5,1,XX,
  0,4,7,XX,XX,XX,
  1,5,8,7,3,0, // 4
  2,6,9,8,4,1,
  XX,XX,XX,9,5,2,
  4,8,11,10,XX,3,
  5,9,12,11,7,4, // 8
  6,XX,13,12,8,5,
  7,11,14,XX,XX,XX,
  8,12,15,14,10,7,
  9,13,16,15,11,8, // 12
  XX,XX,XX,16,12,9,
  11,15,18,17,XX,10,
  12,16,19,18,14,11,
  13,XX,20,19,15,12, // 16
  14,18,21,XX,XX,XX,
  15,19,22,21,17,14,
  16,20,23,22,18,15,
  XX,XX,XX,23,19,16, // 20
  18,22,25,24,XX,17,
  19,23,26,25,21,18,
  20,XX,27,26,22,19,
  21,25,28,XX,XX,XX, // 24
  22,26,29,28,24,21,
  23,27,30,29,25,22,
  XX,XX,XX,30,26,23,
  25,29,32,31,XX,24, // 28
  26,30,33,32,28,25,
  27,XX,34,33,29,26,
  28,32,35,XX,XX,XX,
  29,33,36,35,31,28, // 32
  30,34,37,36,32,29,
  XX,XX,XX,37,33,30,
  32,36,XX,XX,XX,31,
  33,37,XX,XX,35,32, // 36
  34,XX,XX,XX,36,33,
};

#define NUM_PATTERNS 8   // Total number of patterns
// Patterns
//
// 0 = Off
// 1 = Color 1
// 2 = Color 2

uint8_t PatternMatrix[NUM_PATTERNS][38] = {
  {  1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1    },
  
  {  1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1    },
     
  {  2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2    },
  
  {  1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2,
   2,  1,  1,  2,
     1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2,
   2,  1,  1,  2,
     1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2    },
  
  {  1,  1,  1,
   1,  1,  1,  1,
     1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   1,  2,  2,  1,
     1,  2,  1,
   1,  1,  1,  1,
     1,  1,  1    },
     
  {  1,  2,  1,
   2,  1,  2,  1,
     2,  1,  2,
   1,  2,  1,  2,
     1,  2,  1,
   2,  1,  2,  1,
     2,  1,  2,
   1,  2,  1,  2,
     1,  2,  1,
   2,  1,  2,  1,
     2,  1,  2    },
   
  {  1,  2,  1,
   1,  2,  1,  2,
     2,  1,  2,
   2,  1,  2,  1,
     1,  2,  1,
   1,  2,  1,  2,
     2,  1,  2,
   2,  1,  2,  1,
     1,  2,  1,
   1,  2,  1,  2,
     2,  1,  2    },
  
  {  1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1    },
     
};

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  
  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, TOTAL_LEDS);
  FastLED.setBrightness( bright );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setCoordMap(MAX_YCOORD, coords);  // x,y grid of cones
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    led[i].push_frame();
    
    shows[i].setShowDuration(show_duration);
    shows[i].setShowSpeed(show_speed);
    shows[i].setDelayTime(DELAY_TIME);
    shows[i].setFadeAmount(fade_amount);
    shows[i].setColorSpeedMinMax();
    shows[i].pickCycleDuration();
    shows[i].fillBlack();
    shows[i] = Shows(&led[i], i);  // Show library - reinitialized for led mappings
  }
  
  shows[CHANNEL_B].setSmallCycleHalfway();  // Start Channel B offset at halfway through show

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  if (ONLY_RED) {
    hue_center = 0;
    hue_width = 124;
  }

  userScheduler.addTask(taskUpdateLeds);
  taskUpdateLeds.enable();  
}

//
// loop
//
void loop() {
  userScheduler.execute();  // Essential!
}

//
// update leds - Moved to a task-scheduled event
//
void updateLeds() {
  // Moved to a task-scheduled event

  for (uint8_t i = 0; i < DUAL; i++) {
    
    switch(current_show[i]) {      
    
      case 0:    
        patterns(i);
        break;
      case 1:
        shows[i].randomFlip();
        break;
      case 2:
        shows[i].randomColors();
        break;
      case 3:
        shows[i].twoColor();
        break;
      case 4:
        shows[i].sawTooth();
        break;
      case 5:
        shows[i].morphChain();
        break;
      case 6:
        shows[i].bounce();
        break;
      case 7:
        shows[i].bounceGlowing();
        break;
      case 8:
        shows[i].plinko(1);  // 1 = starting pixel
        break;
      case 9:
        vert_back_forth_dots(i);
        break;
      case 10:
        vert_back_forth_bands(i);
        break;
      case 11:
        vert_back_forth_colors(i);
        break;
      case 12:
        horiz_back_forth_dots(i);
        break;
      case 13:
        horiz_back_forth_bands(i);
        break;
      case 14:
        horiz_back_forth_colors(i);
        break;
      case 15:
        diag_back_forth_dots(i);
        break;
      case 16:
        diag_back_forth_bands(i);
        break;
      case 17:
        diag_back_forth_colors(i);
        break;
      case 18:
        shows[i].lightWave();
        break;
      default:
        shows[i].juggle_fastled();
//        scales(i);
        break;
    }
    shows[i].morphFrame();  // Morph the display and update the LEDs
  }
  
  morph_channels();  // morph together the 2 leds channels and deposit on to Channel_A
  advance_clocks();  // advance the cycle clocks and check for next show
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].checkCycleClock();
    if (shows[i].hasShowFinished()) {
      next_show(i);
    }
  }
}

//
// next_show
//
void next_show(uint8_t i) {
  current_show[i] = is_other_channel_show_zero(i) ? random8(1, NUM_SHOWS) : 0 ;
  current_pattern[i] = random8(NUM_PATTERNS);
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging

  shows[i].resetAllClocks();
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
}

boolean is_other_channel_show_zero(uint8_t c) {
  if (c == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}


//// Start of specialized shows

//
// patterns shows
//
void patterns(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();
  }

  for (uint8_t i=0; i < NUM_LEDS; i++) {
    
    switch (PatternMatrix[current_pattern[c]][i]) {
      case 0: {        // Off (black)
        shows[c].setPixeltoBlack(i);
        break;
      }
      case 1: {        // MAIN_COLOR
        shows[c].setPixeltoHue(i, MAIN_COLOR);
        break;
      }
      case 2: {        // The other color
        shows[c].setPixeltoHue(i, shows[c].getForeColor());
        break;
      }
    }
  }
}

//
// vert back forth dots - vertical dots moving back and forth
//
void vert_back_forth_dots(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(50, 1000);
  }
  
  uint8_t temp_x;
  uint16_t cycle = shows[c].getCycle();
  
  shows[c].dimAllPixels(1 + ((101 - shows[c].getCycleDuration()) / 10));

  if (shows[c].isCycleStart()) {
    for (uint8_t x = 0; x < MAX_XCOORD; x++) {
      for (uint8_t y = 0; y < MAX_YCOORD; y++) {
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        if ((temp_x + cycle) % MAX_XCOORD == 0) {
          shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
        }
      }
    }
  }
}

//
// vert back forth bands - vertical bands moving back and forth
//
void vert_back_forth_bands(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(50, 1000);
  }
  
  uint8_t temp_x, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  shows[c].dimAllPixels(1 + ((101 - shows[c].getCycleDuration()) / 10));

  if (shows[c].isCycleStart()) {
    for (uint8_t x = 0; x < MAX_XCOORD; x++) {
      for (uint8_t y = 0; y < MAX_YCOORD; y++) {
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        temp_x = (temp_x + cycle) % MAX_XCOORD;
        intensity = sin8(map(temp_x, 0, MAX_XCOORD, 0, 255) + (cycle % (255 / MAX_XCOORD)) );
        shows[c].setPixeltoColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
      }
    }
  }
}

//
// vert back forth colors - vertical colors moving back and forth
//
void vert_back_forth_colors(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(50, 1000);
  }
  
  uint8_t temp_x, hue;
  uint16_t cycle = shows[c].getCycle();
  
  shows[c].dimAllPixels(1 + ((101 - shows[c].getCycleDuration()) / 10));

  if (shows[c].isCycleStart()) {
    for (uint8_t x = 0; x < MAX_XCOORD; x++) {
      for (uint8_t y = 0; y < MAX_YCOORD; y++) {
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        temp_x = (temp_x + cycle) % MAX_XCOORD;
        hue = sin8(map(temp_x, 0, MAX_XCOORD, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_XCOORD)) );
        shows[c].setPixeltoHue(led[c].getLedFromCoord(x,y), hue);
      }
    }
  }
}

//
// horiz back forth dots - horizontal dots moving back and forth
//
void horiz_back_forth_dots(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(50, 1000);
  }
  
  uint8_t temp_y;
  uint16_t cycle = shows[c].getCycle();
  
  shows[c].dimAllPixels(1 + ((101 - shows[c].getCycleDuration()) / 10));

  if (shows[c].isCycleStart()) {
    for (uint8_t x = 0; x < MAX_XCOORD; x++) {
      for (uint8_t y = 0; y < MAX_YCOORD; y++) {
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        if ((temp_y + cycle) % MAX_YCOORD == 0) {
          shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
        }
      }
    }
  }
}

//
// horiz back forth bands - horizontal bands moving back and forth
//
void horiz_back_forth_bands(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(50, 1000);
  }
  
  uint8_t temp_y, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  shows[c].dimAllPixels(1 + ((101 - shows[c].getCycleDuration()) / 10));

  if (shows[c].isCycleStart()) {
    for (uint8_t x = 0; x < MAX_XCOORD; x++) {
      for (uint8_t y = 0; y < MAX_YCOORD; y++) {
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        temp_y = (temp_y + cycle) % MAX_YCOORD;
        intensity = sin8(map(temp_y, 0, MAX_YCOORD, 0, 255) + (cycle % (255 / MAX_YCOORD)) );
        shows[c].setPixeltoColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
      }
    }
  }
}

//
// horiz back forth colors - horizontal colors moving back and forth
//
void horiz_back_forth_colors(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(50, 1000);
  }
  
  uint8_t temp_y, hue;
  uint16_t cycle = shows[c].getCycle();
  
  shows[c].dimAllPixels(1 + ((101 - shows[c].getCycleDuration()) / 10));

  if (shows[c].isCycleStart()) {
    for (uint8_t x = 0; x < MAX_XCOORD; x++) {
      for (uint8_t y = 0; y < MAX_YCOORD; y++) {
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        temp_y = (temp_y + cycle) % MAX_YCOORD;
        hue = sin8(map(temp_y, 0, MAX_YCOORD, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_YCOORD)) );
        shows[c].setPixeltoHue(led[c].getLedFromCoord(x,y), hue);
      }
    }
  }
}

//
// diag back forth dots - diagonal dots moving back and forth
//
void diag_back_forth_dots(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(50, 1000);
  }
  
  uint8_t temp_x, temp_y;
  uint16_t cycle = shows[c].getCycle();
  
  shows[c].dimAllPixels(1 + ((101 - shows[c].getCycleDuration()) / 10));

  if (shows[c].isCycleStart()) {
    for (uint8_t x = 0; x < MAX_XCOORD; x++) {
      for (uint8_t y = 0; y < MAX_YCOORD; y++) {
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        if ((temp_x + temp_y + cycle) % MAX_YCOORD == 0) {
          shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
        }
      }
    }
  }
}

//
// diag back forth bands - diagonal bands moving back and forth
//
void diag_back_forth_bands(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(50, 1000);
  }
  
  uint8_t temp_x, temp_y, temp, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  shows[c].dimAllPixels(1 + ((101 - shows[c].getCycleDuration()) / 10));

  if (shows[c].isCycleStart()) {
    for (uint8_t x = 0; x < MAX_XCOORD; x++) {
      for (uint8_t y = 0; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        temp = (temp_x + temp_y + cycle) % MAX_YCOORD;
        intensity = sin8(map(temp, 0, MAX_YCOORD, 0, 255) + (cycle % (255 / MAX_YCOORD)) );
        shows[c].setPixeltoColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
      }
    }
  }
}

//
// diag back forth colors - diagonal colors moving back and forth
//
void diag_back_forth_colors(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(50, 1000);
  }
  
  uint8_t temp_x, temp_y, temp, hue;
  uint16_t cycle = shows[c].getCycle();
  
  shows[c].dimAllPixels(1 + ((101 - shows[c].getCycleDuration()) / 10));

  if (shows[c].isCycleStart()) {
    for (uint8_t x = 0; x < MAX_XCOORD; x++) {
      for (uint8_t y = 0; y < MAX_YCOORD; y++) {
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        temp = (temp_x + temp_y + cycle) % MAX_YCOORD;
        hue = sin8(map(temp, 0, MAX_YCOORD, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_YCOORD)) );
        shows[c].setPixeltoHue(led[c].getLedFromCoord(x,y), hue);
      }
    }
  }
}

//
// scales
//
/*
void scales(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].shuffleLeds();
    led[c].fillHue(MAIN_COLOR);
  
    uint8_t num_scales = (shows[c].getShuffleValue(0) / 3) + 3;
    for (uint8_t i=0; i < num_scales; i++) {
      shows[c].setPixeltoHue(shows[c].getShuffleValue(i), shows[c].getForeColor());
    }
  }
}
*/

////  Utility Functions

//
// lookup_petal - convert petals into LEDs - this is where mapping happens
//
uint8_t lookup_petal(uint8_t i) {
  return pgm_read_byte_near(ConeLookUp + i);
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  boolean update_leds = false;
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // New! Changed Interp to Next. No more Interp frames
    CHSV color_b = led[CHANNEL_B].getCurrFrameColor(i);   
    CHSV color_a = led[CHANNEL_A].getCurrFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
    
    // smoothing backstop. smooth constants should be large.
    color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);

    if (led_buffer[i].h == color.h && led_buffer[i].s == color.s && led_buffer[i].v == color.v) {
      continue;
    } else {
      leds[lookup_petal(i)] = color;
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
      update_leds = true;
    }
  }

  if (update_leds) { 
    turn_off_spacers();
    FastLED.show();
  }
}

//
// turn_off_spacers
//
void turn_off_spacers() {
  for (uint8_t i = 0; i < NUM_SPACERS; i++) {
    leds[SpacerPixels[i]] = BLACK;
  }
}

//// End DUAL SHOW LOGIC

//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}
