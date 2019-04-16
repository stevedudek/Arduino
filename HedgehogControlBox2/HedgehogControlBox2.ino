//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Hedgehog! - 50 Lights. This version does not use a keypad
//
//  Hooked up an xBee to receive color, timing, and show information
//
//  Responds to an Xbee control box, but also runs on its own
//
//  Morphing is HSV-corrected (thanks, Greg)
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights.
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 50    // number of lights


// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// For random-fill show (nasty global)
byte shuffle[numLights];  // Will contain a shuffle of lights

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Light colors

byte foreColor =  0;    // Starting foreground color
byte backColor = 83;   // Starting background color
#define MaxColor 255    // Colors go from 0 to 255

// Shows

int show = 8;       // Starting show
#define MAX_SHOW 15 // Total number of shows

int morph = 0;      // Keeps track of morphing steps
int cycle = 0;      // Keeps track of animation
boolean update = true; // flag that tells whether lights need to be recalculated
int wait = 6;
#define MAX_WAIT 12 // Number of stored delay times 

// Timing variables for xBee communication

unsigned long OldTime;
unsigned long NewTime;
boolean ALONE = true; // Flag for if controller has sent a message

#define MAX_AWAY 20     // Slave goes into Alone mode if no signal in this #sec
//#define SYNC_FREQ 500   // How often in msec that the master sends out data
#define NO_DATA 9999

// xBee language
#define COMMAND_PROMPT     '#'
#define COMMAND_WHITE      'W'
#define COMMAND_COLOR      'C'
#define COMMAND_DELAY      'D'
#define COMMAND_SHOW       'S'
#define COMMAND_COLORSENSE 'X'
#define COMMAND_END        '$'

#define COST_NUM  '2'    // Which number costume this is. Quite important!

//
// Bit maps of spine patterns
//

// Order of spines
byte SpineOrder[50] = {
      8,
    9,13,7,
   10,12,14,6,
    11,15,5,
   17,16,3,4,
    18,2,0,
     19,1,
    20,22,24,
     21,23,
    40,25,27,
   41,39,26,28,
    42,38,29,
   43,37,31,30,
    44,36,32,
   45,47,35,33,
    46,48,34,
      49,
};

// Centering: creates a center bright ring
byte centering[50] = {
      0,
    0,1,0,
   0,1,1,0,
    0,2,0,
   0,2,2,0,
    1,2,1,
     2,2,
    2,3,2,
     3,3,
    2,3,2,
   2,3,3,2,
    2,3,2,
   1,2,2,1,
    0,2,0,
   0,1,1,0,
    0,1,0,
      0,
};

// chevpattern: pattern of chevrons
byte chevpattern[50] = {
      9,
    9,8,9,
   9,8,8,9,
    8,7,8,
   8,7,7,8,
    7,6,7,
     6,6,
    6,5,6,
     5,5,
    5,4,5,
   5,4,4,5,
    4,3,4,
   4,3,3,4,
    3,2,3,
   3,2,2,3,
    2,1,2,
      0,
};

// starburstpattern: from center to edge
byte starburstpattern[50] = {
      9,
    9,8,9,
   8,7,7,8,
    6,5,6,
   5,4,4,5,
    3,2,3,
     1,1,
    2,0,2,
     1,1,
    2,1,2,
   4,3,3,4,
    5,4,5,
   6,5,5,6,
    7,6,7,
   8,7,7,8,
    9,8,9,
      9,
};

// centerstripe: a center stripe
byte centerstripe[50] = {
      1,
    0,1,0,
   0,0,1,0,
    0,1,0,
   0,1,0,0,
    0,1,0,
     0,1,
    0,1,0,
     1,0,
    0,1,0,
   0,0,1,0,
    0,1,0,
   0,1,0,0,
    0,1,0,
   0,0,1,0,
    0,1,0,
      1,
}; 


//
// Setup
//

