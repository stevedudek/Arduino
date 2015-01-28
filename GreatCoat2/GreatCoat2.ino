//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"
#include <avr/pgmspace.h>

//
//  Great Eyeball Coat - 113 lights
//
/*****************************************************************************/
//
// 1/16/15
//
// Colors are f----d up due to storage constraints
// The arduino microprocessor is running out of RAM so
// colors are stored reduced as bytes instead of uint32_t's
//
// Re-did shows as state functions
// to enable interrupts by the button and switch

#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 113      // Lots! Causes memory storage problems

// framebuffers
byte current_frame[numLights];
byte next_frame[numLights];

// Position of eyeballs on the coat
#define ARRAY_WIDTH 7
#define ARRAY_HEIGHT 22
#define NONE 255
prog_uchar eye_pos[ARRAY_HEIGHT * ARRAY_WIDTH] PROGMEM = {
  0,2,4,6,8,10,11,
  1,3,5,7,9,12,NONE,
  NONE,22,20,18,16,14,NONE,
  23,21,19,17,15,13,NONE,
  NONE,25,27,29,31,33,NONE,
  24,26,28,30,32,34,NONE,
  47,45,43,41,39,37,35,
  46,44,42,40,38,36,NONE,
  NONE,49,51,53,55,57,NONE,
  48,50,52,54,56,58,NONE,
  NONE,68,66,64,62,60,NONE,
  NONE,69,67,65,63,61,59,
  70,72,74,76,78,80,82,
  71,73,75,77,79,81,NONE,
  NONE,92,90,88,86,84,NONE,
  93,91,89,87,85,83,NONE,
  NONE,94,97,106,109,110,NONE,
  NONE,95,105,107,111,NONE,NONE,
  NONE,96,98,104,108,112,NONE,
  NONE,NONE,99,103,NONE,NONE,NONE,
  NONE,NONE,NONE,101,NONE,NONE,NONE,
  NONE,NONE,100,102,NONE,NONE,NONE
};

// For bouncing ball show
float x_pos = ARRAY_WIDTH / 2;
float y_pos = ARRAY_HEIGHT / 2;
float x_dir = 0.5;
float y_dir = 0.5;

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Hat colors

#define MaxColor 252    // Colors are 6 * 42 : REDUCED SET!
#define BLCK     255    // Hack for black/off color
#define WHTE     254    // hack for white color
#define YELLOW   135

byte foreColor = YELLOW;   // Starting foreground color
byte backColor = 0;   // Starting background color

// Shows

byte morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated
#define MAX_CLOCK   1000      // Total length of the story

byte show = 1;       // Which show we are on
#define MAX_SHOW 10  // Total number of shows

byte wait = 6;    // index to keep track of delay time
#define MAX_WAIT 12     // Number of stored delay times 


// Small control box

#define BUTTON_PIN 10
#define POWER_PIN  11
#define DIAL_PIN    0

// Current and previous status of buttons
boolean powercurr;
boolean buttoncurr;
boolean buttonprev;
long buttontime;
byte button_state = 1;
#define MAX_BUTTON_STATE 3

// Current position of dial
int dialval;
#define MIN_CHANGE 5 // Minimum change in a dial that will register. Prevents flickering

//
// Setup
//

void setup() {

  Serial.begin(9600);
  Serial.println("Start");
     
  // Start up the LED counter
  strip.begin();
  
  // Update the strip, to start they are all 'off'
  forceBlack();
  
  // Set up the dial and button
  pinMode(POWER_PIN, INPUT);
  powercurr = digitalRead(POWER_PIN);
  pinMode(BUTTON_PIN, INPUT);
  buttoncurr = LOW;
  buttonprev = HIGH;
  buttontime = millis();
  dialval = analogRead(DIAL_PIN);
}


