#include <TimerOne.h>
#include "LPD6803.h"

//
//  Black Wings - No Keypad
//        
//
/*****************************************************************************/


// 2 pins for clock and data of lights
 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 21    // 20 lights total
#define wingLights 11   // 10 wing lights

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

#define numShows 5      // number of shows

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
LPD6803 strip = LPD6803(numLights, dataPin, clockPin);

// Spine states

int currColor  = 50;   // Foreground color
int currColor2 = 80;   // Background color
#define MaxColor 96    // Maximum color number on the wheel

// Shows

int show=0;    // default starting show
int f=0;       // indices to keep track of position within a show
int x=0;       // another counter
int wait=6;    // index to keep track of delay time
#define maxWait 12     // Number of stored delay times 

void setup() {
    
  // The Arduino needs to clock out the data to the pixels
  // this happens in interrupt timer 1, we can change how often
  // to call the interrupt. setting CPUmax to 100 will take nearly all all the
  // time to do the pixel updates and a nicer/faster display, 
  // especially with strands of over 100 dots.
  // (Note that the max is 'pessimistic', its probably 10% or 20% less in reality)
  
  strip.setCPUmax(50);  // start with 50% CPU usage. up this if the strand flickers or is slow
  
  // Start up the LED counter
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
  
  //Serial.begin(9600);  // For debugging
}


void loop() {

  // Pick a random show
  
  switch (random(6)) {
      
    case 0:
      randomfill(random(2,4)); 
      break;
    case 1:
      rainbowshow(random(100,500));
      break;
    case 2:
      randomcolors(random(20,40)*10000);
      break;
    case 3:
      twocolor(random(30,100));
      break;
    case 4:
      lightwave(random(2,10));
      break;
    case 5:
      tworays(random(5,10));
      break;
  }
  
  // Then off for a bit
  clear();
  delay(random(30)*1000);
  strip.show();
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
void clearWithFade(int fadeTime) {
  for (int i=0; i<numLights; i++) {
    setPixelColor(i, Color(0,0,0));
  }
  morph_frame(fadeTime);
}

/*
 * randomcolors: turns each pixel on to a random color
 */

void randomcolors(long time) {
  for (int i=0; i < numLights; i++) setPixelColor(i, Wheel(random(MaxColor)));
  morph_frame(1000);
  delay(time);
  clearWithFade(1000);
}

/*
 * twocolor: makes two color of rays that process
 */

void twocolor(int cycles) {
  int color1, color2;
  color1 = random(MaxColor);  // Set up color1
  color2 = (color1 + random(25,MaxColor/2)) % MaxColor;  // Set up color2
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < numLights; i++) {
      if (((i+(count%4))/2)%2) { strip.setPixelColor(i, Wheel(color1)); }
      else { strip.setPixelColor(i, Wheel(color2)); }
    }
    strip.show();
    delay(GetDelayTime(wait));
    
    // Possibly new delay
    if (!random(40)) { wait = random(0,maxWait); }
  }
}

/*
 * randomfill: randomly fills in pixels from blank to all on
 * then takes them away random until blank
 */
 
void randomfill(int cycles) {
  int i, j, save;
  byte shuffle[numLights];  // Will contain a shuffle of 0 to totPixels
  byte randcolor;
  
  for (int count = 0; count < cycles; count++) {
    randcolor = random(0, MaxColor); // pick a random starting color
    
    // Shuffle sort to determine order to turn on lights
    for (i=0; i < numLights; i++) shuffle[i] = i; // before shuffle
    for (i=0; i < numLights; i++) {  // here's position
      j = random(numLights);       // there's position
      save = shuffle[i];
      shuffle[i] = shuffle[j];     // first swap
      shuffle[j] = save;           // second swap
    }
    // Blank the screen
    clear();
    strip.show();
    delay(GetDelayTime(wait));
    
    // Turn on lights one at a time
    for (i=0; i < numLights; i++) {
      setPixelColor(shuffle[i], Wheel(randcolor));
      morph_frame(GetDelayTime(wait));
    }
    // Turn off lights one at a time
    for (i=0; i < numLights; i++) {
      setPixelColor(shuffle[numLights-i-1], Color(0,0,0));
      morph_frame(GetDelayTime(wait));
    }
    // Possibly new delay
    if (!random(2)) { wait = random(0,maxWait); }
  }
}  

/*
 * rainbowshow: fills the spines with a gradient rainbow
 * over time, the starting color changes
 */
 
