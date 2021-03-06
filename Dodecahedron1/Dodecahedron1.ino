/******************
 *
 *   Cone Chandelier - Light 12 Macetech LED cones with patterns through two Octobars
 *
 *   No external controller
 *
 *   Included Greg/Robie morphing of colors
 *
 *   Included crazy Greg/Robie bit-shifting
 *
 *
 ******************/

#define datapin   5 // DI
#define latchpin  6 // LI
#define enablepin 7 // EI
#define clockpin  8 // CI

#define NumLEDs  12        // Number of cones

// framebuffers
uint32_t current_frame[NumLEDs];
uint32_t next_frame[NumLEDs];

// For random-fill show (nasty global)
byte shuffle[NumLEDs];  // Will contain a shuffle of lights

// For the snakes show
#define SNAKE_NUM     2  // How many snakes
#define SNAKE_LENGTH  3  // How long the snakes are
byte snake_path[][SNAKE_LENGTH] = { { 6,1,0 }, { 5,10,11 } };

int LEDChannels[NumLEDs][3] = {0};    // 8 for each octobar. Not all channels are used!

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated
int wait = 6;
#define MAX_WAIT 12 // Number of stored delay times 

int show = 3;       // Which show we are on
#define MAX_SHOW  6  // Total number of shows

int foreColor = 1000;   // Foreground color
int backColor = 2000;   // Background color

#define MaxColor 3071     // Number of color options

byte Opposite[NumLEDs] = { 11,10,9,8,7,6,5,4,3,2,1,0 };  // Opposite faces of the dodecahedron
byte neighbors[][5] = {  // 5 neighbors to each hex
  // Remember to -1 to these values
  { 2,4,6,5,10 },  // 1
  { 7,8,4,1,10 },  // 2
  { 12,11,6,4,8 },  // 3
  { 8,3,6,1,2 },  // 4
  { 1,6,11,9,10 },  // 5
  { 3,11,5,1,4 },  // 6
  { 9,12,8,2,10 },  // 7
  { 12,3,4,2,7 },  // 8
  { 5,11,12,7,10 },  // 9
  { 2,1,5,9,7 },  // 10
  { 9,5,6,3,12 },   // 11
  { 7,9,11,3,8 }   // 12
};
  

unsigned long SB_CommandPacket;
int SB_CommandMode;
int SB_BlueCommand;
int SB_RedCommand;
int SB_GreenCommand;

void setup() {
   pinMode(datapin, OUTPUT);
   pinMode(latchpin, OUTPUT);
   pinMode(enablepin, OUTPUT);
   pinMode(clockpin, OUTPUT);

   digitalWrite(latchpin, LOW);
   digitalWrite(enablepin, LOW);
      
  // Initialize the cones, to start they are all 'off'
  
  for (int i=0; i < NumLEDs; i++) {
    current_frame[i] = Color(0,0,0);        // Black
    next_frame[i] = Color(0,0,0);           // Black
  }
  ShuffleLights();
  
  //Serial.begin(9600);
}

