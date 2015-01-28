/******************
 *
 *   Furry Hat Programme - Lights 2 Shiftbrite LEDs with random patterns
 *
 ******************/

int datapin  = 8; // DI
int latchpin = 7; // LI
int enablepin = 6; // EI
int clockpin = 5; // CI

#define MinOffTime  4    // Shortest time in seconds that horns are off between shows
#define MaxOffTime  20   // Longest time in seconds that horns are off between shows
#define MinOnTime   2    // Shortest time in seconds that horns are on
#define MaxOnTime   20   // Longest time in seconds that horns are on

#define NumLEDs  2        // Number of Shiftbrites

int LEDChannels[NumLEDs][3] = {0};

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
   
   //Serial.begin(9600);
}

void SB_SendPacket() {
   SB_CommandPacket = SB_CommandMode & B11;
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_BlueCommand & 1023);
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_RedCommand & 1023);
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_GreenCommand & 1023);

   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 24);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 16);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 8);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket);

   delay(1); // adjustment may be necessary depending on chain length
   digitalWrite(latchpin,HIGH); // latch data into registers
   delay(1); // adjustment may be necessary depending on chain length
   digitalWrite(latchpin,LOW);
}

void loop() {
  int i, j, show;
  
  /* Most of the time, horns will light up red */
  
  if (random(3) != 0) {  // 3 in 4 chance show will be red lights
    
    for (j=1; j<1024; j++) {
        LightUpSame(j,0,0);   // Turn on red slowly
        delay(10);
    }
    for (j=1023; j>0; j--) {
        LightUpSame(j,0,0);  // Fade red slowly
        delay(10);
    }
  
  } else {  // show is not red lights
    
    switch (random(1,6)) {   // Pick a random show from min to max-1
  
  // Note: removed the police-siren pattern. Change the above 6 to a 7 to restore
  
      case 1: {    // Turn on Blue slowly and then fade
        for (j=1; j<1024; j++) {
          LightUpSame(0,j,0);
          delay(3);
        }
        for (j=1023; j>0; j--) {
          LightUpSame(0,j,0);
          delay(3);
        }
      }
    
      case 2: {    // Turn on Green slowly and then fade
        for (j=1; j<1024; j++) {
          LightUpSame(0,0,j);
          delay(3);
        }
        for (j=1023; j>0; j--) {
          LightUpSame(0,0,j);
          delay(3);
        }
      }
    
      case 3: {    // Rainbow colors
        for (j=0; j<3072; j++) {
          LightUpPos(j);
          delay(20);
        }
      }
      
      case 4: {    // Horns do rainbows quickly
        for (i=0; i<10; i++) {
          for (j=3071; j>0; j=j-20) {
            LightUpPos(j);
            delay(5);
          }
        }
      }
      
      case 5: {    // Random horn color
        LightUpPos(random(3071));  // Light up both lights with random color
        delay(1000*random(MinOnTime,MaxOnTime));   // Pause
      }
      
      case 6: {    // Police siren
        for (i=0; i<20; i++) {
          LEDChannels[0][0] = 1023;  // Red on light 0
          LEDChannels[0][1] = 0;
          LEDChannels[0][2] = 0;
          LEDChannels[1][0] = 0;
          LEDChannels[1][1] = 0;
          LEDChannels[1][2] = 0;
          WriteLEDArray();
          delay(500);
          LEDChannels[0][0] = 0;
          LEDChannels[0][1] = 0;
          LEDChannels[0][2] = 0;
          LEDChannels[1][0] = 0;
          LEDChannels[1][1] = 0;  
          LEDChannels[1][2] = 1023;  // Blue on light 1
          WriteLEDArray();
          delay(500);
        }
      }
    }
  }
  
  // Turn off lights
  
  if (random(3) != 0) {    // 1 in 4 chance program will move straight to next show
    LightUpSame(0,0,0);    // Darken both lights
    delay(1000 * random(MinOffTime, MaxOffTime));  // Turn lights off for a random amount of time
  }
}

/*
 * LightUpSame: Lights both horns with the same color
 */
 
void LightUpSame(int Red, int Blue, int Green) {
  int k;
  
  for (k=0; k<NumLEDs; k++) {
     LEDChannels[k][0] = Red;
     LEDChannels[k][1] = Blue;
     LEDChannels[k][2] = Green;
  }
  WriteLEDArray();
}
  
void WriteLEDArray() {
 
   SB_CommandMode = B01; // Write to current control registers
   SB_RedCommand = 127; // Full current
   SB_GreenCommand = 127; // Full current
   SB_BlueCommand = 127; // Full current
   SB_SendPacket();
  
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
}

//Input a value 0 to 3071 to get a color value.
//The colours are a transition r - g -b - back to r
void LightUpPos(unsigned int WheelPos)
{
  int r,g,b,third;
  
  WheelPos = WheelPos % 3072; // Truncate to get range of 0-3071
  third = 1;
  if (WheelPos<1024) third = 0; // Figure out which third (0-1023)
  if (WheelPos>2047) third = 2; // wheel position is in   (2048-3071)
  
  WheelPos = WheelPos % 1024;   // Truncate again to get 0-1024 range
  
  switch(third)
  {
    case 0:
      r=1023-WheelPos;    //Red down
      g=WheelPos;         //Green up
      b=0;                //Blue off
      break; 
    case 1:
      g=1023-WheelPos;    //Green down
      b=WheelPos;         //Blue up
      r=0;                //Red off
      break; 
    case 2:
      b=1023-WheelPos;    //Blue down 
      r=WheelPos;         //Red up
      g=0;                //Green off
      break;  
  }
  LightUpSame(r,g,b);
}
