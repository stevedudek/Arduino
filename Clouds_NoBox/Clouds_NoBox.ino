/******************
 *
 *   Clouds - 18
 *
 *   12/16/2016
 *
 *   No XBee Communication
 *
 *   Colors: full, morphing, no HSV correction
 *
 ******************/

#define datapin   5 // DI
#define latchpin  6 // LI
#define enablepin 7 // EI
#define clockpin  8 // CI

#define NumLEDs    16

// framebuffers
uint32_t current_frame[NumLEDs];
uint32_t next_frame[NumLEDs];

// For random-fill show (nasty global)
byte shuffle[NumLEDs];  // Will contain a shuffle of lights

int LEDChannels[NumLEDs][3] = {0};    // 8 for each octobar

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated

#define MAX_CLOCK   6000      // Total length of the story

// Delays
int wait = 6;
#define MAX_WAIT 12 // Number of stored delay times 

// Shows
int show = 0;       // Which show we are on
#define MAX_SHOW  5  // Total number of shows

// Colors
int foreColor = 1000;   // Foreground color
int backColor = 2000;   // Background color
#define MaxColor 6144   // Number of colors: 1024 * 6

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
      
   // Initialize the lights, to start they are all 'off'
  
   BlackLights();
   ShuffleLights();
 
   Serial.begin(9600);
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
    
    if (cycle++ > MAX_CLOCK) cycle = 0;
    
    // Update the frame display
  
    for (int i = 0; i < NumLEDs; i++) {
      current_frame[i] = next_frame[i];
    }
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(5)) {
    foreColor = (foreColor + 1) % MaxColor;
    update = true;
  }
  
  if (!random(5)) {
    backColor -= 2;
    if (backColor < 0) backColor += MaxColor;
    update = true;
  }
  
  if (!random(1000)) {  // Change the show
    show = random(MAX_SHOW);
   
    morph = 0;
    clearLights();
  }
  
  if (!random(100)) {
    wait++;
    if (wait >= MAX_WAIT) {
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
    LightLED(i, Wheel(foreColor));
  }
}

// two color
//
// lights half of the cones one color, the other half the other color
//
void twocolor() {
  
  if ((cycle % 100) == 0) ShuffleLights();
  
  for (int i=0; i < NumLEDs; i++) {
    if (shuffle[i] % 2) {
      LightLED(i, Wheel(foreColor));
    } else {
      LightLED(i, Wheel(backColor));
    }
  }
}

// RandomColor
//
// lights the cones with a random gradient rainbow
//
void RandomColor() {
  int currColor;
  
  if ((cycle % 100) == 0) ShuffleLights();
  
  for (int i=0; i < NumLEDs; i++) {
    currColor = (foreColor + (shuffle[i] * MaxColor / NumLEDs)) % MaxColor;
    LightLED(i, Wheel(currColor));
  }
}

