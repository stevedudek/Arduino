#include <FastLED.h>

//
//  Test Pattern to Lay out Tree Y Configurations
//
//  4/23/2019
//
// a 5m strand has 150 LEDs

//#define NUM_LEDS 134  // 27 + 20 Y
#define NUM_LEDS 136  // 28 + 20 Y (closer to 0.72 ratio)

// Can afford 260 LEDs per each of the 3 large Y's
// A full noodle (back and forth) requires 78 LEDs
// Large Y = 2 full noodles = 2 * 78 = 158 LEDs
// Have 100 LEDs remaining for largest noodle = 1.66m max = 65" max
// Originally wanted ~71", so cutting it close, but can do
// Largest noodle by 0.72 ratio should have 53.6 LEDs x 2 = 108 or 110 LEDs

// 12 Ys * 136 = 1632 LEDs
// Large Y each = 78 + 78 + 108 = 264 LEDs (possibly 266) * 3 large Ys = 792 LEDs (or 798)
// Total = 1632 + 792 = 2424 LEDs (bought 2400) Have an additional 136. Cutting it close (as usual).

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40  // in milliseconds. FastLED demo has 8.3 ms delay!

#define DATA_PIN 7

CRGB leds[NUM_LEDS];  // Hook for FastLED library

// To Do: solder on one more LED at position 0

/*
#define NUM_TIP_LEDS 6
uint8_t tip_leds[] = { 0, 133, 46, 47, 86, 87 };

#define NUM_Y_LEDS 6
uint8_t y_leds[] = { 26, 27, 66, 67, 106, 107 };

#define NUM_SOLDER_LEDS 6
uint8_t solder_leds[] = { 25, 108, 28, 65, 68, 105 };
*/

// For 20 + 28 configuration = 136 LEDs Y
#define NUM_TIP_LEDS 6
uint8_t tip_leds[] = { 0, 135, 47, 48, 87, 88 };

#define NUM_Y_LEDS 6
uint8_t y_leds[] = { 27, 28, 67, 68, 107, 108 };

#define NUM_SOLDER_LEDS 0
uint8_t solder_leds[] = { 25, 108, 28, 65, 68, 105 };


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // Only 1 leds object
  FastLED.setBrightness( BRIGHTNESS );
}

void loop() {

  // Blacken all LEDS
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(0,0,0);  // Set all LEDS to Black
  }
  
  // Light Green the start, end, and tips
  for (uint8_t i = 0; i < NUM_TIP_LEDS; i++) {
    leds[tip_leds[i]] = CRGB(0,255,0);  // Green
  }

  // Light Blue the y-junction LEDS
  for (uint8_t i = 0; i < NUM_Y_LEDS; i++) {
    leds[y_leds[i]] = CRGB(0,0,255);  // Blue
  }

  // Light Red the solder LEDS
  for (uint8_t i = 0; i < NUM_SOLDER_LEDS; i++) {
    leds[solder_leds[i]] = CRGB(255,0,0);  // Red
  }
  
  FastLED.show();  // Update the display

  delay(DELAY_TIME);
}