void setup() {
  
  // xBee communication
  
  OldTime = 0;
  NewTime = 0;
  
  Serial.begin(9600);
  //Serial.println("Start");
    
  // Start up the LED counter
  strip.begin();
  
  // Initialize the strip, to start they are all 'off'
  
  for (int i=0; i<numLights; i++) {
    current_frame[i] = Color(0,0,0);        // Black
    next_frame[i] = Color(0,0,0);           // Black
    strip.setPixelColor(i, Color(0,0,0));   // Black
  }
  strip.show();
}

void loop() {
    
  // The only delay!
  
  talkdelay(10);  // Important delay, also involves listening! 
 
  // Check if the lights need updating
  
  if (update) {
    
    update = false;
    
    switch(show) {
    
      case 0:    
        allOn();
        break;
      case 1:
        morphChain();
        break;
      case 2:
        randomfill();
        break;
      case 3:
        randomcolors();
        break;
      case 4:
        twocolor();
        break;
      case 5:
        lightwave();
        break;
      case 6:
        lightrunup();
        break;
      case 7:
        sawtooth();
        break;
      case 8:
        rainbowshow();
        break;
      case 9:
        stripe();
        break;
      case 10:
        bullseye();
        break;
      case 11:
        chevronrainbow();
        break;
      case 12:
        starburst();
        break;
      case 13:
        chevrons();
        break;
      case 14:
        chevronfill();
        break;
    }
  }
  
  // Morph the display
  
  morph_frame();
  
  // Advance the clock
   
  if (morph++ >= GetDelayTime(wait)) {  // Finished morphing
    
    update = true;  // Force an update
    
    morph = 0;  // Reset morphClock
    
    // Advance the cycle clock
    
    if(cycle++ > 1000) cycle = 0;  
    
    // Update the frame display. Push current into next
  
    for (int i = 0; i < numLights; i++) {
      current_frame[i] = next_frame[i];
    }
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(25)) { // foreColor
    foreColor = (foreColor + 1) % MaxColor;
    update = true;
  }
  
  if (!random(25)) { // backColor
    backColor -= 2;
    if (backColor < 0) backColor += MaxColor;
    update = true;
  }
  
  if (!random(10000)) {  // show
    show = (show + 1) % MAX_SHOW;
    morph = 0;
    cycle = 0;
    clearWithFade();
  }
  
  if (!random(1000)) { // wait
    if (wait++ >= MAX_WAIT) {
      wait = 0;
      morph = 0;
    }
  }
}

// clear
//
// set all cells to black but don't call show yet
// ignores buffering
// 
void clear() {
  for (int i=0; i<numLights; i++) {
    strip.setPixelColor(i, Color(0,0,0));
    setPixelColor(i, Color(0,0,0));
  }
}

// clear with fade
//
// Fades lights to black but don't call the show yet
//
void clearWithFade() {
  for (int i=0; i<numLights; i++) {
    setPixelColor(i, Color(0,0,0));
  }
}

