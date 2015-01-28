//
// Roman Hat
//
// 8/18/14
//
// Fast LED control
//
// No xBee

#include "FastLED.h"

//
// Set Up Pins
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 9      // number of panel lights

// framebuffers
byte current_frame[numLights];
byte next_frame[numLights];

// Set up the Fast LEDs
CRGB strip[numLights];

// Hat colors

#define BLACK  25  // Somewhat arbitrary
#define WHITE  75  // Somewhat arbitrary
#define MaxColor 255   // Colors are 0-255
byte foreColor =  0;    // Starting foreground color
byte backColor = 50;   // Starting background color

// Shows

int wait=6;    // index to keep track of delay time
#define maxWait 12     // Number of stored delay times 

//
// Setup
//

void setup() {

  //Serial.begin(9600);
  //Serial.println("Start");
    
  // Initialize the LED strip
  FastLED.addLeds <WS2801, dataPin, clockPin> (strip, numLights);
}


void loop() {
   
  // Start with a fun random show
  
  switch(random(6)) {
    
    case 0:    
      allOn(random(20,100));
      break;
    case 1:
      randomfill(random(5,20));
      break;
    case 2:
      twocolor(random(1,30)*10000);
      break;
    case 3:
      rainbowshow(random(100,500));
      break;
    case 4:
      lightwave(random(10,50));
      break;
    case 5:
      lightrunup(random(10,40));
      break;
  }
  
  // Then a break with all the lights off
  clear();
  FastLED.show();
  delay(random(1,4)*10000);  // Hopefully 10-1200 seconds
}

/*
 * set all cells to black but don't call show yet
 * ignores buffering
 */
void clear() {
  for (int i=0; i<numLights; i++) {
    strip[i] = CRGB::Black;
    setPixel(i, BLACK);
  }
}

/*
 * Fades lights to black over fade time
 */
void clearWithFade(int fadeTime) {
  for (int i=0; i<numLights; i++) {
    setPixel(i, BLACK);
  }
  morph_frame(fadeTime);
}

/*
 * All On
 *
 * Simply turns all the pixes on to a random color and then cycles
 *
 */
 
void allOn (int cycles) {
   for (int count = 0; count < cycles; count++) {
    for (int i=0; i < numLights; i++) {
      setPixel(i, foreColor);
    }
    morph_frame(GetDelayTime(wait));
  
    if (!random(100)) { 
      if (wait++ > maxWait) wait = 0;
    }
    foreColor = IncColor(foreColor, 5);
  }
  clearWithFade(1000);
}

/*
 * randomfill: randomly fills in pixels from blank to all on
 * then takes them away random until blank
 */
 
void randomfill(int cycles) {
  int i, j, save;
  byte shuffle[numLights];  // Will contain a shuffle of 0 to totPixels
  
  for (int count = 0; count < cycles; count++) {
    
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
    FastLED.show();
    delay(GetDelayTime(wait));
    
    // Turn on lights one at a time
    for (i=0; i < numLights; i++) {
      setPixel(shuffle[i], foreColor);
      morph_frame(GetDelayTime(wait));
    }
    // Turn off lights one at a time
    for (i=0; i < numLights; i++) {
      setPixel(shuffle[numLights-i-1], BLACK);
      morph_frame(GetDelayTime(wait));
    }
    // Possibly new delay
    if (!random(2)) { wait = random(0,maxWait); }
  }
}  

/*
 * randomcolors: turns each pixel on to a random color
 */

void randomcolors(long time) {
  for (int i=0; i < numLights; i++) setPixel(i, random8());
  morph_frame(1000);
  delay(time);
  clearWithFade(1000);
}

/*
 * twocolor: alternates the color of pixels between two colors
 */

void twocolor(long time) {
  for (int i=0; i < numLights; i++) {
    if (i%2) { setPixel(i, foreColor); }
    else { setPixel(i, backColor); }
  }
  morph_frame(1000);
  delay(time);
  clearWithFade(1000);
  backColor = IncColor(backColor, 10);
}

/*
 * rainbowshow: fills the spines with a gradient rainbow
 * over time, the starting color changes
 */
 
void rainbowshow(int cycles) {

  for (int count = 0; count < cycles; count++) {
    //int diff = abs(foreColor - backColor);
    int diff = MaxColor;
    for (int i=0; i < numLights; i++) {
      setPixel(i, map(i, 0, numLights, 0, 255) + foreColor); 
    }  
    morph_frame(GetDelayTime(wait));
    foreColor = IncColor(foreColor, 2);
    
    // Possibly new delay
    if (!random(10)) { if (wait++ > maxWait) wait = 0; }
  }
  clearWithFade(1000);
}