void rainbowshow(int cycles) {
  int frame = random(MaxColor);  // Starting position for pixel 0
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < numLights; i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel(((i*256 / numLights) + frame) % 256) );
    }  
    strip.show();   // write all the pixels out
    delay(50);
    if (frame+=15 > MaxColor) frame = frame - MaxColor;
  }
}

/*
 * lightwave - Just one pixel traveling
 */
 
void lightwave(int cycles) {
  int randcolor = random(MaxColor); // pick a random starting color
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < numLights; i++) {
      for (int j=0; j < numLights; j++) {
        if (i == j) { 
          setPixelColor(j, Wheel(randcolor));
        } else {
          setPixelColor(j, Color(0,0,0));
        }
      }
      morph_frame(GetDelayTime(wait));
      if (!random(100)) { if (wait++ > maxWait) wait = 0; }
      if (!random(10)) {
        if (randcolor+=5 > MaxColor) randcolor = randcolor - MaxColor;
      }
    }
  }
}

//
// tworays - two colored rays, one from the left, one from the right
//           they fill in the wings and then disappear
//
 
void tworays(int cycles) {
  int i,j;
  int raydelay = random(50,500);    // pick a random speed
  int color1 = random(MaxColor);    // Set up color1
  int color2 = (color1 + random(25,MaxColor/2)) % MaxColor;  // Set up color2

  clearWithFade(100);  // blank the screen gracefully
  
  for (int count = 0; count < cycles; count++) {
  
    // Fill up the wings
    for (i=0; i < numLights/2; i++) {
      setPixelColor(i*2, Wheel(color1));
      setPixelColor(numLights - 2 - (i*2), Wheel(color2));
      
      morph_frame(raydelay);
    }
    setPixelColor(numLights-1, Wheel(color1));  // Catch the last light. Fix for odd number of lights
    morph_frame(raydelay);
    
    delay(5000);  // Brief pause with all the lights on
    
    // Clear the wings
    for (i=0; i < numLights/2; i++) {
      setPixelColor(i*2, Color(0,0,0));
      setPixelColor(numLights - 2 - (i*2), Color(0,0,0));
      
      morph_frame(raydelay);
    }
    setPixelColor(numLights-1, Color(0,0,0));  // Catch the last light. Fix for odd number of lights
    morph_frame(raydelay);
    
    color1 = (color1 + 25) % MaxColor;
  }
}
    

//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code from Greg and Robie
//

void draw_frame(int delay_ms) {
  for (int i = 0; i < numLights; i++) {
    strip.setPixelColor(i, next_frame[i] & 0x7fff);  // 15 bit
    current_frame[i] = next_frame[i];
  }
  strip.show();
  delay(delay_ms);
}

void draw_frame() {
  draw_frame(250);
}

#define FRAME_MSEC 100
#define MORPH_STEPS 25
#define STEP_LENGTH 10

