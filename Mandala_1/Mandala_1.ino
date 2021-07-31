#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Mandala with FastLED
//
//  37 pixels with 64 LEDs
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  11/15/20
//
//
#define NUM_LEDS 37  // Chance of memory shortage for large NUM_LEDS
#define ACTUAL_LEDS 64

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 20;  // in milliseconds (ArduinoBlue)
long last_time;
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 7
#define CLOCK_PIN 8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define XX  255  // Empty LED

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[ACTUAL_LEDS];  // Hook for FastLED library

#define ONLY_RED true  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;

// Shows
#define START_SHOW_CHANNEL_A  11  // Channels A starting show
#define START_SHOW_CHANNEL_B  0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 12

// Clocks and time
#define SHOW_DURATION 30  // seconds
uint8_t FADE_TIME = 20;  // seconds to fade in + out (Arduino Blue)
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  // Set up the various mappings here (1D lists in PROGMEM)
  //  for (uint8_t i = 0; i < DUAL; i++) {
  //    led[i].setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  //    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
  //  }

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show
  last_time = millis();
  
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();
  }

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_start = 192;
    hue_width = 124;
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

  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
  fixed_delay();
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
  
  led[i].push_frame();
  
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
}

//
// Test Strand
//
void test_strand(uint8_t channel) {
  shows[channel].fillBlack();
  uint8_t i = shows[channel].getCycle() % NUM_LEDS;
  led[channel].setPixelHue(i, shows[channel].getForeColor());
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getInterpFrameColor(i);
    CHSV color_a = led[CHANNEL_A].getInterpFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = narrow_palette(color);
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }

//    leds[i] = color;
    remap_leds(i, color); // instead of leds[i] = color;
    led_buffer[i] = color;
  }
}

//
// remap_leds - send color to the correct leds
//
void remap_leds(uint8_t i, CHSV color) {
  uint8_t led_lookup_table[NUM_LEDS * 3] = {
     0,  2,  3,  // 0
     6, XX, XX,
     8, XX, XX,
    10, XX, XX,
    13, 14, 16,
     1, XX, XX,  // 5
     4, XX, XX,
     5,  7, XX,
     9, 11, XX,
    12, XX, XX,
    15, XX, XX,  // 10
    29, 31, XX,
    27, XX, XX,
    25, XX, XX,
    23, XX, XX,
    19, 22, XX,  // 15
    32, XX, XX,
    30, XX, XX,
    24, 26, 28,
    21, XX, XX,
    20, XX, XX,  // 20
    33, 35, XX,
    36, XX, XX,
    38, XX, XX,
    40, XX, XX,
    42, 44, XX,  // 25
    62, XX, XX,
    59, XX, XX,
    56, 58, XX,
    52, 54, XX,
    51, XX, XX,  // 30
    48, XX, XX,
    60, 61, 63,
    57, XX, XX,
    55, XX, XX,
    53, XX, XX,  // 35
    47, 49, 50
  };

  if (i == 18) {  // Center star
    light_center(color);
  } else {
    for (uint8_t j = 0; j < 3; j++) {
      uint8_t led_position = led_lookup_table[((i % NUM_LEDS) * 3) + j];
      if (led_position != XX) {
        leds[led_position % ACTUAL_LEDS] = color;
      }
    }
  }
}

//
// light_center - light the center 6 leds with the same color
//
void light_center(CHSV color) {
  uint8_t center_pixels[] = { 24, 26, 28, 37, 39, 41 };
  for (uint8_t i = 0; i < 6; i++) {
    leds[center_pixels[i]] = color;
  }
}

//
// turn_off_spacers - turn off the 4 spacer pixels
//
void turn_off_spacers() {
  uint8_t spacer_pixels[] = { 17, 18, 45, 46 };
  for (uint8_t i = 0; i < 4; i++) {
    leds[spacer_pixels[i]] = CHSV(0, 0, 0);
  }
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();

  if (small_cycle <= FADE_CYCLES) {
    intensity = map(small_cycle, 0, FADE_CYCLES, 0, 255);  // rise
  } else if (small_cycle <= (MAX_SMALL_CYCLE / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((MAX_SMALL_CYCLE / 2) + FADE_CYCLES)) {
    intensity = map(small_cycle - (MAX_SMALL_CYCLE / 2), 0, FADE_CYCLES, 255, 0);  // decay
  } else {
    intensity = 0;
  }
  return ease8InOutQuad(intensity);
}

//// End DUAL SHOW LOGIC

//
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  color.h = map8(color.h, hue_start, (hue_start + hue_width) % 256 );
  return color;
}
