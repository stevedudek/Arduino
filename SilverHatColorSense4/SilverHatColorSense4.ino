//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include "Adafruit_WS2801.h"

//
//  Siler Hat! - 35 Lights. This version does not use a keypad
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights. Reversed from usual!
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 35      // number of spikes on the hat

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Color Sensor

#define colorbuttonPin 11 // For color button. Connect other wire to GND

unsigned long OldButtonTime; // Timing for color button
unsigned long NewButtonTime; // Timing for color button
#define debouncetime 1000    // 1s?
#define keepcolortime 50000  // 50s? How long to keep sensed color
boolean freecolor = true;    // If a color is sensed, flag goes false

byte gammatable[256];

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

// Silver Hat colors

int color1  = 85;     // Starting foreground color
int color2 = 170;     // Starting background color
#define MaxColor 255  // Colors go from 0 to 255 (8-bit)

uint32_t bigcolor;    // Sensed color (24-bit)

// Shows

int f=0;       // indices to keep track of position within a show
int x=0;       // another counter
int temp;      // temporary place holder
int wait=6;    // index to keep track of delay time
#define maxWait 12     // Number of stored delay times 
  
// Order of spines
byte SpineOrder[35] = {
        10,
   11,  20,   9,
     19,   21,
  12,18,23,22,8,
  13,17,24,25,7,
  14,16,27,26,6,
     15,28,5,
        29,
       30,4,
       31,3,
       32,2,
       33,1,
       34,0,
};

// Tail pattern
byte Tails[35] = {
         0,
    0,   0,   0,
      0,    0,
   0, 0, 0, 0,0,
   0, 0, 0, 0,0,
   0, 0, 0, 0,0,
      0, 0, 0,
        1,
       2,2,
       3,3,
       4,4,
       5,5,
       6,6,
};

// Hood Ring
byte Hood[35] = {
         1,
    2,   0,   2,
      0,    0,
   3, 0, 0, 0,3,
   4, 0, 0, 0,4,
   5, 0, 0, 0,5,
      6, 0, 6,
         0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
};

// Stripe Patten
byte Stripes[35] = {
         1,
    0,   2,   0,
      0,    0,
   0, 0, 3, 0,0,
   0, 0, 4, 0,0,
   0, 0, 5, 0,0,
      0, 6, 0,
         7,
        8,8,
        9,9,
        10,10,
        11,11,
        12,12,
};

void setup() {
  Serial.begin(9600);
  //not sure if this is necessary but try it anyway
  randomSeed(analogRead(0));
  
  // Initialize the button color sensor
  pinMode(colorbuttonPin, INPUT);
  digitalWrite(colorbuttonPin, HIGH);  // turn on internal pull-up resistor
  
  OldButtonTime = 0;
  NewButtonTime = 0;
  
  // Set up Gamma table for the color sensor
  // it helps convert RGB colors to what humans see
  for (int i=0; i<256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;
      
    //gammatable[i] = 255 - x;
    gammatable[i] = x;      
  }
  
  for (int i=0; i < numLights; i++) current_frame[i] = 0; // clear display
  
  // Start up the LED counter
  strip.begin();

  // Update the strip, to start they are all 'off'
  clear();
  strip.show();
}

void loop() {
   
  // Start with a fun random show
  
  switch(0) {
  //switch(random(10)) {
    case 0:    
      allOn(random(20,100));
      break;
    case 1:
      randomfill(random(3,10)); 
      break;
    case 2:
      randomcolors(random(5,30)*10000);
      break;
    case 3:
      twocolor(random(1,30)*10000);
      break;
    case 4:
      rainbowshow(random(100,500));
      break;
    case 5:
      lightwave(random(2,10));
      break;
    case 6:
      lighttails(random(1,30)*10000);
      break;
    case 7:
      lighthood(random(1,30)*10000);
      break;
    case 8:
      centergradient(random(1,30)*10000);
      break;
    case 9:
      centerpulse(random(5,40));
      break;
  }
  
  // Then a break with all the lights off, if the color sensor if off
  if(freecolor) {
    clear();
    strip.show();
    ButtonDelay(random(1,4)*10000);  // Hopefully 10-1200 seconds 
  } 
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
 * All On
 *
 * Simply turns all the pixes on to a random color and then cycles
 *
 */
 
void allOn (int cycles) {
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      setPixelColor(i, MainColor());
    }
    morph_frame(GetDelayTime(wait));
  
    if (!random(100)) { if (wait++ > maxWait) wait = 0; }
    color1 = (color1 + 5) % MaxColor;  
  }
  clearWithFade(1000);
}