void loop() {
  
  if (digitalRead(POWER_PIN) == LOW) {  // Box & Lights are off
    if (powercurr == HIGH) {  // Lights just got turned off
      powercurr = LOW;
      forceBlack();
      delay(500);
    }
  } else {
    
    if (powercurr == LOW) {
      powercurr = HIGH;
    }
   
    delay(20);   // The only delay!
    
    CheckBox();
    
    // Check if the lights need updating
    
    if (update) {
      
      update = false;
    
      switch (show) {
        
        case 0:    
          allOn();
          break;
        case 1:
          alternatebands();
          break;
        case 2:
          twocolor();
          break;
        case 3:
          lightwave();
          break;
        case 4:
          twowave();
          break;
        case 5:
          Pulse();
          break;
        case 6:
          horizbands();
          break;
        case 7:
          randomColors();
          break;
        case 8:
          bouncingBall();
          break;
        default:
          vertbands();
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
      
      if (cycle++ > MAX_CLOCK) cycle = 0;
      
      // Update the frame display
    
      for (int i = 0; i < numLights; i++) {
        current_frame[i] = next_frame[i];
      }
    }
    
    // Randomly change backColor, show, and timing
    /*
    if (!random(50)) {
      foreColor = (foreColor + 1) % MaxColor;
      update = true;
    }
    */
    if (!random(50)) {
      if (backColor <= 0) backColor = MaxColor;
      backColor -= 1;
      update = true;
    }
    
    if (!random(10000)) {  // Change the show
      show = random(MAX_SHOW);
     
      morph = 0;
      clear();
    }
    
    if (!random(1000)) {
      wait++;
      if (wait >= MAX_WAIT) {
        wait = 0;
        morph = 0;
      }
    }
  }
}

//
// Check Box
//
// Checks whether the button and dial have changed position

void CheckBox() {
  
  // Start with the dial
  
  int dial = analogRead(DIAL_PIN);
  if (abs(dial - dialval) > MIN_CHANGE) { // Has the dial budged?
    dialval = dial;      // Update dial
    byte tmp;
    
    switch (button_state) {
    
      case 0:
        tmp = map(dial, 0, 1024, 0, MaxColor);
        if (tmp != foreColor) {
          foreColor = tmp;
          Serial.println(foreColor);
          update = true;
        }
        break;
        
      case 1:
        tmp = map(dial, 0, 1024, 0, MAX_WAIT);
        if (tmp != wait) {
          wait = tmp;
        }
        break;
        
      default:
        tmp = map(dial, 0, 1024, 0, MAX_SHOW);
        if (tmp != show) {
          show = tmp;
          update = true;
          morph = 0;
          clear();
        }
        break; 
    }
  }
  
  // Look next at the button
  
  if (ButtonPushed()) {
    button_state = (button_state + 1) % MAX_BUTTON_STATE;
    if (button_state != 0) {
      foreColor = YELLOW;  // Reset light color back to initial yellow
    }
  }
}

//
// ButtonPushed
//
// Returns true if the button has been pushed
// Includes a timer to prevent flickering

boolean ButtonPushed() {  
  
  if (millis() < 5000) return false;  // prevents initial button bouncing
  
  boolean bstate = digitalRead(BUTTON_PIN);
  
  if (bstate == HIGH && buttonprev == LOW && millis() - buttontime > 500) { // Has button been pushed?
      buttoncurr = LOW;                                                        // Yes, reset the button
      buttonprev = HIGH;
      buttontime = millis();      // ... and remember when the last button press was    
      return true;          // Button pushed
  } else {                  // Button not pushed
      buttonprev = bstate;
      return false;           
  }
}

//
// set all cells to black but don't call show yet
// ignores buffering
//
void clear() {
  for (int i=0; i<numLights; i++) {
    setPixelColor(i, BLCK);
  }
}

//
// forceBlack
//
// Immediately turns off all lights
void forceBlack() {
  for (int i=0; i<numLights; i++) {
    current_frame[i] = BLCK;
    next_frame[i] = BLCK;
    morph = 0;
    strip.setPixelColor(i, Color(0,0,0));
  }
  strip.show();
}

/*
 * All On
 *
 * Simply turns all the pixes on to a random color and then cycles
 *
 */
 
void allOn () {
  for (int i=0; i < numLights; i++) {
    setPixelColor(i, foreColor);
  }
}

/*
 * randomfill: randomly fills in pixels from blank to all on
 * then takes them away random until blank
 */
 /*
void randomfill() {
  int pos = cycle % (numLights*2);  // Where we are in the show
  
  if (pos == 0) ShuffleLights();
  
  if (pos < numLights) {  
    setPixelColor(shuffle[pos], foreColor);  // Turning on lights one at a time
  } else { 
    setPixelColor(shuffle[(numLights*2)-pos-1], BLCK);  // Turning off lights one at a time
  }
}
*/
//
// Shuffle Lights
//
// Shuffles the global shuffle[] array with the order of lights
// Shuffle sort to determine order to turn on lights
/*
void ShuffleLights() {
  byte i,j,save;
  
  for (i=0; i < numLights; i++) shuffle[i] = i; // before shuffle
  for (i=0; i < numLights; i++) {  // here's position
    j = random(numLights);         // there's position
    save = shuffle[i];
    shuffle[i] = shuffle[j];       // first swap
    shuffle[j] = save;             // second swap
  }
}
*/

/*
 * randomcolors: turns each pixel on to a random color
 */

void randomColors() {
  for (int i=0; i < numLights; i++) {
    setPixelColor(i, foreColor * (i * 13) % MaxColor);
  }
}

/*
 * twocolor: alternates the color of pixels between two colors
 */

void twocolor() {
  
  for (int i=0; i < numLights; i++) {
    if (i % 2) setPixelColor(i, foreColor);
    else setPixelColor(i, backColor);
  }
}

//
// horizbands
//
// Rising horizontal bands of lights
void horizbands() {
  byte LED;
  byte max_col = cycle % (ARRAY_HEIGHT * 2);
  if (max_col > ARRAY_HEIGHT) max_col = (ARRAY_HEIGHT * 2) - max_col - 1;
  
  clear();  // Turn all lights black
  
  for (byte col = 0; col < max_col; col++) {
    for (byte row = 0; row < ARRAY_WIDTH; row++) {
      SetLight(row, col, foreColor);
    }
  }
}

//
// vertbands
//
// Moving vertical bands of light
void vertbands() {
  byte LED;
  clear();  // Turn all lights black
  
  for (byte col = 0; col < ARRAY_HEIGHT; col++) {
    SetLight(cycle % ARRAY_WIDTH, col, foreColor);
  }
}

//
// alternatebands
//
// Moving alternating vertical bands
void alternatebands() {
  byte LED;
  clear();  // Turn all lights black
  
  for (byte row = 0; row < ARRAY_WIDTH; row++) {
    if (row % 2) {
      SetLight(row, cycle % ARRAY_HEIGHT, foreColor);
    } else {
      SetLight(row, ARRAY_HEIGHT - (cycle % ARRAY_HEIGHT) - 1, foreColor);
    }
  }
}

//
// bouncingBall
//
// Moving set of lights bounce like a ball

void bouncingBall() {
  clear();
  x_pos += x_dir;
  y_pos += y_dir;
  
  if (x_pos < 0.0 || x_pos >= ARRAY_WIDTH) {
    x_pos -= x_dir;
    x_dir = (x_dir * -1) + (random(-1,2) / 10.0);
    x_pos += x_dir;
  }
  
  if (y_pos < 0.0 || y_pos >= ARRAY_HEIGHT) {
    y_pos -= y_dir;
    y_dir = (y_dir * -1) + (random(-1,2) / 10.0);
    y_pos += y_dir;
  }
  
  byte row = (int)(y_pos);
  byte col = (int)(x_pos);
  SetLight(col, row, foreColor);
  SetLight(col, row+1, foreColor);
  SetLight(col, row-1, foreColor);
  SetLight(col-1, row+1, foreColor);
  SetLight(col-1, row-1, foreColor);
}
  
/*
 * lightwave - Just one pixel traveling from 0 to numLights
 */
 
void lightwave() {
  for (int i = 0; i < numLights; i++) {
    if (i == (cycle % numLights)) {
      setPixelColor(i, foreColor);
    } else {
      setPixelColor(i, BLCK);
    }
  }
}

/*
 * twowave - Two pixels traveling in opposite directions
 */
 
void twowave() {
  for (int i = 0; i < numLights; i++) {
    if (i == (cycle % numLights) || i == (numLights - 1 - (cycle % numLights))) {
      setPixelColor(i, foreColor);
    } else {
      setPixelColor(i, BLCK);
    }
  }
}

//
// Pulse
//
void Pulse() {
  
  for (int i=0; i < numLights; i++) {
    if (cycle % 2) setPixelColor(i, foreColor);
    else setPixelColor(i, BLCK);
  }
}

//
// Color morphing
//
void morph_frame() {
  for (int i = 0; i < numLights; i++) {
     strip.setPixelColor(i, RGBinterWheel(current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait)));
   }
   strip.show();  // Update the display
}

