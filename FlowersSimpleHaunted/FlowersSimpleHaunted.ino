/******************
 *
 *   Flowers - Light 8 Shiftbrite LED flowers with patters through an Octobar
 *
 *   Disconnected motion sensors
 *
 *   Included Greg/Robie morphing of colors
 *
 *   Included crazy Greg/Robie bit-shifting
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
boolean button1;     // Status of button1
boolean button1prev; // Previous status of button1
boolean button2;     // Status of button2
boolean button2prev; // Previous status of button2

int LEDChannels[NumLEDs][3] = {0};

unsigned long time = 1;         // Keeps track of time
unsigned long butt1time = 1;    // Button 2 time
unsigned long butt2time = 1;    // Button 2 time

int show = 0; // Which show we are on
#define MaxShow  4  // Total number of shows

int currDelay = 2000;  // Delay in milliseconds between animation
#define MinDelay 1000  // Minimum delay
#define MaxDelay 5000 // Maximum delay

uint32_t color1 = 1000;   // Foreground color
uint32_t color2 = 2000;   // Background color

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
   
   for (int i=0; i<NumLEDs; i++) current_frame[i] = 0;          // clear display
   
   // Read the initial position of the box
   dial1 = GetDial(1);
   dial2 = GetDial(2);
   dial3 = GetDial(3);
   button2 = digitalRead(button2pin);
   
   // All the start, all flowers are off
   clear();
   morph_frame(100);
   
   //Serial.begin(9600);
}

void loop() {
  
  switch(show) {
    case 0:
      OneLightRunDown();
      break;
    case 1:
      AlternateLights();
      break;
    case 2:
      BlackWave();
      break;
    case 3:
      PairedLightRunDown();
      break;
  }      
  morph_frame(currDelay);
}

//
// All On - Boring! Turn all the lights on to color1
//

void AllOn() {
  for (int i=0; i < NumLEDs; i++) {
    if (EffectOn()) { LightFlower(i, color2); } else { LightFlower(i, color1); }
  }
}

//
// One Light Run Down - One light travels from top to bottom
//

void OneLightRunDown() {
  int onLight = time % NumLEDs;
  
  for (int i=0; i < NumLEDs; i++) {
    if (i == onLight) {
       if (EffectOn()) { LightFlower(i, 1); } else { LightFlower(i,color1); }
    } else { LightFlower(i, 0); }
  }
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
 * Alternate Lights - Alternately light up and keep dark the lights
 */

void AlternateLights() {
  for (int i=0; i < NumLEDs; i++) {
    if ((i%2)^(time%2)) {  // Exclusive Or: tricky!
      if (!EffectOn()) { LightFlower(i, color1); } else { LightFlower(i, 0); }
    } else {
      if (!EffectOn()) { LightFlower(i, 0); } else { LightFlower(i, color1); }
    }
  }
}

/*
 * set all lights to black but does not call the show yet
 * ignores buffering
 */
void clear() {
  for (int i=0; i<NumLEDs; i++) {
    LightFlower(i,0);
  }
}

/*
 * Light a Flower
 *
 * Put color into a particular numbered flower
 * Values go into the next_frame array
 * Does not light up the array
 */

void LightFlower(int flower, uint32_t color) {
  next_frame[flower] = Wheel(color);
}

//
// Update Flower
//
// Decompresses 30-bit color into 10-bit r,g,b
// Updates the r,g,b values into the LED array
// Does not call the array; WriteFlowers() does that

void UpdateFlower(int flower, uint32_t color) {
  for (int j = 0; j < 3; j++) {
    LEDChannels[flower][j] = (color >> (j * 10)) & 0x3ff;
  }
}

//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code from Greg and Robie
//

#define STEP_LENGTH 20