/*
 * randomcolors: turns each pixel on to a random color
 */

void randomcolors(long time) {
  for (int i=0; i < numLights; i++) setPixelColor(i, Wheel(random(MaxColor)));
  morph_frame(1000);
  ButtonDelay(time);
  clearWithFade(1000);
}

/*
 * twocolor: turns each pixel on to either of two random colors
 */

void twocolor(long time) {
  
  for (int i=0; i < numLights; i++) {
    if (random(2)) { setPixelColor(i, MainColor()); }
    else { setPixelColor(i, Wheel(color2)); }
  }
  morph_frame(1000);
  ButtonDelay(time);
  clearWithFade(1000);
  color2 = (color2 + 20) % MaxColor;
}

/*
 * lighttails: turn on the tail lights to a gradient
 */
 
void lighttails(long time) {
  uint32_t tempcolor;
  
  int spacing = random(2,20);   // Spacing between colors
  
  for (long t=time; t=t-500; t>0) {
    for (int i=0; i < numLights; i++) {
      if(freecolor) { tempcolor = Wheel((color1 + (spacing*Tails[i]) % MaxColor)); }
      else { tempcolor = bigcolor; }
      
      if (!Tails[i]) { strip.setPixelColor(SpineOrder[i], Color(0,0,0)); }
      else { strip.setPixelColor(SpineOrder[i], tempcolor); }
    }
    strip.show();
    ButtonDelay(500);
    if (!random(5)) color1 = (color1 + random(5,20)) % MaxColor;
  }
}

/*
 * lighthood: ring the hood with one color; fill everything else with another color
 */
 
void lighthood(long time) {
  uint32_t tempcolor;
  
  int spacing = random(2,20);   // Spacing between colors
   
  for (long t=time; t=t-500; t>0) {
    for (int i=0; i < numLights; i++) {
      if(freecolor) { tempcolor = Wheel((color1 + (spacing*Hood[i]) % MaxColor)); }
      else { tempcolor = bigcolor; }
    
      if (!Hood[i]) { strip.setPixelColor(SpineOrder[i], Wheel(color2)); }
      else { strip.setPixelColor(SpineOrder[i], tempcolor); }
    }
    strip.show();
    ButtonDelay(500);
    if (!random(5)) color1 = (color1 + random(5,20)) % MaxColor;
  }
}

/*
 * centergradient: turn on the central spine with a light gradient
 */
 
void centergradient(long time) {
  uint32_t tempcolor;

  int spacing = random(2,6);   // Spacing between colors
  
  for (long t=time; t=t-500; t>0) {
    for (int i=0; i < numLights; i++) {
     if(freecolor) { tempcolor = Wheel((color1 + (spacing*Stripes[i]) % MaxColor)); }
     else { tempcolor = bigcolor; }
     
     if (!Stripes[i]) { strip.setPixelColor(SpineOrder[i], Color(0,0,0)); }
     else { strip.setPixelColor(SpineOrder[i], tempcolor); }
    }
    strip.show();
    ButtonDelay(500);
    if (!random(5)) color1 = (color1 + random(5,20)) % MaxColor;
  }
}

/*
 * centerpulse - One pixel travelling down the center spine
 */
 
