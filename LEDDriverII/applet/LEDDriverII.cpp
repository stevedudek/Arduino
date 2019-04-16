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
void LightUp(byte dato[]);
void loop();
int data = 9;
int strob = 8;
int clock = 10;
int oe = 11;
int button = 7;

int i = 0;
int j = 0;
int buttonstate;
int buttonprev = LOW;
long time = 0;         // the last time the button was pushed
long debounce = 200;   // the debounce time, increase if the output flickers

int numlights = 48;  // number of eyeballs
int LightBytes = 6;  // =(numlights/8) rounded up; constant
byte lights[6] = {0, 0, 0, 0, 0, 0};      // current state of lights
byte dato[6]= {0, 0, 0, 0, 0, 0};        // temporary array to manipulate lights

int currshow = 0;
int totalshows = 2;

void setup()
{
  beginSerial(9600);
  pinMode(data, OUTPUT);
  pinMode(clock, OUTPUT);
  pinMode(strob, OUTPUT);
  pinMode(oe, OUTPUT);
  pinMode(button, INPUT);
}

void PulseClock(void) {
    digitalWrite(clock, LOW);
    delayMicroseconds(20);
    digitalWrite(clock, HIGH);
    delayMicroseconds(50);
    digitalWrite(clock, LOW);
}
  
void LightUp(byte dato[])
{ 
  for (i=0; i < LightBytes; i++) {
    for (j = 0; j < 8; j++) {
      digitalWrite(data, dato[i] & 01);
      dato[i]>>=1;
      if (j == 7 && i == LightBytes-1) {
        digitalWrite(oe, LOW);
        digitalWrite(strob, HIGH);
      } 
      PulseClock();
      digitalWrite(oe, HIGH);
   }
  }
  delayMicroseconds(20);
  digitalWrite(strob, LOW);
}

void loop()
{
  /*
    buttonstate = digitalRead(button);
    if (buttonstate == HIGH && buttonprev == LOW && millis() - time > debounce) {
      if (buttonstate = LOW) buttonstate = LOW; else buttonstate = HIGH;             // reset the button
      currshow++;
      if (currshow == totalshows) currshow = 0;
      // ... and remember when the last button press was
      time = millis();    
    }
    buttonprev = buttonstate;
 */
/*
 * Shows!
 */

/* 
if (currshow == 1) {  // Shift one light from #0 to #end
    long lights = 1;  // Turn all lights off but 1
    
    for (count = 0; count < numlights; count++) {
      LightUpLong(lights); 
      lights<<=1; 
      delay(1000);
    }
}
*/
if (currshow == 0) {
    for (i=0; i<LightBytes; i++) {lights[i] = B00000000; } // Clear lights
    lights[0] = B0000001; // Turn on just one light;
    LightUp(lights);
}
/*
    for (i=0; i < LightBytes; i++) {
      for (j=0; j < 8; j++) {
        LightUp(lights);
        delay(2000);
        lights[j]>>=1;
      }
    }
}
*/

if (currshow == 1) {
    for (i=0; i<LightBytes; i++) lights[i] = B01010101;
    LightUp(lights); 
    delay(1000);
    for (i=0; i<LightBytes; i++) lights[i] = B10101010;
    LightUp(lights); 
    delay(1000);
    }
}

int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

