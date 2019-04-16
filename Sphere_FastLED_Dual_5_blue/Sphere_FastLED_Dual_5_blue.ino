#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>

//
//  Sphere
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  2/20/2019
//
//  ToDo: 
//    Lines
//    Test Symmetries
//    Test LED layout

#define NUM_LEDS 144

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 30;  // in milliseconds (ArduinoBlue)

#define DATA_PIN 8
#define CLOCK_PIN 7

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[NUM_LEDS];  // Hook for FastLED library

// Shows
#define START_SHOW_CHANNEL_A  0  // Channels A starting show
#define START_SHOW_CHANNEL_B  1  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 12

// ArduinoBlue
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;
#define MAX_COLOR 256   // Colors are 0-255

// Clocks and time
#define SHOW_DURATION 30  // seconds
uint8_t FADE_TIME = 2;  // seconds to fade in + out (Arduino Blue)
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// Spherical Effects
#define MIN_EFFECTS_BEATS 3
#define MAX_EFFECTS_BEATS 30
uint8_t effect_number;  // 0, 1, or 2
uint8_t EffectBeatsPerMinute;
uint32_t effects_cycle = MAX_SMALL_CYCLE / 4;  // starting effects_cycle
uint8_t center_pixel;

// symmetries
#define NUM_SYMMETRIES 6
const uint8_t symmetries[] = { 144, 72, 36, 24, 12, 10 };  // Don't change
uint8_t symmetry[] = { 0, 0 };

// ArduinoBlue
ArduinoBlue phone(Serial2); // Blue Tx = pin 9; Blue Rx = pin 10
#define HUE_SLIDER        0
#define HUE_WIDTH_SLIDER  1
#define SPEED_SLIDER      2
#define BRIGHTNESS_SLIDER 3
#define FADE_TIME_SLIDER  4
#define BAM_BUTTON        0
#define BOLT_TIME        20

const uint8_t COORDS[] PROGMEM = {
  215, 86, 215, 231, 113, 205, 231, 141, 205, 215, 168, 215, 
  192, 176, 229, 165, 166, 246, 151, 143, 255, 151, 111, 255, 
  165, 88, 246, 192, 78, 229, 206, 127, 254, 206, 127, 254,   // End of face 0
  176, 229, 192, 166, 246, 165, 143, 255, 151, 111, 255, 151, 
  88, 246, 165, 78, 229, 192, 86, 215, 215, 113, 205, 231, 
  141, 205, 231, 168, 215, 215, 127, 254, 206, 127, 254, 206,   // End of face 1
  49, 231, 113, 39, 215, 86, 25, 192, 78, 8, 165, 88, 
  0, 151, 111, 0, 151, 143, 8, 165, 166, 25, 192, 176, 
  39, 215, 168, 49, 231, 141, 0, 206, 127, 0, 206, 127,   // End of face 2
  8, 89, 88, 25, 62, 78, 39, 39, 86, 49, 23, 113, 
  49, 23, 141, 39, 39, 168, 25, 62, 176, 8, 89, 166, 
  0, 103, 143, 0, 103, 111, 0, 48, 127, 0, 48, 127,   // End of face 3
  111, 0, 151, 143, 0, 151, 166, 8, 165, 176, 25, 192, 
  168, 39, 215, 141, 49, 231, 113, 49, 231, 86, 39, 215, 
  78, 25, 192, 88, 8, 165, 127, 0, 206, 127, 0, 206,   // End of face 4
  103, 111, 255, 103, 143, 255, 89, 166, 246, 62, 176, 229, 
  39, 168, 215, 23, 141, 205, 23, 113, 205, 39, 86, 215, 
  62, 78, 229, 89, 88, 246, 48, 127, 255, 48, 127, 255,   // End of face 5
  205, 231, 141, 215, 215, 168, 229, 192, 176, 246, 165, 166, 
  255, 151, 143, 255, 151, 111, 246, 165, 88, 229, 192, 78, 
  215, 215, 86, 205, 231, 113, 255, 206, 127, 255, 206, 127,   // End of face 6
  246, 89, 166, 229, 62, 176, 215, 39, 168, 205, 23, 141, 
  205, 23, 113, 215, 39, 86, 229, 62, 78, 246, 89, 88, 
  255, 103, 111, 255, 103, 143, 254, 48, 127, 254, 48, 127,   // End of face 7
  143, 0, 103, 111, 0, 103, 88, 8, 89, 78, 25, 62, 
  86, 39, 39, 113, 49, 23, 141, 49, 23, 168, 39, 39, 
  176, 25, 62, 166, 8, 89, 127, 0, 48, 127, 0, 48,   // End of face 8
  39, 86, 39, 23, 113, 49, 23, 141, 49, 39, 168, 39, 
  62, 176, 25, 89, 166, 8, 103, 143, 0, 103, 111, 0, 
  89, 88, 8, 62, 78, 25, 48, 127, 0, 48, 127, 0,   // End of face 9
  78, 229, 62, 88, 246, 89, 111, 255, 103, 143, 255, 103, 
  166, 246, 89, 176, 229, 62, 168, 215, 39, 141, 205, 23, 
  113, 205, 23, 86, 215, 39, 127, 255, 48, 127, 255, 48,   // End of face 10
  215, 168, 39, 231, 141, 49, 231, 113, 49, 215, 86, 39, 
  192, 78, 25, 165, 88, 8, 151, 111, 0, 151, 143, 0, 
  165, 166, 8, 192, 176, 25, 206, 127, 0, 206, 127, 0,   // End of face 11
};

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");
  Serial2.begin(9600);  // Serial2: Bluetooth serial (ArduinoBlue)

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);  // Only 1 leds object
  FastLED.setBrightness( BRIGHTNESS );

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show
  shows[CHANNEL_A].setWaitRange(15, 50, 35);  // Slow down Channel A
  
  reset_effects();
  
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].fillBlack();
    led[i].push_frame();
  }
}