void loop() {

  delay(20);   // The only delay!
  
   // Check if the lights need updating
  
  if (update) {
    
    update = false;
  
    switch(show) {
      case 0:
        AllOn();
        break;
      case 1:
        twocolor();
        break;
      case 2:
        RandomFill();
        break;
      case 3:
        RandomColor();
        break;
      case 4:
        Swirls();
        break;
      case 5:
        Pulse();
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
  
    for (int i = 0; i < NumLEDs; i++) {
      current_frame[i] = next_frame[i];
    }
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(10)) {
    foreColor = (foreColor + 1) % MaxColor;
    update = true;
  }
  
  if (!random(10)) {
    backColor -= 2;
    if (backColor < 0) backColor += MaxColor;
    update = true;
  }
  
  if (!random(10000)) {  // Change the show
    show = random(MAX_SHOW);
   
    morph = 0;
    cycle = 0;
    clearLights();
  }
  
  if (!random(1000)) {
    wait++;
    if (wait == MAX_WAIT) {
      wait = 0;
      morph = 0;
    }
  } 
}

//
// All On - Boring! Turn all the lights on to color1
//

void AllOn() {
  for (int i=0; i < NumLEDs; i++) {
    LightCone(i, foreColor);
  }
}

// two color
//
// lights half of the cones one color, the other half the other color
//
void twocolor() {
  
  if (cycle % 100 == 0) ShuffleLights();
  
  for (int i=0; i < NumLEDs; i++) {
    if (shuffle[i] % 2) LightCone(i, Wheel(foreColor));
    else LightCone(i, Wheel(backColor));
  }
}

// RandomColor
//
// lights the cones randomly
//
void RandomColor() {
  
  if (cycle % 100 == 0) ShuffleLights();
  
  for (int i=0; i < NumLEDs; i++) {
    LightCone(i, Wheel(shuffle[i] * 255.0 / NumLEDs));
  }
}

//
// Unit Test
//
// Debugging for the lights
void Unit_Test() {
  for (int i = 0; i < NumLEDs; i++) {
    if (i == cycle % NumLEDs) { 
      if (i == 0) { LightCone(i, Color(255,0,0)); }
      else { LightCone(i, Color(0,0,255)); }
    } else {
      LightCone(i, Color(0,0,0));
    }
  }
}
//
// Pulse
//
void Pulse() {
  
  if (cycle % 100 == 0) ShuffleLights();
  
  float intense = (sin( (cycle % 360) / (2 * 3.14159) ) + 1) / 2.0;
  
  for (int i=0; i < NumLEDs; i++) {
    if (shuffle[i] % 2) { LightCone(i, Gradient_Wheel(backColor, intense)); }
    else                { LightCone(i, Gradient_Wheel(backColor, 1.0 - intense)); }
  }
}

// Random Fill
//
// randomfill: randomly fills in pixels from blank to all on
// then takes them away random until blank
//
 
void RandomFill() {
  
  int pos = cycle % (NumLEDs*2);  // Where we are in the show
  
  if (pos == 0) ShuffleLights();
  
  if (pos < NumLEDs) {  
    LightCone(shuffle[pos], Wheel(foreColor));  // Turning on lights one at a time
  } else { 
    LightCone(shuffle[(NumLEDs*2)-pos-1], Color(0,0,0));  // Turning off lights one at a time
  }
}

//
// Shuffle Lights
//
// Shuffles the global shuffle[] array with the order of lights
// Shuffle sort to determine order to turn on lights
void ShuffleLights() {
  int i,j,save;
  
  for (i=0; i < NumLEDs; i++) shuffle[i] = i; // before shuffle
  for (i=0; i < NumLEDs; i++) {  // here's position
    j = random(NumLEDs);         // there's position
    save = shuffle[i];
    shuffle[i] = shuffle[j];       // first swap
    shuffle[j] = save;             // second swap
  }
}

//
// clear
// 
// set all lights to black
//
void clearLights() {
  for (int i=0; i<NumLEDs; i++) {
    LightCone(i, Color(0,0,0));    // Black
  }
}

//
// Swirls
//

void Swirls() {
  byte center = snake_path[0][0];
  byte opp = Opposite[center];
  
  clearLights();  // Clear everything
  LightCone(center, Wheel(foreColor));  // Swirl centers are foreColor
  LightCone(opp, Wheel(foreColor));
  
  //for (int i = 0; i < 5; i++) {
    LightCone(get_path(center, (/*i + */cycle) % 5), Wheel(backColor));//Gradient_Wheel(backColor, 1 - (i * 0.25)));
    LightCone(get_path(opp,    (/*i + */cycle) % 5), Wheel(backColor));//Gradient_Wheel(backColor, 1 - (i * 0.25)));
  //}
}

//
// Snakes
//
// A snake is a chain of cones with decreasing light intensities

void Snakes() {
  clearLights();
  DrawSnakes();
  MoveSnakes();
}

//
// Draw Snakes
//
// Snake 0 is drawn in the foreColor; Snake 1 is drawn in the backColor
// Background is black

void DrawSnakes() {
  for (int i = 0; i < SNAKE_LENGTH; i++) {
    LightCone(snake_path[0][i], Wheel(foreColor));//Gradient_Wheel(foreColor, 1 - (i * 1.0/SNAKE_LENGTH)));
    LightCone(snake_path[1][i], Wheel(backColor));//Gradient_Wheel(backColor, 1 - (i * 1.0/SNAKE_LENGTH)));
  }
}

// Move Snakes
//
// To move a snake, pick randomly direction 0 or 1 from the head of the snake
// and then shift up the snake path

void MoveSnakes() {
  for (int s = 0; s < SNAKE_NUM; s++) {  // 0,1
    // Push the head further down the chain
    for (int i = 0 ; i < (SNAKE_LENGTH-1); i++) {  // 0,1
      snake_path[s][i+1] = snake_path[s][i];
    }
  }
  // Choose a new head spot
  byte snake_head = get_path(snake_path[0][0], 0);
  snake_path[0][0] = snake_head;
  snake_path[1][0] = Opposite[snake_head];
  //Serial.println(snake_head);
}

// get_path
//
// Input a position and a direction. Returns the pent in that direction

byte get_path(byte pos, byte dir) {  
  return (neighbors[pos][dir] - 1);
}

//
// Light Cone
//
// Put color into a particular numbered cone
// Values go into the next_frame array
// Does not light up the array
//

void LightCone(int cone, uint32_t color) {
  next_frame[cone] = color;
}

//
// Update Cone
//
// Decompresses 30-bit color into 10-bit r,g,b
// Updates the r,g,b values into the LED array
// Does not call the array; WriteCones() does that


void UpdateCone(int cone, uint32_t color) {
  for (int j = 0; j < 3; j++) {
    LEDChannels[cone][j] = (color >> (j * 10)) & 0x3ff;
  }
}

//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code initially from Greg and Robie
//

void morph_frame() {
   for (int i = 0; i < NumLEDs; i++) {
     UpdateCone(i, RGBinter30(current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait)));
   }
   WriteCones();  // Update the display 
}


