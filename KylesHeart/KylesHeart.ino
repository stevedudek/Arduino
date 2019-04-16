
#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Kyle's Heart - 50 lights that pulse red
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights.
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 1      // number of lights

#define MIN_DELAY  20    // Fastest pulse time

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

float clock, clock_adjust;

//
// Setup
//

void setup() {
  
  clock = 0;
  
  // Start up the LED counter
  strip.begin();
  
  //Serial.begin(9600);
}


void loop() {
  
  int intensity = 127;//calcIntense(clock);  // 0 - 255 value;
  
  for (int i = 0; i < numLights; i++) {
    if (i % 2) strip.setPixelColor(i, Color(intensity, 0, 0));
    else strip.setPixelColor(i, Color(255 - intensity, 0, 0));
  }
  
  strip.show();
  
  clock_adjust = (analogRead(A1) / 256.0) + 0.2;
  clock += clock_adjust;  // 0.2 - 4 ; adjust this with a pot
  if (clock > 360) clock -= 360;
  
  delay(MIN_DELAY);
}

//
// calcIntense
//
// Figure out how bright (0 - 255) to make a light based on the clock time
// and the state of the potentiometer

byte calcIntense(float timer) {
  float clock_angle = timer / (2 * 3.14159);
  return (sin(clock_angle) + 1) * (255/2.0);
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