// Turn all one color
//
// Turn all pixels foreceably one color, then delays
//
void TurnAllOneColor(uint32_t color, int rest) {
  for (int i=0; i<numLights; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
  talkdelay(rest);  // May have a lot of threading oddness
}

// Turn all white
//
// Turns all pixels forceably white, then delays
//
void TurnAllWhite(int rest) {
  TurnAllOneColor(Color(255,255,255), rest);
}

//
// All On
//
// Simply turns all the pixels on to one color
// 
void allOn() {
   for (int i=0; i < strip.numPixels(); i++) {
     setPixelColor(i, Wheel(foreColor));
   }
}

// random fill
//
// randomfill: randomly fills in pixels from blank to all on
// then takes them away random until blank
//
 
void randomfill() {
  int i, j, save, pos;
  
  pos = cycle % (numLights*2);  // Where we are in the show
  
  if (pos == 0) {  // Start of show
  
    // Shuffle sort to determine order to turn on lights
    for (i=0; i < numLights; i++) shuffle[i] = i; // before shuffle
    for (i=0; i < numLights; i++) {  // here's position
      j = random(numLights);         // there's position
      save = shuffle[i];
      shuffle[i] = shuffle[j];       // first swap
      shuffle[j] = save;             // second swap
    }
  }
  
  if (pos < numLights) {  
    setPixelColor(shuffle[pos], Wheel(foreColor));  // Turning on lights one at a time
  } else { 
    setPixelColor(shuffle[numLights-(pos % numLights)-1], Color(0,0,0));  // Turning off lights one at a time
  }
}  

// random colors
//
// randomcolors: turns each pixel on to a random color
//
void randomcolors() {
  int i;
  
  if (cycle == 0) {  // Start of show: assign lights to random colors
    for (i=0; i < numLights; i++) shuffle[i] = random(MaxColor);
  }
  
  // Otherwise, fill lights with their color
  for (i=0; i < numLights; i++) {
    setPixelColor(i, Wheel(shuffle[i]));
  }
}

// two color
//
// randomly turns each pixel on to either fore or back color
//
void twocolor() {
  int i;
  
  if (cycle == 0) {  // Start of show: assign lights randomly to fore or back
    for (i=0; i < numLights; i++) shuffle[i] = random(2);
  }
  
   // Otherwise, fill lights with their color
  for (i=0; i < numLights; i++) {
    if (shuffle[i]) setPixelColor(i, Wheel(foreColor));
    else setPixelColor(i, Wheel(backColor));
  }
}

//
// Morph Chain
//
// Morphs color 1 from position x to
// color 2 at position x+n

void morphChain() {
  float attenuation;
  
  for (int i=0; i < numLights; i++) {
    attenuation = ((i+(cycle%numLights)) % numLights)/(float)(numLights-1);
    setPixelColor(i, HSVinter24(Wheel(foreColor),Wheel(backColor), attenuation));
  }
}

//
// Saw tooth
//
// Fills in pixels with a sawtooth of intensity
//
// Peak of the sawtooth moves with the cycle

void sawtooth() {
  float attenuation;
  
  for (int i=0; i < numLights; i++) {
    attenuation = 2*(((i+(cycle%numLights)) % numLights)/(float)(numLights-1));
    if (attenuation > 1) attenuation = 2 - attenuation;  // the '2 -' creates the sawtooth
    attenuation = attenuation * attenuation;  // magnify effect - color brightness is not linear
    setPixelColor(i, HSVinter24(Color(0,0,0), Wheel(foreColor), attenuation));
  }
}

//
// Rainbow Show
//
// fills the spines with a gradient rainbow
// over time, the starting color changes
//
 
void rainbowshow() {
  int diff = abs(foreColor - backColor);
  
  for (int i=0; i < numLights; i++) {
    setPixelColor( (i+(cycle%numLights)) % numLights, Wheel(((i*diff / numLights)+foreColor) % diff) );
  }
}

//
// lightwave
//
// Just one pixel traveling along the chain
 
void lightwave() {
 
  for (int i=0; i < numLights; i++) {
     if (i == numLights-(cycle % numLights)-1) setPixelColor(i, Wheel(foreColor));
     else setPixelColor(i, Color(0,0,0));  // Black
  }
}

//
// lightrunup
//
// lights fill up one at time
//
void lightrunup() {
  int pos = cycle % (numLights*2);  // Where we are in the show
  if (pos >= numLights) pos = (numLights*2) - pos - 1;
  
  for (int i=0; i < numLights; i++) {
    if (i <= pos) {
      setPixelColor(i, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setPixelColor(i, Color(0,0,0));   // black
    }
  }
}

//
// Stripe
//
// Puts a stripe down the center
//
void stripe() {
  for (int i=0; i < numLights; i++) {
    if (centerstripe[i]) { setPixelColor(SpineOrder[i], Wheel(foreColor));
    } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }   // Black
  }
}

//
// Bullseye
//
// lights up the centering image with 4 colors
//
void bullseye() {
  byte colors[4];
  byte tempcolor = backColor;
  byte colorspacing = foreColor / 5;
  
  // Calculate colors for the bullseye 
  for (int i=0; i<4; i++) {
    colors[i] = tempcolor;
    tempcolor += colorspacing;
    if (tempcolor > MaxColor) tempcolor = tempcolor - MaxColor;
  }
  
  // Assign spines to the proper color
  for (int j=0; j < strip.numPixels(); j++) {
      setPixelColor(SpineOrder[j], Wheel(colors[centering[j]]));
  }
}

//
// Chevron Rainbow
//
// lights up the chevron with a rainbow pattern with 10 colors
//
void chevronrainbow() {
  byte colors[10];
  byte tempcolor = backColor;
  byte colorspacing = foreColor / 11;
  
  // Calculate colors for the chevron 
  for (int i=0; i<10; i++) {
    colors[i] = tempcolor;
    tempcolor += colorspacing;
    if (tempcolor > MaxColor) tempcolor = tempcolor - MaxColor;
  }
  
  // Assign spines to the proper color
  for (int j=0; j < strip.numPixels(); j++) {
      setPixelColor(SpineOrder[j], Wheel(colors[chevpattern[j]]));
  }
}

//
// Starburst
//
// Burst from center to outer ring
//
void starburst() {
  for (int i = 0; i < numLights; i++) {
    if (starburstpattern[i] == cycle % 10) {
      setPixelColor(SpineOrder[i], Wheel((foreColor+(starburstpattern[i]*3))%MaxColor));
    } else {
      setPixelColor(SpineOrder[i], Color(0,0,0));  // Black
    }
  }
}

//
// Chevrons
//
// Upward chevrons
//
void chevrons() {
  for (int i = 0; i < numLights; i++) {
    if (chevpattern[i] == cycle % 10) {
      setPixelColor(SpineOrder[i], Wheel(foreColor));
    } else {
      setPixelColor(SpineOrder[i], Color(0,0,0));  // Black
    }
  }
}

//
// Chevron Fill
//
// upward chevrons that fill and fall back
//
void chevronfill() {
  int chev = cycle % (10*2);
  if (chev >= 10) chev = (10*2) - chev - 1;
  
  for (int i = 0; i < numLights; i++) {
    if (chev >= chevpattern[i]) {
      setPixelColor(SpineOrder[i], Wheel((foreColor+(chevpattern[i]*2))%MaxColor));
    } else {
      setPixelColor(SpineOrder[i], Color(0,0,0));
    }
  }
}

/* Morph Functions */

//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code initially from Greg and Robie
//

void morph_frame() {
   for (int i = 0; i < numLights; i++) {
     strip.setPixelColor(i, HSVinter24(current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait)));
   }
   strip.show();  // Update the display 
}