void loop() {

  for (uint8_t i = 0; i < DUAL; i++) {

    switch (current_show[i]) {
    
      case 0:
        shows[i].morphChain();
        break;
      case 1:
        shows[i].twoColor();
        break;
      case 2:
        shows[i].lightWave();
        break;
      case 3:
        shows[i].sawTooth();
        break;
      case 4:
        shows[i].lightRunUp();
        break;
      case 5:
        shows[i].packets();
        break;
      case 6:
        shows[i].packets_two();
        break;
      case 7:
        shows[i].sinelon_fastled();
        break;
      case 8:
        shows[i].bpm_fastled();
        break;
      case 9:
        shows[i].juggle_fastled();
        break;
      case 10:
        shows[i].allOn();
        break;
      default:
        shows[i].bands();
        break;
    }
  
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  check_phone();  // Check the phone settings (ArduinoBlue)
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
  
  if (effects_cycle++ >= MAX_SMALL_CYCLE) {
    effects_cycle = 0;
    reset_effects();
  }

  if (curr_lightning > 0 ) {  // (ArduinoBlue)
    curr_lightning--; // Reduce the current bolt
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  symmetry[i] = random8(NUM_SYMMETRIES);
  if (symmetry[i]  < 4) { symmetry[i] = 0; }  // symmetries 1-3 are bogus
  shows[i].resetNumLeds(symmetries[symmetry[i]]);  // resets the virtual number of LEDs
  
  led[i].fillBlack();
  led[i].push_frame();
  
  shows[i].resetAllClocks();
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
}

//
// reset_effects - start over the effects clock
//
void reset_effects() {
  effect_number = random8(3);  // 0, 1, or 2 (h, s, or v)
  EffectBeatsPerMinute = random8(MIN_EFFECTS_BEATS, MAX_EFFECTS_BEATS);
  center_pixel = random8(NUM_LEDS);
}

//
// update_leds - push the interp_frame on to the leds - handle symmetry
//
void update_leds() {
  CHSV color;
 
  for (uint8_t channel = 0; channel < DUAL; channel++) {
    uint8_t sym = symmetry[channel];
    for (uint8_t i = 0; i < shows[channel].getNumLeds(); i++) {
      
      color = led[channel].getInterpFrameColor(i);

      if (sym == 5) {
        for (uint8_t j = 0; j < 12; j++) {
          leds_buffer[channel][(i * 12) + j] = color;
        }
      } else {
        for (uint8_t j = 0; j < (144 / symmetries[sym]); j++) {
          leds_buffer[channel][(j * symmetries[sym]) + i] = color;
        }
      }
    }
  }
}

///// SPECIALIZED SHOWS

// No specialized shows yet

///// END SPECIALIZED SHOWS


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  CHSV color;
  
  uint8_t effect_fract = get_intensity_for_cycle(effects_cycle);
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    color = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][i], 
                                        leds_buffer[CHANNEL_A][i], 
                                        fract);  // interpolate a + b channels
    color = add_effect(color, get_effect_amount(i), effect_fract);
    color = lightning(narrow_palette(color));  // (ArduinoBlue)
    leds[i] = color;
  }
}

