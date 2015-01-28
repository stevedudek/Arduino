#include <TimerOne.h>
#include "LPD6803.h"

//
//  DINOSAUR - Uses the Keypad interface for changing lights
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

// 2 pins for clock and data of lights
 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

// Microphone stuff
#define soundPin 5   // analog-in pin for microphone
#define minSound 500 // minimum reading from microphone
#define maxSound 600 // maximum reading from microphone
#define minMicTime 2  // minimum integration constant
#define maxMicTime 60 // maximum integration constant
int micTime = 20;   // integration constrant for the microphone

#define numLights 25    // 25 lights in the strand
#define numShows 7      // number of shows

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
LPD6803 strip = LPD6803(numLights, dataPin, clockPin);

// Spine states

int currColor = 50;               // Starting spine color in 'int' form
unsigned int lightcol[numLights]; // current color for each light
#define MaxColor 96               // Maximum color number on the wheel
#define maxWait 12                // Number of stored delay times 

// Shows

int show=0;    // default starting show
int f=0;       // indices to keep track of position within a show
int x=0;       // another counter
int y=0;       // temporary counter
int wait=2;    // index to keep track of delay time

void setup() {
  
  // Initially set up the keypad pins
  
  pinMode(p51, INPUT);
  pinMode(p53, INPUT);
  pinMode(p52, INPUT);
  digitalWrite(p51, HIGH);
  digitalWrite(p52, HIGH);
  digitalWrite(p53, HIGH);
  
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
  
  Serial.begin(9600);  // For debugging
}


