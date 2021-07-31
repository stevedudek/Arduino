#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Blue Dinosaur with 9 stegosauran paddles
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  2/26/2020
//
#define NUM_LEDS 99
#define ACTUAL_LEDS 123

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40  // in milliseconds. FastLED demo has 8.3 ms delay!

#define DATA_PIN 7

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[ACTUAL_LEDS];  // Hook for FastLED library

CHSV morph_buffer[DUAL][NUM_LEDS];

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

boolean simplespikes[] = { false, false };

// Paddle lookup tables
const uint8_t paddle_size_lookup[] = { 6, 10, 12, 14, 16, 14, 12, 10, 5 };
const uint8_t paddle_first_led_lookup[] = { 0, 6, 16, 28, 42, 58, 72, 84, 94, 99 };

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, ACTUAL_LEDS);;  // Only 1 leds object
  FastLED.setBrightness( BRIGHTNESS );

  // Set up the various mappings here (1D lists in PROGMEM)
  //  for (uint8_t i = 0; i < DUAL; i++) {
  //    led[i].setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  //    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
  //  }
  
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].fillBlack();
    led[i].push_frame();
    shows[i].setWaitRange(5, 50, 45);
    shows[i].setBandsBpm(10, 30);
  }
  
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);
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
  unpack_leds();
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

  /*
  simplespikes[i] = (random8(2) == 0) ? true : false ;
  set_led_number(i);  // This may not work
  */
  
  led[i].fillBlack();
  led[i].push_frame();

  shows[i].turnOnMorphing();
  shows[i].resetAllClocks();
  shows[i].setWaitRange(5, 50, 45);
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
}

//
// set_led_number
//
void set_led_number(uint8_t i) {
  led[i].fillBlack();  // clear leds before symmetry change
  uint8_t numSymLeds = (simplespikes[i]) ? 8 : 54; // This may not work
  shows[i].resetNumLeds(numSymLeds);  // resets the virtual number of LEDs
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
// unpack_leds - if simplespines, need to unpack into component leds
//
void unpack_leds() {
  for (uint8_t channel = 0; channel < DUAL; channel++) {
    if (simplespikes[channel]) {
      for (uint8_t spike = 0; spike < 9; spike++) {
        // Light all the LEDs on the numbered spike
        uint8_t first_led = paddle_first_led_lookup[spike];
        for (uint8_t i = 0; i < paddle_size_lookup[spike]; i++) {
          morph_buffer[channel][first_led + i] = leds_buffer[channel][spike];
        }
      }
    } else {
      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        morph_buffer[channel][i] = leds_buffer[channel][i];
      }
    }
  }
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color = led[CHANNEL_A].getInterpHSV(morph_buffer[CHANNEL_B][i], 
                                             morph_buffer[CHANNEL_A][i], 
                                             fract);  // interpolate a + b channels
    leds[add_spacer_leds(i)] = color;
  }
}

uint8_t add_spacer_leds(uint8_t i) {
  uint8_t num_spacers = 0;
  for (uint8_t spike = 1; i >= paddle_first_led_lookup[spike]; spike++) {
    num_spacers+=3;
  }
  return i + num_spacers;
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
