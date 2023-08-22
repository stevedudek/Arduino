#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

#define FASTLED_ESP32_FLASH_LOCK 1

//
//  Cuddlefish - 42 cones on a hat
//
//  8/14/23
//
//  Modern Software
//
uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME 20 // in milliseconds

#define DATA_PIN 0  //  9
#define CLOCK_PIN 2  //  8

#define NUM_LEDS 42

// Smoothing constants - lower is slower smoothing
#define SMOOTHING_SHOWS_HUE    4   // Fastest full rainbow = DELAY_TIME * (255 / this value) = 150 ms
#define SMOOTHING_SHOWS_VALUE  30   // Fastest turn off/on = DELAY_TIME * (255 / this value) = 150 ms

#define XX  255

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define MAX_XCOORD  7  // 7 tentacles
#define MAX_YCOORD  6  // each 6 cones

#define MAX_COLOR 256   // Colors are 0-255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // Hook for FastLED library
CHSV led_buffer[NUM_LEDS];  // For smoothing

#define ONLY_RED true
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

// Shows
#define START_SHOW_CHANNEL_A 10  // Channels A starting show
#define START_SHOW_CHANNEL_B  0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 20

// Clocks and time
uint8_t show_duration = 60;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 128;  // 0 = no fading, to 255 = always be fading

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

// Lookup tables

uint8_t ConeLookUp[] = {
 17,11, 5,23,29,35,41,
 16,10, 4,22,28,34,40,
 15, 9, 3,21,27,33,39,
 14, 8, 2,20,26,32,38,
 13, 7, 1,19,25,31,37,
 12, 6, 0,18,24,30,36,
};

const uint8_t ConeGrid[] PROGMEM = {
 17,11, 5,23,29,35,41,
 16,10, 4,22,28,34,40,
 15, 9, 3,21,27,33,39,
 14, 8, 2,20,26,32,38,
 13, 7, 1,19,25,31,37,
 12, 6, 0,18,24,30,36,
};

const uint8_t neighbors[] PROGMEM = {
  7,1,19,18,6,XX, // 0
  8,2,0,9,0,7,
  9,3,21,20,1,8,
  10,4,22,21,2,9,
  11,5,23,22,3,10, // 4
  XX,XX,XX,23,4,11,
  12,7,0,XX,XX,XX,
  13,8,1,0,6,12,
  14,9,2,1,7,13, // 8
  15,10,3,2,8,14,
  16,11,4,3,9,15,
  17,XX,5,4,10,16,
  XX,13,7,6,XX,XX, // 12
  XX,14,8,7,12,XX,
  XX,15,0,8,13,XX,
  XX,16,10,9,14,XX,
  XX,17,11,10,15,XX, // 16
  XX,XX,XX,11,16,XX,
  0,19,24,XX,XX,XX,
  1,20,25,24,18,0,
  2,21,26,25,19,1, // 20
  3,22,27,26,20,2,
  4,23,28,27,21,3,
  5,XX,29,28,22,4,
  19,25,31,30,XX,18, // 24
  20,26,32,31,24,19,
  21,27,33,32,25,20,
  22,28,34,33,26,21,
  23,29,35,34,27,22, // 28
  XX,XX,XX,35,28,23,
  24,31,36,XX,XX,XX,
  25,32,37,36,30,24,
  26,33,38,37,31,25, // 32
  27,34,39,38,32,26,
  28,35,40,39,33,27,
  34,30,29,36,XX,38,
  31,37,XX,XX,XX,30, // 36
  32,38,XX,XX,36,31,
  33,39,XX,XX,37,32,
  34,40,XX,XX,38,33,
  35,41,XX,XX,39,34, // 40
  XX,XX,XX,XX,40,35
};

const uint8_t PatternLookUp[45] = { 
  0,   2,   4,  6,
     1,   3,   5,
  7,   9,  11,  13,
     8,  10,  12,
  14,  16,  18,  20,
     15,  17,  19,
  21,  23,  25,  27,
     22,  24,  26,
  28,  30,  32,  34,
     29,  31,  33,
  35,  37,  39,  36,
     36,  38,  40,
};

const uint8_t Stripe_Pattern[45] PROGMEM = {
  0,  0,  1,  1,
    0,  1,  1,
  0,  1,  1,  0,
    1,  1,  0,
  1,  1,  0,  0,
    1,  0,  0,
  1,  0,  0,  1,
    0,  0,  1,
  0,  0,  1,  1,
    0,  1,  1,
  0,  1,  1,  0,
    1,  1,  0,
};

