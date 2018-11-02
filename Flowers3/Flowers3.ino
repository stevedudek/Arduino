/******************
 *
 *   Flowers - Light 8 Shiftbrite LED flowers with patters through an Octobar
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

int MotionPin[NumLEDs] = {2,3,4,9,10,11,12,13}; // Pin-in of motion detectors

int dial1;           // Position of Dial 1
int dial2;           // Position of Dial 2
int dial3;           // Position of Dial 3
boolean button2;     // Status of button2
boolean button2prev; // Previous status of button2
long button2time=0;  // Timer for button2

int LEDChannels[NumLEDs][3] = {0};

unsigned int time = 1;    // Keeps track of time
unsigned int BoxWait = 0; // How long it has been since the box was last touched
#define MaxWait 1000      // How long box's setting stay untouched

int show = 5; // Which show we are on
#define MaxShow  7  // Total number of shows

int pace = 5;        // How slow the patterns are. 1 = fast.
#define MaxPace 20   // Slowest pace of patterns
#define MinDelay 100  // Minimum delay

int color1 = 1000;   // Foreground color
int color2 = 2000;   // Background color
#define MaxColor 3071     // Number of color options

int SB_CommandMode;
int SB_BlueCommand;
int SB_RedCommand;
int SB_GreenCommand;

void setup() {
   pinMode(datapin, OUTPUT);
   pinMode(latchpin, OUTPUT);
   pinMode(enablepin, OUTPUT);
   pinMode(clockpin, OUTPUT);
   SPCR = (1<<SPE)|(1<<MSTR)|(0<<SPR1)|(0<<SPR0);   
   digitalWrite(latchpin, LOW);
   digitalWrite(enablepin, LOW);
   
   pinMode(button1pin, INPUT);
   pinMode(button2pin, INPUT);
   
   for (int i=0; i<NumLEDs; i++) pinMode(MotionPin[i], INPUT);
   
   // Read the initial position of the box
   dial1 = GetDial(1);
   dial2 = GetDial(2);
   dial3 = GetDial(3);
   button2 = digitalRead(button2pin);
   
   //Serial.begin(9600);
}

void loop() {
  
  // Check the status of the Box
  if (CheckBox()) {     // Has Box been altered?
    BoxWait = MaxWait;  // Yes, set count down until random shows can begin again
  } else {              // Box has not been touched
    if (BoxWait > 0) BoxWait--;  // Count down until random shows can begin again
  }
   
  // Randomize if the box has not been used for a while
  if (!BoxWait) {
    if(!random(2)) {  // Color1
      if (color1++ > MaxColor) color1 = 0;
    }
    if(!random(3)) {  // Color2
      if (color2-- <= 0) color2 = MaxColor;
    }
    if(!random(50)) { // Pacing
      if (pace++ > MaxPace) pace = 1;
    }
    if(!random(1000)) { // Show
      if (show++ >= MaxShow) show = 0;
    }
  }
  
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
    case 6:
      GradedFill();
      break;    
  }
    
  WriteFlowers();  // Push the data into the flowers
  
  // Advance the timer
  if (time++ > 10000) time = 1;  // Advance timer
  delay(MinDelay * pace); // Smallest delay. Adjust if flickering or too slow 
}

/*
 * All On - Boring! Turn all the lights on to color1
 */

void AllOn() {
  for (int i=0; i < NumLEDs; i++) {
    if (!EffectOn()) { LightFlower(i, color1); } else { LightFlower(i, color2); }
  }
}

/*
 * One Light Run Down - One light travels from top to bottom
 */

void OneLightRunDown() {
  int onLight = time % NumLEDs;
  
  for (int i=0; i < NumLEDs; i++) {
    if (i == onLight || EffectOn()) { LightFlower(i, color1); } else { LightFlower(i, color2); }
  }
}

/*
 * Graded Fill - One light travels from top to bottom
 *
 * Lights further from the traveling light are darkened
 */

void GradedFill() {
  int gradient;
  int onLight = time % NumLEDs;

  for (int i=0; i < NumLEDs; i++) {
    gradient = 100 * ((NumLEDs/2) - CircDiff(i,onLight,NumLEDs)) / (NumLEDs/2);
    if (!EffectOn()) { GradientFlower(i, color1, gradient); } else { GradientFlower(i, color2, gradient); }
  }
}

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
    if (WhichOn[i]) { LightFlower(i, color1); } else { TurnOffFlower(i); }
  }
}

/*
 *  Rainbow
 */