void setPixelColor(int pos, uint32_t color) {
  next_frame[pos] = color;
}

//
// Get Delay Time
//
// Returns a delay time from an array
//
 
int GetDelayTime(int wait) {
  int DelayValues[MAX_WAIT] = { 2, 3, 4, 6, 8, 10, 15, 20, 30, 50, 75, 100 };
  return (DelayValues[wait % MAX_WAIT]);
}

//
// talk delay
//
// Runs a normal timed delay
//
// Meanwhile, listen to the box every POLL_TIMEms


#define POLL_TIME 5

void talkdelay(int delaytime) {
  int i;
  for(i = delaytime; i > POLL_TIME; i -= POLL_TIME) {
    if(HearBox()) { // Got a command that requires immediate return?
      delay(i);
      return;   // Yes, go fix things
    }
    delay(POLL_TIME);
  }
  delay(i); // The remainder
}

//
// HearBox
//
// Listen for the Box
//

boolean HearBox() {
  char incoming;
  long value;
  
  NewTime = millis();  // Take a time point
  
  if (Serial.available()) {
    
    incoming = Serial.read();
    if (incoming == COMMAND_PROMPT) {  // Heard from the box
       incoming = Serial.read();       // For which number costume?
       if (incoming == COST_NUM || incoming == '9') {  // Talking to me!
           OldTime = NewTime;    // Reset the clock
           ALONE = false;        // We're in contact! 
       
           incoming = Serial.read();  // Get that one-letter command
           //Serial.print(incoming);
           value = HearNum();         // Get the number that follows the command
           //Serial.println(value);
           if (value == NO_DATA) {
              //Serial.println("No legitimate number read");
             return(false); // Not getting a legit number
           }
           // Execute the command and return any flags from it
           return(ExecuteCommand(incoming, value));
       }
    }
  }
   
  // No received Serial.available or not talking to me
  //
  // Check how long since last hearing from the Master

  if((NewTime - OldTime)/1000 > MAX_AWAY) {
    OldTime = NewTime;    // Reset the clock (for some reason?)
    ALONE = true;         // Lost contact with the Master
    //Serial.println("Feeling very alone");
  }
  return(false);  // no system update required
}

