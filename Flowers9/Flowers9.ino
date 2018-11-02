/******************
 *
 *   Flowers - Light 8 Shiftbrite LED flowers with patters through an Octobar
 *
 *   Disconnected motion sensors
 *
 *   Included Greg/Robie morphing of colors
 *
 *   Made display more DJ friendly
 *
 ******************/

#define datapin   5 // DI
#define latchpin  6 // LI
#define enablepin 7 // EI
#define clockpin  8 // CI

#define button1pin 0  // Button 1 digital pin - the show pin
#define button2pin 1  // Button 2 digital pin - the Effect pin

#define dial1pin  0  // Dial 1 analog pin: Foreground color
#define dial2pin  1  // Dial 2 analog pin: Background color
#define dial3pin  2  // Dial 3 analog pin: Pacing

#define NumLEDs  8        // Number of flowers

// framebuffers
uint32_t current_frame[NumLEDs];
uint32_t next_frame[NumLEDs];

//int MotionPin[NumLEDs] = {2,3,4,9,10,11,12,13}; // Pin-in of motion detectors

int dial1;           // Position of Dial 1
int dial2;           // Position of Dial 2
int dial3;           // Position of Dial 3
boolean button2;     // Status of button2
boolean button2prev; // Previous status of button2
boolean boxChange;   // Has the box been changed?
long button2time=0;  // Timer for button2

int LEDChannels[NumLEDs][3] = {0};

unsigned int time = 1;    // Keeps track of time

int show = 5; // Which show we are on
#define MaxShow  6  // Total number of shows

int currDelay;  // Delay in milliseconds between animation
#define MinDelay 100  // Minimum delay
#define MaxDelay 2000 // Maximum delay

uint32_t color1 = 1000;   // Foreground color
uint32_t color2 = 2000;   // Background color

uint32_t r,g,b;           // Ugly globals! Make computation easy. Don't touch!

#define MaxColor 3071     // Number of color options

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
   
   pinMode(button1pin, INPUT);
   pinMode(button2pin, INPUT);
   
   for (int i=0; i<NumLEDs; i++) {
     //pinMode(MotionPin[i], INPUT);  // set up pins
     current_frame[i] = 0;          // clear display
   }
   
   // Read the initial position of the box
   dial1 = GetDial(1);
   dial2 = GetDial(2);
   dial3 = GetDial(3);
   button2 = digitalRead(button2pin);
   
   // All the start, all flowers are off
   clear();
   WriteFlowers;
   
   //Serial.begin(9600);
}

void loop() {
  
  switch(show) {
    case 0:
      OneLightRunDown();
      break;
    case 1:
      Rainbow();
      break;
    case 2:
      AlternateLights();
      break;
    case 3:
      BlackWave();
      break;
    case 4:
      PairedLightRunDown();
      break;
    case 5:
      AllOn();
      break;
  //case 6:
    //GradedFill();
    //break;    
  }    
  
  morph_frame(currDelay);
  time = (time + 1) % 10000;  // Advance timer
}

/*
 * All On - Boring! Turn all the lights on to color1
 */

void AllOn() {
  for (int i=0; i < NumLEDs; i++) {
    if (EffectOn()) { LightFlower(i, color2); } else { LightFlower(i, color1); }
  }
}

/*
 * One Light Run Down - One light travels from top to bottom
 */

void OneLightRunDown() {
  int onLight = time % NumLEDs;
  
  for (int i=0; i < NumLEDs; i++) {
    if (i == onLight || EffectOn()) { LightFlower(i, color1); } else { LightFlower(i,0); }
  }
}

/*
 * Graded Fill - One light travels from top to bottom
 *
 * Lights further from the traveling light are darkened
 */
/*
void GradedFill() {
  int gradient;
  int onLight = time % NumLEDs;

  for (int i=0; i < NumLEDs; i++) {
    gradient = 100 * ((NumLEDs/2) - CircDiff(i,onLight,NumLEDs)) / (NumLEDs/2);
    if (!EffectOn()) { GradientFlower(i, color1, gradient); } else { GradientFlower(i, color2, gradient); }
  }
}
*/
//
// CircDiff
//
// Calculates the closest difference of two positions as if they were on a circle of length "length"

