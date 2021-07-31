#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Black Spiked Coat with 21 white spikes
//
//  Works!
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  7/29/20
//
#define NUM_LEDS 11
#define ACTUAL_LEDS 21

#define BRIGHTNESS  255 // (0-255)

uint8_t DELAY_TIME = 30;  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
long last_time;
#define MIN_DELAY_CYCLES  10
#define MAX_DELAY_CYCLES 100

#define DATA_PIN 7
#define CLOCK_PIN 8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[ACTUAL_LEDS];  // Hook for FastLED library

// Shows
#define START_SHOW_CHANNEL_A  0  // Channels A starting show
#define START_SHOW_CHANNEL_B  1  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 12

// Clocks and time
#define SHOW_DURATION 30  // seconds
#define FADE_TIME 30   // seconds to fade in. If FADE_TIME = SHOW_DURATION, then Always Be Fading
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

uint8_t LED_LOOKUP[] = { 0, 1, 2, 3, 4, 5, 6, 7, 12, 11, 10 };
uint8_t LED_LOOKUP_PAIRED[] = { 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10 };

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS);  // Only 1 leds object
  FastLED.setBrightness( BRIGHTNESS );

  // Set up the various mappings here (1D lists in PROGMEM)
  //  for (uint8_t i = 0; i < DUAL; i++) {
  //    led[i].setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  //    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
  //  }

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show
  last_time = millis();
  
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].fillBlack();
    led[i].push_frame();
    shows[i].setWaitRange(MIN_DELAY_CYCLES, MAX_DELAY_CYCLES);
  }
}

void loop() {

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
        shows[i].packets();
        break;
      case 7:
        shows[i].packets_two();
        break;
      case 8:
        shows[i].sinelon_fastled();
        break;
      case 9:
        shows[i].bpm_fastled();
        break;
      case 10:
        shows[i].juggle_fastled();
        break;
      default:
        shows[i].bands();
        break;
    }
  
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  update_leds();  // morph together the 2 chanels & push the interp_frame on to the leds
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  fixed_delay();
  advance_clocks();  // advance the cycle clocks and check for next show
}

//
// fixed_delay - make every cycle the same time
//
void fixed_delay() {
  long new_time = millis();
  long time_delta = new_time - last_time;  // how much time has elapsed? Usually 3-5 milliseconds
  last_time = new_time;  // update the counter
  if (time_delta < DELAY_TIME) {  // if we have excess time,
    delay(DELAY_TIME - time_delta);  // delay for the excess
  }
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { 
      next_show(i); 
    }
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  
  led[i].fillBlack();
  led[i].push_frame();
  
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
}

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (uint8_t channel = 0; channel < DUAL; channel++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds_buffer[channel][i] = led[channel].getInterpFrameColor(i);
    }
  }
}

//
// morph_channels
// 1. morph together the 2 chanels and update the LEDs 
// 2. Mirror each LED
//
void morph_channels(uint8_t fract) {
  CHSV color;
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    color = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][i], 
                                        leds_buffer[CHANNEL_A][i], 
                                        fract);  // interpolate a + b channels
    leds[LED_LOOKUP[i]] = color;
    leds[LED_LOOKUP_PAIRED[i]] = color;
  }
  set_eye_color();
}

void set_eye_color() {
  CHSV eye_color = led[0].wheel(shows[0].getForeColor());
  leds[8] = eye_color;
  leds[9] = eye_color;
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();

  // Similar logic to check_fades (deprecated)
  if (small_cycle <= FADE_CYCLES) {
    intensity = map(small_cycle, 0, FADE_CYCLES, 0, 255);  // rise
  } else if (small_cycle <= (MAX_SMALL_CYCLE / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((MAX_SMALL_CYCLE / 2) + FADE_CYCLES)) {
    intensity = map(small_cycle - (MAX_SMALL_CYCLE / 2), 0, FADE_CYCLES, 255, 0);  // decay
  } else {
    intensity = 0;
  }
  return intensity;
}
