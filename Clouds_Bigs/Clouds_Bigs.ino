/******************
 *
 *   Clouds - 24 (but adjustable)
 *
 *   2/8/2016
 *
 *   Colors: full, morphing, no HSV correction
 *
 ******************/

#define datapin   5 // DI
#define latchpin  6 // LI
#define enablepin 7 // EI
#define clockpin  8 // CI

#define NumLEDs    24

// framebuffers
uint32_t current_frame[NumLEDs];
uint32_t next_frame[NumLEDs];

int LEDChannels[NumLEDs][3] = {0};    // 8 for each octobar

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int clock = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true;

#define MAX_CLOCK   10000

// Delays
#define WAIT_TIME 10 // Number of morphs in a cycle; decrease to speedu up cycles

// Wave Parameters
#define MIN_FREQ 24 // fastest pendulum does this many cycles in one loop
#define CYCLE_TIME 5 // speed of one cycle
int cloudOffset = 0;

// Colors
#define BRIGHTNESS 100  // 0 - 100
#define MaxColor 6144   // Number of colors: 1024 * 6
#define YELLOW 600
int lightColor[NumLEDs] = {YELLOW};

unsigned long SB_CommandPacket;
int SB_CommandMode;
int SB_BlueCommand;
int SB_RedCommand;
int SB_GreenCommand;

void setup() {
   Serial.begin(9600);
   
   pinMode(datapin, OUTPUT);
   pinMode(latchpin, OUTPUT);
   pinMode(enablepin, OUTPUT);
   pinMode(clockpin, OUTPUT);

   digitalWrite(latchpin, LOW);
   digitalWrite(enablepin, LOW);
      
   // Initialize the lights, to start they are all 'off'
  
   BlackLights();
   
   // Put a random color into each cloud
   
   for (int i = 0; i < NumLEDs; i++ ) {
     lightColor[i] = GetRandomCloudColor();
   }
}

void loop() {

  delay(20);   // The only delay!
  
  // Check if the lights need updating
  
  if (update) {
    update = false;
    CalculateWave(clock);
  }
  
  // Morph the display
    
  morph_frame();
  
  // Advance the clock
   
  if (morph++ >= WAIT_TIME) {  // Finished morphing
    
    update = true;  // Force an update
    
    morph = 0;  // Reset morphClock
    
    // Advance the cycle clock
    
    if (clock++ > MAX_CLOCK) clock = 0;
    
    // Update the frame display
  
    for (int i = 0; i < NumLEDs; i++) {
      current_frame[i] = next_frame[i];
    }
  }
}

//
// CalculateWave
//
void CalculateWave(int clock) {
  float w, intensity;
  
  for (int i = 0; i < NumLEDs; i++) {
    w = (MIN_FREQ + ((i + cloudOffset) % NumLEDs)) / float(CYCLE_TIME * WAIT_TIME);  // pendulum frequency
    intensity = (1.0 + cos(w * clock)) / 2;
    
    if (intensity < 0.002 && !random(3)) {  // The closer to zero, the briefer the lightning blast
      LightLED(i, Wheel(YELLOW));  // Lighting!
      if (!random(10)) {
        lightColor[i] = GetRandomCloudColor();  // Pick a new color
      }
    } else {
      LightLED(i, Gradient_Wheel(lightColor[i] , intensity));
    }
  }
  if ((clock % int(2 * 3.1415 * CYCLE_TIME * WAIT_TIME)) == 0) {
    cloudOffset = random(NumLEDs);
  }
}

//
// GetRandomCloudColor - pick blues, reds, and oranges / no greens or yellows
//
int GetRandomCloudColor() {
  int randColor = random(4244);
  return (randColor + 1900);
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
// Light Cloud
//
// Put color into a particular numbered Cloud
// Values go into the next_frame array
// Does not light up the array
//
void LightLED(int Cloud, uint32_t color) {
  next_frame[Cloud] = color;
}

// Black Clouds
//
// Immediately turns off all Clouds
//
void BlackLights() {
  for (int i=0; i<NumLEDs; i++) {
    current_frame[i] = Color(0,0,0);        // Black
    next_frame[i] = Color(0,0,0);           // Black
    UpdateCloud(i, Color(0,0,0));    // Black
  }
  WriteClouds();  // Immediately turn the Clouds off
}

//
// Update Cloud
//
// Decompresses 30-bit color into 10-bit r,g,b
// Updates the r,g,b values into the LED array
// Does not call the array; WriteClouds() does that
//
void UpdateCloud(int Cloud, uint32_t color) {
  for (int j = 0; j < 3; j++) {
    LEDChannels[Cloud][j] = (color >> (j * 10)) & 0x3ff;
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
     UpdateCloud(i, RGBinter30(current_frame[i], next_frame[i], (float)morph/WAIT_TIME));
   }
   WriteClouds();  // Update the display
}

//
// Create a 30-bit color value from 10-bit R,G,B values
//
uint32_t Color(uint32_t r, uint32_t g, uint32_t b)
{ 
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
  //color = color * 2 / 3;  // Break color into 4 (not 6) channels
  
  channel = color / 1024;
  value = color % 1024;
  
  intensity = intensity * BRIGHTNESS / 100.0;
  
  if (intensity > 1) intensity = 1;
  if (intensity < 0) intensity = 0;
  
  // Red party
  /*
  switch(channel)
  {
    case 0:
      r = 255;
      g = value;
      b = 0;        
      break;
    case 1:
      r = 255;
      g = 255 - value;
      b = 0;        
      break;
    case 2:
      r = 255;
      g = 0;
      b = value;         
      break;
    default:
      r = 255;
      g = 0;
      b = 255 - value;        
      break;
   }
   */
   /*
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
  */
  // This is the usual wheel
  
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
  
   
  return(Color(r * intensity, g * intensity, b * intensity));
}

//
// WriteClouds
//
// Push the color data into the lights
// From MaceTech website: http://docs.macetech.com/doku.php/shiftbrite#command_format
//
//
void WriteClouds() {
 
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

