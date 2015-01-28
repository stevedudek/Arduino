//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "WS2801.h"

//
//  Wings - Uses the Keypad interface for changing lights
//
/*****************************************************************************/
//
// Set Up Pins
//

// Keypad pins

#define LEDR 8 // Believe that LEDR pin 8 is useless
#define p51  7 // See SparkFun Keypad example code          
#define p53  6 // 3 pins are for the 5 outputs
#define p52  5 // Up, down, left, right, center
  
// Keypad states

#define NONE   0
#define UP     1
#define DOWN   2
#define LEFT   3
#define RIGHT  4
#define CENTER 5
uint8_t key;    // Current keypress
//
// 2 pins for clock and data of lights
// 
#define dataPin 2       // 'yellow' wire
#define clockPin 3      // 'green' wire

/* Microphone stuff - not used
#define soundPin 5   // analog-in pin for microphone
#define minSound 500 // minimum reading from microphone
#define maxSound 600 // maximum reading from microphone
#define minMicTime 2  // minimum integration constant
#define maxMicTime 60 // maximum integration constant
int micTime = 20;   // integration constrant for the microphone
*/

#define numLights 35    // 40 lights total
#define wingLights 17   // 2 symmetric wings
#define numShows 5      // number of shows

// Timer 1 is also used by the strip to send pixel clocks

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
WS2801 strip = WS2801(numLights, dataPin, clockPin);

// Spine states

byte currColor  = 85;   // Foreground color
byte currColor2 = 170;  // Background color
#define MaxColor 255    // Colors go from 0 to 255

// Shows

int show=2;    // default starting show
int f=0;       // indices to keep track of position within a show
int x=0;       // another counter
int wait=6;    // index to keep track of delay time
#define maxWait 12     // Number of stored delay times 

void setup() {

   // Initially set up the keypad pins
  
  pinMode(p51, INPUT);
  pinMode(p53, INPUT);
  pinMode(p52, INPUT);
  digitalWrite(p51, HIGH);
  digitalWrite(p52, HIGH);
  digitalWrite(p53, HIGH);
     
  // Start up the LED counter
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
}