/*
 * lightwave - Just one traveling pixel  
 */
 
void lightwave(int cycles) {
 
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < numLights; i++) {
      for (int j=0; j < numLights; j++) {
        if (i == j) setPixel(j, foreColor);
        else setPixel(j, BLACK);
      }
      morph_frame(GetDelayTime(wait));
      if (!random(100)) wait = (wait + 1) % maxWait;
      if (!random(10)) foreColor = IncColor(foreColor, 2);
    }
  }
}

/*
 * lightrunup. Show where the lights fill up one at time
 */
 
void lightrunup(int cycles) {
  int i,j,k,count;
  
  for (count = 0; count < cycles; count++) {
    for (k=numLights; k > 0; k--) {
      for (i=0; i < k; i++) {
        for (j=0; j < k; j++) {
          if (i == j) setPixel(j, foreColor);
          else setPixel(j, BLACK);
        }
        morph_frame(GetDelayTime(wait));
        if (!random(100)) wait = (wait + 1) % maxWait;
        if (!random(10)) foreColor = IncColor(foreColor,-2);
      }
    }
  }
  clearWithFade(1000);
}
    
//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code from Greg and Robie
//

#define FRAME_MSEC 100
#define MORPH_STEPS 25
#define STEP_LENGTH 10

void morph_frame(int msec) {
  int steps = msec / STEP_LENGTH;

  for (int t = 1; t < steps; t++) {
    for (int i = 0; i < numLights; i++) {
      if (current_frame[i] != next_frame[i]) {
         calcPixelColor(i, current_frame[i], next_frame[i], (float)t/steps);
      }
    }
    FastLED.show();
    delay(STEP_LENGTH);
  }
  for (int i = 0; i < numLights; i++) {
    if (current_frame[i] != next_frame[i]) {
      sendPixelColor(i, current_frame[i]);
    }
  }
  FastLED.show();
  delay(STEP_LENGTH);
}

void setPixel(byte pos, byte color) {
  next_frame[pos] = color;
}

void sendPixelColor(byte pos, byte color) {
  switch(color) {
    case BLACK:
      strip[pos] = CRGB::Black;
      break;
    case WHITE:
      strip[pos] = CRGB::White;
      break;
    default:
      strip[pos].setHue(color); 
      break;
  }
}

void calcPixelColor(byte i, byte init, byte final, float amount) {
  if (amount == 0.0) {
    sendPixelColor(i, init);
    return;
  }
  if (amount == 1.0) {
    sendPixelColor(i, final);
    return;
  }
  if (final == BLACK) {
    fadePixel(i, init, amount);
    return;
  }
  if (init == BLACK) {
    fadePixel(i, final, 1.0 - amount);
    return;
  }
  morphColor(i, init, final, amount);
  return;
}

void fadePixel(byte i, byte color, float amount) {
  strip[i].setHue(color);
  CRGB hsv;
  CRGB rgb = strip[i];
  rgb2hsv_rainbow(rgb,hsv);
  hsv.v = dim8_video(amount * 255);
  strip[i]. = hsv;
}

void morphColor(byte i, byte init, byte final, float amount) {
  CHSV c1, c2;
  c1.setHue(init);
  c2.setHue(final);
  strip[i].setHSV( lerp8by8(c1.h,c2.h,amount), 
                    lerp8by8(c1.s,c2.s,amount)
                     lerp8by8(c1.v,c2.v,amount));
}

  
//
// IncColor
//
// Adds amount to color
// Corrects for out of bounds and BLACK and WHITE
byte IncColor(byte color, int amount) {
  byte value = color + amount;
  
  if (color == BLACK || color == WHITE) return color;
  if (value < 0) value += 255;
  if (value > 255) value -= 255;
  if value == BLACK value -= 1;
  if value == WHITE value += 1;
  return value;
}

/*
 * Get Delay Time - Returns a delay time from an array.
 */
 
int GetDelayTime(int wait) {
  int DelayValues[maxWait] = { 10, 20, 40, 60, 80, 100, 150, 200, 300, 500, 750, 1000 };
  if (wait < maxWait) { return (DelayValues[wait]); } else { return (DelayValues[maxWait]); }
}
