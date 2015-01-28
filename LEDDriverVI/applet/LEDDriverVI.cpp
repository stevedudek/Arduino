/* Shift Out Data
 * --------------
 * http://arduino.cc/en/Tutorial/LEDDriver
 *
 * Shows a byte, stored in "dato" on a set of 8 LEDs
 *
 * (copyleft) 2005 K3, Malmo University
 * @author: David Cuartielles, Marcus Hannerstig
 * @hardware: David Cuartielles, Marcos Yarza
 * @project: made for SMEE - Experiential Vehicles
 */


#include "WProgram.h"
void setup();
void PulseClock(void);
boolean ButtonPushed (void);
int GetDelay();
void LightUp(byte dato1, byte dato2, byte dato3, byte dato4, byte dato5, byte dato6, int delaytime);
void loop();
int data = 9;
int strob = 8;
int clock = 10;
int oe = 11;
int button = 7;

int PotPin = 0;        // input pin for delay pot
int PotValue = 0;      // variable to store the delay pot's value
int HighVolt = 1023;   // Expected high read (analog in goes 0-1023)

int count = 0;
int i = 0;

boolean ButtonState;
boolean ButtonPrev = LOW;
boolean AutoState = LOW;     // For automode, records either blink mode (LOW) or auto-show (HIGH)
long time = 0;         // the last time the button was pushed
long debounce = 100;   // the debounce time, increase if the output flickers

byte dato1 = 0;
byte dato2 = 0;
byte dato3 = 0;
byte dato4 = 0;
byte dato5 = 0;
byte dato6 = 0;

int strob1Delay = 20;
int strob2Delay = 20;
int oeDelay = 20;
int ClockDelay = 20;
int DefaultDelay = 200;  // default delay time for auto mode
int delaytime;           // variable delay time

int numlights = 48;  // number of eyeballs

void setup() {
  beginSerial(9600);
  pinMode(data, OUTPUT);
  pinMode(clock, OUTPUT);
  pinMode(strob, OUTPUT);
  pinMode(oe, OUTPUT);
  pinMode(button, INPUT);
}

void PulseClock(void) {
    digitalWrite(clock, LOW);
    delayMicroseconds(ClockDelay);
    digitalWrite(clock, HIGH);
    delayMicroseconds(ClockDelay*2);
    digitalWrite(clock, LOW);
    delayMicroseconds(ClockDelay);
    digitalWrite(oe, HIGH);
    delayMicroseconds(oeDelay);
}

boolean ButtonPushed (void) {  
  
  ButtonState = digitalRead(button);
  
  if (ButtonState == HIGH && ButtonPrev == LOW && millis() - time > debounce) {      // Has button been pushed?
      ButtonState = LOW;                                                             // Yes, reset the button
      ButtonPrev == HIGH;
      time = millis();      // ... and remember when the last button press was
      if (analogRead(PotPin) >= HighVolt) {    // In automatic mode?
        if (AutoState == LOW) AutoState = HIGH; else AutoState = LOW;   // Yes, toggle AutoState
      }
      return HIGH;          // Button pushed
  } else {                  // Button not pushed
      ButtonPrev = ButtonState;
      return LOW;           
  }
}

int GetDelay() {
  if (analogRead(PotPin) >= HighVolt) return DefaultDelay; else return analogRead(PotPin);
}

void LightUp(byte dato1, byte dato2, byte dato3, byte dato4, byte dato5, byte dato6, int delaytime)
{ 
  for (count = 0; count < 8; count++) {
    digitalWrite(data, dato6 & 01);
    dato6>>=1;
    PulseClock();
   }
  
  for (count = 0; count < 8; count++) {
    digitalWrite(data, dato5 & 01);
    dato5>>=1;
    PulseClock();
   } 
  
  for (count = 0; count < 8; count++) {
    digitalWrite(data, dato4 & 01);
    dato4>>=1;
    PulseClock();
   }
   
   for (count = 0; count < 8; count++) {
    digitalWrite(data, dato3 & 01);
    dato3>>=1;
    PulseClock();
   }
   
   for (count = 0; count < 8; count++) {
    digitalWrite(data, dato2 & 01);
    dato2>>=1;
    PulseClock();
   }
   
   for (count = 0; count < 8; count++) {
    digitalWrite(data, dato1 & 01);
    dato1>>=1;
    
    if (count == 7) {
      digitalWrite(oe, LOW);
      delayMicroseconds(strob1Delay);
      digitalWrite(strob, HIGH);
    }
    PulseClock();
   }

    delayMicroseconds(strob2Delay);
    digitalWrite(strob, LOW);
    delay(delaytime);  // Hold light pattern for this many microseconds
}

