#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Linear Lights with FastLED
//
//  Dual shows - Blend together 2 shows running at once
//
//  Try to save on memory
//
//  Removed: Noise, Shuffle shows
//
//  10/22/2018
//
#define NUM_LEDS 40  // Chance of memory shortage for large NUM_LEDS

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40  // in milliseconds. FastLED demo has 8.3 ms delay!

#define DATA_PIN 9
#define CLOCK_PIN 8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // Hook for FastLED library

#define CAN_CHANGE_PALETTES false  // Palettes

// Shows
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 12

// Clocks and time
#define SHOW_DURATION 30  // seconds
#define FADE_TIME 10   // seconds to fade in + out
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);  // Only 1 leds object
  FastLED.setBrightness( BRIGHTNESS );
  
  if (CAN_CHANGE_PALETTES) {
    for (uint8_t i = 0; i < DUAL; i++) {
      led[i].setPalette();  // turns on palettes with default values
    }
  }

  // Set up the various mappings here (1D lists in PROGMEM)
  //  for (uint8_t i = 0; i < DUAL; i++) {
  //    led[i].setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  //    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
  //  }

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show
  
  for (uint8_t i = 0; i < DUAL; i++) {
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
  FastLED.show();  // Update the display

  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { 
      next_show(i); 
    }
  }
  // Debugging
  Serial.print(get_intensity(CHANNEL_A));
  Serial.print(", ");
  Serial.println(get_intensity(CHANNEL_B));
  
  delay(DELAY_TIME); // The only delay  
  
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  
  led[i].fillBlack();
  led[i].push_frame();
  led[i].randomizePalette();
  
  shows[i].resetAllClocks();
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();

  // Set foreColor + backColor to those of the other channel
  uint8_t other_channel = (i == CHANNEL_A) ? CHANNEL_B : CHANNEL_A;
  shows[i].setForeColor(shows[other_channel].getForeColor());
  shows[i].setBackColor(shows[other_channel].getBackColor());
}

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  uint8_t intensity_a = get_intensity(CHANNEL_A);
  uint8_t intensity_b = get_intensity(CHANNEL_B);
  
  if (intensity_a == 0) {
    update_leds_one_channel(CHANNEL_B);  // Channel A is off. Use only Channel B.
  } else if (intensity_b == 0) {
    update_leds_one_channel(CHANNEL_A);  // Channel B is off. Use only Channel A.
  } else {
    uint8_t fract = get_weighted_fract(intensity_a, intensity_b);
    morph_channels(fract);
  }
}

//
// update_leds_one_channel - push the a single channel's interp_frame on to the leds
//
void update_leds_one_channel(uint8_t channel) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = led[channel].getInterpFrameColor(i);
  }
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  CHSV led_a, led_b;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    led_a = led[CHANNEL_A].getInterpFrameColor(i);
    led_b = led[CHANNEL_B].getInterpFrameColor(i);
    leds[i] = led[CHANNEL_A].getInterpHSV(led_a, led_b, fract);  // interpolate a + b channels
  }
}

//
// Calculate the amount of b (0-255) from the intensities of A and B
//
uint8_t get_weighted_fract(uint8_t a, uint8_t b) {
  if (a == 0 && b == 0) { return 0; }  // Prevent divide-by-0
  
  return b * 255 / (a + b);
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