//
//  RGB Interpolate Wheel
//
//  Wrapper for RGB Interpolate RGB below
//  start and end colors are 0-252 wheel colors

uint32_t RGBinterWheel(byte c1, byte c2, float fract)
{
  return(RGBinter(Wheel(c1), Wheel(c2), fract));
}

//
//  RGB Interpolate
//

uint32_t RGBinter(uint32_t c1, uint32_t c2, float fract)
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
  if (fract <= 0) return(Color(r1,g1,b1));
  if (fract >= 1) return(Color(r2,g2,b2));
  return(Color(interpolate(r1,r2,fract), interpolate(g1,g2,fract), interpolate(b1,b2,fract) ));
}

//
// Interpolate
//
// Returns the fractional point from a to b

float interpolate(byte a, byte b, float fract)
{
  return(a + (fract*(b-a)));
}

// Extracts the red part of a 24-bit color
byte GetRed(uint32_t c)
{
  return (c & 0xff);  
}

// Extracts the green part of a 24-bit color
byte GetGreen(uint32_t c)
{
  return ((c >> 8) & 0xff);
}

// Extracts the green part of a 24-bit color
byte GetBlue(uint32_t c)
{
  return ((c >> 16) & 0xff);
}

void setPixelColor(int pos, byte color) {
  next_frame[pos] = color;
}