//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code from Greg and Robie
//
/*
#define STEP_LENGTH 20

void morph_frame(int msec) {
  int t;
  int steps = msec / STEP_LENGTH;
  int deltas[NumLEDs][3];
  
  for (int i = 0; i < NumLEDs; i++) {    
    for (int j = 0; j < 3; j++) {
      uint32_t old_color = (current_frame[i] >> (j * 10)) & 0x3ff; 
      uint32_t new_color = (next_frame[i] >> (j * 10)) & 0x3ff;
      deltas[i][j] = (int)new_color - (int)old_color;
    }
  }

  for (t = 1; t < steps; t++) {
    for (int i = 0; i < NumLEDs; i++) {
      uint32_t color = 0;
      for (int j = 0; j < 3; j++) {
        uint32_t old_color = (current_frame[i] >> (j * 10)) & 0x3ff;
        color |= ((old_color + (deltas[i][j] * t / steps)) & 0x3ff) << (j * 10);
      }
      UpdateCone(i, color);
    }

    WriteCones();
    delay(STEP_LENGTH);      
  }
  
  // Finish with the final case
  for (int i = 0; i < NumLEDs; i++) {  
    current_frame[i] = next_frame[i];
    UpdateCone(i, current_frame[i]);
  }
  WriteCones();
  delay(STEP_LENGTH);
  
  // Randomize settings over time
  // The longer the delay, the more likely to be random
  int chance = 1 + ((MaxDelay - currDelay) / 100) ;
  if(!random(chance)) { foreColor = (foreColor + 10) % MaxColor; }
  if(!random(chance)) { backColor = (backColor - 10) % MaxColor; }
  if(!random(chance*2)) { if (currDelay+=10 > MaxDelay) currDelay = MinDelay; } 
  if(!random(chance*10)) { show = (show + 1) % MaxShow; }
  
  // Update timer. Don't advance if box is changing.
  cycle = (cycle + 1) % 100000;  // Advance timer
}
*/
// Create a 30-bit color value from 10-bit R,G,B values
uint32_t Color(uint32_t r, uint32_t g, uint32_t b)
{ 
  uint32_t colorbit;
  uint32_t result = (g & 0x3FF)<<20 | (b & 0x3FF)<<10 | r & 0x3FF;
     
  colorbit = (result >> 10) & 0x3ff;

  //Serial.print("Out:");
  //Serial.print(colorbit);
  //Serial.println("");
  
  //Take the lowest 10 bits of each value and append them end to end
  //return( ((unsigned int)g & 0x3FF )<<20 | ((unsigned int)b & 0x3FF)<<10 | (unsigned int)r & 0x3FF);
  return((g & 0x3FF)<<20 | (b & 0x3FF)<<10 | r & 0x3FF);
}
  
//Input a value 0 to 3071 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(int color)
{
  //Serial.println(color);
  
  int r,g,b,third;
  
  color = color % 3071;  // Keep colors within 0 to 3071 bounds 
  third = 1;
  if (color<1024) third = 0; // Figure out which third (0-1023)
  if (color>2047) third = 2; // wheel position is in (2048-3071)
  
  color = color % 1024;   // Truncate again to get 0-1024 range  
  
  switch(third)
  {
    case 0:
      r=1023-color;    //Red down
      g=color;         //Green up
      b=0;             //Blue off
      break; 
    case 1:
      g=1023-color;    //Green down
      b=color;         //Blue up
      r=0;             //Red off
      break; 
    case 2:
      b=1023-color;    //Blue down 
      r=color;         //Red up
      g=0;             //Green off
      break;  
  }
  /*
  Serial.print("In:");
  Serial.print(b);
  Serial.print(" ");
  */
  return(Color(r,g,b));
}