const uint8_t Section_Pattern[45] PROGMEM = {
  0,  0,  0,  0,
    0,  1,  0,
  0,  1,  1,  0,
    1,  2,  1,
  1,  2,  2,  1,
    1,  2,  1,
  1,  2,  2,  1,
    1,  2,  1,
  0,  1,  1,  1,
    0,  1,  0,
  0,  0,  0,  0,
    0,  0,  0,
};

const uint8_t Explode_Pattern[45] PROGMEM = {
  5,  5,  5,  5,
    4,  4,  4,
  4,  3,  3,  4,
    3,  2,  1,
  3,  1,  1,  3,
    2,  0,  2,
  3,  1,  1,  3,
    3,  2,  3,
  4,  3,  3,  4,
    4,  4,  4,
  5,  5,  5,  5,
    5,  5,  5,
};

const uint8_t Alternate_Pattern[45] PROGMEM = {
  0,  0,  0,  0,
    1,  1,  1,
  0,  0,  0,  0,
    1,  1,  1,
  0,  0,  0,  0,
    1,  1,  1,
  0,  0,  0,  0,
    1,  1,  1,
  0,  0,  0,  0,
    1,  1,  1,
  0,  0,  0,  0,
    1,  1,  1,
};

const uint8_t SideSide_Pattern[45] PROGMEM = {
  0,  2,  4,  6,
    1,  3,  5,
  0,  2,  4,  6,
    1,  3,  5,
  0,  2,  4,  6,
    1,  3,  5,
  0,  2,  4,  6,
    1,  3,  5,
  0,  2,  4,  6,
    1,  3,  5,
  0,  2,  4,  6,
    1,  3,  5,
};

const uint8_t Diag_Pattern[45] PROGMEM = {
  0,  1,  2,  3,
    1,  2,  3,
  1,  2,  3,  4,
    2,  3,  4,
  2,  3,  4,  5,
    3,  4,  5,
  3,  4,  5,  6,
    4,  5,  6,
  4,  5,  6,  7,
    5,  6,  7,
  5,  6,  7,  8,
    6,  7,  8,
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( bright );
  
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setLedMap(ConeLookUp);  // turned off mapping - it's handled explicitly here in update_leds()
    led[i].setCoordMap(MAX_YCOORD, ConeGrid);  // x,y grid of cones
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    led[i].setAsHexagon();
    shows[i] = Shows(&led[i], i);  // Show library - reinitialized for led mappings
    shows[i].setShowDuration(show_duration);
    shows[i].setShowSpeed(show_speed);
    shows[i].setDelayTime(DELAY_TIME);
    shows[i].setFadeAmount(fade_amount);
    shows[i].setColorSpeedMinMax();
    shows[i].pickCycleDuration();
    shows[i].fillForeBlack();
    led[i].push_frame();
  }

  shows[CHANNEL_B].setStartShowTime(millis() - shows[CHANNEL_A].getElapsedShowTime() - (show_duration * 1000 / 2));

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  if (ONLY_RED) {  // (ArduinoBlue)
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
      
    switch (current_show[i]) {
  
      case 0:
        shows[i].morphChain();
        break;
      case 1:
        shows[i].randomColors();
        break;
      case 2:
        shows[i].twoColor();
        break;
      case 3:
        shows[i].lightRunUp();
        break;
      case 4:
        stripe(i);
        break;
      case 5:
        alternate(i);
        break;
      case 6:
        diagcolor(i);
        break;
      case 7:
        sidesidecolor(i);
        break;
      case 8:
        explodecolor(i);
        break;
      case 9:
        diagbright(i);
        break;
      case 10:
        sidesidebright(i);
        break;
      case 11:
        explodebright(i);
        break;
      case 12:
        shows[i].plinko(15);
        break;
      case 13:
        shows[i].bounce();
        break;
      case 14:
        shows[i].bounceGlowing();
        break;
      case 15:
        sectioncolor(i);
        break;
      case 16:
        shows[i].confetti();
        break;
      case 17: 
        shows[i].sinelon_fastled();
        break;
      case 18:
        shows[i].juggle_fastled();
        break;
      default:
        shows[i].lightWave();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }
  
  morph_channels();
  advance_clocks();
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].checkCycleClock();
    if (change_show(i)) {
      next_show(i);
      pick_next_show(i);
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
// next_show
//
void next_show(uint8_t i) {
  shows[i].resetAllClocks();
//  log_status(i);  // For debugging
}

//
// pick_next_show
//
void pick_next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
}