void morph_frame(int msec) {
  boolean boxChange = false;
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
      UpdateFlower(i, color);
    }
    //Serial.println(" ");
    WriteFlowers();
    delay(STEP_LENGTH);
      
    // See what the box is doing
    if(CheckBox()) {
      boxChange = true;
      t = steps;  // finish the loop immediately for quick update
    }
  }
  
  // Finish with the final case
  for (int i = 0; i < NumLEDs; i++) {  
    current_frame[i] = next_frame[i];
    UpdateFlower(i, current_frame[i]);
  }
  WriteFlowers();
  delay(STEP_LENGTH);
  
  // Randomize settings over time
  // The longer the delay, the more likely to be random
  int chance = 1 + ((MaxDelay - currDelay) / 100) ;
  if(!random(chance)) { color1 = (color1 + 10) % MaxColor; }
  if(!random(chance)) { color2 = (color2 - 10) % MaxColor; }
  if(!random(chance*2)) { if (currDelay+=10 > MaxDelay) currDelay = MinDelay; } 
  if(!random(chance*10)) { show = (show + 1) % MaxShow; }
  
  // Update timer. Don't advance if box is changing.
  if (!boxChange) time = (time + 1) % 100000;  // Advance timer
}

// Create a 30-bit color value from 10-bit R,G,B values
uint32_t Color(uint16_t r, uint16_t g, uint16_t b)
{
  //Take the lowest 10 bits of each value and append them end to end
  return( ((unsigned long)g & 0x3FF )<<20 | ((unsigned long)b & 0x3FF)<<10 | (unsigned long)r & 0x3FF);
}
  
//Input a value 0 to 3071 to get a color value.
//The colours are a transition r - g -b - back to r
unsigned int Wheel(uint32_t WheelPos)
{
  if (WheelPos == 1) return (Color(1023,1023,1023));
  if (WheelPos == 0) return (Color(0,0,0));
  
  uint32_t r,g,b;
  switch(WheelPos >> 10)
  {
    case 0:
      r=1023-(WheelPos % 1024);   //Red down
      g=WheelPos % 1024;      // Green up
      b=0;                  //blue off
      break; 
    case 1:
      g=1023-(WheelPos % 1024);  //green down
      b=WheelPos % 1024;      //blue up
      r=0;                  //red off
      break; 
    case 2:
      b=1023-(WheelPos % 1024);  //blue down 
      r=WheelPos % 1024;      //red up
      g=0;                  //green off
      break; 
  }
  return(Color(r,g,b));
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
// Goes through the button2 and 3 dials and see whether they have changed position
// Returns whether the box has changed

boolean CheckBox() {
  boolean boxUpdate = false;
  
  if (Button2Pushed()) { // Button2 has been pushed
    show = (show + 1) % MaxShow; // Next show
    boxUpdate = true;
  }
  if (abs(GetDial(1) - dial1) > 10) { // dial1 has been turned!
    dial1 = GetDial(1);
    color1 = dial1 * 3;   // Change the foreground color
    boxUpdate = true;  
  }
  if (abs(GetDial(2) - dial2) > 10) { // dial2 has been turned!
    dial2 = GetDial(2);
    currDelay = (4 * dial2) + MinDelay;    // Change pacing   
  }
  if (abs(GetDial(3) - dial3) > 10) { // dial2 has been turned!
    dial3 = GetDial(3);
    color2 = dial3 * 3;   // Change the background color
    boxUpdate = true;
  }
  return(boxUpdate);  // Nobody has changed the controller
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
// Button1pushed
//
// Returns true is button 1 has been pushed
// Includes a timer to sync frequency of button push to show delays

boolean Button1Pushed(void) {  
  int button1 = digitalRead(button1pin);
  
  if (button1 == HIGH && button1prev == LOW && millis() - butt1time >= MinDelay) { // Has button been pushed?
      button1 = LOW;             // Yes, reset the button
      button1prev == HIGH;
      // Sync show timing to the interval between two button pushes
      //if (millis() - butt1time < MaxDelay) currDelay = millis() - butt1time;
      butt1time = millis();      // ... and remember when the last button press was    
      return true;               // Button pushed
  } else {                  // Button not pushed
      button1prev = button1;
      return false;           
  }
}

//
// Button2pushed
//
// Returns true is button 2 has been pushed
// Includes a timer to prevent flickering

boolean Button2Pushed(void) {  
  int button2 = digitalRead(button2pin);
  
  if (button2 == HIGH && button2prev == LOW && millis() - butt2time > 200) { // Has button been pushed?
      button2 = LOW;                                                         // Yes, reset the button
      button2prev == HIGH;
      butt2time = millis();      // ... and remember when the last button press was    
      return true;               // Button pushed
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