//
// Pulse
//
void Pulse() {
  
  if ((cycle % 100) == 0) ShuffleLights();
  
  float intense = (sin( (cycle % 360) / (2 * 3.14159) ) + 1) / 2.0;
  
  for (int i=0; i < NumLEDs; i++) {
    if (shuffle[i] % 2) {
      LightLED(i, Gradient_Wheel(backColor, intense));
    } else {
      LightLED(i, Gradient_Wheel(backColor, 1.0 - intense));
    }
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
    LightLED(shuffle[pos], Wheel(foreColor));  // Turning on lights one at a time
  } else { 
    LightLED(shuffle[(NumLEDs*2)-pos-1], Color(0,0,0));  // Turning off lights one at a time
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
// clear Lights
// 
// set all lights to black
//
void clearLights() {
  for (int i=0; i<NumLEDs; i++) {
    LightLED(i, Color(0,0,0));    // Black
  }
}

//
// Light Cone
//
// Put color into a particular numbered cone
// Values go into the next_frame array
// Does not light up the array
//
void LightLED(int cone, uint32_t color) {
  next_frame[cone] = color;
}

// Black Cones
//
// Immediately turns off all cones
//
void BlackLights() {
  for (int i=0; i<NumLEDs; i++) {
    current_frame[i] = Color(0,0,0);        // Black
    next_frame[i] = Color(0,0,0);           // Black
    UpdateCone(i, Color(0,0,0));    // Black
  }
  WriteCones();  // Immediately turn the cones off
}

//
// Update Cone
//
// Decompresses 30-bit color into 10-bit r,g,b
// Updates the r,g,b values into the LED array
// Does not call the array; WriteCones() does that
//
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
// RGBinter30 interpolates between current and next frames
//
void morph_frame() {
   for (int i = 0; i < NumLEDs; i++) {
     UpdateCone(i, RGBinter30(current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait)));
   }
   WriteCones();  // Update the display
}

//
// Create a 30-bit color value from 10-bit R,G,B values
//
uint32_t Color(uint32_t r, uint32_t g, uint32_t b)
{ 
  // uint32_t colorbit;
  // int32_t result = (g & 0x3FF)<<20 | (b & 0x3FF)<<10 | r & 0x3FF;
  // colorbit = (result >> 10) & 0x3ff;

  //Take the lowest 10 bits of each value and append them end to end
  //return( ((unsigned int)g & 0x3FF )<<20 | ((unsigned int)b & 0x3FF)<<10 | (unsigned int)r & 0x3FF);
  return((g & 0x3FF)<<20 | (b & 0x3FF)<<10 | r & 0x3FF);
}

//
//Input a value 0 to 6144 to get a color value.
//The colours are a transition r - g -b - back to r
//
uint32_t Wheel(int color)
{
  return Gradient_Wheel(color, 1);  // Intensity = 1
}

//
//Input a value 0 to 6144 to get a color value.
//The colours are a transition r - g -b - back to r
//Intensity must be 0 <= intensity <= 1
//
uint32_t Gradient_Wheel(int color, float intensity)
{
  int r,g,b;
  int channel, value;
  
  color = color % MaxColor;  // Keep colors within bounds
  color = color * 2 / 3;  // Break color into 4 (not 6) channels
  
  channel = color / 1024;
  value = color % 1024;
  
  if (intensity > 1) intensity = 1;
  if (intensity < 0) intensity = 0;
  
  // Blue party - These values are different
  
  switch(channel)
  {
    case 0:
      b = 1023;
      g = value;
      r = 0;        
      break;
    case 1:
      b = 1023;
      g = 1023 - value;
      r = 0;        
      break;
    case 2:
      b = 1023;
      g = 0;
      r = value;        
      break;
    default:
      b = 1023;
      g = 0;
      r = 1023 - value;        
      break; 
   }
  
  /*  This is the usual wheel
  
  switch(channel)
  {
    case 0:
      r = 1023;
      g = value;
      b = 0;        
      break;
    case 1:
      r = 1023 - value;
      g = 1023;
      b = 0;        
      break;
    case 2:
      r = 0;
      g = 1023;
      b = value;        
      break;
    case 3:
      r = 0;
      g = 1023 - value;
      b = 1023;        
      break;
    case 4:
      r = value;
      g = 0;
      b = 1023;        
      break;
    default:
      r = 1023;
      g = 0;
      b = 1023 - value;        
      break; 
   }
   */
   
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
//
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
  int DelayValues[MAX_WAIT] = { 20, 30, 40, 60, 80, 100, 150, 200, 300, 500, 750, 1000 };
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
  if (fract <= 0) return(Color(r1,g1,b1));
  if (fract >= 1) return(Color(r2,g2,b2));
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
  return (c & 0x3ff);  
}

// Extracts the green part of a 30-bit color
int GetGreen(uint32_t c)
{
  return ((c >> 10) & 0x3ff);
}

// Extracts the green part of a 30-bit color
int GetBlue(uint32_t c)
{
  return ((c >> 20) & 0x3ff);
}

// If r=g=b=0, return true
boolean IsBlack(int r, int g, int b)
{
  if(r=g=b=0) return (true);
  else return (false);
}

