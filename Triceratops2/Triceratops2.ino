#include "FastLED.h"

//
//  Triceratops - 45 cones on a coat
//
//  5/17/16
//
//  Fast LED control
//
/*****************************************************************************/
//
//  Set Up Pins
//
//  2 pins for clock and data of lights.
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 47

// framebuffers
byte current_frame[numLights];
byte next_frame[numLights];

// For random-fill show (nasty global)
byte shuffle[numLights];  // Will contain a shuffle of lights

// Set up the Fast LEDs
CRGB strip[numLights];

// Light colors

#define BLACK     89   // Somewhat arbitrary dummy position
#define WHITE    171   // Somewhat arbitrary dummy position
#define maxColor 256   // Colors are 0-255

byte foreColor = 200;    // Starting foreground color
byte backColor = 50;    // Starting background color

// Shows

byte show = 2;       // Starting show
#define MAX_SHOW 17  // Total number of shows

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated

// Delays
int wait = 0;
#define MAX_WAIT 12 // Number of stored delay times 

// Lookup tables

byte ConeLookUp[45] = {
      44,
    42, 43,
  41, 40, 32,
    39, 33,
  38, 34, 31,
    35, 30,
  36, 29, 20,
37, 28, 21, 19,
  27, 22, 18,
26, 23, 17,  9,
  24, 16,  8,
25, 15,  7, 10,
  14,  6, 11,
     5, 12,
   4, 13,  0,
     3,  1,
       2
};

byte Stripe_Pattern[45] = {
       1,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
       0
};

byte Explode_Pattern[45] = {
       6,
     5,  5,
   5,  4,  5,
     4,  4,
   4,  3,  4,
     3,  3,
   3,  2,  3,
 3,  1,  1,  3,
   2,  0,  2,
 3,  1,  1,  3,
   3,  2,  3,
 4,  3,  3,  4,
   4,  3,  4,
     4,  4,
   5,  4,  5,
     5,  5,
       6
};

byte Alternate_Pattern[45] = {
       0,
     1,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
 0,  0,  1,  0,
   1,  0,  0,
 1,  0,  1,  0,
   1,  0,  0,
 0,  0,  0,  1,
   0,  1,  0,
     0,  0,
   1,  0,  1,
     0,  0,
       1
};

byte SideSide_Pattern[45] = {
       3,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
       3
};

byte Diag_Pattern[45] = {
       0,
     1,  0,
   2,  1,  0,
     2,  1,
   3,  2,  1,
     3,  2,
   4,  3,  2,
 5,  4,  3,  2,
   5,  4,  3,
 6,  5,  4,  3,
   6,  5,  4,
 7,  6,  5,  4,
   7,  6,  5,
     7,  6,
   8,  7,  6,
     8,  7,
       8
};

byte ConeSize[45] = { 5,5,5,5,5,3,5,1,3,5,1,1,5,4,3,4,5,5,6,3,6,2,1,6,6,4,2,6,2,1,3,4,2,4,4,5,3,2,2,3,1,6,3,3,1 };

//
// Setup
//
void setup() {
  
  Serial.begin(9600);
  Serial.println("Start");
  
  randomSeed(analogRead(0));
  
  // Start up the LED counter
  FastLED.addLeds <WS2801, dataPin, clockPin> (strip, numLights);
  
  clear();
}