void loop()
{
  
/*
 * Show - collar lights blink; good way to see blink rate
 */
for (i=0; i<1; i++)
{  
  if (analogRead(PotPin) >= HighVolt && AutoState == HIGH) break;  // Automatic mode On; Wink mode only On so skip this show
  LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000011, GetDelay()); if (ButtonPushed()) break;
  LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000001, GetDelay()); if (ButtonPushed()) break;
  LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000011, GetDelay()); if (ButtonPushed()) break;
  LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000010, GetDelay()); if (ButtonPushed()) break;
  if (analogRead(PotPin) < HighVolt) i = -1;  // On manual mode; show goes until button is pushed
}

/*
 * Show - one light scrolls from left to right, bottom to top
 */
for (i=0; i<1; i++)
{
    if (analogRead(PotPin) >= HighVolt && AutoState == HIGH) break;  // Automatic mode On; Wink mode only On so skip this show
    LightUp(B10000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00100000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00001000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000100, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000010, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000001, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B10000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B01000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00100000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00010000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00001000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000100, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000010, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000001, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B10000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B01000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00100000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00010000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00001000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000100, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000010, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000001, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B10000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B01000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00100000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00010000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00001000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000100, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000010, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000001, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B01000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00100000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00010000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00001000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000100, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000010, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000001, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B10000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B01000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00100000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00010000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00001000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000100, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000001, GetDelay()); if (ButtonPushed()) break;
    if (analogRead(PotPin) < HighVolt) i = -1;  // On manual mode; show goes until button is pushed
}
/*
 * Show - one row of lights scrolls from bottom to top
 */
for (i=0; i<10; i++)
{
    if (analogRead(PotPin) >= HighVolt && AutoState == HIGH) break;  // Automatic mode On; Wink mode only On so skip this show
    LightUp(B11111110, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000001, B11111000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000111, B11110000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00001111, B11000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00111111, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B01111110, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000001, B11001100, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00110000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000011, GetDelay()); if (ButtonPushed()) break;
    if (analogRead(PotPin) < HighVolt) i = -1;  // On manual mode; show goes until button is pushed
}

/*
 * Show - lights randomly turn on from none to all and then back to none
 */ 