void Rainbow() {
  int color;
  
  for (int i=0; i < NumLEDs; i++) {
    color = (color1 + (color2 * i / (NumLEDs-1))) % MaxColor;
    if (EffectOn()) { WhiteFlower(i); } else { LightFlower(i, color); }
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
 * Turn Off Flower
 *
 * Turns a particular flower off
 * Does not update the array
 */

void TurnOffFlower(int flower) {
 if (flower >= NumLEDs) return;  // flower number is out of bounds
 
 LEDChannels[flower][0] = Motion(flower, 0);
 LEDChannels[flower][1] = Motion(flower, 0);
 LEDChannels[flower][2] = Motion(flower, 0);
}

/*
 * White Flower
 *
 * Makes a flower white
 * Does not update the array
 */

void WhiteFlower(int flower) {
 if (flower >= NumLEDs) return;  // flower number is out of bounds
 
 LEDChannels[flower][0] = Motion(flower, 1023);
 LEDChannels[flower][1] = Motion(flower, 1023);
 LEDChannels[flower][2] = Motion(flower, 1023);
}

/*
 * Light a Flower
 *
 * Wrapper for Gradient flower
 * In this case, no gradient, so 100%
 */

void LightFlower(int flower, int color) {
  GradientFlower(flower, color, 100);
}

/*
 * Gradient Light a Flower
 *
 * Puts a color into a particular numbered flower
 * Gradient is a percentage of color intensity: 0-100%
 * Does not light up the array
 */

void GradientFlower(int flower, int color, int perc) {
  int r,g,b,third;
  
  if (flower >= NumLEDs) return;  // flower number is out of bounds
  if (color == 0) {  // 0 = Black
    TurnOffFlower(flower);
    return;
  }
    
  third = 1;
  if (color<1024) third = 0; // Figure out which third (0-1023)
  if (color>2047) third = 2; // wheel position is in   (2048-3071)
  
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
  
  if (perc>100) perc = 100;
  
  LEDChannels[flower][0] = Motion(flower, r * perc / 100);
  LEDChannels[flower][1] = Motion(flower, b * perc / 100);
  LEDChannels[flower][2] = Motion(flower, g * perc / 100);
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
 
    SPDR = SB_CommandMode << 6 | SB_BlueCommand>>4;
    while(!(SPSR & (1<<SPIF)));
    SPDR = SB_BlueCommand<<4 | SB_RedCommand>>6;
    while(!(SPSR & (1<<SPIF)));
    SPDR = SB_RedCommand << 2 | SB_GreenCommand>>8;
    while(!(SPSR & (1<<SPIF)));
    SPDR = SB_GreenCommand;
    while(!(SPSR & (1<<SPIF)));
 
}

//
// Motion
//
// Only routine that addresses the motion detectors
// Basically, if motion is detected, invert the color of the flower
// Function works at a low level, right when flowers are updated

int Motion(int flower, int value) {
   return(value);  
   //if(digitalRead(MotionPin[flower])) { return(value); } else { return(1023-value); }
}  // High on Motion Pin means no motion

//
// Check Box
//
// Goes through the 2 buttons and 3 dials and see whether they have changed position

boolean CheckBox() {
  if (Button2Pushed()) { // Button2 has been pushed
    if (show++ >= MaxShow) show = 0; // Next show
    return(true);
  }
  if (abs(GetDial(1) - dial1) > 10) { // dial1 has been turned!
    dial1 = GetDial(1);
    color1 = dial1 * 3;   // Change the foreground color
    return(true);
  }
  if (abs(GetDial(2) - dial2) > 10) { // dial2 has been turned!
    dial2 = GetDial(2);
    color2 = dial2 * 3;   // Change the foreground color
    return(true);
  }
  if (abs(GetDial(3) - dial3) > 10) { // dial2 has been turned!
    dial3 = GetDial(3);
    pace = 1 + (dial3 * MaxPace / 1023);    // Change pacing
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
  return(false);
  /*
  button2 = digitalRead(button2pin);
  
  if (button2 == HIGH && button2prev == LOW && millis() - time > 500) {      // Has button been pushed?
      button2 = LOW;                                                             // Yes, reset the button
      button2prev == HIGH;
      time = millis();      // ... and remember when the last button press was    
      return true;          // Button pushed
  } else {                  // Button not pushed
      button2prev = button2;
      return false;           
  }*/
}

//
// EffectOn
//
// Returns(true) is the effect button is currently pressed

boolean EffectOn() {
  return(false);
  //return(digitalRead(button2pin));
}
