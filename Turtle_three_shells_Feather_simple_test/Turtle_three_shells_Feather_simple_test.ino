#include <FastLED.h>

//
//  Turtle! - 29 Lights in a hexagonal grid
//
//  3 Turtle Shells with a different wiring layout
//
//  6/15/21 - On the Adafruit Feather
//
//  Can I prevent flicker wobble?
//

uint8_t BRIGHTNESS = 255;  // (0-255)

#define DELAY_TIME  12  // in milliseconds

#define DATA_PIN 0  //  11 // 7 // data = 7 (Teensy) or 0 (Feather)
#define CLOCK_PIN 2  //  13 // 8 // clock = 8 (Teensy) or 2 (Feather)

#define NUM_LEDS 29

#define ACTUAL_LEDS 37  // There are 8 dummy spacer LEDs

CRGB leds[ACTUAL_LEDS];  // The Leds themselves

uint8_t color = 0;
uint8_t cycle = 0;

const uint8_t rewire[] PROGMEM = {
  30, 26, 27,  0,  1,
   3,  2, 25, 24, 32,
  33, 22, 23,  4,  5,
   7,  6, 21, 20, 35,
  36, 18, 19,  8,  9,
  10, 17, 16, 15
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS).setCorrection(TypicalLEDStrip);  // only use of ACTUAL_LEDS
  FastLED.setBrightness( BRIGHTNESS );
}

//
// loop
//
void loop() {
  if (cycle % 10 == 0) {
    CHSV chsv_color = CHSV(color, 255, 255);
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      leds[pgm_read_byte_near(rewire + i)] = chsv_color;
    }
    FastLED.show();  // Update the display
    color++;
  }
  cycle++;
  delay(DELAY_TIME);
}