void centerpulse(int cycles) {  
  for (int count = 0; count < cycles; count++) {
    for (int i=1; i < 13; i++) {  // Where we are on the center spine
      for (int j=0; j < strip.numPixels(); j++) {
        if (Stripes[j] == i) { setPixelColor(SpineOrder[j], MainColor()); }
        else { setPixelColor(SpineOrder[j], Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
      if (!random(100)) { if (wait++ > maxWait) wait = 0; }
      if (!random(5)) color1 = (color1 + random(5,20)) % MaxColor;
    }
  }
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
    strip.show();
    ButtonDelay(GetDelayTime(wait));
    
    // Turn on lights one at a time
    for (i=0; i < numLights; i++) {
      setPixelColor(shuffle[i], MainColor());
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
 
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel(((i*256 / strip.numPixels()) + color1) % 256) );
    }  
    strip.show();   // write all the pixels out
    ButtonDelay(50);
    if (!random(5)) color1 = (color1 + random(5,20)) % MaxColor;
  }
}

/*
 * visit each light in order and illuminate its neighbors
 * ignores buffering
 */
void unit_test() {
  
  for (int i=0; i<numLights; i++) {
    clear();
    strip.setPixelColor(SpineOrder[i], Color(255,0,0));
    strip.show();
    delay(500);    
  }
}

/*
 * lightwave - Just one traveling pixel
 */
 
void lightwave(int cycles) {
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      for (int j=0; j < strip.numPixels(); j++) {
        if (i == j) { setPixelColor(j, MainColor()); }
        else { setPixelColor(j, Color(0,0,0)); }
      }
      morph_frame(GetDelayTime(wait));
      if (!random(100)) { if (wait++ > maxWait) wait = 0; }
      if (!random(5)) color1 = (color1 + random(10,40)) % MaxColor;
    }
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
    strip.setPixelColor(i, next_frame[i] & 0xffffff);
    current_frame[i] = next_frame[i];
  }
  strip.show();
  ButtonDelay(delay_ms);
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
      uint32_t old_color = (current_frame[i] >> (j * 8)) & 0xff; 
      uint32_t new_color = (next_frame[i] >> (j * 8)) & 0xff;
      deltas[i][j] = (int)new_color - (int)old_color;
    }
  }

  for (int t = 1; t < steps; t++) {
    for (int i = 0; i < numLights; i++) {
      uint32_t color = 0;
      for (int j = 0; j < 3; j++) {
        uint32_t old_color = (current_frame[i] >> (j * 8)) & 0xff;
        color |= ((old_color + (deltas[i][j] * t / steps)) & 0xff) << (8 * j);
      }
      strip.setPixelColor(i, color);
    }
    strip.show();
    ButtonDelay(STEP_LENGTH);
  }
  for (int i = 0; i < numLights; i++) {
    current_frame[i] = next_frame[i];
    strip.setPixelColor(i, current_frame[i]);
  }
  strip.show();
  ButtonDelay(STEP_LENGTH);
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
 * Get Delay Time - Returns a delay time from an array.
 */
 
int GetDelayTime(int wait) {
  int DelayValues[maxWait] = { 10, 20, 40, 60, 80, 100, 150, 200, 300, 500, 750, 1000 };
  
  wait = wait % maxWait;  // Prevent going out of bounds
  
  return(DelayValues[wait]);
}

// ButtonDelay
//
// Runs the standard delay but also polls the button to see if it got pushed or
// whether to release the color flag

void ButtonDelay(long time) { 
  for (int i = 0; i < time; i++) {
    delay(1); // The delay part of the delay loop
    
    // Deal with color button
    
    NewButtonTime = millis();  // Take a time point
    
    if(digitalRead(colorbuttonPin) == LOW && (NewButtonTime > (OldButtonTime + debouncetime))) {
      bigcolor = ReadColorSensor();   // Update the sensed color
      freecolor = false;              // trap the main color
      OldButtonTime = NewButtonTime;  // Reset timer
      
      // Turns all the lights to the new color for 2 seconds
      
      for (int j=0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, bigcolor);
      }
      strip.show();
      delay(2000);
      return;   
    
    } else {  // Need to check whether to free up the sensed color
      if(!freecolor && (NewButtonTime > (OldButtonTime + keepcolortime))) {
        freecolor = true;                // free up the color
        color1 = MatchColor(bigcolor);   // revert to 255-bit color
      }
    }
  }
}
  
void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 96 * 3; j++) {     // 3 cycles of all 96 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 96));
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t frame, uint8_t wait) {
  int i;
  for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
     strip.setPixelColor(i, Wheel(((i*24 / strip.numPixels()) + frame) % 96) );
  }  
  strip.show();   // write all the pixels out
  ButtonDelay(wait);
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

// Main Color
//
// Returns the 24-bit main color
//
// If the color has been trapped by the sensor, returns the big 24-big color
// If the color is free to move, returns the processed 8-bit color

uint32_t MainColor() {
  if(freecolor) return(Wheel(color1)); else return (bigcolor);
}

