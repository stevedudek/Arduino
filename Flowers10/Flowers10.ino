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
boolean button2;     // Status of button2
boolean button2prev; // Previous status of button2
boolean boxChange;   // Has the box been changed?
long button2time=0;  // Timer for button2

int LEDChannels[NumLEDs][3] = {0};

unsigned long time = 1;    // Keeps track of time

int show = 5; // Which show we are on
#define MaxShow  6  // Total number of shows

int currDelay = 1000;  // Delay in milliseconds between animation
#define MinDelay 300  // Minimum delay
#define MaxDelay 3000 // Maximum delay

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
  }    
  
  morph_frame(currDelay);
  time = (time + 1) % 100000;  // Advance timer
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
  int t;
  int steps = msec / STEP_LENGTH;
  int deltas[NumLEDs][3];
  
  for (int i = 0; i < NumLEDs; i++) {    
    for (int j = 0; j < 3; j++) {
      uint32_t old_color = (current_frame[i] >> (j * 10)) & 0x3ff; 
      uint32_t new_color = (next_frame[i] >> (j * 10)) & 0x3ff;
      deltas[i][j] = (int)new_color - (int)old_color;
    }
    /*if (i == 0) {
          Serial.print("Starting color: ");
          Serial.println(current_frame[i]);
          Serial.print("Finishing color: ");
          Serial.println(next_frame[i]);
    }*/
  }

  for (t = 1; t < steps; t++) {
    //Serial.print(t);
    //Serial.print("=step ");
    for (int i = 0; i < NumLEDs; i++) {
      uint32_t color = 0;
      for (int j = 0; j < 3; j++) {
        uint32_t old_color = (current_frame[i] >> (j * 10)) & 0x3ff;
        color |= ((old_color + (deltas[i][j] * t / steps)) & 0x3ff) << (j * 10);
        /*if (i == 0) {
          Serial.print(j);
          Serial.print(") ");
          Serial.print((old_color + (deltas[i][j] * t / steps)) & 0x3ff);
          Serial.print(" ");
        }*/
      }
      UpdateFlower(i, color);
    }
    //Serial.println(" ");
    WriteFlowers();
    delay(STEP_LENGTH);
    
    // Other garbage has to occur here at this fast time scale
  
    // Randomize settings over time
    if(!random(5)) { color1 = (color1 + 10) % MaxColor; }
    if(!random(8)) { color2 = (color2 - 10) % MaxColor; }
    if(!random(100)) { if (currDelay+=10 > MaxDelay) currDelay = MinDelay; } 
    if(!random(1000)) { show = (show + 1) % MaxShow; }
    
    // See what the box is doing
    if(CheckBox()) t = steps;  // finish the loop immediately
  }
  for (int i = 0; i < NumLEDs; i++) {  // Finish with the final case
    current_frame[i] = next_frame[i];
    UpdateFlower(i, current_frame[i]);
        /*if (i == 0) {
          Serial.print("Final color: ");
          Serial.println(next_frame[i]);
          Serial.println("");
        }*/
  }
  WriteFlowers();
  delay(STEP_LENGTH);
}

// Create a 30-bit color value from 10-bit R,G,B values
uint32_t Color(uint16_t r, uint16_t g, uint16_t b)
{
  //Take the lowest 10 bits of each value and append them end to end
  return( ((unsigned int)g & 0x3FF )<<20 | ((unsigned int)b & 0x3FF)<<10 | (unsigned int)r & 0x3FF);
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
    currDelay = (4 * dial2) + MinDelay;    // Change pacing   
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