//
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  color.h = map8(color.h, hue_start, (hue_start + hue_width) % MAX_COLOR );
  return color;
}

//
// lightning - ramp all pixels quickly up to white (down sat & up value) and back down
//
CHSV lightning(CHSV color) {  // (ArduinoBlue)
  if (curr_lightning > 0) {
    uint8_t increase = 255 - cos8( map(curr_lightning, 0, BOLT_TIME, 0, 255));
    color.s -= increase;
    color.v += increase;
  }
  return color;
}

//
// check_phone - poll the phone for updated values  (ArduinoBlue)
//
void check_phone() {
  int8_t sliderId = phone.getSliderId();  // ID of the slider moved
  int8_t buttonId = phone.getButton();  // ID of the button

  if (sliderId != -1) {
    int16_t sliderVal = phone.getSliderVal();  // Slider value goes from 0 to 200
    sliderVal = map(sliderVal, 0, 200, 0, 255);  // Recast to 0-255

    switch (sliderId) {
      case BRIGHTNESS_SLIDER:
        BRIGHTNESS = sliderVal;
        FastLED.setBrightness( BRIGHTNESS );
        break;
      case HUE_SLIDER:
        hue_start = sliderVal;
        break;
      case HUE_WIDTH_SLIDER:
        hue_width = sliderVal;
        break;
      case SPEED_SLIDER:
        DELAY_TIME = map8(sliderVal, 10, 100);
        break;
      case FADE_TIME_SLIDER:
        FADE_TIME = map8(sliderVal, 0, SHOW_DURATION);
        break;
    }
  }
  
  if (buttonId == BAM_BUTTON) { curr_lightning = BOLT_TIME; }
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  return get_intensity_for_cycle(shows[i].getSmallCycle());
}

uint8_t get_intensity_for_cycle(uint16_t small_cycle) {
  uint8_t intensity;  // 0 = Off, 255 = full On

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

//
// add_effect - modify hue, sat, or value in a changing, spherical fashion
//
CHSV add_effect(CHSV color, uint8_t effect_amount, uint8_t effect_fract) {

  // Convert full 0-255 to something more mild, inverse with speed
  effect_fract = map8(effect_fract, 0, 255 * MIN_EFFECTS_BEATS / EffectBeatsPerMinute);
  
  switch(effect_number) {  
    
    case 0:  // hue
      color.h = led[CHANNEL_A].interpolate_wrap(color.h, effect_amount, effect_fract);
      break;
    case 1:  // sat
      color.s = lerp8by8(color.s, map8(effect_amount, 255, 128), effect_fract);
      break;
    case 2:  // value
      color.v = lerp8by8(color.v, effect_amount, effect_fract);
      break;
  }
  return color;
}

//
// get_effect_amount - From pixel, dist, and cycle, calculate 0-255 effect amount
//
uint8_t get_effect_amount(uint8_t i) {
  return sin8_C( beatsin8( EffectBeatsPerMinute, 0, 255) + get_distance(i, center_pixel) );
}

//
// get_distance
//
uint8_t get_distance(uint8_t c1, uint8_t c2) {
  // 3D distance = sqrt(x2 + y2 + z2) scaled to 0-255
  uint16_t d = sqrt(get_diff_sq_by_index(c1, c2, 0) +
                    get_diff_sq_by_index(c1, c2, 1) +
                    get_diff_sq_by_index(c1, c2, 2));
  return map(d, 0, 262, 0, 255);
}

uint16_t get_diff_sq_by_index(uint8_t c1, uint8_t c2, uint8_t index) {
  return sq( get_coord_value(c2, index) - get_coord_value(c1, index) );
}

//
// get_coord_value
//
uint8_t get_coord_value(uint8_t coord, uint8_t index) {
  return pgm_read_byte_near(COORDS + (coord * 3) + index);
}
