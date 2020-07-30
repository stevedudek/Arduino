#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Linear Lights with FastLED
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  Removed: Noise, palettes, 2D shows (need to do this on Show repository)
//
//  3/16/2019
//
//#define NUM_LEDS 1170  // A lot!
int NUM_LEDS = 1170;  // A lot!
#define TOTAL_LEDS 2340   // even more!!

#define BRIGHTNESS 96  // (0-255)

#define DELAY_TIME 40  // in milliseconds. FastLED demo has 8.3 ms delay!

#define DATA_PIN 7

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][1170];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[TOTAL_LEDS];  // Hook for FastLED library

// Shows
#define START_SHOW_CHANNEL_A  3  // Channels A starting show
#define START_SHOW_CHANNEL_B  1  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 12

// Clocks and time
#define SHOW_DURATION 30  // seconds
#define FADE_TIME 15   // seconds to fade in. If FADE_TIME = SHOW_DURATION, then Always Be Fading
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// LED Mapping
uint16_t SMALL_MAP[] = { 141, 0, 161, 20, 209, 68, 229, 88, 315, 174, 335, 194, 383, 242, 403, 262 };
uint16_t LARGE_MAP[] = { 0, 55, 752, 807, 56, 93, 366, 403,
                         94, 121, 202, 229, 122, 141, 142, 161,
                         162, 181, 182, 201, 230, 257, 338, 365,
                         258, 277, 278, 297, 298, 317, 318, 337,
                         404, 441, 714, 751, 442, 469, 550, 577,
                         470, 489, 490, 509, 510, 529, 530, 549,
                         578, 605, 686, 713, 606, 625, 626, 645,
                         646, 665, 666, 685 };
//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, TOTAL_LEDS);  // Only 1 leds object
  FastLED.setBrightness( BRIGHTNESS );

  // Set up the various mappings here (1D lists in PROGMEM)
  //  for (uint8_t i = 0; i < DUAL; i++) {
  //    led[i].setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  //    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
  //  }

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show
  
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].turnOnBlur();
    
    led[i].fillBlack();
    led[i].push_frame();
  }
}

void loop() {

  for (uint8_t i = 0; i < DUAL; i++) {

    if (get_intensity(i) > 0) {
      
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
  }

  update_leds();  // morph together the 2 chanels & push the interp_frame on to the leds
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
  
  delay(DELAY_TIME); // The only delay  
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
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][i], 
                                             leds_buffer[CHANNEL_A][i], 
                                             fract);  // interpolate a + b channels
    
    for (uint8_t x = 0; x < 2; x++) {
      uint16_t n = i + (NUM_LEDS * x);

      for (uint8_t m = 0; m < 15; m += 2) {
        if (n <= SMALL_MAP[m]) {
          n += SMALL_MAP[m+1];
          Serial.println(n);
          break;
        }
      }
      leds[n] = color;
      leds[get_paired_led(n)] = color;
    }
  }
}

//
// get_paired_led
//
uint16_t get_paired_led(uint16_t n) {
  for (uint8_t m = 1; m < 60; m += 4) {
    if (n <= LARGE_MAP[m]) {
      return (LARGE_MAP[m] - n + LARGE_MAP[m+1]);
    }
  }
  return n;
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