int CircDiff(int pos1, int pos2, int length) {
  int diff = abs(pos1-pos2);
  return(smaller(diff,length-diff));
}

int smaller(int num1, int num2) {
  if (num1 < num2) { return(num1); } else { return(num2); }
}

/*
 * Paired Light Run Down - Pair of lights travels from top to bottom
 */

void PairedLightRunDown() {
  int onLight = time % (NumLEDs/2);
  
  for (int i=0; i < (NumLEDs/2); i++) {
    if (i == onLight || EffectOn()) {
      LightFlower(i, color1);
      LightFlower(NumLEDs-i-1, color1);
    } else {
      LightFlower(i, color2);
      LightFlower(NumLEDs-i-1, color2);
    }
  }
}

/*
 * Blackwave - Lightwave with a black backgroun
 *
 * Thickness of wave is determined by color2
 */

void BlackWave() {
  boolean WhichOn[NumLEDs];
  int i;
  int onLight = time % NumLEDs;
  int thickness = 1 + ((NumLEDs-2) * color2 / MaxColor);
  
  // Is the Effect on?
  if (EffectOn()) {
    for (i=0; i < NumLEDs; i++) { LightFlower(i, color2); }
    return;
  }
  
   // Fill array first with all false
  for (i=0; i < NumLEDs; i++) WhichOn[i] = false;
 
  // Figure out which ones should be made true
  for (i=0; i < thickness; i++) { WhichOn[(i+onLight) % NumLEDs] = true; }
  
  for (int i=0; i < NumLEDs; i++) { 
    if (WhichOn[i]) { LightFlower(i, color1); } else { LightFlower(i,0); }
  }
}

/*
 *  Rainbow
 */
void Rainbow() {
  int color;
  
  for (int i=0; i < NumLEDs; i++) {
    color = (color1 + (color2 * i / (NumLEDs-1))) % MaxColor;
    if (EffectOn()) { LightFlower(i,1); } else { LightFlower(i, color); }
  }
}

/*
 * Alternate Lights - Alternate light colors
 */

void AlternateLights() {
  for (int i=0; i < NumLEDs; i++) {
    if ((i%2)^(time%2)) {  // Exclusive Or: tricky!
      if (!EffectOn()) { LightFlower(i, color1); } else { LightFlower(i, color2); }
    } else {
      if (!EffectOn()) { LightFlower(i, color2); } else { LightFlower(i, color1); }
    }
  }
}

/*
 * set all lights to black but does not call the show yet
 * ignores buffering
 */
void clear() {
  for (int i=0; i<NumLEDs; i++) {
    LightFlower(i, 0);
  }
}

/*
 * Light a Flower
 *
 * Puts a color into a particular numbered flower
 * Values go into the next_frame array
 * Does not light up the array
 */

void LightFlower(int flower, int color) {
  next_frame[flower] = color;
}

//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code from Greg and Robie
//

#define FRAME_MSEC 100
#define MORPH_STEPS 25
#define STEP_LENGTH 10