//// Start specialized shows

//
// calcIntensity - use sine wave + cycle + variable to calculate intensity
//
// x ranges from 0 to max_x-1
// output is 0 to max_y
// speed spd (1, 2, 3, etc.) determines the rate of color change
//
uint8_t calcIntensity(uint8_t c, uint8_t x, uint8_t max_x, uint8_t max_y, uint8_t spd) {
  uint8_t intense = map8(sin(map(x, 0, max_x, 0, 255) + (shows[c].getCycle() * spd)), 0, max_y);
  return intense;
}

//
// stripe
//
void stripe(uint8_t c) {
  for (int i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Stripe_Pattern + i) == 0) {
      shows[c].setPixeltoForeColor(PatternLookUp[i]);
    } else {
      shows[c].setPixeltoBackColor(PatternLookUp[i]);
    }
  }
}

//
// alternate
//
void alternate(uint8_t c) {
  for (int i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Alternate_Pattern + i)) {
      shows[c].setPixeltoForeColor(PatternLookUp[i]);
    } else {
      shows[c].setPixeltoBackColor(PatternLookUp[i]);
    }
  }
}

void diagcolor(uint8_t c) {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(i, pgm_read_byte_near(Diag_Pattern + i), 9, 200, shows[c].getForeColorSpeed());
    shows[c].setPixeltoHue(PatternLookUp[i], IncColor(shows[c].getForeColor(), change));
  }
}

void sectioncolor(uint8_t c) {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(i, pgm_read_byte_near(Section_Pattern + i), 3, 128, shows[c].getForeColorSpeed());
    shows[c].setPixeltoHue(PatternLookUp[i], IncColor(shows[c].getForeColor(), change));
  }
}

void sidesidecolor(uint8_t c) {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(i, pgm_read_byte_near(SideSide_Pattern + i), 7, 256, shows[c].getForeColorSpeed());
    shows[c].setPixeltoHue(PatternLookUp[i], IncColor(shows[c].getForeColor(), change));
  }
}

void explodecolor(uint8_t c) {
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(i, pgm_read_byte_near(Explode_Pattern + i), 6, 256, shows[c].getForeColorSpeed());
    shows[c].setPixeltoHue(PatternLookUp[i], IncColor(shows[c].getForeColor(), change));
  }
}

void diagbright(uint8_t c) {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(i, pgm_read_byte_near(Diag_Pattern + i), 9, 255, shows[c].getForeColorSpeed() * 3);
    shows[c].setPixeltoColor(PatternLookUp[i], led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
}

void sidesidebright(uint8_t c) {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(i, pgm_read_byte_near(SideSide_Pattern + i), 7, 255, shows[c].getForeColorSpeed() * 2);
    shows[c].setPixeltoColor(PatternLookUp[i], led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
}

void explodebright(uint8_t c) {
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(i, pgm_read_byte_near(Explode_Pattern + i), 6, 255, shows[c].getForeColorSpeed() * 2);
    shows[c].setPixeltoColor(PatternLookUp[i], led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
}

//// End specialized shows

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  boolean update_leds = false;
  CRGB rgb_color;
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getCurrFrameColor(i);   // New! Changed Interp to Next. No more Interp frames.
    CHSV color_a = led[CHANNEL_A].getCurrFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
    
    // smoothing backstop. smooth constants should be large.
    color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);
    
    if (led_buffer[i].h == color.h && led_buffer[i].s == color.s && led_buffer[i].v == color.v) {
      continue;
    } else {
      hsv2rgb_rainbow(color, rgb_color);
      leds[i] = led[CHANNEL_A].smooth_rgb_color(leds[i], rgb_color, 4);
//      leds[i] = color;   // put HSV color on to LEDs
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
      update_leds = true;
    }
  }

  if (update_leds) { 
    FastLED.show();
  }
}

//// End DUAL SHOW LOGIC

//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}


//
// log status
//
void log_status(uint8_t i) {
  Serial.print(F("Channel: "));
  Serial.print(i);
  Serial.print(F(", Show: "));
  Serial.print(current_show[i]);
}
