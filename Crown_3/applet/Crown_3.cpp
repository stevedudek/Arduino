/* Crown Driver
 * --------------
 * Lights 13 LEDs in different patterns via a Lilypad microcontroller
*/

#include "WProgram.h"
void setup();
void LightUp(byte dato1, byte dato2, int delaytime);
void AllLight (void);
void loop();
int i = 0;
int j = 0;
int k = 0;
int count = 0;

byte dato1 = 0;
byte dato2 = 0;

int numlights = 13;      // number of lights

void setup() {  for (i=1; i<(numlights+1); i++) pinMode(i, OUTPUT);  }// set up all the lights

void LightUp(byte dato1, byte dato2, int delaytime)
{ 
   for (count = 0; count < 8; count++) {
    digitalWrite(count+1, !(dato2 & 01));  // NAND the rightmost bit with a 1
    dato2>>=1;
   }
   
   for (count = 0; count < 5; count++) {
    digitalWrite(count+9, !(dato1 & 01));  // NAND the rightmost bit with a 1
    dato1>>=1;
   }
 
   delay(delaytime);  // Hold light pattern for this many microseconds
}

/*
 * Show - All Light - strobe the center light
 */
void AllLight (void) { LightUp(B11111, B11111111, 2000); }

void loop()
{

AllLight();

/*
 * Show - Cascading bands out from center
 */
for (i=0;i<5;i++) {
  LightUp(B00000, B01000000, 75);
  LightUp(B00000, B11100000, 75);
  LightUp(B00001, B11110000, 75);  
  LightUp(B00011, B11111000, 75);
  LightUp(B00111, B11111100, 75);
  LightUp(B01111, B11111110, 75);
  LightUp(B11111, B11111111, 75);
  LightUp(B11111, B10111111, 75);
  LightUp(B11111, B00011111, 75);
  LightUp(B11110, B00001111, 75);
  LightUp(B11100, B00000111, 75);
  LightUp(B11000, B00000011, 75);
  LightUp(B10000, B00000001, 75);
  LightUp(B00000, B00000000, 75);
}

AllLight();

/*
 * Show - Lights race each other
 */
for (i=0;i<5;i++) {
  LightUp(B10000, B00000010, 200);
  LightUp(B01000, B00000001, 200);
  LightUp(B10100, B00000000, 200);
  LightUp(B01010, B00000000, 200);
  LightUp(B00101, B00000000, 200);
  LightUp(B00010, B10000000, 200);
  LightUp(B00001, B01000000, 200);
  LightUp(B00000, B10100000, 200);
  LightUp(B00000, B01010000, 200);  
  LightUp(B00000, B00101000, 200); 
  LightUp(B00000, B00010100, 200);
  LightUp(B00000, B00001010, 200);
  LightUp(B00000, B00000101, 200);
}

AllLight();

/*
 * Show - One light scrolls across progressively faster
 */
for (i=0;i<10;i++) {
  LightUp(B00000, B00000001, 100-10*i);
  LightUp(B00000, B00000010, 100-10*i);
  LightUp(B00000, B00000100, 100-10*i);
  LightUp(B00000, B00001000, 100-10*i);
  LightUp(B00000, B00010000, 100-10*i);
  LightUp(B00000, B00100000, 100-10*i);
  LightUp(B00000, B01000000, 100-10*i);
  LightUp(B00000, B10000000, 100-10*i);
  LightUp(B00001, B00000000, 100-10*i);
  LightUp(B00010, B00000000, 100-10*i);
  LightUp(B00100, B00000000, 100-10*i);
  LightUp(B01000, B00000000, 100-10*i);
  LightUp(B10000, B00000000, 100-10*i);
}

AllLight();

/*
 * Show - Lights fill up, fall back
 */
for (i=0;i<2;i++) { 
  LightUp(B00000, B00000000, 100);
  LightUp(B10000, B00000000, 100);
  LightUp(B11000, B00000000, 100);
  LightUp(B11100, B00000000, 100);
  LightUp(B11110, B00000000, 100);
  LightUp(B11111, B00000000, 100);
  LightUp(B11111, B10000000, 100);
  LightUp(B11111, B11000000, 100);
  LightUp(B11111, B11100000, 100);
  LightUp(B11111, B11110000, 100);
  LightUp(B11111, B11111000, 100);
  LightUp(B11111, B11111100, 100);
  LightUp(B11111, B11111110, 100);
  LightUp(B11111, B11111111, 100);
  LightUp(B11111, B11111110, 100);
  LightUp(B11111, B11111100, 100);
  LightUp(B11111, B11111000, 100);
  LightUp(B11111, B11110000, 100);
  LightUp(B11111, B11100000, 100);
  LightUp(B11111, B11000000, 100);
  LightUp(B11111, B10000000, 100);
  LightUp(B11111, B00000000, 100);
  LightUp(B11110, B00000000, 100);
  LightUp(B11100, B00000000, 100);
  LightUp(B11000, B00000000, 100);
  LightUp(B10000, B00000000, 100);
}

AllLight();

/*
 * Show - Two lights cascade out from the center
 */
for (i=0;i<10;i++) {
  LightUp(B00000, B01000000, 150);
  LightUp(B00000, B10100000, 150);
  LightUp(B00001, B00010000, 150);  
  LightUp(B00010, B00001000, 150);
  LightUp(B00100, B00000100, 150);
  LightUp(B01000, B00000010, 150);
  LightUp(B10000, B00000001, 150);
}

AllLight();

/*
 * Show - lights alternate
 */
for (i=0;i<15;i++) {  
  LightUp(B10101, B01010101, 200);
  LightUp(B01010, B10101010, 200);
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

