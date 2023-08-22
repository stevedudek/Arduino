#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Gold Dinosaur with 8 spikes
//
//  Modern Software
//
//  6/7/23
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
// #define CLOCK_PIN 2  //  13 // 8 // clock = 8 (Teensy) or 2 (Feather)

#define NUM_LEDS 52
#define ACTUAL_LEDS 66

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
#define NUM_SHOWS 10
#define START_SHOW_CHANNEL_A  9  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  0
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

// wait times
uint8_t show_duration = 20;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 196;  // 0 = no fading, to 255 = always be fading

uint8_t show_symmetry[] = { 0, 0 };

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

// Lookup tables

const uint8_t spine_lookup_table_1[] PROGMEM = {
   0, 0, 0, 0,
   1, 1, 1, 1, 1, 1,
   2, 2, 2, 2, 2, 2, 2, 2,
   3, 3, 3, 3, 3, 3, 3, 3,
   4, 4, 4, 4, 4, 4, 4, 4,
   5, 5, 5, 5, 5, 5, 5, 5,
   6, 6, 6, 6, 6, 6,
   7, 7, 7, 7
};

const uint8_t spine_lookup_table_2[] PROGMEM = {
   0, 2, 4, 6,
   0, 2, 3, 4, 6, 7,
   0, 1, 2, 3, 4, 5, 6, 7,
   0, 1, 2, 3, 4, 5, 6, 7,
   0, 1, 2, 3, 4, 5, 6, 7,
   0, 1, 2, 3, 4, 5, 6, 7,
   0, 2, 3, 4, 6, 7,
   0, 2, 4, 6
};

const uint8_t led_lookup_table[] PROGMEM = {
   0, 1, 2, 3, 6, 7, 8, 9,10,11,14,15,16,17,18,19,20,21,
  24,25,26,27,28,29,30,31,34,35,36,37,38,39,40,41,44,45,
  46,47,48,49,50,51,54,55,56,57,58,59,62,63,64,65
};

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  // pinMode(CLOCK_PIN, OUTPUT);

  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);
  // FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);  // only use of ACTUAL_LEDS
  FastLED.setBrightness( bright );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    set_show_symmetry(i);
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
        break;
      case 1:
        shows[i].morphChain();  // works — lots of flicker
        break;
      case 2:
        shows[i].randomFlip();  // works
        break;
      case 3:
        shows[i].twoColor();
        break;
      case 4:
        shows[i].sawTooth();  // works
        break;
      case 5:
        shows[i].sinelon_fastled();  // works
        break;
      case 6:
        shows[i].bpm_fastled();  // works — fast!
        break;
      case 7:
        shows[i].juggle_fastled();  // works
        break;
      case 8:
        shows[i].confetti();  // more confetti - longer dim
        break;
      default:
        shows[i].lightRunUp();  // works
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }
  morph_channels();  // morph together the 2 leds channels
  FastLED.show();  // Update the display
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
  show_symmetry[i] = random(3);
  set_show_symmetry(i);
//  log_status(i);  // For debugging
}

//
// pick_next_show
//
void pick_next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
//  log_status(i);  // For debugging
}

//
// set_led_number
//
void set_show_symmetry(uint8_t i) {
  uint8_t symmetry_led_number[] = { 54, 8, 8 };
  led[i].fillBlack();  // clear leds before symmetry change
  shows[i].resetNumLeds(symmetry_led_number[show_symmetry[i]]);  // resets the virtual number of LEDs
}

boolean is_other_channel_show_zero(uint8_t c) {
  if (c == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // New! Changed Interp to Next. No more Interp frames
    CHSV color_b = led[CHANNEL_B].getCurrFrameColor(get_pixel_from_led(i, CHANNEL_B));   
    CHSV color_a = led[CHANNEL_A].getCurrFrameColor(get_pixel_from_led(i, CHANNEL_A));
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
    
    // smoothing backstop. smooth constants should be large.
    color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);
    
    led_buffer[i] = color;  // push new color into led_buffer (both HSV)
    leds[pgm_read_byte_near(led_lookup_table + i)] = color;
  }
}

//
// unpack_leds - based on symmetry and led, find the assigned pixel
//
uint8_t get_pixel_from_led(uint8_t i, uint8_t c) {
  switch (show_symmetry[c]) {
      case 0:
        return i; // No mapping changes for symmetry 0
      case 1:
        return pgm_read_byte_near(spine_lookup_table_1 + i);
      default:
        return pgm_read_byte_near(spine_lookup_table_2 + i);
  }
}

//// End DUAL SHOW LOGIC

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
