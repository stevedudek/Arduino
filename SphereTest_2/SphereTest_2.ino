#include <FastLED.h>

//
//  Sphere Test
//
//  1/28/19
//
//  Includes extended colors
//  Removes xBee hardware
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights.
// 
#define DATA_PIN 8       // 'yellow' wire
#define CLOCK_PIN 7      // 'green' wire

#define NUM_LEDS 144

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

// Shows

int show = 0;       // Starting show

int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)

CRGB leds[NUM_LEDS];  // The Leds themselves

uint8_t core = random(NUM_LEDS);

//
// Setup
//

void setup() {

  delay( 3000 ); // power-up safety delay
  
  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( 255 );

  // Initialize the strip, to start they are all 'off'
  
  for (int i=0; i<NUM_LEDS; i++) {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();  // Update the display
}

void loop() { 
   
  delay(30);   // The only delay!
  
  // Check if the lights need updating
  
  test_show();
  FastLED.show();  // Update the display
  
  cycle = cycle + 1;

}

//
// test_show
// 
void test_show() {
  uint8_t band = sin8_C(cycle);
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t dist = get_distance(i, core);
    leds[i] = CHSV(band - dist, 255, 255);
//    leds[i] = CHSV(60, 255, band - dist);
  }
}

//
// test_show
// 
void test_show_2() {
  for (int i=0; i < NUM_LEDS; i++) {
    if (i % 12 == 0) {
      leds[i] = CRGB(0, 255, 0);
    } else {
      leds[i] = CRGB(0, 0, 0);
    }
  }
}

//
// get_distance
//
uint8_t get_distance(uint8_t c1, uint8_t c2) {
  uint16_t d = sqrt(get_diff_sq_by_index(c1, c2, 0) +
                    get_diff_sq_by_index(c1, c2, 1) +
                    get_diff_sq_by_index(c1, c2, 2));
  return map(d, 0, 262, 0, 255);
}

uint16_t get_diff_sq_by_index(uint8_t c1, uint8_t c2, uint8_t index) {
  return sq(get_coord_value(c2, index) - get_coord_value(c1, index));
}

//
// get_coord_value
//
uint8_t get_coord_value(uint8_t coord, uint8_t index) {
  return pgm_read_byte_near(COORDS + (coord * 3) + index);
}

