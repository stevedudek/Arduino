#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Turtle! - 29 Lights in a hexagonal grid
//
//  Modern Software - a lot to test
//
//  6/15/23
//

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME  15  // in milliseconds

// Smoothing constants - lower is slower smoothing
// At 15 ms cycles: 40 = 96 ms, 30 = 127 ms, 20 = 191 ms, 10 = 382 ms, 5 = 765 ms, 2 = 1.9 s, 1 = 3.8 s
#define SMOOTHING_SHOWS_HUE    10   // Fastest full rainbow = DELAY_TIME * (255 / this value)
#define SMOOTHING_SHOWS_VALUE  10   // Fastest turn off/on = DELAY_TIME * (255 / this value)

#define DATA_PIN 0  //  11 // 7 // data = 7 (Teensy) or 0 (Feather)
#define CLOCK_PIN 2  //  13 // 8 // clock = 8 (Teensy) or 2 (Feather)

#define NUM_LEDS 29
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number

#define ACTUAL_LEDS 37  // There are 8 dummy spacer LEDs

#define XX  255

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // Required for smoothing
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 16
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 0 };

// wait times
uint8_t show_duration = 200;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 0;  // 0 = no fading, to 255 = always be fading

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

// Lookup tables

const uint8_t rewire[] PROGMEM = {
  30, 26, 27,  0,  1,
   3,  2, 25, 24, 32,
  33, 22, 23,  4,  5,
   7,  6, 21, 20, 35,
  36, 18, 19,  8,  9,
  10, 17, 16, 15
};

const uint8_t PatternMatrix[] PROGMEM = {
    1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,
    1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,1,
    1,1,1,1,1,1,2,2,2,1,1,2,1,2,1,1,2,2,2,1,1,2,1,2,1,1,2,1,1,
    1,1,1,1,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,1,1,
    1,1,1,1,1,2,2,2,2,2,2,1,1,1,2,2,1,1,1,2,1,2,1,2,1,1,2,1,1,
    2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,2,
    1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,1,1,1,2,1,2,1,2,1,1,2,1,1,
    1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,2,1,2,1 
};

const uint8_t neighbors[] PROGMEM = {
  XX,1,8,9,XX,XX,
  XX,2,7,8,0,XX,
  XX,XX,3,7,1,XX,
  XX,XX,4,6,7,2,
  XX,XX,XX,5,6,3, // 4
  4,XX,XX,14,13,6,
  3,4,5,13,12,7,
  2,3,6,12,8,1,
  1,7,12,11,9,0,  // 8
  0,8,11,10,XX,XX,
  9,11,18,19,XX,XX,
  8,12,17,18,10,9,
  7,6,13,17,11,8, // 12
  6,5,14,16,17,12,
  5,XX,XX,15,16,13,
  14,XX,XX,24,23,16,
  13,14,15,23,22,17, // 16
  12,13,16,22,18,11,
  11,17,22,21,19,10,
  10,18,21,20,XX,XX,
  19,21,27,XX,XX,XX,
  18,22,26,27,20,19,
  17,16,23,26,21,18, // 22
  16,15,24,25,26,22,
  15,XX,XX,XX,25,23,
  23,24,XX,XX,28,26,
  22,23,25,28,27,21,
  21,26,28,XX,XX,20,
  26,25,XX,XX,XX,27 // 28
};

#define NUM_PATTERNS 8   // Total number of patterns 

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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);  // only use of ACTUAL_LEDS
  FastLED.setBrightness( bright );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    shows[i] = Shows(&led[i], i);  // Show library - reinitialized for led mappings
    shows[i].setAsHexagon();
    shows[i].setShowDuration(show_duration);
    shows[i].setShowSpeed(show_speed);
    shows[i].setDelayTime(DELAY_TIME);
    shows[i].setFadeAmount(fade_amount);
    shows[i].setColorSpeedMinMax();
    shows[i].pickCycleDuration();
    shows[i].fillForeBlack();
    led[i].push_frame();
  }
  
  shows[CHANNEL_B].setSmallCycleHalfway();  // Start Channel B offset at halfway through show

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
        shows[i].allOn();
//        patterns(i);  // works
        break;
      case 1:
        warp1(i);  // goes all one color?
        break;
      case 2:
        warp2(i);  // works
        break;
      case 3:
        rainbow_show(i);  // works
        break;
      case 4:
        shows[i].morphChain();  // works — lots of flicker
        break;
      case 5:
        shows[i].bounce();  // fast! dim hue?
        break;
      case 6:
        shows[i].bounceGlowing();  // does not work
        break;
      case 7:
        shows[i].plinko(2);  // works
        break;
      case 8:
        shows[i].randomFlip();  // works
        break;
      case 9:
        shows[i].sawTooth();  // works
        break;
      case 10:
        shows[i].sinelon_fastled();  // works
        break;
      case 11:
        shows[i].bpm_fastled();  // works — fast!
        break;
      case 12:
        shows[i].juggle_fastled();  // works
        break;
      case 13:
        shows[i].confetti();  // more confetti - longer dim
        break;
      case 14:
        shows[i].allOn();
        break;
      default:
        shows[i].lightRunUp();  // works
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
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
      pick_next_show(i);
    }
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
  current_show[i] = (current_show[i] != 0 && !is_other_channel_show_zero(i)) ? 0 : random8(1, NUM_SHOWS) ;
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
//  log_status(i);  // For debugging
}