//
// HearNum
//
// Converts xBee characters into a number
// '$' character terminates the number string
//
// If no number, returns NO_DATA signifier

long HearNum() {
  #define BUFSIZE 20
  #define TIMEOUT_TRIES 100
  
  int waitTime = 0;  // Start counter
  int charplace = 0; // Start of number
  char buf[BUFSIZE];
  
  // Clear character buffer
  for (int i = 0; i <BUFSIZE; i++) buf[i] = 0;
  
  while (waitTime < TIMEOUT_TRIES) {
   
    if (Serial.available()) {
      char tmp = Serial.read();
      //Serial.print(tmp);
      if (tmp != '$') { // Hopefully 0-9 and '-'
        waitTime = 0;  // Reset wait counter
        buf[charplace] = tmp;
        if (charplace++ >= BUFSIZE) { return(NO_DATA); } // Number too long
       } else {      // Got a '$' to end the word
         if (charplace>0) return atoi(buf); else {  // successfully reached the end of a number 
           //Serial.println("i=0 number read");
           return(NO_DATA);  // Returning an empty word
         }
       }
      } else { 
        delay(1);
        waitTime++;
      }
    }
  //Serial.println("Out_of_Time");
  return(NO_DATA);  // ran out of time for next character: bail and return NO_DATA
}

//
// Execute Command
//
// Interprets a letter command and a number value
// Returns a flag alerting whether to break out of the current loop

boolean ExecuteCommand(char command, long value) {
  int newDelay;
  
  switch(command) {
      case COMMAND_COLOR:
        foreColor = value % MaxColor;
        return(true);
        break;
      case COMMAND_WHITE:
        TurnAllWhite(value);
        return(false);
        break;
      case COMMAND_DELAY:
        wait = (MAX_WAIT-1) * value / 255;
        morph = min(morph, GetDelayTime(wait));
        return(false);
        break;
      case COMMAND_SHOW:
        if (value % MAX_SHOW != show) {
          show = value % MAX_SHOW;
          morph = 0;
          cycle = 0;
          clearWithFade();
          return(true);
        }
        break;
      case COMMAND_COLORSENSE:
        TurnAllOneColor(value, 1000);
        return(false);
        break;
      default:
        return(false);
  }
  return(false);
} 

/* Helper functions */

// Color
//
// Create a 24-bit color value from R,G,B
//
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c = 0;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

// Extracts the red part of a 24-bit color
byte GetRed(uint32_t c)
{
  return ((c >> 16) & 0xff);
}

// Extracts the green part of a 24-bit color
byte GetGreen(uint32_t c)
{
  return ((c >> 8) & 0xff);
}

// Extracts the green part of a 24-bit color
byte GetBlue(uint32_t c)
{
  return (c & 0xff);
}

// If r=g=b=0, return true
boolean IsBlack(byte r, byte g, byte b)
{
  if(r=g=b=0) return (true);
  else return (false);
}


//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
  int r,g,b;
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
  return Color(r,g,b);
}
 

// r,g,b values are from 0 to 255
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
// 
// code from http://www.cs.rit.edu/~ncs/color/t_convert.html