void loop() { 
   
  delay(50);   // The only delay!
  
  // Check if the lights need updating
  
  if (update) {
    
    update = false;
    
    switch(show) {
    
      case 0:    
        allOn();
        break;
      case 1:
        randomfill();
        break;
      case 2:
        randomcolors();
        break;
      case 3:
        twocolor();
        break;
      case 4:
        lightwave();
        break;
      case 5:
        lightrunup();
        break;
      case 6:
        colorsize();
        break;
      case 7:
        brightsize();
        break;
      case 8:
        stripe();
        break;
      case 9:
        alternate();
        break;
      case 10:
        diagcolor();
        break;
      case 11:
        sidesidecolor();
        break;
      case 12:
        explodecolor();
        break;
      case 13:
        diagbright();
        break;
      case 14:
        sidesidebright();
        break;
      case 15:
        explodebright();
        break;
      default:
        clear();
        break;
    }
  }
  
  // Morph the display
  
  morph_frame();
  
  // Advance the clock
  
  if (morph++ >= GetDelayTime(wait)) {  // Finished morphing
    update = true;  // Force an update
    push_frame();
    morph = 0;  // Reset morphClock

    if (cycle++ > 1000) cycle = 0;  // Advance the cycle clock
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(100)) {
    foreColor = IncColor(foreColor, -1);
    Serial.print("New foreColor :");
    Serial.println(foreColor);
    update = true;
  }
  
  if (!random(200)) {
    backColor = IncColor(backColor, -1);
    update = true;
  }
  
  if (!random(4000)) {
    //show = random(MAX_SHOW);
    //Serial.print("New show :");
    //Serial.println(show);
    morph = 0;
    cycle = 0;
    update = true;
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

//
// push_frame()
//
void push_frame() {
  for (int i = 0; i < numLights; i++) {
    current_frame[i] = next_frame[i];
  }
}

// clear
//
// set all cells to black but don't call show yet
// ignores buffering
// 
void clear() {
  for (int i=0; i < numLights; i++) {
    strip[i] = CRGB::Black;
    setPixel(i, BLACK);
  }
  FastLED.show();
  push_frame();
}

// clear with fade
//
// Fades lights to black
//

void clearWithFade() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, BLACK);
  }
}

//
// All On
//
// Simply turns all the pixels on to one color
// 
void allOn() {
   for (int i=0; i < numLights; i++) {
     setPixel(i, foreColor);
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
  if (pos >= numLights) {
    pos = (numLights*2) - pos;  // For a sawtooth effect
  }
  
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
  
  for (i=0; i < numLights; i++) {
    if (i < pos) {  
      setPixel(shuffle[i], foreColor);  // Turning on lights one at a time
    } else { 
      setPixel(shuffle[i], BLACK);  // Turning off lights one at a time
    }
  }
}  

// random colors
//
// randomcolors: turns each pixel on to a random color
//
void randomcolors() {
  int i;
  
  if (cycle == 0) {  // Start of show: assign lights to random colors
    for (i=0; i < numLights; i++) {
      shuffle[i] = random8();
    }
  }
  
  // Otherwise, fill lights with their color
  for (i=0; i < numLights; i++) {
    setPixel(i, CheckColor(shuffle[i], 0));
  }
}

// colorsize
//
// Light each cone according to its cone size
//
void colorsize() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(foreColor, ((ConeSize[i]-1) * backColor) % maxColor));
  }
}

// brightsize
//
// Light just one cone size
//
void brightsize() {
  for (int i=0; i < numLights; i++) {
    if (cycle % 6 == ConeSize[i] - 1) {
      setPixel(i, IncColor(backColor, (5 * (ConeSize[i]-1)) % maxColor));
    } else {
      setPixel(i, BLACK);
    }
  }
}

void stripe() {
  for (int i=0; i < numLights; i++) {
    if (Stripe_Pattern[ConeLookUp[i]]) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, backColor);
    }
  }
}

void alternate() {
  for (int i=0; i < numLights; i++) {
    if (Alternate_Pattern[ConeLookUp[i]]) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, backColor);
    }
  }
}

void diagcolor() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(foreColor, 10 * (cycle + Diag_Pattern[ConeLookUp[i]] % 9)));
  }
}

void sidesidecolor() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(foreColor, 20 * (cycle + SideSide_Pattern[ConeLookUp[i]] % 7)));
  }
}

void explodecolor() {
  for (int i=0; i < numLights; i++) {
    setPixel(i, IncColor(foreColor, 5 * (cycle + Explode_Pattern[ConeLookUp[i]] % 7)));
  }
}

