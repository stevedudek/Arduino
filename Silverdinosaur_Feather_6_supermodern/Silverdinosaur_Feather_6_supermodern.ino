#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Silver Dinosaur with 3 folds
//
//  Modern Software
//
//  10/7/23
//

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME  6  // in milliseconds

// Smoothing constants - lower is slower smoothing
// At 15 ms cycles: 40 = 96 ms, 30 = 127 ms, 20 = 191 ms, 10 = 382 ms, 5 = 765 ms, 2 = 1.9 s, 1 = 3.8 s
#define SMOOTHING_SHOWS_HUE   20   // Fastest full rainbow = DELAY_TIME * (255 / this value)
#define SMOOTHING_SHOWS_VALUE 20   // Fastest turn off/on = DELAY_TIME * (255 / this value)

#define DATA_PIN 0  //  11 // 7 // data = 7 (Teensy) or 0 (Feather)
// #define CLOCK_PIN 2  //  13 // 8 // clock = 8 (Teensy) or 2 (Feather)

#define NUM_LEDS 125

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), Shows(&led[CHANNEL_B], CHANNEL_B)  };
CHSV led_buffer[NUM_LEDS];  // Required for smoothing
CRGB leds[NUM_LEDS];  // The Leds themselves

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 14
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

// wait times
uint8_t show_duration = 120;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 192;  // 0 = no fading, to 255 = always be fading

#define NUM_SYMMETRIES 7
uint8_t show_symmetry[] = { 0, 0 };

#define NUM_PATTERNS 6
uint8_t current_pattern[] = { 0, 0 };
const uint8_t spike_remap[] PROGMEM = { 0, 2, 4, 6, 7, 5, 3, 1 };

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

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

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  // FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);  // only use of NUM_LEDS
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
    shows[i].fillBlack();
    led[i].push_frame();
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
  morph_channels();
}

//
// update leds - Moved to a task-scheduled event
//
void updateLeds() {
  // Moved to a task-scheduled event

  for (uint8_t i = 0; i < DUAL; i++) {

    switch (current_show[i]) {
  
      case 0:
        patterns(i);
        break;
      case 1:
        shows[i].lightWave();
        break;
      case 2:
        shows[i].morphChain();  // works
        break;
      case 3:
        shows[i].randomFlip();  // good
        break;
      case 4:
        shows[i].randomOneColorBlack();  // good
        break;
      case 5:
        shows[i].randomTwoColorBlack();  // good
        break;
      case 6:
        shows[i].lightRunUp();  // good
        break;
      case 7:
        shows[i].twoColor();  // good
        break;
      case 8:
        shows[i].sawTooth();  // dark? not fast enough
        break;
      case 9:
        shows[i].sinelon_fastled();  // good
        break;
      case 10:
        shows[i].bpm_fastled();  // colors change too much
        break;
      case 11:
        shows[i].juggle_fastled();  // good
        break;
      case 12:
        shows[i].confetti();  // more confetti - longer dim
        break;
      default:
        shows[i].lightRunUp();  // colors change too fast
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }
//  morph_channels();  // morph together the 2 leds channels
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
  current_pattern[i] = random8(NUM_PATTERNS);
  show_symmetry[i] = random(NUM_SYMMETRIES);
  set_show_symmetry(i);
}

//
// pick_next_show
//
void pick_next_show(uint8_t i) {
  uint8_t other_channel = (i == CHANNEL_A) ? CHANNEL_B : CHANNEL_A ;
  current_show[i] = (current_show[other_channel] != 0) ? 0 : random8(1, NUM_SHOWS) ;
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
//  log_status(i);  // For debugging
}

boolean is_other_channel_show_zero(uint8_t c) {
  if (c == CHANNEL_A) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

//
// set_led_number
//
void set_show_symmetry(uint8_t i) {
  uint8_t symmetry_led_number[] = { 125, 125, 8, 8, 8, 15, 15 };
  led[i].fillBlack();  // clear leds before symmetry change
  shows[i].resetNumLeds(symmetry_led_number[show_symmetry[i]]);  // resets the virtual number of LEDs
}

//
// patterns shows
//
void patterns(uint8_t i) {

  switch (current_pattern[i]) {
  
      case 0:
        shows[i].allOn();
        break;
      case 1:
        shows[i].twoColor();
        break;
      case 2:
        shows[i].randomColors();
        break;
      case 3:
        shows[i].randomOneColorBlack();
        break;
      case 4:
        shows[i].stripes();
        break;
      default:
        shows[i].randomTwoColorBlack();
        break;
  }
}


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  CRGB rgb_color;
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getCurrFrameColor(get_pixel_from_led(i, CHANNEL_B));   
    CHSV color_a = led[CHANNEL_A].getCurrFrameColor(get_pixel_from_led(i, CHANNEL_A));
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
    
    // smoothing backstop. smooth constants should be large.
//    color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);

    if (led_buffer[i].h == color.h && led_buffer[i].s == color.s && led_buffer[i].v == color.v) {
      continue;
    } else {
      hsv2rgb_rainbow(color, rgb_color);
      leds[i] = led[CHANNEL_A].smooth_rgb_color(leds[i], rgb_color, 4);
//      leds[i] = color;   // put HSV color on to LEDs
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
    }
  }

  FastLED.show();
}

//
// remap_leds - fix the switchbacks
//
uint8_t remap_leds(uint8_t i) {
  if (i < 32) {
    return i;
  } else if (i < 64) {
    return (63 - i) + 32;
  } else if (i < 105) {
    return i;
  } else if (i < 114) {
    return (113 - i) + 105;
  } else {
    return i;
  }
}

//
// get_pixel_from_led - based on symmetry and led, find the assigned pixel
//
uint8_t get_pixel_from_led(uint8_t i, uint8_t c) {
  
  switch (show_symmetry[c]) {
      
      case 0:  // Fill from bottom to top (125)
        return get_dinosaur_fill(i);
        break;
      case 1:  // Fill from top to bottom (125)
        return NUM_LEDS - get_dinosaur_fill(i) - 1;
        break;
      case 2: // fill each spike (8)
        return get_spike_fill(i);
        break;
      case 3: //  (8)
        return 7 - get_spike_fill(i);
        break;
      case 4:  //  (8)
        return pgm_read_byte_near(spike_remap + get_spike_fill(i));
        break;
      case 5:  // 15
        return get_which_spike(i);
        break;
      default:
        return 14 - get_which_spike(i);
        break;
  }
}

uint8_t get_which_spike(uint8_t i) {
  // Get which 0-14 whole spike
  if (i < 96) {
    return i / 8;
  } else if (i < 105) {
    return 12;
  } else if (i < 113) {
    return 13;
  } else {
    return 14;
  }
}

uint8_t get_spike_fill(uint8_t i) {
  // Get the 0-7 position on a particular spike
  if (i < 96) {
    return i % 8;
  } else if (i < 105) {
    return map(i - 96, 0, 8, 0, 7);
  } else if (i < 113) {
    return map(i - 105, 0, 8, 0, 7);
  } else {
    return map(i - 114, 0, 10, 0, 7);
  }
}

uint8_t get_dinosaur_fill(uint8_t i) {
  // Fill from bottom to top
  if (i < 96) {
    return ((i % 32) * 3) + (i / 32);
  } else if (i < 123) {
    i -= 96;
    return 96 + ((i % 9) * 3) + (i / 9);
  } else {
    return i;
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
