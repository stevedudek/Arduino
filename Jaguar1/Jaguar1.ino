#include <TimerOne.h>
#include <LPD6803.h>
//#include "LPD6803.h"


//
//  JAGUAR hat
//
//  Just 4 full-color LEDs on a hat
//
//  The lights are chained into a strip
//
/*****************************************************************************/
//
// Set Up Pins
//

// 2 pins for clock and data of lights
// 
#define dataPin  9      // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 4    // 25 lights in the strand
// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// Timer 1 is also used by the strip to send pixel clocks

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
LPD6803 strip = LPD6803(numLights, dataPin, clockPin);

// Hat states

uint8_t topColor = 50;     // Starting top-teeth color in 'int' form
uint8_t botColor = 50;     // Starting bottom-teeth color in 'int' form

#define MaxColor 96     // Maximum color number on the wheel

void setup() {

  // The Arduino needs to clock out the data to the pixels
  // this happens in interrupt timer 1, we can change how often
  // to call the interrupt. setting CPUmax to 100 will take nearly all all the
  // time to do the pixel updates and a nicer/faster display, 
  // especially with strands of over 100 dots.
  // (Note that the max is 'pessimistic', its probably 10% or 20% less in reality)
  
  strip.setCPUmax(50);  // start with 50% CPU usage. up this if the strand flickers or is slow

  //not sure if this is necessary but try it anyway
  randomSeed(analogRead(0));
  
  for (int i=0; i < numLights; i++) current_frame[i] = 0; // clear display
    
  // Start up the LED counter
  strip.begin();

  // Update the strip, to start they are all 'off'
  clear();
  strip.show();
}


void loop() {
  
  // 4 stages for lights
  //
  // 1) Lights slowly turn on
  // 2) Lights stay on
  // 3) Lights slowly fade off
  // 4) Lights stay off
  
  // Pick colors for top and bottom teeth
  
  uint8_t tempcolor1 = random(MaxColor); // Pick a random color 1
  uint8_t tempcolor2 = random(MaxColor); // Pick a random color 2
  
  switch(random(6)) {
    case 0:                    // All teeth set to tempcolor1
      topColor = tempcolor1;
      botColor = tempcolor1;
      break;
    case 1:                    // All teeth set to tempcolor2
      topColor = tempcolor1;
      botColor = tempcolor1;
      break;
    case 2:                    // Top and bottom teeth are different colors
      topColor = tempcolor1;
      botColor = tempcolor2;
      break;
    case 3:                    // Top and bottom teeth are different colors
      topColor = tempcolor2;
      botColor = tempcolor1;
      break;  
    case 4:                    // Only top teeth are on
      topColor = tempcolor1;
      botColor = 0;
      break;
    case 5:                    // Only bottom teeth are on
      topColor = 0;
      botColor = tempcolor2;
      break;  
  }
  SetTopTeeth(topColor);
  SetBottomTeeth(botColor); 
  
  morph_frame(random(5,20)*1000);    // 1) Turn lights on slowly
  delay(random(20,50)*1000);         // 2) Keep lights on for a while
  clearWithFade(random(5,20)*1000);  // 3) Turn lights off slowly
  delay(random(5,20)*1000);          // 4) Keep lights off for a while
}

/*
 * set all cells to black but don't call show yet
 * ignores buffering
 */
void clear() {
  for (int i=0; i<numLights; i++) {
    strip.setPixelColor(i, Color(0,0,0));
    setPixelColor(i, Color(0,0,0));
  }
}

/*
 * Fades lights to black over fade time
 */
void clearWithFade(long fadeTime) {
  for (int i=0; i<numLights; i++) {
    setPixelColor(i, Color(0,0,0));
  }
  morph_frame(fadeTime);
}

//
// Set Top Teeth
//
// Sets the two top teeth to the given color

void SetTopTeeth (uint8_t color) {
  if (color) {
    setPixelColor(1, Wheel(color));
    setPixelColor(2, Wheel(color));
  } else {
    setPixelColor(1, Color(0,0,0));
    setPixelColor(2, Color(0,0,0));
  }
}

//
// Set Bottom Teeth
//
// Sets the two bottom teeth to the given color

void SetBottomTeeth (uint8_t color) {
    if (color) {
    setPixelColor(0, Wheel(color));
    setPixelColor(3, Wheel(color));
  } else {
    setPixelColor(0, Color(0,0,0));
    setPixelColor(3, Color(0,0,0));
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t frame, uint8_t division, uint8_t wait) {
  int i;
  for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
     strip.setPixelColor(i, Wheel(((i*24*division / strip.numPixels()) + frame) % 96) );
  }  
  strip.show();   // write all the pixels out
  delay(wait);
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint16_t c, uint8_t wait) {
  int i;
  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
  }  
  strip.show();
  delay(wait);
}

/* Helper functions */

// Create a 15 bit color value from R,G,B
unsigned int Color(byte r, byte g, byte b)
{
  //Take the lowest 5 bits of each value and append them end to end
  return( ((unsigned int)g & 0x1F )<<10 | ((unsigned int)b & 0x1F)<<5 | (unsigned int)r & 0x1F);
}
  
//Input a value 0 to 127 to get a color value.
//The colours are a transition r - g -b - back to r
unsigned int Wheel(uint8_t WheelPos)
{
  byte r,g,b;
  switch(WheelPos >> 5)
  {
    case 0:
      r=31- WheelPos % 32;   //Red down
      g=WheelPos % 32;      // Green up
      b=0;                  //blue off
      break; 
    case 1:
      g=31- WheelPos % 32;  //green down
      b=WheelPos % 32;      //blue up
      r=0;                  //red off
      break; 
    case 2:
      b=31- WheelPos % 32;  //blue down 
      r=WheelPos % 32;      //red up
      g=0;                  //green off
      break; 
  }
  return(Color(r,g,b));
}

//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code from Greg and Robie
//

void draw_frame(int delay_ms) {
  for (int i = 0; i < numLights; i++) {
    strip.setPixelColor(i, next_frame[i]);
    current_frame[i] = next_frame[i];
  }
  strip.show();
  delay(delay_ms);
}

void draw_frame() {
  draw_frame(250);
}

#define STEP_LENGTH 10

void morph_frame(int msec) {
  int steps = msec / STEP_LENGTH;
  int deltas[numLights][3];
  for (int i = 0; i < numLights; i++) {
    for (int j = 0; j < 3; j++) {
      uint32_t old_color = (current_frame[i] >> (j * 5)) & 0x1F; 
      uint32_t new_color = (next_frame[i] >> (j * 5)) & 0x1F;
      deltas[i][j] = (int)new_color - (int)old_color;
    }
  }

  for (int t = 1; t < steps; t++) {
    for (int i = 0; i < numLights; i++) {
      uint32_t color = 0;
      for (int j = 0; j < 3; j++) {
        uint32_t old_color = (current_frame[i] >> (j * 5)) & 0x1F;
        color |= ((old_color + (deltas[i][j] * t / steps)) & 0x1F) << (5 * j);
      }
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(STEP_LENGTH);
  }
  for (int i = 0; i < numLights; i++) {
    current_frame[i] = next_frame[i];
    strip.setPixelColor(i, current_frame[i]);
  }
  strip.show();
  delay(STEP_LENGTH);
}

void setPixelColor(int pos, uint32_t color) {
  next_frame[pos] = color;
}