boolean is_other_channel_show_zero(uint8_t c) {
  if (c == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

//
// patterns shows
//
void patterns(uint8_t c) {

  if (shows[c].isShowStart()) {
    current_pattern[c] = random8(NUM_PATTERNS);
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  uint8_t pattern_number = current_pattern[c] % NUM_PATTERNS;
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t pattern = pgm_read_byte_near(PatternMatrix + (pattern_number * NUM_LEDS) + i);
 
    switch (pattern) {
      case 0: {        // Off
        shows[c].setPixeltoForeBlack(i);
        break;
      }
      case 1: {        // always green
        shows[c].setPixeltoColor(i, CHSV(96, 255, 255));
        break;
      }
      case 2: {        // the other color
        shows[c].setPixeltoForeColor(i);
        break;
      }
    }
  }
}

//
// draw_ring
//
void draw_ring(uint8_t i, CHSV color, uint8_t c) {
  uint8_t rings[] = {
    17, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX,
    12, 13, 16, 22, 18, 11, XX, XX, XX, XX, XX, XX,
     7,  6,  5, 14, 15, 23, 26, 21, 19, 10,  9,  8,
     0,  1,  2,  3,  4, 24, 25, 28, 27, 20, XX, XX 
  };

  for (uint8_t j = 0; j < 12; j++) {
    uint8_t r = rings[(i * 12) + j];
    if (r != XX) {
      shows[c].setPixeltoColor(r, color);
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
void tunnelvision(CHSV color, CHSV background, uint8_t c) {
  shows[c].dimAllPixelsFrames(3);
  if (background != CHSV(0,0,0)) {
    shows[c].fill(background);
  }
  
  uint8_t i = shows[c].getCycle() % 5;
  if (i < 4) { 
    draw_ring(i, color, c); 
  }
}

//
// warp1 - colors on a black field
// 
void warp1(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(250, 800);
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  switch ((shows[c].getCycle() / 5) % 6) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0, 255, 0)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,0,255)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,255,255)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(0,0,0)), c);  
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,0,0)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(255,0,255)), rgb_to_hsv(CRGB(0,0,0)), c);  
      break;
  }
}

//
// warp2 - colors on a green field
//
void warp2(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(250, 800);
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  switch ((shows[c].getCycle() / 5) % 8) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0,255,100)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,200,100)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,150,100)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(0,100,100)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 5:
      tunnelvision(rgb_to_hsv(CRGB(200,200,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 6:
      tunnelvision(rgb_to_hsv(CRGB(150,150,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(100,100,0)), rgb_to_hsv(CRGB(0,40,0)), c);  
      break;
  }
}

//
// rainbow show - distribute a rainbow wheel equally distributed along the chain
//
void rainbow_show(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
    shows[c].pickRandomCycleDuration(20, 100);
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i = 0; i < HALF_LEDS; i++) {
    uint8_t hue = ((shows[c].getForeColorSpeed() * i) + shows[c].getBackColorSpeed() + shows[c].getCycle()) % MAX_COLOR;
    shows[c].setPixeltoHue(i, hue);
    shows[c].setPixeltoHue(NUM_LEDS - i - 1, hue);
  }  
}

//// End specialized shows

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  boolean update_leds = false;
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
      leds[pgm_read_byte_near(rewire + i)] = color;   // put HSV color on to LEDs
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
      update_leds = true;
    }
  }
  
  if (update_leds) { 
    turn_off_spacer_leds();
    FastLED.show();
  }
}

//// End DUAL SHOW LOGIC

void turn_off_spacer_leds() {
  uint8_t spacer_leds[] = { 11, 12, 13, 14, 28, 29, 31, 34 };
  
  for (uint8_t i = 0; i < 8; i++) {
    leds[spacer_leds[i]] = CRGB(0, 0, 0);
  }
}

//
// RGB to HSV - save having to translate RGB colors by hand
//
CHSV rgb_to_hsv( CRGB color) {
  return led[0].rgb_to_hsv(color);  // static method
}

//
// log status
//
void log_status(uint8_t i) {
  Serial.print(F("Channel: "));
  Serial.print(i);
  Serial.print(F(", Show: "));
  Serial.print(current_show[i]);
  Serial.print(F(", Cycle Duration: "));
  Serial.print(shows[i].getCycleDuration());
  Serial.print(F(", Cycle: "));
  Serial.print(shows[i].getCycle());
  Serial.print(F(", Sm Cycle: "));
  Serial.println(shows[i].getSmallCycle());
}

void log_shows() {
  Serial.print(current_show[0]);
  Serial.print(" ");
  Serial.print(shows[CHANNEL_A].get_intensity());
  Serial.print(" ");
  Serial.println(current_show[1]);
}

void log_light(uint8_t i) {
  Serial.print(i);
  Serial.print(F(": "));
  Serial.print(leds[i].r);
  Serial.print(F(" "));
  Serial.print(leds[i].g);
  Serial.print(F(" "));
  Serial.println(leds[i].b);
}
