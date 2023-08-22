#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

#define FASTLED_ESP32_FLASH_LOCK 1

//
//  Blue Dinosaur with 9 stegosauran paddles
//
//  Modern Software
//
//  8/13/23
//
#define NUM_LEDS 99
#define ACTUAL_LEDS 123

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 255;  // (0-255)

#define DELAY_TIME 15 // in milliseconds

// Smoothing constants - lower is slower smoothing
#define SMOOTHING_SHOWS_HUE    4   // Fastest full rainbow = DELAY_TIME * (255 / this value) = 150 ms
#define SMOOTHING_SHOWS_VALUE  30   // Fastest turn off/on = DELAY_TIME * (255 / this value) = 150 ms

#define DATA_PIN 0  //  11 // 7 // data = 7 (Teensy) or 0 (Feather)

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CRGB leds[ACTUAL_LEDS];  // Hook for FastLED library
CHSV led_buffer[NUM_LEDS];  // For smoothing

CHSV morph_buffer[DUAL][NUM_LEDS];

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
#define BLACK  CHSV(0, 0, 0)

// Shows
#define START_SHOW_CHANNEL_A 10  // Channels A starting show
#define START_SHOW_CHANNEL_B  0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 11

// Clocks and time
uint8_t show_duration = 60;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 128;  // 0 = no fading, to 255 = always be fading

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

boolean simplespikes[] = { false, false };

// Paddle lookup tables
const uint8_t paddle_size_lookup[] = { 6, 10, 12, 14, 16, 14, 12, 10, 5 };
const uint8_t paddle_first_led_lookup[] = { 0, 6, 16, 28, 42, 58, 72, 84, 94, 99 };

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  
  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, ACTUAL_LEDS);;  // Only 1 leds object
  FastLED.setBrightness( bright );

  for (uint8_t i = 0; i < DUAL; i++) {
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
        shows[i].allOn();
        break;
      case 1:
        shows[i].morphChain();
        break;
      case 2:
        shows[i].twoColor();
        break;
      case 3:
        shows[i].lightWave();
        break;
      case 4:
        shows[i].sawTooth();
        break;
      case 5:
        shows[i].lightRunUp();
        break;
      case 6:
        shows[i].sinelon_fastled();  // Great Show: make more of this
        break;
      case 7:
        shows[i].juggle_fastled();  // Great Show: make more of this
        break;
      case 8:
        shows[i].confetti();
        break;
      case 9:
        shows[i].randomTwoColorBlack();
        break;
      default:
        shows[i].bpm_fastled();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  unpack_leds();
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

  simplespikes[i] = (random8(2) == 0) ? true : false ;
  set_led_number(i);
  
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
}

//
// set_led_number
//
void set_led_number(uint8_t i) {
  led[i].fillBlack();  // clear leds before symmetry change
  uint8_t numSymLeds = (simplespikes[i]) ? 8 : 54; // This may not work
  shows[i].resetNumLeds(numSymLeds);  // resets the virtual number of LEDs
  led[i].fillBlack();  // and clear leds after symmetry change
  led[i].push_frame();
}

//
// unpack_leds - if simplespines, need to unpack into component leds
//
void unpack_leds() {
  for (uint8_t channel = 0; channel < DUAL; channel++) {
    if (simplespikes[channel]) {
      for (uint8_t spike = 0; spike < 9; spike++) {
        // Light all the LEDs on the numbered spike
        uint8_t first_led = paddle_first_led_lookup[spike];
        for (uint8_t i = 0; i < paddle_size_lookup[spike]; i++) {
          morph_buffer[channel][first_led + i] = led[channel].getCurrFrameColor(spike);
        }
      }
    } else {
      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        morph_buffer[channel][i] = led[channel].getCurrFrameColor(i);
      }
    }
  }
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  boolean need_update_leds = false;
  CRGB rgb_color;
  CHSV color_a, color_b; 
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B

  for (int i = 0; i < NUM_LEDS; i++) {
    if (simplespikes[CHANNEL_A]) {
      color_a = morph_buffer[CHANNEL_A][i];
    } else {
      color_a = led[CHANNEL_A].getCurrFrameColor(i);
    }
    if (simplespikes[CHANNEL_B]) {
      color_b = morph_buffer[CHANNEL_B][i];
    } else {
      color_b = led[CHANNEL_B].getCurrFrameColor(i);
    }
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);

    if (led_buffer[i].h == color.h && led_buffer[i].s == color.s && led_buffer[i].v == color.v) {
      continue;
    } else {
      hsv2rgb_rainbow(color, rgb_color);
      leds[add_spacer_leds(i)] = led[CHANNEL_A].smooth_rgb_color(leds[add_spacer_leds(i)], rgb_color, 4);
//      leds[i] = color;   // put HSV color on to LEDs
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
      need_update_leds = true;
    }
  }
    
  if (need_update_leds) {
    FastLED.show();
  }
}

uint8_t add_spacer_leds(uint8_t i) {
  uint8_t num_spacers = 0;
  for (uint8_t spike = 1; i >= paddle_first_led_lookup[spike]; spike++) {
    num_spacers+=3;
  }
  return i + num_spacers;
}