//Input a value 0 to 3071 to get a color value.
//The colours are a transition r - g -b - back to r
//Intensity must be 0 <= intensity <= 1
uint32_t Gradient_Wheel(int color, float intensity)
{
  if (intensity > 1) intensity = 1;
  if (intensity < 0) intensity = 0;

  int r,g,b,third;
  
  color = color % 3071;  // Keep colors within 0 to 3071 bounds 
  third = 1;
  if (color<1024) third = 0; // Figure out which third (0-1023)
  if (color>2047) third = 2; // wheel position is in (2048-3071)
  
  color = color % 1024;   // Truncate again to get 0-1024 range  
  
  switch(third)
  {
    case 0:
      r=1023-color;    //Red down
      g=color;         //Green up
      b=0;             //Blue off
      break; 
    case 1:
      g=1023-color;    //Green down
      b=color;         //Blue up
      r=0;             //Red off
      break; 
    case 2:
      b=1023-color;    //Blue down 
      r=color;         //Red up
      g=0;             //Green off
      break;  
  }

  return(Color(r * intensity, g * intensity, b * intensity));
}

//
// WriteCones
//
// Push the color data into the lights
// From MaceTech website: http://docs.macetech.com/doku.php/shiftbrite#command_format
//
//
 
void WriteCones() {
 
    SB_CommandMode = B00; // Write to PWM control registers
    for (int h = 0; h < NumLEDs; h++) {
      
      	  SB_RedCommand = LEDChannels[h][0];
	  SB_GreenCommand = LEDChannels[h][1];
	  SB_BlueCommand = LEDChannels[h][2];
	  SB_SendPacket();
    }
 
    delayMicroseconds(15);
    digitalWrite(latchpin,HIGH); // latch data into registers
    delayMicroseconds(15);
    digitalWrite(latchpin,LOW);
 
    SB_CommandMode = B01; // Write to current control registers
    for (int z = 0; z < NumLEDs; z++) SB_SendPacket();
    delayMicroseconds(15);
    digitalWrite(latchpin,HIGH); // latch data into registers
    delayMicroseconds(15);
    digitalWrite(latchpin,LOW);
 
}

// SB_SendPacket()
//
// From MaceTech website: http://docs.macetech.com/doku.php/shiftbrite#command_format

void SB_SendPacket() {
 
   if (SB_CommandMode == B01) {
     SB_RedCommand = 127;
     SB_GreenCommand = 127;
     SB_BlueCommand = 127;
   }
   
   SB_CommandPacket = SB_CommandMode & B11;
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_BlueCommand & 1023);
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_RedCommand & 1023);
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_GreenCommand & 1023);

   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 24);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 16);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 8);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket); 
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
//  RGB Interpolate Wheel
//
//  Wrapper for RGB Interpolate RGB below
//  start and end colors are 0-255 wheel colors

uint32_t RGBinterWheel(int c1, int c2, float fract)
{
  return(RGBinter30(Wheel(c1),Wheel(c2),fract));
}

//
//  RGB Interpolate 30-bit
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 30-bit colors

uint32_t RGBinter30(uint32_t c1, uint32_t c2, float fract)
{
  return(RGBinterRGB(GetRed(c1),GetGreen(c1),GetBlue(c1), GetRed(c2),GetGreen(c2),GetBlue(c2), fract));
}

//  RGB Interpolate RGB
//
//  Given a start rgb, an end rgb, and a fractional distance (0-1)
//  This function interpolates between the two points
//  and returns the properly interpolated rgb
//  as a 30-bit rgb color

uint32_t RGBinterRGB(int r1, int g1, int b1, int r2, int g2, int b2, float fract)
{
  if (fract < 0) return(Color(r1,g1,b1));
  if (fract > 1) return(Color(r2,g2,b2));
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

// Extracts the red part of a 30-bit color
int GetRed(uint32_t c)
{
  return ((c >> 20) & 0x3ff);
}

// Extracts the green part of a 30-bit color
int GetGreen(uint32_t c)
{
  return ((c >> 10) & 0x3ff);
}

// Extracts the green part of a 30-bit color
int GetBlue(uint32_t c)
{
  return (c & 0x3ff);
}

// If r=g=b=0, return true
boolean IsBlack(int r, int g, int b)
{
  if(r=g=b=0) return (true);
  else return (false);
}

