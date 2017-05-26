#include "FastLED.h"

//
//  Ankylosaurus - Attempt at 2-Dimensional Light Shows
//
//  10/18/15
//
//  Uses Fast LED
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights.
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 28
#define bodyLights 26

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

// Light Positions

#define LEFT_TAIL  26
#define RIGHT_TAIL 27

#define XX 99
#define light_width 3
#define light_height 16

// Wiring lay-out for the lights
byte address[][3] = {
  {XX, 0,XX},
  {XX, 1,XX},
  {XX, 2,XX},
  {XX, 3,XX},
  {XX, 4,XX},
  {13,14,15},
  {12, 5,16},
  {11, 6,17},
  {10, 7,18},
  { 9, 8,19},
  {XX,20,XX},
  {XX,21,XX},
  {XX,22,XX},
  {XX,23,XX},
  {XX,24,XX},
  {XX,25,XX}
};

byte foreColor = 10;    // Starting foreground color
byte backColor = 50;    // Starting background color

byte tailforeColor = 10;    // Starting tail foreground color
byte taildiffColor = 50;    // Starting tail different color

// Shows

byte show = 5;       // Starting show
#define MAX_SHOW 5  // Total number of shows

byte tail_show = 0;       // Starting tail_show
#define MAX_TAIL_SHOW 5  // Total number of tail_shows

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated

// Delays
int wait = 6;
#define MAX_WAIT 12 // Number of stored delay times 

// Globals for the 2D show
#define x_max 25
#define y_max 10
int x_grade, y_grade, x_dir, y_dir;
  
//
// Setup
//
void setup() {
  
  //Serial.begin(9600);
  //Serial.println("Start");
  
  randomSeed(analogRead(0));
  randomize_2D_show();
  
  // Start up the LED counter
  FastLED.addLeds <WS2801, dataPin, clockPin> (strip, numLights);
  
  // Initialize the strip, to start they are all 'off'
  
  for (int i = 0; i < numLights; i++) {
    current_frame[i] = BLACK;        // Black
    next_frame[i] = BLACK;           // Black
    strip[i] = CRGB::Black;
  }
  FastLED.show();
}

void loop() { 
   
  delay(20);   // The only delay!
  
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
      default:
        two_d_color(); // 2D show
        break;
    }
    
    switch(tail_show) {
    
      case 0:    
        tailon();
        break;
      case 1:
        tailpulse();
        break;
      case 2:
        tailblink();
        break;
      case 3:
        tailtwocolor();
        break;
      default:
        tailrainbow();
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
    
    // Update the frame display
  
    for (int i = 0; i < numLights; i++) {
      current_frame[i] = next_frame[i];
    }
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(100)) {
    foreColor = IncColor(foreColor, 1);
    update = true;
  }
  
  if (!random(200)) {
    backColor = IncColor(backColor, -1);
    update = true;
  }
  
  if (!random(50)) {
    tailforeColor = IncColor(tailforeColor, 1);
    update = true;
  }
  
  if (!random(50)) {
    taildiffColor = IncColor(taildiffColor, -1);
    update = true;
  }
  
  if (!random(4000)) {
    // Alternate between a random show and the 2D-show
    if (show == MAX_SHOW) {
      show = random(MAX_SHOW);
    } else {
      show = MAX_SHOW;
    }
    morph = 0;
    cycle = 0;
    clearWithFade();
  }
  
  if (!random(1000)) {
    tail_show = random(MAX_TAIL_SHOW);
  }
  
  if (!random(1000)) {
    wait++;
    if (wait >= MAX_WAIT) {
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
  for (int i=0; i < numLights; i++) {
    strip[i] = CRGB::Black;
    setPixel(i, BLACK);
  }
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
   for (int i=0; i < bodyLights; i++) {
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
  
  pos = cycle % (bodyLights*2);  // Where we are in the show
  if (pos >= bodyLights) {
    pos = (bodyLights*2) - pos;  // For a sawtooth effect
  }
  
  if (pos == 0) {  // Start of show
  
    // Shuffle sort to determine order to turn on lights
    for (i=0; i < bodyLights; i++) shuffle[i] = i; // before shuffle
    for (i=0; i < bodyLights; i++) {  // here's position
      j = random(bodyLights);         // there's position
      save = shuffle[i];
      shuffle[i] = shuffle[j];       // first swap
      shuffle[j] = save;             // second swap
    }
  }
  
  for (i=0; i < bodyLights; i++) {
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
    for (i=0; i < bodyLights; i++) {
      shuffle[i] = random8();
    }
  }
  
  // Otherwise, fill lights with their color
  for (i=0; i < bodyLights; i++) {
    setPixel(i, shuffle[i]);
  }
}

// two color
//
// alternates the color of pixels between two colors
//
void twocolor() {
  for (int i=0; i < bodyLights; i++) {
    if (i%2) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, backColor);
    }
  }
}

