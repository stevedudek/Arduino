#include "FastLED.h"

//
//  Antlers - a rack of 18 LEDS
//
//  7/20/16
//
//  Fast LED control
//
/*****************************************************************************/
//
//  Set Up Pins
//
//  2 pins for clock and data of lights.
// 
#define dataPin 0 // 7 // 9       // 'yellow' wire
#define clockPin 2 // 8 // 8      // 'green' wire

#define numLights 18

// Light colors

#define COLOR 60
#define BRIGHTNESS 255
#define YELLOW    20  // Yellow = 20
#define START_BRIGHT 255  // 256 = Full brightness, 0 = BLACK

#define FLICKER_CHANCE 10000 // one-in chance to start flicking
#define BURST_CHANCE 100 // one-in chance a flicker is a burst
#define MIN_SPEED 10
#define MAX_SPEED 100

// framebuffers
int intense_buffer[numLights][2];    // 0 = color, 1 = brightness
int speed_buffer[numLights][2];  // 0 = color, 1 = brightness

int START_VALUE[2] = { 20, 50 };  // color, brightness
int MAX_VALUE[2] = { 30, 256 };  // color, brightness
int MIN_VALUE[2] = { -100, 10 }; // color, brightness

// Set up the Fast LEDs
CRGB strip[numLights];



//
// Setup
//
void setup() {
  
  Serial.begin(9600);
  Serial.println("Start");
  
  randomSeed(analogRead(0));
  
  // Start up the LED counter
  FastLED.addLeds <WS2801, dataPin, clockPin> (strip, numLights);
  
  // Initialize the strip, to start they are all 'off'
  
  setUpLights();
  FastLED.show();
}

void loop() { 
   
  delay(20);   // The only delay!
  
  for (int type = 0; type < 2; type++) {
    for (int i = 0; i < numLights; i++) {
      updateLight(i, type);
    }
  }
}

//
// updateLight
//
void updateLight(int i, int type) {
  int spd = get_speed(i, type);
  int value = get_value(i, type);
  int new_value = value + spd;
  
  if (spd > 0) {  // Going up
    if (new_value > MAX_VALUE[type]) {  // Above maximum
      set_value(i, type, MAX_VALUE[type]);
      spd = spd * -1;
      set_speed(i, type, spd);
      return;
    }
    if (value < START_VALUE[type] && new_value >= START_VALUE[type]) {  // Back to starting point
      set_value(i, type, START_VALUE[type]);
      set_speed(i, type, 0);
      return;
    }
    set_value(i, type, new_value);
    return;
  }
  
  if (spd < 0) {  // Going down
    if (new_value < MIN_VALUE[type]) {  // Above maximum
      set_value(i, type, MIN_VALUE[type]);
      spd = spd * -1;
      set_speed(i, type, spd);
      return;
    }
    if (value > START_VALUE[type] && new_value <= START_VALUE[type]) {  // Back to starting point
      set_value(i, type, START_VALUE[type]);
      set_speed(i, type, 0);
      return;
    }
    set_value(i, type, new_value);
    return;
  }
  
  if (spd == 0) {
    if (random(FLICKER_CHANCE) == 0) {  // Start a new flicker?
      int new_speed = random(MIN_SPEED, MAX_SPEED);
      if (random(2) == 0) {
        new_speed *= -1;
      }
      if (random(BURST_CHANCE) == 0) {  // With all lights?
        for (int j = 0; j < numLights; j++) {
          set_speed(j, type, new_speed);  // Yes, all
        }
      } else {
        set_speed(i, type, new_speed);
      }
      return;
    }
  }
}

//
// Getters and setters
//
int get_speed(int light, int type) {
  return speed_buffer[light][type];
}

void set_speed(int light, int type, int spd) {
  speed_buffer[light][type] = spd;
}

int get_value(int light, int type) {
  return intense_buffer[light][type];
}

void set_value(int light, int type, int value) {
  intense_buffer[light][type] = value;
  if (type == 0) {
    if (value < 0) {
      value += 256;
    }
    strip[light].setHue(value);
    strip[light].fadeToBlackBy(256 - get_value(light, BRIGHTNESS));
    FastLED.show();
  } else {
    strip[light].setHue(get_value(light, COLOR));
    strip[light].fadeToBlackBy(256 - value);
    FastLED.show();
  }
}
  
//
// setUpLights - Initial configuration for the lights
// 
void setUpLights() {
  for (int type = 0; type < 2; type++) {
    for (int i = 0; i < numLights; i++) {
      set_value(i, type, START_VALUE[type]);
      set_speed(i, type, 0);
    }
  }
}
