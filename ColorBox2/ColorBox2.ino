#include "SPI.h"
#include <Wire.h>
#include "Adafruit_TCS34725.h"

/******************
 *
 *   Color Box - Battery-operated black box with
 *
 *   3 knobs and 3 buttons
 *
 *   and an Arduino and xBee
 *
 *   use controller to operate remotely light-up outfits
 *
 ******************/

// Digital pins for buttons 1, 2, 3
char buttonpins[3] = { 8,9,10 };

// Current and previous status of buttons
boolean buttoncurr[3];
boolean buttonprev[3];
long buttontime[3];

// Analog pins for dials 1, 2, 3
char dialpins[3] = { 0,1,2 };

// Color Sensor stuff

byte gammatable[256];

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);


// Current position of dials
int dialval[3];
#define MinDialChange 5 // Minimum change in a dial that will register. Prevents flickering

int costnum;        // Which costume is dial 0 set to
#define MaxCost 4   // 0=No control, 1-2-3-etc=specific costume, 9=All costumes 

int show = 0;       // Which show we are on
#define MaxShow 15  // Total number of shows

int pacing;
#define AvgDelay 100  // Delay if the dial is turned "off"

byte color;
#define MaxColor 255     // Number of color options

// xBee language
#define COMMAND_PROMPT     '#'
#define COMMAND_WHITE      'W'
#define COMMAND_COLOR      'C'
#define COMMAND_DELAY      'D'
#define COMMAND_SHOW       'S'
#define COMMAND_COLORSENSE 'X'
#define COMMAND_END        '$'

void setup() {
   
   Serial.begin(9600);
  
   SetUpGammaTable();   // For color sensor
   
   for (int p=0; p<3; p++) {
     pinMode(buttonpins[p], INPUT);
     buttonprev[p] = false;
     buttoncurr[p] = digitalRead(buttonpins[p]);
     buttontime[p] = 0;
  
     dialval[p] = GetDial(p);
   }
   costnum = CalcCostume(dialval[0]);   
   //Serial.begin(9600);
}

// Main loop is easy: keep polling the control box
void loop() {
  CheckBox();
}



//
// Check Box
//
// Goes through the 3 buttons and 3 dials and see whether they have changed position

void CheckBox() {
  
  delay(100);   // Information bandwidth
  
  // Start with the dials
  
  // Dial #0 - The costume dial
  if (abs(GetDial(0) - dialval[0]) > MinDialChange) { // Has dial #0 budged?
    dialval[0] = GetDial(0);  // Update dial
    int newCost = CalcCostume(dialval[0]);
    if (newCost != costnum) {  // Dial has moved to new costume!
      costnum = newCost;
      //SendMessage(COMMAND_COLOR, color);  // Update the costume with color info?
      //SendMessage(COMMAND_DELAY, pacing); // Update the costume with speed info?
      //SendMessage(COMMAND_SHOW, show);    // Update the costume with show info?
      SendMessage(COMMAND_WHITE, 1000);     // Make the new costumes glow white for 1s
    }
  }
  
  // Dial #1 - The color dial
  if (abs(GetDial(1) - dialval[1]) > MinDialChange) { // Has dial #1 budged?
    dialval[1] = GetDial(1);  // Update dial
    color = dialval[1] / 4;   // Set new color. 0-1024 on dial into 0-255 for color
    SendMessage(COMMAND_COLOR, color);              // Send out new color by xBee
  }
  
   // Dial #2 - The speed dial (Done)
  if (abs(GetDial(2) - dialval[2]) > MinDialChange) { // Has dial #2 budged?
    dialval[2] = GetDial(2);  // Update dial
    if (dialval[2] == 0) {    // Dial is switched off
      pacing = AvgDelay;      // Off state means use average delay
    } else {
      pacing = dialval[2] / 4;  // Set new delay. 0-1024 on dial into 0-255 for delay
    }
    SendMessage(COMMAND_DELAY, pacing);              // Send out new speed by xBee
  }
  
  // Look next at the buttons
  
  // Button #0 - Turns the particular outfit white
  if (digitalRead(buttonpins[0])) {     // Button #0 is pressed. No debouncing here
    SendMessage(COMMAND_WHITE, 200);    // Make the new costumes glow white
  }
 
  // Button #1 - The color picker
  if (ButtonPushed(1)) { // Button #1 is pressed
    SendMessage(COMMAND_COLORSENSE, ReadColorSensor()); // Send out the sensed color
  }

  // Button #2 - The show button
  if (ButtonPushed(2)) { // Button #2 is pressed
    show = show++ % MaxShow;
    SendMessage(COMMAND_SHOW, show);    // Send out new show by xBee
  }
}