for (i=0; i<1; i++)
{  
    if (analogRead(PotPin) >= HighVolt && AutoState == HIGH) break;  // Automatic mode On; Wink mode only On so skip this show
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00100000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00100000, B00000000, B00000000, B00000000, B00001000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00100000, B00000000, B00000000, B00000000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00100000, B00000100, B00000000, B00000000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000001, B00100000, B00000100, B00000000, B00000000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000001, B00100000, B00000100, B01000000, B00000000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000001, B00100000, B00000100, B01000100, B00000000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000001, B00100000, B00000100, B01000100, B00000000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000011, B00100000, B00000100, B01000100, B00000000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000011, B00100000, B00000100, B01000100, B01000000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000011, B00100000, B10000100, B01000100, B01000000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000011, B00100000, B10000100, B01000100, B01001000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000011, B00100000, B10001100, B01000100, B01001000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000011, B00100100, B10001100, B01000100, B01001000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000011, B00101100, B10001100, B01000100, B01001000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000011, B00101100, B10001100, B01010100, B01001000, B00001010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000011, B00101100, B10001100, B01010100, B01001000, B00101010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01010011, B00101100, B10001100, B01010100, B01001000, B00101010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01010011, B00101100, B10101100, B01010100, B01001000, B00101010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01010011, B00101100, B10101100, B11010100, B01001000, B00101010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01010011, B00101100, B10101100, B11010100, B01001000, B10101010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B00101100, B10101100, B11010100, B01001000, B10101010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B00101100, B11101100, B11010100, B01001000, B10101010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B00101100, B11101100, B11010100, B01001000, B10111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B00101100, B11111100, B11010100, B01001000, B10111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B00101100, B11111100, B11010100, B11001000, B10111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B00101100, B11111100, B11010100, B11011000, B10111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B00101100, B11111100, B11010100, B11011001, B10111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B00101110, B11111100, B11010100, B11011001, B10111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B00101110, B11111100, B11110100, B11011001, B10111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B10101110, B11111100, B11110100, B11011001, B10111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B10101110, B11111100, B11110100, B11011101, B10111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11010011, B10101110, B11111100, B11110100, B11011101, B11111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11011011, B10101110, B11111100, B11110100, B11011101, B11111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111011, B10101110, B11111100, B11110100, B11011101, B11111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111011, B10101110, B11111110, B11110100, B11011101, B11111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111011, B10101110, B11111110, B11110100, B11011111, B11111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111011, B10101110, B11111110, B11110110, B11011111, B11111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111011, B10111110, B11111110, B11110110, B11011111, B11111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111011, B10111110, B11111110, B11111110, B11011111, B11111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111011, B10111110, B11111110, B11111111, B11011111, B11111010, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111011, B10111110, B11111110, B11111111, B11011111, B11111011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111011, B10111111, B11111110, B11111111, B11011111, B11111011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B10111111, B11111110, B11111111, B11011111, B11111011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B10111111, B11111110, B11111111, B11111111, B11111011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B10111111, B11111110, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111110, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    delay(GetDelay());
    delay(GetDelay());
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B10111111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B10111111, B11111111, B11111111, B11111101, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01111111, B10111111, B11111111, B11111111, B11111101, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01111111, B10111111, B11111111, B11110111, B11111101, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01111111, B10111111, B11111111, B11110111, B11111100, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01111111, B10111111, B11111111, B11110111, B10111100, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01111111, B10111101, B11111111, B11110111, B10111100, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01111111, B10111101, B11111111, B11110111, B10111100, B11111101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10111101, B11111111, B11110111, B10111100, B11111101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10111101, B11111111, B11110111, B10111100, B11011101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10110101, B11111111, B11110111, B10111100, B11011101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10110101, B11101111, B11110111, B10111100, B11011101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10110101, B11101111, B11110111, B10111100, B01011101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10100101, B11101111, B11110111, B10111100, B01011101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10100101, B11101111, B11110111, B10111100, B01010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10100101, B11001111, B11110111, B10111100, B01010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10100101, B11001101, B11110111, B10111100, B01010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10100101, B11001101, B10110111, B10111100, B01010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10100101, B11001101, B10110111, B10110100, B01010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110111, B10100101, B11001101, B10110111, B10100100, B01010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110101, B10100101, B11001101, B10110111, B10100100, B01010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110101, B10100101, B11001101, B10110011, B10100100, B01010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110101, B10100101, B11001101, B10110011, B10100100, B00010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110101, B10100101, B01001101, B10110011, B10100100, B00010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110101, B10100101, B01001101, B10110011, B10100000, B00010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110101, B10100001, B01001101, B10110011, B10100000, B00010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110101, B10100001, B01000101, B10110011, B10100000, B00010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110101, B10100001, B01000101, B10100011, B10100000, B00010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01010101, B10100001, B01000101, B10100011, B10100000, B00010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010101, B10100001, B01000101, B10100011, B10100000, B00010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010101, B10100001, B01000101, B10100011, B10000000, B00010101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010101, B10100001, B01000101, B10100011, B10000000, B00010001, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010101, B10100001, B01000101, B10100011, B10000000, B00010000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010101, B10000001, B01000101, B10100011, B10000000, B00010000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010101, B10000001, B01000101, B10100010, B10000000, B00010000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010101, B10000001, B01000101, B10100000, B10000000, B00010000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010101, B10000001, B01000101, B10100000, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010001, B10000001, B01000101, B10100000, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010001, B10000001, B00000101, B10100000, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010001, B10000001, B00000101, B10000000, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010000, B10000001, B00000101, B10000000, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B10000001, B00000101, B10000000, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B10000001, B00000101, B00000000, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B10000001, B00000001, B00000000, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000001, B00000001, B00000000, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000001, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    if (analogRead(PotPin) < HighVolt) i = -1;  // On manual mode; show goes until button is pushed
}

/*
 * Show - exploding ring
 */