/*
 * Get Delay Time - Returns a delay time from an array.
 */
 
int GetDelayTime(int wait) {
  byte DelayValues[MAX_WAIT] = { 2, 3, 4, 6, 8, 10, 15, 20, 30, 50, 75, 100 };
  return (DelayValues[wait % MAX_WAIT]);
}

/* Helper functions */

//
// SetLight
//
// Returns the LED position from a [row][column] array
// Array is stored in PROGMEM to save on RAM
//
void SetLight (byte column, byte row, byte color) {
  if (column < 0 || column >= ARRAY_WIDTH || row < 0 || row >= ARRAY_HEIGHT) {
    return;  // column or row is out of bounds
  }
  byte LED = pgm_read_byte_near(eye_pos + column + (row * ARRAY_WIDTH));
  
  if (LED != NONE) {
    setPixelColor(LED, color);
  }
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

// Input a value 42 * 6 to get a color value.
// The colours are a transition r - g -b - back to r
// Intensity is scaled down with increasing number of lights
// to prevent overloading the power supply

uint32_t Wheel(byte color)
{
  return Gradient_Wheel(color, 25.0 / numLights);
}

//Input a value 24 * 6 to get a color value.
//The colours are a transition r - g -b - back to r
//Intensity must be 0 <= intensity <= 1
uint32_t Gradient_Wheel(byte color, float intensity)
{
  byte r,g,b;
  byte channel, value;
  
  if (intensity > 1) intensity = 1;
  if (intensity < 0) intensity = 0;
  
  if (color == BLCK) return (Color(0,0,0));
  if (color == WHTE) {
    return (Color(255*intensity, 255*intensity, 255*intensity));
  }
  
  color = color % MaxColor;  // Keep colors within bounds
  
  channel = color / 42;
  value = (color % 42) * 6.22;
  
  switch(channel)
  {
    case 0:
      r = 255;
      g = value;
      b = 0;        
      break;
    case 1:
      r = 255 - value;
      g = 255;
      b = 0;        
      break;
    case 2:
      r = 0;
      g = 255;
      b = value;        
      break;
    case 3:
      r = 0;
      g = 255 - value;
      b = 255;        
      break;
    case 4:
      r = value;
      g = 0;
      b = 255;        
      break;
    default:
      r = 255;
      g = 0;
      b = 255 - value;        
      break; 
   }
  return(Color(r * intensity, g * intensity, b * intensity));
}   