void morph_frame(int msec) {
  int steps = msec / STEP_LENGTH;
  int deltas[numLights][3];
  for (int i = 0; i < numLights; i++) {
    for (int j = 0; j < 3; j++) {
      uint32_t old_color = (current_frame[i] >> (j * 5)) & 0x1f; 
      uint32_t new_color = (next_frame[i] >> (j * 5)) & 0x1f;
      deltas[i][j] = (int)new_color - (int)old_color;
    }
  }

  for (int t = 1; t < steps; t++) {
    for (int i = 0; i < numLights; i++) {
      uint32_t color = 0;
      for (int j = 0; j < 3; j++) {
        uint32_t old_color = (current_frame[i] >> (j * 5)) & 0x1f;
        color |= ((old_color + (deltas[i][j] * t / steps)) & 0x1f) << (5 * j);
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

void fill(uint32_t color) {
  for (int i = 0; i < numLights; i++) {
    setPixelColor(i, color);
  }
}

/*
 *  turnOffLights - Quick function that turns off all the lights
 */ 
void turnOffLights(uint8_t wait) {             // Turn all lights off
  colorWipe(Color(0, 0, 0), wait);
}

/*
 * LightUp - individual addresses lights via the shift registry
 */ 
/*
void LightUp(long data, uint16_t c, uint8_t wait) {
  int i;
  for (i=0; i < strip.numPixels(); i++) {
      if (bitRead(data, i)) { strip.setPixelColor(i, c); } else { strip.setPixelColor(i, Color(0, 0, 0)); }
  } 
  strip.show();
  delay(wait);
}
*/

/*
 * LightOneLight - Lights one light 'l' with a color 'c'
 *                 Lights are symmetric for each wing
 */ 
void LightOneLight(uint8_t l, uint16_t c, uint8_t wait) {
  int i;
  for (i=0; i < wingLights; i++) {
      if (i==l) {
        strip.setPixelColor(i, c);
        strip.setPixelColor(strip.numPixels()-1-i, c);
      } 
      else {
        strip.setPixelColor(i, Color(0, 0, 0));
        strip.setPixelColor(strip.numPixels()-1-i, Color(0, 0, 0));
     }
  } 
  strip.show();
  delay(wait);
}

/*
 * LightWave - Lights a selection
 */ 
void LightWave(uint8_t frame, uint8_t wavepos, uint16_t c, uint16_t c2, uint8_t wait) {
  int i;
  for (i=0; i < wingLights; i++) {
    if (i>(wingLights-wavepos-1) || i==frame) {
      strip.setPixelColor(i, c);
      strip.setPixelColor(strip.numPixels()-1-i, c);
    } 
    else {
      strip.setPixelColor(i, c2);
      strip.setPixelColor(strip.numPixels()-1-i, c2);
    }
  }
  strip.show();
  delay(wait);
}

/*
 * LightRunUp - Posts one frame of one or more lights running from bottom to top
 */

void LightRunUp(int frame, int numRunLights, uint16_t c, uint16_t c2, uint8_t wait) {
  int i;
  for (i=0; i < wingLights; i++) {
    if ((i>=frame && i<frame+numRunLights) || 
        (frame+numRunLights>wingLights && i<frame+numRunLights-wingLights)) {
           strip.setPixelColor(wingLights-i-1, c);
           strip.setPixelColor(numLights-wingLights+i, c);
    } 
    else {
           strip.setPixelColor(wingLights-i-1, c2);
           strip.setPixelColor(numLights-wingLights+i, c2);
    }    
  }
  strip.show();
  delay(wait);
}

/*
 * One Light Run Up - One light travels from top to bottom in current color
 */

void oneLightRunUp(uint16_t c, uint8_t wait) {
  int i,j;
  for (i=0; i < wingLights; i++) {
    for (j=0; j < wingLights; j++) {
      if (i==j) {
        strip.setPixelColor(wingLights-j-1, c);
        strip.setPixelColor(numLights-wingLights+j, c);
      }
      else {
        strip.setPixelColor(wingLights-j-1, Color(0, 0, 0));
        strip.setPixelColor(numLights-wingLights+j, Color(0, 0, 0));
      }
    }
    strip.show();
    delay(wait);
  }
}

/*
 * One Light Run Down - One light travels from top to bottom in current color
 */

void oneLightRunDown(uint16_t c, uint8_t wait) {
  int i,j;
  for (i=0; i < wingLights; i++) {
    for (j=0; j < wingLights; j++) {
      if (i==j) { 
        strip.setPixelColor(j, c);
        strip.setPixelColor(strip.numPixels()-1-j, c);
      } 
      else {
        strip.setPixelColor(j, Color(0, 0, 0));
        strip.setPixelColor(strip.numPixels()-1-j, Color(0, 0, 0));
      }
    }
    strip.show();
    delay(wait);
  }
}

/*
 * Get Delay Time - Returns a delay time from an array.
 */
 
int GetDelayTime(int wait) {
  int DelayValues[maxWait] = { 10, 20, 40, 60, 80, 100, 150, 200, 300, 500, 750, 1000 };
  if (wait < maxWait) { return (DelayValues[wait]); } else { return (DelayValues[maxWait]); }
}
  
void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 96 * 3; j++) {     // 3 cycles of all 96 colors in the wheel
    for (i=0; i < wingLights; i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 96));
      strip.setPixelColor(strip.numPixels()-1-i, Wheel( (i + j) % 96));
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t frame, uint8_t division, uint8_t wait) {
  int i;
  for (i=0; i < wingLights; i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
     strip.setPixelColor(i, Wheel(((i*24*division / strip.numPixels()) + frame) % 96) );
     strip.setPixelColor(strip.numPixels()-1-i, Wheel(((i*24*division / strip.numPixels()) + frame) % 96) );
  }  
  strip.show();   // write all the pixels out
  delay(wait);
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint16_t c, uint8_t wait) {
  int i;
  
  for (i=0; i < wingLights; i++) {
      strip.setPixelColor(i, c);
      strip.setPixelColor(strip.numPixels()-1-i, c);
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

//Input a value 0 to 96 to get a color value.
//The colours are a transition r - g -b - back to r
unsigned int Wheel(byte WheelPos)
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

/*
 * Converts 4 bytes into a long
 */
 /*
long BytesToLong (byte byte1, byte byte2, byte byte3, byte byte4) {
  return ( (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4 );
}
*/
      