void RGBtoHSV( byte red, byte green, byte blue, float *h, float *s, float *v )
{
  float r = red/float(255);
  float g = green/float(255);
  float b = blue/float(255);
  
  float MIN = min(r, min(g,b));  // min(r,g,b)
  float MAX = max(r, max(g,b));  // max(r,g,b)
 
  *v = MAX;		        // v

  float delta = MAX - MIN;

  if (MAX != 0 ) *s = delta / MAX;	// s
  else { // r = g = b = 0		// s = 0, v is undefined
    *s = 0;
    *h = -1;
    return;
  }
  if( r == MAX ) *h = 60.0 * ( g - b ) / delta; // between yellow & magenta
  else {
    if( g == MAX ) {
      *h = 120.0 + 60.0 * ( b - r ) / delta; // between cyan & yellow
    } else {
      *h = 240.0 + 60.0 * ( r - g ) / delta;	// between magenta & cyan
    }
  }
  if( *h < 0 ) *h += 360;
}

// r,g,b values are from 0 to 255
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
//
// code from http://www.cs.rit.edu/~ncs/color/t_convert.html

void HSVtoRGB( byte *r, byte *g, byte *b, float h, float s, float v )
{
  int i;
  float f, p, q, t;
  
  if( s == 0 ) {
    // achromatic (grey)
    *r = *g = *b = (v*255);
    return;
  }
  
  h /= 60;			// sector 0 to 5
  i = floor( h );
  f = h - i;			// factorial part of h
  p = v * ( 1 - s );
  q = v * ( 1 - s * f );
  t = v * ( 1 - s * ( 1 - f ) );
  
  switch( i ) {
    case 0:
      *r = v * 255;
      *g = t * 255;
      *b = p * 255;
      break;
    case 1:
      *r = q * 255;
      *g = v * 255;
      *b = p * 255;
      break;
    case 2:
      *r = p * 255;
      *g = v * 255;
      *b = t * 255;
      break;
    case 3:
      *r = p * 255;
      *g = q * 255;
      *b = v * 255;
      break;
    case 4:
      *r = t * 255;
      *g = p * 255;
      *b = v * 255;
      break;
    default:		// case 5:
      *r = v * 255;
      *g = p * 255;
      *b = q * 255;
      break;
    }
}

//
//  HSV Interpolate Wheel
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 0-255 wheel colors

uint32_t HSVinterWheel(byte c1, byte c2, float fract)
{
  return(HSVinter24(Wheel(c1),Wheel(c2),fract));
}

//
//  HSV Interpolate 24-bit
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 24-bit colors

uint32_t HSVinter24(uint32_t c1, uint32_t c2, float fract)
{
  return(HSVinterRGB(GetRed(c1),GetGreen(c1),GetBlue(c1), GetRed(c2),GetGreen(c2),GetBlue(c2), fract));
}

//  HSV Interpolate RGB
//
//  Given a start rgb, an end rgb, and a fractional distance (0-1)
//  This function converts start and end colors to hsv
//  and interpolates between the two points
//  The function returns the properly interpolated rgb
//  as a 24-bit rgb color
//  Whew.