void loop() {
  
 key = KeyPad();

  if (key == DOWN) {
    f=0;  // reset the frame position to 0
    x=0;  // reset the floating index to 0
    show++;
    if (show>=numShows) show=0; // advance show counter
    LightOneLight(wingLights-show-1, Color(255, 255, 255), 1000); // Light up one light white with show number
  }

  
  switch (show) {
    
    case 0: {  // Push-sensitive on/off show
      
      if(key == CENTER) colorWipe(Wheel(currColor), 20);       // CENTER = turn on all lights
      if(key == LEFT) oneLightRunDown(Wheel(currColor), 40);   // LEFT = one light run down
      if(key == RIGHT) oneLightRunUp(Wheel(currColor), 40);    // RIGHT = one light run up
      if(key == UP) {    // UP = change color
        if (currColor++ >= MaxColor) currColor = 0;
        colorWipe(Wheel(currColor), 100);
      }
      // Neither center nor up? Then turn all lights off
      if(key != CENTER && key != UP) turnOffLights(20);
      break;
    }
  
    case 1: {   // Light running show
      
      if (x<1 || x>wingLights) x=1;  // Sets bounds for number of running lights
      if (key == CENTER) {  // CENTER = add another running light
        if (x++>wingLights) x=1;
      }
      if (key == LEFT) { // LEFT = slow speed
        if (wait++ >= maxWait) wait=0;
      }
      if (key == RIGHT) { // RIGHT = change background color
        if (currColor2++ >= MaxColor) currColor2 = 0;
      }
      if (key == UP) {       // UP = change foreground color
        if (currColor++ >= MaxColor) currColor = 0;
      }
      LightRunUp(f, x, Wheel(currColor), Wheel(currColor2), GetDelayTime(wait)); // post frame of show
      if (f++ >= wingLights) f=0;      // advance frame. Set to 0 if at end
      break;
      
    }
    
    case 2: {  // Rainbow show
      
      if (x<1 || x>wingLights/2) x=1;  // Sets bounds for number of color divisions
      if (f++ >= MaxColor) f=0;      // advance frame. Set to 0 if at end
      if (key == CENTER) { turnOffLights(20); }  // CENTER = turn off lights
        else { rainbowCycle(f, x, GetDelayTime(wait)/5); }  // else run Rainbow show
      if (key == LEFT) { // LEFT = slow speed
        if (wait < maxWait) wait++ ;
      }
      if (key == RIGHT) { // RIGHT = increase speed
        if (wait > 1) wait--;
      }
      if(key == UP) { x++; }  // UP = increase color divisions
      break;
      
    }
    /*
    case 3: {  // Sound Activated
      
      if (key == CENTER) {  // CENTER = rainbow on
        rainbowCycle(f, 4, 20);        // show rainbow
        if (f++ >= MaxColor) f=0;      // advance frame. Set to 0 if at end
      }
      if (key == UP) {       // UP = change color
        if (++ >= MaxColor)  = 0;
      }
      if (key == LEFT) { // Decrease microphone integration constrant
        if (micTime > minMicTime) { micTime--; }
      }
      if (key == RIGHT) { // Increase microphone integration constrant
        if (micTime < maxMicTime) { micTime++; }
      }
      if (key != CENTER && key != UP) {  // Doing nothing else?
        Equalizer(Wheel(currColor), micTime); // Light up using microphone
      }
      break;
      
    }
    */
    case 3: {   // Lightwaves show
      
      if (x<0 || x>=wingLights) x=0;  // Sets bounds for number of running lights
      if (f<0 || f>=wingLights) f=0;  // Sets bounds for frame of show
 
      if (key == CENTER) { colorWipe(Wheel(currColor), 20); }  // CENTER = turn on all lights
      if (key == UP) {       // UP = change color
        if (currColor++ >= MaxColor) currColor = 0;
      }
      if (key == LEFT) { // LEFT = slow speed
        if (wait++ >= maxWait) wait=0;
      }
      if (key == RIGHT) { // RIGHT = change background color
        if (currColor2++ >= MaxColor) currColor2 = 0;
      }
      if (key != CENTER) {     // normal mode
        LightWave(f, x, Wheel(currColor), Wheel(currColor2), GetDelayTime(wait)); // run the light wave
        f++;
        if (f>=(wingLights-x)) {   // At end of frame
          x++;      // increment x
          f=0;      // reset frame to x
          if (x>=wingLights) {  // Filled up all pixels
            x=0;  // start over
            f=0;  // start at first fram
            if (currColor+=8 >= MaxColor) currColor = 0;
          }
        }
      }
      break;
    }
    
    case 4: {  // Simple show
      
      if(key == CENTER) turnOffLights(20);    // CENTER = turn off all lights
      if(key == UP) {    // UP = change color
        if (currColor++ >= MaxColor) currColor = 0;
      }
      if(key == LEFT) oneLightRunDown(Wheel(currColor), 40);   // LEFT = one light run down
      if(key == RIGHT) oneLightRunUp(Wheel(currColor), 40);    // RIGHT = one light run up
      // Index not on? Turn lights on
      if(key != CENTER) colorWipe(Wheel(currColor), 50);
      break;
    } 
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
void LightUp(long data, uint32_t c, uint8_t wait) {
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
void LightOneLight(uint8_t l, uint32_t c, uint8_t wait) {
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
void LightWave(uint8_t frame, uint8_t wavepos, uint32_t c, uint32_t c2, uint8_t wait) {
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
 * Equalizer - Number of lit lights determined by microphone intensity
 */
/* 
void Equalizer(uint32_t c, uint8_t wait) {
  int i;
  long sound=0;
  
  for (i=0; i < wait; i++) {      // Averaging algorithm
    sound=sound+analogRead(soundPin);  // Total up sounds
    delay(1);        // delay 1 x wait = wait
  }
  sound = sound/wait; // Take average
  
  for (i=0; i < strip.numPixels(); i++) {
    if ((sound-minSound) > (((maxSound-minSound)*i)/strip.numPixels()) )
         { strip.setPixelColor(numLights-i-1, c); }
    else { strip.setPixelColor(numLights-i-1, Color(0, 0, 0)); }
  }
  strip.show();
}
*/

/*
 * LightRunUp - Posts one frame of one or more lights running from bottom to top
 */

void LightRunUp(int frame, int numRunLights, uint32_t c, uint32_t c2, uint8_t wait) {
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

void oneLightRunUp(uint32_t c, uint8_t wait) {
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

void oneLightRunDown(uint32_t c, uint8_t wait) {
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
   
  for (j=0; j < 256; j++) {     // 3 cycles of all 256 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 255));
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
     strip.setPixelColor(i, Wheel(((i*256*division / strip.numPixels()) + frame) % 256) );
     strip.setPixelColor(strip.numPixels()-1-i, Wheel(((i*256*division / strip.numPixels()) + frame) % 256) );
  }  
  strip.show();   // write all the pixels out
  delay(wait);
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint32_t c, uint8_t wait) {
  int i;
  
  for (i=0; i < wingLights; i++) {
      strip.setPixelColor(i, c);
      strip.setPixelColor(strip.numPixels()-1-i, c);
  }
  
  strip.show();
  delay(wait);
}

/*
 * KeyPad
 *
 * Returns the state of the keypad. Does not debounce.
 */
 
uint8_t KeyPad() {
  // Initially set all buttons as inputs, and pull them up
  pinMode(p52, INPUT);
  digitalWrite(p52, HIGH);
  pinMode(p51, INPUT);
  digitalWrite(p51, HIGH);
  pinMode(p53, INPUT);
  digitalWrite(p53, HIGH);
  
  // Read the d/u/flame buttons
  if (!digitalRead(p53)) {
    Serial.println("down");
    return DOWN;  // Down
  }
  if (!digitalRead(p52)) {
    Serial.println("up");
    return UP;  // Up
  }
  if (!digitalRead(p51)) {
    Serial.println("center");
    return CENTER;  // Center
  }
    
  // Read right button
  pinMode(p52, OUTPUT);  // set p52 to output, set low
  digitalWrite(p52, LOW);
  if (!digitalRead(p53)) {
    Serial.println("right");
    return RIGHT;  // Right
  }
  pinMode(p52, INPUT);  // set p52 back to input and pull-up
  digitalWrite(p52, HIGH);
  
  // Read left button
  pinMode(p51, OUTPUT);  // Set p51 to output and low
  digitalWrite(p51, LOW);
  if (!digitalRead(p53)) {
    Serial.println("left");
    return LEFT;  // Left
  }
  pinMode(p51, INPUT);  // Set p51 back to input and pull-up
  pinMode(p51, HIGH);
  
  return NONE;
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

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
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

/*
 * Converts 4 bytes into a long
 */
 /*
long BytesToLong (byte byte1, byte byte2, byte byte3, byte byte4) {
  return ( (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4 );
}
*/
      