void diagbright() {
  for (int i=0; i < numLights; i++) {
    if (cycle % 9 == Diag_Pattern[ConeLookUp[i]]) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, BLACK);
    }
  }
}

void sidesidebright() {
  for (int i=0; i < numLights; i++) {
    if (cycle % 7 == SideSide_Pattern[ConeLookUp[i]]) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, BLACK);
    }
  }
}

void explodebright() {
  for (int i=0; i < numLights; i++) {
    if (cycle % 7 == Explode_Pattern[ConeLookUp[i]]) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, BLACK);
    }
  }
}
// two color
//
// alternates the color of pixels between two colors
//
void twocolor() {
  for (int i=0; i < numLights; i++) {
    if (i%2) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, backColor);
    }
  }
}

//
// lightwave
//
// Just one pixel traveling along the chain
 
void lightwave() {
  for (int i=0; i < numLights; i++) {
     if (i == numLights-(cycle % numLights)-1) {
       setPixel(ConeLookUp[i], foreColor);
     } else {
       setPixel(ConeLookUp[i], BLACK);
     }
  }
}

//
// lightrunup
//
// Wave filling in and out

void lightrunup() {
  int i, pos;
  
  pos = cycle % (numLights*2);  // Where we are in the show
  if (pos >= numLights) {
    pos = (numLights*2) - pos;  // For a sawtooth effect
  }
  
  for (i=0; i < numLights; i++) {
    if (i < pos) {
      setPixel(ConeLookUp[i], BLACK);
    } else {
      setPixel(ConeLookUp[i], foreColor);
    }
  }
}
    
//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
void morph_frame() {
   for (int i = 0; i < numLights; i++) {
     if (current_frame[i] != next_frame[i]) {
       calcPixelColor(i, current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait));
     }
   }
   FastLED.show();
}

//
// setPixel
//
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
      strip[pos].setHSV(color, 255, 255);
      break;
  }
}

void calcPixelColor(byte i, byte init, byte final, float amount) {
  if (amount <= 0.0) {
    sendPixelColor(i, init);
    return;
  }
  if (amount >= 1.0) {
    sendPixelColor(i, final);
    return;
  }
  if (init == BLACK && final == BLACK) {
    strip[i] = CRGB::Black;
    return;
  }
  if (final == BLACK) {
    sendPixelColor(i, init);                // Reload/refresh full color
    strip[i].fadeToBlackBy( 256 * amount ); // Now fade it
    return;
  }
  if (init == BLACK) {
    sendPixelColor(i, final);                       // Reload/refresh full color
    strip[i].fadeToBlackBy( 256 * (1.0 - amount) ); // Now fade it
    return;
  }
  morphColor(i, init, final, amount);
  return;
}

void morphColor(byte i, byte init, byte final, float amount) {
  if (init == BLACK || final == BLACK) {
    Serial.println("Morphing on black")
  }
  strip[i].setHue( ((final-init)*amount) + init);
}
  
//
// IncColor
//
// Adds amount to color
// Corrects for out of bounds and BLACK and WHITE
//
byte IncColor(byte color, int amount) {
  byte value = color + amount;
  while (value < 0) value += 255;
  while (value >= 255) value -= 255;
  return CheckColor(value, amount);  // Avoid stepping into black or white
}

//
// CheckColor
//
// Checks for the BLACK and WHITE condition
//
byte CheckColor(byte color, int amount) {
  if (color == BLACK || color == WHITE) {
    if (amount > 0) {
      return (color+1);
    } else {
      return (color-1);
    }
  }
  return color;
}

//
// Get Delay Time
//
// Returns a delay time from an array
//
 
int GetDelayTime(int wait) {
  int DelayValues[MAX_WAIT] = { 6, 8, 10, 15, 20, 30, 50, 75, 100, 150, 200, 250 };
  return (DelayValues[wait % MAX_WAIT]);
}
