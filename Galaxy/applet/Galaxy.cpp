/* Galaxyn Driver
 * --------------
 * Lights 8 strands of Christmas lights and 5 blue LEDs in different patterns via a Lilypad microcontroller
*/

#include "WProgram.h"
void setup();
void LightUp(byte dato1, byte dato2, int DelayMultiplier);
int GetDelay();
boolean PotOn();
void loop();
int i = 0;
int count = 0;

int PotPin = 0;        // input pin for delay pot
int PotValue = 0;      // variable to store the delay pot's value

int DefaultDelay = 5;     // Delay when Pot is switched off

byte dato1 = 0;
byte dato2 = 0;

int numlights = 13;      // number of lights

void setup() {  for (i=1; i<(numlights+1); i++) pinMode(i, OUTPUT);  }// set up all the lights

void LightUp(byte dato1, byte dato2, int DelayMultiplier)
{
   for (count = 0; count < 8; count++) {   // These are the Christmas lights
    digitalWrite(count+1, !(dato1 & 01));  // NAND the leftmost bit with a 1
    dato1>>=1;
   }
   
   for (count = 0; count < 5; count++) {   // These are the blue LEDs - center ball is rightmost bit
    digitalWrite(count+9, !(dato2 & 01));  // NAND the rightmost bit with a 1
    dato2>>=1;
   }
 
   delay(DelayMultiplier*GetDelay());  // Hold light pattern for this many microseconds
}

int GetDelay() {
  if (analogRead(PotPin) == 0) return (DefaultDelay);  // Pot turned off; returned a medium value
  else return (1+(analogRead(PotPin)/100));         // Pot on; calculate delay
}

boolean PotOn() { if (analogRead(PotPin) >= 1023) return false; else return true; }

void loop()
{

/*
 * Show - Just light up the blue center ball
 */
LightUp(B00000000, B00001, 40);
  
/*
 * Show - fill up the blue balls 
 */
for (i=5;i>0;i--) { 
  LightUp(B00000000, B00000, 10);
  LightUp(B00000000, B00001, 10);
  LightUp(B00000000, B00011, 10);
  LightUp(B00000000, B00111, 10);
  LightUp(B00000000, B01111, 10);
  LightUp(B00000000, B11111, 10);
  if (i==1 && PotOn()==false) { i=2; }
}

/*
 * Show - Twirling blue balls 
 */
for (i=10;i>0;i--) { 
  LightUp(B00000000, B00011, 15);
  LightUp(B00000000, B00101, 15);
  LightUp(B00000000, B01001, 15);
  LightUp(B00000000, B10001, 15);
  LightUp(B00000000, B01001, 15);
  LightUp(B00000000, B00101, 15);
  if (i==1 && PotOn()==false) { i=2; }
}

/*
 * Show - Blue balls progressive faster
 */
for (i=15;i>0;i--) {
  LightUp(B00000000, B00001, i);
  LightUp(B00000000, B00010, i);
  LightUp(B00000000, B00100, i);
  LightUp(B00000000, B01000, i);
  LightUp(B00000000, B10000, i);
  if (i==1 && PotOn()==false) { i=15; }
}
  LightUp(B11111111, B11111, 10);  // Finale - everything lights up
  LightUp(B00000000, B00000, 30);  // And then goes dark
  
/*
 * Show - One galaxy strand progressiving
 */
for (i=20;i>0;i--) {
  LightUp(B10000000, B00000, 10);
  LightUp(B01000000, B00000, 10);
  LightUp(B00100000, B00000, 10);
  LightUp(B00010000, B00000, 10);
  LightUp(B00001000, B00000, 10);
  LightUp(B00000100, B00000, 10);
  LightUp(B00000010, B00000, 10);
  LightUp(B00000001, B00000, 10);
  if (i==1 && PotOn()==false) { i=2; }
}

/*
 * Show - Galaxy Twinkles
 */
for (i=4;i>0;i--) {
  LightUp(B11111111, B00001, 100);
  LightUp(B11111101, B00001, 100);
  LightUp(B11111101, B10000, 100);
  LightUp(B11011111, B10000, 100);
  LightUp(B11011111, B00000, 100);
  LightUp(B01111111, B00000, 100);
  LightUp(B01111011, B00100, 100);
  LightUp(B10111011, B00101, 100);
  LightUp(B10101111, B00100, 100);
  LightUp(B11101110, B00000, 100);
  LightUp(B11110110, B01000, 100);
  LightUp(B11110111, B00010, 100);
  if (i==1 && PotOn()==false) { i=2; }
}

/*
 * Show - Galaxy fills up faster
 */
for (i=15;i>0;i--) {
  LightUp(B00000000, B00000, i);
  LightUp(B10000000, B00000, i);
  LightUp(B11000000, B00000, i);
  LightUp(B11100000, B00000, i);
  LightUp(B11110000, B00000, i);
  LightUp(B11111000, B00000, i);
  LightUp(B11111100, B00000, i);
  LightUp(B11111110, B00000, i);
  LightUp(B11111111, B00001, i);
  if (i==1 && PotOn()==false) { i=15; }
}

  LightUp(B11111111, B11111, 20);  // Finale - everything lights up
  LightUp(B00000000, B00000, 60);  // And then goes dark

}  // Start at the top

int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