for (i=0; i<6; i++)
{  
    if (analogRead(PotPin) >= HighVolt && AutoState == HIGH) break;  // Automatic mode On; Wink mode only On so skip this show
    LightUp(B00000000, B00000000, B00000000, B00000100, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000011, B00001010, B00011000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B10000100, B10010001, B00100100, B10000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B01100001, B01001000, B01100000, B11000011, B01001100, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00010000, B10010010, B00100000, B00000000, B00000000, B00110000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00101001, B00001100, B00010000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01000100, B00000000, B00000000, B00000000, B00000000, B00000011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B10000010, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00100000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    if (analogRead(PotPin) < HighVolt) i = 0;  // On manual mode; show goes until button is pushed
}
/*
 * Show - eyes wink
 */ 
for (i=0; i<4; i++)
{  
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111101, GetDelay()); if (ButtonPushed()) break;
    LightUp(B10111111, B11111111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111110, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B10111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11101111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11011111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111110, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111101, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111110, B11111111, B11111011, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;    
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B10111111, GetDelay()); if (ButtonPushed()) break;    
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11110111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11101111, B11111111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111101, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    if (analogRead(PotPin) < HighVolt) i = 0;  // On manual mode; show goes until button is pushed
    if (analogRead(PotPin) >= HighVolt && AutoState == HIGH) i = 0;  // Automatic mode On; Wink mode only On so repeat till button push
}

/*
 * Show - rows increase from bottom to top until full and then fall back to bottom
 * Show - rows increase from top to bottom until full and then rise back to top
 */
for (i=0; i<3; i++)
{  
    if (analogRead(PotPin) >= HighVolt && AutoState == HIGH) break;  // Automatic mode On; Wink mode only On so skip this show
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111110, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11110000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111110, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11001100, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111100, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111100, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11001100, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111110, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11110000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111110, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    delay(GetDelay());
    delay(GetDelay());
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00110011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000001, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B01111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00001111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000001, B11111111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000001, B11111111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000111, B11111111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00001111, B11111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00111111, B11111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B01111111, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000001, B11111111, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00110011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    if (analogRead(PotPin) < HighVolt) i = 0;  // On manual mode; show goes until button is pushed
}

/*
 * Show - Pair of Eyes
 */
for (i=0; i<6; i++)
{  
    if (analogRead(PotPin) >= HighVolt && AutoState == HIGH) break;  // Automatic mode On; Wink mode only On so skip this show
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break; 
    LightUp(B00000000, B00000000, B00000000, B00000011, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01100000, B00000000, B00000000, B00000011, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01100000, B00000000, B00000000, B00000011, B00000000, B00110000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01100000, B00000000, B11000000, B00000000, B00000000, B00110000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B11000000, B00000000, B00110000, B00110000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000110, B00000000, B11000000, B00000000, B00110000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000110, B00000000, B00000000, B00000000, B00110000, B00001100, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000110, B00000000, B00110000, B00000000, B00000000, B00001100, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00110000, B00011000, B00000000, B00001100, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00110000, B00011000, B00000000, B00000011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B01100000, B00000000, B00011000, B00000000, B00000011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B01100000, B00000000, B00000000, B00000000, B11000011, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B01100000, B00000000, B00000001, B10000000, B11000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000001, B10000000, B11000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000001, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    if (analogRead(PotPin) < HighVolt) i = 0;  // On manual mode; show goes until button is pushed
}

/*
 * Show - horizontal streamers
 */
for (i=0; i<1; i++)
{  
    if (analogRead(PotPin) >= HighVolt && AutoState == HIGH) break;  // Automatic mode On; Wink mode only On so skip this show
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00100000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00110000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00011000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00001100, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000110, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00010000, B00000011, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00110000, B00000001, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B01100000, B00000000, B10000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B11000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000001, B10000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000011, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000110, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000100, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B10000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B11100000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B01110000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00111000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00011100, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00001110, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000110, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000010, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000010, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000100, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00001000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00010000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00100000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B01000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00001000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00001100, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000110, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000011, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000001, B10000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00001000, B00000000, B11000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00011000, B00000000, B01000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00111000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B01110000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B11100000, B00000000, B00000000, B00000001, B00000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000001, B11000000, B00000000, B00000000, B00000000, B10000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000001, B10000000, B00000000, B00000000, B00000000, B01000000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000001, B00000000, B00000000, B00000000, B00000000, B00000100, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00001000, GetDelay()); if (ButtonPushed()) break;
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, GetDelay()); if (ButtonPushed()) break;
    if (analogRead(PotPin) < HighVolt) i = -1;  // On manual mode; show goes until button is pushed
}

// Start shows at beginning
}

int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