uint32_t HSVinterRGB(byte r1, byte g1, byte b1, byte r2, byte g2, byte b2, float fract)
{  
  //Serial.print("r1 = ");
  //Serial.print(r1);
  //Serial.print(", g1 = ");
  //Serial.print(g1);
  //Serial.print(", b1 = ");
  //Serial.print(b1);
  //Serial.print("r2 = ");
  //Serial.print(r2);
  //Serial.print(", g2 = ");
  //Serial.print(g2);
  //Serial.print(", b2 = ");
  //Serial.println(b2);
  
  if (fract < 0 || fract > 1) {
     Serial.print("fract = ");
     Serial.println(fract);
     return(Color(255,0,0));  // Fract is out of 0-1 bounds. Color defaults to red
  }
  // Check to see if either 1 or 2 are black. If black, just attenuate the other color.
   
  if (r1+g1+b1 == 0) return(Color(r2*fract, g2*fract, b2*fract));
  if (r2+g2+b2 == 0) return(Color(r1*(1-fract), g1*(1-fract), b1*(1-fract)));
   
  // Set up HSV1 and HSV2 variables and pointers
  
  float h1,s1,v1,h2,s2,v2,hi,si,vi;
  float* p_h1;
  float* p_s1;
  float* p_v1;
  p_h1 = &h1;
  p_s1 = &s1;
  p_v1 = &v1;
  float* p_h2;
  float* p_s2;
  float* p_v2;
  p_h2 = &h2;
  p_s2 = &s2;
  p_v2 = &v2;
  
  // Calculate HSV1 and HSV2
  
  RGBtoHSV(r1,g1,b1,p_h1,p_s1,p_v1);
  RGBtoHSV(r2,g2,b2,p_h2,p_s2,p_v2);
  
  // Calculate the interpolated HSVi
  
  hi = interpolateWrap(h1,h2,fract);
  si = interpolate(s1,s2,fract);
  vi = interpolate(v1,v2,fract);
  
  // Convert back to rgb via pointers
  
  byte r,g,b;
  byte* p_r;
  byte* p_g;
  byte* p_b;
  p_r = &r;
  p_g = &g;
  p_b = &b;
  
  HSVtoRGB(p_r,p_g,p_b,hi,si,vi);
  
  // Print test bed
  /*
  Serial.print("r1 = ");
  Serial.print(r1);
  Serial.print(", g1 = ");
  Serial.print(g1);
  Serial.print(", b1 = ");
  Serial.print(b1);
  Serial.print(", r = ");
  Serial.print(r);
  Serial.print(", g = ");
  Serial.print(g);
  Serial.print(", b = ");
  Serial.println(b);
  */
  // Return the proper 24-bit color
   
  return(Color(r,g,b));
}

//
//  RGB Interpolate Wheel
//
//  Wrapper for RGB Interpolate RGB below
//  start and end colors are 0-255 wheel colors

uint32_t RGBinterWheel(byte c1, byte c2, float fract)
{
  return(RGBinter24(Wheel(c1),Wheel(c2),fract));
}

//
//  RGB Interpolate 24-bit
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 24-bit colors

uint32_t RGBinter24(uint32_t c1, uint32_t c2, float fract)
{
  return(RGBinterRGB(GetRed(c1),GetGreen(c1),GetBlue(c1), GetRed(c2),GetGreen(c2),GetBlue(c2), fract));
}

//  RGB Interpolate RGB
//
//  Given a start rgb, an end rgb, and a fractional distance (0-1)
//  This function interpolates between the two points
//  and returns the properly interpolated rgb
//  as a 24-bit rgb color

uint32_t RGBinterRGB(byte r1, byte g1, byte b1, byte r2, byte g2, byte b2, float fract)
{
  if (fract < 0 || fract > 1) {
     Serial.print("fract = ");
     Serial.println(fract);
     return(Wheel(0));  // Fract is out of 0-1 bounds. Color defaults to red
  }
  return(Color(interpolate(r1,r2,fract), interpolate(g1,g2,fract), interpolate(b1,b2,fract) ));
}
//
// Interpolate
//
// Returns the fractional point from a to b

float interpolate(float a, float b, float fract)
{
  return(a + (fract*(b-a)));
}

//
// Interpolate Wrap
//
// Returns the fractional point from a to b
// checking both ways around a circle

float interpolateWrap(float a, float b, float fract)
{
  if(should_wrap(a,b)) {
    if(a<b) a += 360;
    else b += 360;
  }
  float result = interpolate(a,b,fract);
  if (result>360) result -= 360;
  return(result);
}

//
// should_wrap
//
// determines whether the negative way
// around the circle is shorter

boolean should_wrap(float a, float b) {
  if (a>b) {
    float temp = a;  // basic a <-> b swap
    a = b;
    b = temp;
  }
  return(abs(b-a) > abs((a+360) - b));
}