void loop() {
  
  key = KeyPad();

  if (key == DOWN) {
    f=0;  // reset the frame position to 0
    x=0;  // reset the floating index to 0
    show++;
    if (show>=numShows) show=0; // advance show counter
    LightOneLight(numLights-show-1, Color(63, 63, 63), 1000); // Light up one light white with show number
  }

  
  switch (show) {
    
    case 0: {  // Push-sensitive on/off show
      
      if(key == CENTER) colorWipe(Wheel(currColor), 20);       // CENTER = turn on all lights
      if(key == LEFT) oneLightRunDown(Wheel(currColor), 20);   // LEFT = one light run down
      if(key == RIGHT) oneLightRunUp(Wheel(currColor), 20);    // RIGHT = one light run up
      if(key == UP) {    // UP = change color
        if (currColor++ >= MaxColor) currColor = 0;
        colorWipe(Wheel(currColor), 100);
      }
      // Neither center nor up? Then turn all lights off
      if(key != CENTER && key != UP) turnOffLights(20);
      break;
    }
  
    case 1: {   // Light running show
      
      if (x<1 || x>strip.numPixels()) x=1;  // Sets bounds for number of running lights
      if (key == CENTER) {  // CENTER = add another running light
        if (x++>strip.numPixels()) x=1;
      }
      if (key == LEFT) { // LEFT = slow speed
        if (wait < maxWait) wait++;
      }
      if (key == RIGHT) { // RIGHT = increase speed
        if (wait > 1) wait--;
      }
      if (key == UP) {       // UP = change color
        if (currColor++ >= MaxColor) currColor = 0;
      }
      LightRunUp(f, x, Wheel(currColor), GetDelayTime(wait)); // post frame of show
      if (f++ >= strip.numPixels()) f=0;      // advance frame. Set to 0 if at end
      break;
      
    }
    
    case 2: {  // Rainbow show
      
      if (x<1 || x>strip.numPixels()/3) x=1;  // Sets bounds for number of color divisions
      if (f++ >= MaxColor) f=0;      // advance frame. Set to 0 if at end
      if (key == CENTER) { turnOffLights(20); }  // CENTER = turn off lights
        else { rainbowCycle(f, x, GetDelayTime(wait)/5); }
      if (key == LEFT) { // LEFT = slow speed
        if (wait < maxWait) wait++ ;
      }
      if (key == RIGHT) { // RIGHT = increase speed
        if (wait > 1) wait--;
      }
      if(key == UP) { x++; }  // UP = increase color divisions
      break;
      
    }
    
    case 3: {  // Sound Activated
      
      if (key == CENTER) {  // CENTER = rainbow on
        rainbowCycle(f, 4, 20);        // show rainbow
        if (f++ >= MaxColor) f=0;      // advance frame. Set to 0 if at end
      }
      if (key == UP) {       // UP = change color
        if (currColor++ >= MaxColor) currColor = 0;
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
    
    case 4: {   // Lightwaves show
      
      if (x<0 || x>=strip.numPixels()) x=0;  // Sets bounds for number of running lights
      if (f<0 || f>=strip.numPixels()) f=0;  // Sets bounds for frame of show
 
      if (key == CENTER) { colorWipe(Wheel(currColor), 20); }  // CENTER = turn on all lights
      if (key == UP) {       // UP = change color
        if (currColor++ >= MaxColor) currColor = 0;
      }
      if (key == LEFT) { // LEFT = slow speed
        if (wait < maxWait) wait++;
      }
      if (key == RIGHT) { // RIGHT = increase speed
        if (wait > 1) wait--;
      }
      if (key != CENTER) {     // normal mode
        LightWave(f, x, Wheel(currColor), GetDelayTime(wait)); // run the light wave
        f++;
        if (f>=(strip.numPixels()-x)) {   // At end of frame
          x++;      // increment x
          f=0;      // reset frame to x
          if (x>=strip.numPixels()) {  // Filled up all pixels
            x=0;  // start over
            f=0;  // start at first fram
            currColor=currColor+8;  // increment the color
            if (currColor >= MaxColor) currColor = 0;
          }
        }
      }
      break;
    }
    
    case 5: {  // Simple show
      
      if(key == CENTER) turnOffLights(20);    // CENTER = turn off all lights
      if(key == UP) {    // UP = change color
        if (currColor++ >= MaxColor) currColor = 0;
      }
      if(key == LEFT) oneLightRunDown(Wheel(currColor), 20);   // LEFT = one light run down
      if(key == RIGHT) oneLightRunUp(Wheel(currColor), 20);    // RIGHT = one light run up
      // Index not on? Turn lights on
      if(key != CENTER) colorWipe(Wheel(currColor), 50);
      break;
    } 
    
    case 6: {  // Random Lights!
      
      if (key == DOWN || key == UP || !random(10000)) {
        for (y=0; y<numLights; y++) { lightcol[y] = random(127); }  // Fill lights with random colors
      }       
      if(key == LEFT) {
        for (y=numLights-1; y>0; y--) { lightcol[y] = lightcol[y-1]; } // Push the lights up one in the array
        lightcol[0] = random(127); // Position 0 is set to random
        delay(250);
      }
      if(key == RIGHT) {
        for (y=1; y<numLights; y++) { lightcol[y-1] = lightcol[y]; } // Push the lights down one in the array
        lightcol[numLights-1] = random(127); // Position nth (end) is set to random
        delay(250);
      }
      
      for (y=0; y < strip.numPixels(); y++) { strip.setPixelColor(y, Wheel(lightcol[y])); }  // Set the lights to array values
      strip.show();   // write all the pixels out
      delay(50);
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
 */ 
void LightOneLight(uint8_t l, uint16_t c, uint8_t wait) {
  int i;
  for (i=0; i < strip.numPixels(); i++) {
      if (i==l) { strip.setPixelColor(i, c); } else { strip.setPixelColor(i, Color(0, 0, 0)); }
  } 
  strip.show();
  delay(wait);
}

/*
 * LightWave - Lights a selection
 */ 
void LightWave(uint8_t frame, uint8_t wavepos, uint16_t c, uint8_t wait) {
  int i;
  for (i=0; i < strip.numPixels(); i++) {
    if (i>(numLights-wavepos-1) || i==frame)
      { strip.setPixelColor(i, c); } else { strip.setPixelColor(i, Color(0, 0, 0)); }
  }
  strip.show();
  delay(wait);
}

/*
 * Equalizer - Number of lit lights determined by microphone intensity
 */
 
void Equalizer(uint16_t c, uint8_t wait) {
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


/*
 * LightRunUp - Posts one frame of one or more lights running from bottom to top
 */

void LightRunUp(int frame, int numRunLights, uint16_t c, uint8_t wait) {
  int i;
  for (i=0; i < strip.numPixels(); i++) {
    if ((i>=frame && i<frame+numRunLights) || 
        (frame+numRunLights>numLights && i<frame+numRunLights-numLights))
      { strip.setPixelColor(numLights-i-1, c); } else { strip.setPixelColor(numLights-i-1, Color(0, 0, 0)); }
    }
  strip.show();
  delay(wait);
}

/*
 * One Light Run Up - One light travels from top to bottom in current color
 */

void oneLightRunUp(uint16_t c, uint8_t wait) {
  int i,j;
  for (i=0; i < strip.numPixels(); i++) {
    for (j=0; j < strip.numPixels(); j++) {
      if (i==j) { strip.setPixelColor(strip.numPixels()-j-1, c); }
      else { strip.setPixelColor(strip.numPixels()-j-1, Color(0, 0, 0)); }
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
  for (i=0; i < strip.numPixels(); i++) {
    for (j=0; j < strip.numPixels(); j++) {
      if (i==j) { strip.setPixelColor(j, c); } else { strip.setPixelColor(j, Color(0, 0, 0)); }
    }
    strip.show();
    delay(wait);
  }
}

/*
 *  LightUpWithValues - Light up strand with a determined color array
 */
 
void LightUpWithValues(unsigned int c[]) {
  int i;
  for (i=0; i < strip.numPixels(); i++) strip.setPixelColor(i, c[i]);
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
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 96));
    }  
    strip.show();   // write all the pixels out
    delay(wait);
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
  // j = ((analogRead(soundPin)-minSoundLvl)*(strip.numPixels()+1))/(maxSoundLvl); // Read the microphone and figure out how lights to turn on
  
  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
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

// Create a 15 bit color value from R,G,B
unsigned int Color(byte r, byte g, byte b)
{
  //Take the lowest 5 bits of each value and append them end to end
  return( ((unsigned int)g & 0x1F )<<10 | ((unsigned int)b & 0x1F)<<5 | (unsigned int)r & 0x1F);
}

//Input a value 0 to 127 to get a color value.
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
      