// Read color sensor
//
// Reads the Adafruit color sensor. Returns a unit32_t color from the r,g,b values
uint32_t ReadColorSensor() {
  
  uint16_t clear, red, green, blue;
  
  // Poll the sensor
  tcs.setInterrupt(false);      // turn on LED
  delay(60);  // takes 50ms to read 
  tcs.getRawData(&red, &green, &blue, &clear);
  tcs.setInterrupt(true);  // turn off LED
  
  //Serial.print("C:\t"); Serial.print(clear);
  //Serial.print("\tR:\t"); Serial.print(red);
  //Serial.print("\tG:\t"); Serial.print(green);
  //Serial.print("\tB:\t"); Serial.print(blue);

  // Figure out some basic hex code for visualization
  uint32_t sum = red;
  sum += green;
  sum += blue;
  //sum = clear;
  float r, g, b;
  r = red; r /= sum;
  g = green; g /= sum;
  b = blue; b /= sum;
  r *= 256; g *= 256; b *= 256;
  //Serial.print("\t");
  //Serial.print((int)r, HEX); Serial.print((int)g, HEX); Serial.print((int)b, HEX);
  //Serial.println();

  //Serial.print(gammatable[(int)r]); Serial.print(" "); Serial.print(gammatable[(int)g]); Serial.print(" "); Serial.println(gammatable[(int)b]);

  return(Color(gammatable[(int)r], gammatable[(int)g], gammatable[(int)b]));
  //return(Color(r,g,b));
}

// MatchColor
//
// Try to turn a 24-bit color into the closest approximate 0-255 wheeled color
//
// Algorithm works by brute-force polling all possible 255 answers to determine which answer
// is closest to the 24-bit color

byte MatchColor(uint32_t inColor) {
  int colorDiff;
  int minDiff = 255+255+255;    // Current closest (minimum) difference
  byte bestColor = 0;  // Which of the 255 wheeled colors is currently the best
  
  for (int i=0; i <= MaxColor; i++) {  // Poll all 255 colors
    //Serial.print(i);
    //Serial.print(") ");
    //Serial.println(colorDiff);
    colorDiff = CompareColors(inColor, Wheel(i));  // How close is current color?
    if (colorDiff <= minDiff) {
        minDiff = colorDiff;  // Closest yet
        bestColor = i;
    }
  }
  Serial.print("Color = ");
  Serial.println(bestColor);
  return(bestColor);
}

// CompareColors
//
// Compares two 24-bit colors: abs(r1-r2)+abs(g1-g2)+abs(b1-b2)
int CompareColors(uint32_t colorA, uint32_t colorB) {
  int diff = 0;
  byte a,b;
  
  for (int j = 0; j < 3; j++) {
    a = (colorA >> (j * 8)) & 0xff;
    b = (colorB >> (j * 8)) & 0xff;
    //Serial.print(j);
    //Serial.print(" ");
    //Serial.print(a);
    //Serial.print(" ");
    //Serial.print(b);
    //Serial.print(" ");
    //if (a > b) { diff+= a-b; } else { diff+= b-a; }
    diff+=abs((int)a-(int)b);
    //diff+= abs(((colorA >> (j * 8)) & 0xff) - ((colorB >> (j * 8)) & 0xff));
  }
  //Serial.println();
  return(diff);
}

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

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
  //if (WheelPos == 0) return Color(0,0,0);       // Black
  //if (WheelPos == 1) return Color(255,255,255); // White
  
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170; 
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
 
//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
//Furthermore, the colors are attenuated:
//3=full strength, 2=2/3rd strength, 1=1/3rd strength, 0=barely on
uint32_t GradientWheel(byte WheelPos, int strength)
{
  int r,g,b;
  int s[4] = { 2, 10, 50, 255 };  // This is the attenuation percent
  
  if (WheelPos < 85) {
   r = WheelPos * 3;
   g = 255 - WheelPos *3;
   b = 0;
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   r = 255 - WheelPos *3;
   g = 0;
   b = WheelPos * 3;
  } else {
   WheelPos -= 170; 
   r = 0;
   g = WheelPos * 3;
   b = 255 - WheelPos *3;
  }
  
  // Attenuate r,g,b
  r = r * s[strength] / 100;
  g = g * s[strength] / 100;
  b = b * s[strength] / 100;
  return(Color(r,g,b));
}     
