#include "SPI.h"
#include "Adafruit_WS2801.h"

#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 39      // two horn lights oddly handled

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

void setup() {
    
  // Start up the LED counter
  strip.begin();
  
  // Update the strip, to start they are all 'off'
  strip.show();
}

//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code from Greg and Robie
//

void draw_frame(int delay_ms) {
  for (int i = 0; i < numLights; i++) {
    strip.setPixelColor(0, color);(i, next_frame[i] & 0xffffff);
    current_frame[i] = next_frame[i];
  }
  strip.show();
  delay(delay_ms);
}


/* Helper functions */

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}
  