//
// two_d_color
//
// Maps forecolor into 2 dimensions: width and height
//
void two_d_color() {
  byte pixel;
  float color, move_x, move_y;
  
  // Randomize 2D-show conditions at the start of each cycle
  if (cycle == 0) {
    randomize_2D_show();
  }
  
  change_gradients();
  
  for (int x=0; x < light_width; x++) {
    for (int y=0; y < light_height; y++) {
      pixel = address[y][x];
      if (pixel == XX) {
        continue;  // Not an addressable light
      }
      
      color = (float)foreColor;
      
      move_x = x + (cycle / 5.0);
      move_y = y + (cycle / 5.0);
      
      color += (move_x * x_grade);
      color += (move_y * y_grade);
      
      strip[pixel].setHue((int)color);
    }
  }
}

// Slowly change gradients
void change_gradients() {
  if (!random(20)) {
    if (abs(x_grade + x_dir) > x_max) {
      x_dir *= -1;
    }
    x_grade += x_dir;
  }
  if (!random(20)) {
    if (abs(y_grade + y_dir) > y_max) {
      y_dir *= -1;
    }
    y_grade += y_dir;
  }
}
  
void randomize_2D_show() {
  x_grade = random(-x_max, x_max);
  y_grade = random(-y_max, y_max);
  x_dir = pick_plus_or_minus();
  y_dir = pick_plus_or_minus();
}

// Returns either +1 or -1
int pick_plus_or_minus() {
  return ((random(2) * 2) - 1);
}

// tail shows - simple!
//
void tailon() {
  set_tail(tailforeColor);
}

void set_tail(byte color) {
  setPixel(LEFT_TAIL, color);
  setPixel(RIGHT_TAIL, color);
}

void tailpulse() {
  if (cycle % 20 < 10) {
    set_tail(tailforeColor);
  } else {
    set_tail(BLACK);
  }
}

void tailblink() {
  if (cycle % 20 < 10) {
    setPixel(LEFT_TAIL, tailforeColor);
    setPixel(RIGHT_TAIL, BLACK);
  } else {
    setPixel(LEFT_TAIL, BLACK);
    setPixel(RIGHT_TAIL, tailforeColor);
  }
}

void tailtwocolor() {
  setPixel(LEFT_TAIL, tailforeColor);
  setPixel(RIGHT_TAIL, taildiffColor);
}

void tailrainbow() {
  set_tail(tailforeColor);
  tailforeColor = IncColor(tailforeColor, 1);
}

//
// lightwave
//
// Just one pixel traveling along the chain
 
void lightwave() {
  for (int i=0; i < numLights; i++) {
     if (i == numLights-(cycle % numLights)-1) {
       setPixel(i, foreColor);
     } else {
       setPixel(i, BLACK);
     }
  }
}
    
//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//

void morph_frame() {
   for (int i = 0; i < numLights; i++) {
     if (show == MAX_SHOW && i < bodyLights) {  // Work-around for 2-d show
       continue;
     }
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
      strip[pos].setHue(color);
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
  while (value > 255) value -= 255;
  return CheckColor(value);  // Avoid stepping into black or white
}

//
// CheckColor
//
// Checks for the BLACK and WHITE condition
//
byte CheckColor(byte color) {
  if (color == BLACK || color == WHITE) {
    return (color+1);
  } else {
    return color;
  }
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