//
// Get Dial
//
// Returns the position of the dial pot
// The dial input needs to be 0-2

int GetDial(char dial) {
  return(analogRead(dialpins[dial]));
}

// CalcCostume
//
// Divides the 0-255 costume dial into even portions of MaxCost+2
// Returns a key code: 0=No control, 1-2-3-etc=specific costume, 9=All costumes

int CalcCostume(int potvalue) {
  int partition = potvalue/(1024/(MaxCost+2));
  if (partition > MaxCost) partition = 9;
  return (partition);
}

//
// ButtonPushed
//
// Returns true if the button has been pushed
// Includes a timer to prevent flickering

boolean ButtonPushed(char b) {  
  //return(false);
  
  boolean bstate = digitalRead(buttonpins[b]);
  
  if (bstate == HIGH && buttonprev[b] == LOW && millis() - buttontime[b] > 100) { // Has button been pushed?
      buttoncurr[b] = LOW;                                                        // Yes, reset the button
      buttonprev[b] = HIGH;
      buttontime[b] = millis();      // ... and remember when the last button press was    
      return true;          // Button pushed
  } else {                  // Button not pushed
      buttonprev[b] = bstate;
      return false;           
  }
}

//
// Send Message
//
// Sends out the command prompt '#'
// Sends out the commander number: 0=No control, 1-2-3-etc=specific costume, 9=All costumes
// Sends out the command (one letter)
// Sends out the value
// Sends out the end prompt '$'

void SendMessage(char command, int value) {
  Serial.print(COMMAND_PROMPT);  // '#'
  Serial.print(costnum);         // 0=No control, 1-2-3-etc=specific costume, 9=All costumes
  Serial.print(command);
  Serial.print(value);
  Serial.println(COMMAND_END);
}

// Set up Gamma Table
//
// For the color sensor
// Set up Gamma table for the color sensor
// it helps convert RGB colors to what humans see

void SetUpGammaTable() {
  
  for (int i=0; i<256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;
      
    //gammatable[i] = 255 - x;
    gammatable[i] = x; 
  }
}

// Read color sensor
//
// Reads the Adafruit color sensor. Returns a unit32_t color from the r,g,b values
uint32_t ReadColorSensor() {
  return(Color(0,255,0));     // For testing until I install the color sensor
  /*
  uint16_t clear, red, green, blue;
  
  // Poll the sensor
  tcs.setInterrupt(false);      // turn on LED
  delay(60);  // takes 50ms to read 
  tcs.getRawData(&red, &green, &blue, &clear);
  tcs.setInterrupt(true);  // turn off LED
  
  //Serial.print("C:\t"); Serial.print(clear);
  //Serial.print("\tR:\t"); Serial.print(red);
  //Serial.print("\tG:\t"); Serial.print(green);
  //Serial.print("\tB:\t"); Serial.print(blue);

  // Figure out some basic hex code for visualization
  uint32_t sum = red;
  sum += green;
  sum += blue;
  //sum = clear;
  float r, g, b;
  r = red; r /= sum;
  g = green; g /= sum;
  b = blue; b /= sum;
  r *= 256; g *= 256; b *= 256;
  //Serial.print("\t");
  //Serial.print((int)r, HEX); Serial.print((int)g, HEX); Serial.print((int)b, HEX);
  //Serial.println();

  //Serial.print(gammatable[(int)r]); Serial.print(" "); Serial.print(gammatable[(int)g]); Serial.print(" "); Serial.println(gammatable[(int)b]);

  return(Color(gammatable[(int)r], gammatable[(int)g], gammatable[(int)b]));
  //return(Color(r,g,b));
  */
}

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}
