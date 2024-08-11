#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
//
//  Small White Wings: paired Linear Lights
//
//  2/14/24
//
//  Modern Software
//
#define ACTUAL_LEDS 84
#define NUM_LEDS 48

uint8_t bright = 128;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME 20 // in milliseconds

#define DATA_PIN 0
#define CLOCK_PIN 2

// Smoothing constants - lower is slower smoothing
#define SMOOTHING_SHOWS_HUE    4   // Fastest full rainbow = DELAY_TIME * (255 / this value) = 150 ms
#define SMOOTHING_SHOWS_VALUE  30   // Fastest turn off/on = DELAY_TIME * (255 / this value) = 150 ms

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[NUM_LEDS];  // The Leds themselves

#define ONLY_RED true
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
#define BLACK  CHSV(0, 0, 0)

// Shows
#define START_SHOW_CHANNEL_A  0  // Channels A starting show
#define START_SHOW_CHANNEL_B  1  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 12

// Clocks and time

uint8_t show_duration = 20;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 128;  // 0 = no fading, to 255 = always be fading

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  
  Serial.begin(115200);
  Serial.println("Start");

  FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, BGR>(leds, ACTUAL_LEDS);  // Only 1 leds object
  FastLED.setBrightness( bright );

  // Set up the various mappings (1D lists in PROGMEM)
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
          shows[i].stripes();
          break;
        case 7:
          shows[i].sinelon_fastled();
          break;
        case 8:
          shows[i].juggle_fastled();
          break;
        default:
          shows[i].confetti();
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
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  shows[i].resetAllClocks();
//  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
}

boolean is_other_channel_show_zero(uint8_t channel) {
  if (channel == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

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
      hsv2rgb_rainbow(color, rgb_color);  // Need to understand smoothing
      
      leds[i] = rgb_color;
      leds[ACTUAL_LEDS - i - 1] = rgb_color;

      // Need to understand smoothing
//      leds[i] = led[CHANNEL_A].smooth_rgb_color(leds[i], rgb_color, 4);
//      leds[ACTUAL_LEDS - i -1] = led[CHANNEL_A].smooth_rgb_color(leds[ACTUAL_LEDS - i -1], rgb_color, 4);
//      leds[i] = color;   // put HSV color on to LEDs
      
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
      update_leds = true;
    }
  }

  if (update_leds) { 
    FastLED.show();
  }
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
