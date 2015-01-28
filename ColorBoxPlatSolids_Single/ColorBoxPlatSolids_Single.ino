#include "SPI.h"
#include <Wire.h>
#include "Adafruit_TCS34725.h"

/******************
 *
 *   8/5/14
 *
 *   Color Box - Battery-operated black box with
 *
 *   3 knobs and 3 buttons
 *
 *   and an Arduino and xBee
 *
 *   use as controller for Platonic Solids
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

// Current position of dials
int dialval[3];
#define MIN_CHANGE 5 // Minimum change in a dial that will register. Prevents flickering

byte solid = 0;        // Which solid is dial 0 set to
#define MAX_SOLID  6   // 0=All, 1-2-3-etc=specific solid 

byte show = 0;         // Which show we are on
#define MAX_SHOW 6     // Total number of shows

byte wait;
#define MAX_WAIT 12 // Number of stored delay times

int foreColor;
int backColor;
#define MaxColor 6144   // Number of colors: 1024 * 6

int brightness;  // 0 - 1024

// xBee language
#define COMMAND_PROMPT     '#'
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_BRIGHTNESS 'R'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'

void setup() {
   
   Serial.begin(9600);
  
   for (int p=0; p<3; p++) {
     pinMode(buttonpins[p], INPUT);
     buttonprev[p] = false;
     buttoncurr[p] = digitalRead(buttonpins[p]);
     buttontime[p] = 0;
  
     dialval[p] = GetDial(p);
   }   
   //Serial.begin(9600);
}

// Main loop is easy: keep polling the control box
void loop() {
  
  CheckBox();
  delay(100);   // Information bandwidth
}

//
// Check Box
//
// Goes through the 3 buttons and 3 dials and see whether they have changed position

void CheckBox() {
  
  boolean message = false;  // Flag to for sending message
  int dial;
  
  // Start with the dials
  
  // Dial #0 - The foreColor dial
  dial = GetDial(0);
  if (abs(dial - dialval[0]) > MIN_CHANGE) { // Has dial #0 budged?
    dialval[0] = dial;      // Update dial
    foreColor = dial * (MaxColor / 1024);  
    message = true;
  }
  
  // Dial #1 - The Brightness dial
  dial = GetDial(1);
  if (abs(dial - dialval[1]) > MIN_CHANGE) { // Has dial #1 budged?
    dialval[1] = dial;      // Update dial
    brightness = dial;
    message = true;
  }
  
  // Dial #2 - The wait (speed) dial
  dial = GetDial(2);
  if (abs(dial - dialval[2]) > MIN_CHANGE) { // Has dial #2 budged?
    dialval[2] = dial;      // Update dial
    wait = dial * MAX_WAIT / 1024;
    message = true;
  }
  
  // Look next at the buttons
  
  // Button #0 - The solid button
  /* Deactivate Button #0
  if (ButtonPushed(0)) {
    solid = (solid + 1) % MAX_SOLID;
    message = true;
  }
  */
  
  // Button #1 - The show button
  if (ButtonPushed(1)) {
    show = (show + 1) % MAX_SHOW;
    message = true;
  }
 

  // Button #2 - Swaps fore and back colors
  if (ButtonPushed(2)) {
    int swap = foreColor;
    foreColor = backColor;
    backColor = swap;
    message = true;
  }
  
  // Messaging
  
  if (message) SendAllData();
}

//
// SendAllData
//
// Dumps all color + time data into the serial buffer
//
void SendAllData() {
  SendMessage(COMMAND_PROMPT, solid);             
  SendMessage(COMMAND_FORE, foreColor);
  SendMessage(COMMAND_BRIGHTNESS, brightness);
  SendMessage(COMMAND_WAIT, wait);
  SendMessage(COMMAND_SHOW, show);
  Serial.print(COMMAND_PERIOD);
}

//
// Send Message
//
// Sends out the command (one letter)
// Sends out the value
// Sends out the end prompt ','

void SendMessage(char command, int value) {
  Serial.print(command);
  Serial.print(value);
  Serial.print(COMMAND_COMMA);
}

//
// Get Dial
//
// Returns the position of the dial pot
// The dial input needs to be 0-2

int GetDial(char dial) {
  return(analogRead(dialpins[dial]));
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