void morph_frame(int msec) {
  int steps = msec / STEP_LENGTH;
  uint32_t current[NumLEDs][3];
  uint32_t deltas[NumLEDs][3];
  uint32_t next[NumLEDs][3];
  
  // Convert colors to RGB values and take differences
  for (int i = 0; i < NumLEDs; i++) {
    ColorToRGB(current_frame[i]);
    r = current[i][0];
    g = current[i][1];
    b = current[i][2];
    
    ColorToRGB(next_frame[i]);
    r = next[i][0];
    g = next[i][1];
    b = next[i][2];
    
    for (int j = 0; j < 3; j++) {
      deltas[i][j] = next[i][j] - current[i][j];
    }
  }

  for (int t = 1; t < steps; t++) {
    for (int i = 0; i < NumLEDs; i++) {
      uint32_t color[3];
      for (int j = 0; j < 3; j++) {
        LEDChannels[i][j] = current[i][j] + (deltas[i][j] * t / steps);
      }
    }
    WriteFlowers();
    delay(STEP_LENGTH);
    
    // Other garbage has to occur here at this fast time scale
    
    // Randomize settings over time
    if(!random(50)) { color1 = (color1 + 10) % MaxColor; }
    if(!random(80)) { color2 = (color2 - 10) % MaxColor; }
    if(!random(1000)) { if (currDelay+=10 > MaxDelay) currDelay = MinDelay; } 
    if(!random(5000)) { show = (show + 1) % MaxShow; }
    
    // See what the box is doing
    if(CheckBox()) t = steps;  // finish the loop immediately
  }
  for (int i = 0; i < NumLEDs; i++) {
    current_frame[i] = next_frame[i];
    for (int j = 0; j < 3; j++) {
      LEDChannels[i][j] = next[i][j];
    }
  }
  WriteFlowers();
  delay(STEP_LENGTH);
}

//
// ColorToRGB
//
// Converts a color into a RGB value
// Plugs RGB into Global(!) r,g,b
// Outside function will have to rescue these globals
// Be careful
//

void ColorToRGB(uint32_t color) {
  char third;
  
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
    
  if (color == 0) {  // 0 = Black
    r = 0;
    g = 0;
    b = 0;
  }
  
  if (color == 1) {  // 0 = White
    r = 1023;
    g = 1023;
    b = 1023;
  }
}

/*
 * WriteFlowers
 *
 * Push the color data into the lights
 * From MaceTech website: http://docs.macetech.com/doku.php/shiftbrite#command_format
 *
 */
 
void WriteFlowers() {
 
    SB_CommandMode = B00; // Write to PWM control registers
    for (int h = 0;h<NumLEDs;h++) {
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
// Check Box
//
// Goes through the 2 buttons and 3 dials and see whether they have changed position

boolean CheckBox() {
  if (Button2Pushed()) { // Button2 has been pushed
    show = (show + 1) % MaxShow; // Next show
    return(true);
  }
  if (abs(GetDial(1) - dial1) > 10) { // dial1 has been turned!
    dial1 = GetDial(1);
    color1 = dial1 * 3;   // Change the foreground color
    return(true);
  }
  if (abs(GetDial(2) - dial2) > 10) { // dial2 has been turned!
    dial2 = GetDial(2);
    currDelay = MinDelay + (dial2 * (MaxDelay-MinDelay) / 1023);    // Change pacing   
    return(false);
  }
  if (abs(GetDial(3) - dial3) > 10) { // dial2 has been turned!
    dial3 = GetDial(3);
    color2 = dial3 * 3;   // Change the background color
    return(true);
  }
  return(false);  // Nobody has changed the controller
}

//
// Get Dial
//
// Returns the position of the dial
// Only use dial 1 to 3

int GetDial(char dial) {
  int potValue = 0;
  switch (dial) {
    case 1:
      potValue = analogRead(dial1pin);
      break;
    case 2:
      potValue = analogRead(dial2pin);
      break;
    case 3:
      potValue = analogRead(dial3pin);
      break;
  }
  return(potValue);
}

//
// Button2pushed
//
// Returns true is button 2 has been pushed
// Includes a timer to prevent flickering

boolean Button2Pushed(void) {  
  //return(false);
  
  button2 = digitalRead(button2pin);
  
  if (button2 == HIGH && button2prev == LOW && millis() - time > 500) {      // Has button been pushed?
      button2 = LOW;                                                             // Yes, reset the button
      button2prev == HIGH;
      time = millis();      // ... and remember when the last button press was    
      return true;          // Button pushed
  } else {                  // Button not pushed
      button2prev = button2;
      return false;           
  }
}

//
// EffectOn
//
// Returns(true) is the effect button is currently pressed

boolean EffectOn() {
  //return(false);
  return(digitalRead(button1pin));
}


