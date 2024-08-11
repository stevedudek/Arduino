#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Linear Lights with FastLED - Modern Software
//
//  Dual shows - Blend together 2 shows running at once
//
//  11/3/23
//
#define NUM_LEDS 35  // Chance of memory shortage for large NUM_LEDS

uint8_t bright = 128;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 255;  // (0 = fast, 255 = slow)

#define MIN_VALUE 10

#define DELAY_TIME  15  // in milliseconds

// Smoothing constants - lower is slower smoothing
#define SMOOTHING_SHOWS_HUE    4   // Fastest full rainbow = DELAY_TIME * (255 / this value) = 150 ms
#define SMOOTHING_SHOWS_VALUE  30   // Fastest turn off/on = DELAY_TIME * (255 / this value) = 150 ms

#define DATA_PIN 0  // 7
#define CLOCK_PIN 2  //  8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // Required for smoothing
CRGB leds[NUM_LEDS];  // The Leds themselves

#define ONLY_BLUE true
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;
CHSV dim_blue = CHSV(160, 255, MIN_VALUE);

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 13
#define START_SHOW_CHANNEL_A  1  // Channels A starting show
#define START_SHOW_CHANNEL_B  0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

// wait times
uint8_t show_duration = 40;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 196;  // 0 = no fading, to 255 = always be fading

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

// Lookup tables

// Order of spines
const uint8_t SpineOrder[35] = {
        10,
   11,  20,   9,
     19,   21,
  12,18,23,22,8,
  13,17,24,25,7,
  14,16,27,26,6,
     15,28,5,
        29,
       30,4,
       31,3,
       32,2,
       33,1,
       34,0,
};

// Tail pattern
const uint8_t Tails[35] = {
         0,
    0,   0,   0,
      0,    0,
   0, 0, 0, 0,0,
   0, 0, 0, 0,0,
   0, 0, 0, 0,0,
      0, 0, 0,
        1,
       2,2,
       3,3,
       4,4,
       5,5,
       6,6,
};

// Hood Ring
const uint8_t Hood[35] = {
         1,
    2,   0,   2,
      0,    0,
   3, 0, 0, 0,3,
   4, 0, 0, 0,4,
   5, 0, 0, 0,5,
      6, 0, 6,
         0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
};

// Stripe Patten
const uint8_t Stripes[35] = {
         1,
    0,   2,   0,
      0,    0,
   0, 0, 3, 0,0,
   0, 0, 4, 0,0,
   0, 0, 5, 0,0,
      0, 6, 0,
         7,
        8,8,
        9,9,
        10,10,
        11,11,
        12,12,
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );

  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
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

  shows[CHANNEL_B].setStartShowTime(millis() - shows[CHANNEL_A].getElapsedShowTime() - (show_duration * 1000 / 2));
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  if (ONLY_BLUE) {
    hue_center = 190;
    hue_width = 80;
  }

  userScheduler.addTask(taskUpdateLeds);
  taskUpdateLeds.enable();
}

//
// loop
//
void loop() {
  userScheduler.execute();  // Essential!
  morph_channels();  // On every cycle
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
        shows[i].sinelon_fastled();
        break;
      case 7:
        shows[i].bpm_fastled();
        break;
      case 8:
        shows[i].juggle_fastled();
        break;
      case 9:
        lighttails(i);
        break;
      case 10:
        lighthood(i);
        break;
      case 11:
        centergradient(i);
        break;
      default:
        centerpulse(i);
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

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
  shows[i].fillBlack();
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
// Specialized Shows
//

//
// lighttails: turn on the tail lights to a gradient
// 
void lighttails(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (Tails[i] == 0) {
      shows[c].setPixeltoForeBlack(SpineOrder[i]);
    } else {
      shows[c].setPixeltoColor(SpineOrder[i], led[c].wheel(shows[i].getForeColor() + (10 * Tails[i])));
    }
  }
}

//
// lighthood: ring the hood with one color; fill everything else with another color
//
void lighthood(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (Hood[i] == 0) {
      shows[c].setPixeltoForeBlack(i);
    } else {
      shows[c].setPixeltoColor(i, led[c].wheel(shows[c].getForeColor() + (10 * Hood[i])));
    }
  }
}

//
// centergradient: turn on the central spine with a light gradient
//
 
void centergradient(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (Stripes[i] == 0) {
      shows[c].setPixeltoForeBlack(i);
    } else {
      shows[c].setPixeltoColor(i, led[c].wheel(shows[c].getForeColor() + (10 * Stripes[i])));
    }
  }
}

//
// centerpulse - One pixel travelling down the center spine
//
 
void centerpulse(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (Stripes[i] == (1 + (shows[c].getCycle() % 12))) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoForeBlack(i);
    }
  }
}

//// End specialized shows

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  CRGB rgb_color;
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getCurrFrameColor(i);   // New! Changed Interp to Next. No more Interp frames.
    CHSV color_a = led[CHANNEL_A].getCurrFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
    
    // smoothing backstop. smooth constants should be large.
    color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);
    
    hsv2rgb_rainbow(color, rgb_color);
    leds[SpineOrder[i]] = led[CHANNEL_A].smooth_rgb_color(leds[i], rgb_color, 4);
//      leds[i] = color;   // put HSV color on to LEDs
    led_buffer[i] = color;  // push new color into led_buffer (both HSV)
  }
  FastLED.show();
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
