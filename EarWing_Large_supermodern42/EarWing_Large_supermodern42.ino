#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

#define FASTLED_ESP32_FLASH_LOCK 1

//
//  Ear Wing - Symmetric somehow
//
//  Modern Software - a lot to test
//
//  8/13/23
//

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME  15  // in milliseconds

// Smoothing constants - lower is slower smoothing
#define SMOOTHING_SHOWS_HUE    4   // Fastest full rainbow = DELAY_TIME * (255 / this value) = 150 ms
#define SMOOTHING_SHOWS_VALUE  30   // Fastest turn off/on = DELAY_TIME * (255 / this value) = 150 ms

#define DATA_PIN 0  //  11 // 7 // data = 7 (Teensy) or 0 (Feather)
#define CLOCK_PIN 2  //  13 // 8 // clock = 8 (Teensy) or 2 (Feather)

#define NUM_LEDS 84 // 25  // 27 is too many
#define ACTUAL_LEDS 42 //  52

#define XX  255

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // Required for smoothing
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

#define ONLY_RED true
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
#define BLACK  CHSV(0, 0, 0)

// Shows
#define NUM_SHOWS 11
#define START_SHOW_CHANNEL_A  4  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  0
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

// wait times
uint8_t show_duration = 60;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 128;  // 0 = no fading, to 255 = always be fading

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);

  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");

//  FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);  // Only 1 leds object
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);
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
        shows[i].twoColor();
        break;
      case 2:
        shows[i].lightRunUp();
        break;
      case 3:
        shows[i].morphChain();  // works — lots of flicker
        break;
      case 4:
        shows[i].lightWave();
        break;
      case 5:
        shows[i].randomFlip();  // works
        break;
      case 6:
        shows[i].sawTooth();  // works
        break;
      case 7:
        shows[i].sinelon_fastled();  // works
        break;
      case 8:
        shows[i].bpm_fastled();  // works — fast!
        break;
      case 9:
        shows[i].juggle_fastled();  // works
        break;
      default:
        shows[i].confetti();  // more confetti - longer dim
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

//// Start specialized shows

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
//      leds[i] = color;
      uint8_t offset = (i + 19) % (ACTUAL_LEDS - NUM_LEDS);
      leds[NUM_LEDS + offset] = led[CHANNEL_A].smooth_rgb_color(leds[NUM_LEDS + offset], rgb_color, 4);  // symmetry!
      
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
      update_leds = true;
    }
  }
  
  if (update_leds) { 
    FastLED.show();
  } // Update the display only if something changed
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
