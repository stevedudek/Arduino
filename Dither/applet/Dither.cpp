/* Dither Test
 * --------------
 * Can I control light intensity through dithering?
*/

#include "WProgram.h"
void setup();
void LightUp(byte dato1, byte dato2, int Delay);
void loop();
int i = 0;
int count = 0;
int DefaultDelay = 1;

byte dato1 = 0;
byte dato2 = 0;

int numlights = 13;      // number of lights

void setup() {  for (i=1; i<(numlights+1); i++) pinMode(i, OUTPUT);  }// set up all the lights

void LightUp(byte dato1, byte dato2, int Delay)
{
   for (count = 0; count < 8; count++) {   // These are the Christmas lights
    digitalWrite(count+1, !(dato1 & 01));  // NAND the leftmost bit with a 1
    dato1>>=1;
   }
   
   for (count = 0; count < 5; count++) {   // These are the blue LEDs - center ball is rightmost bit
    digitalWrite(count+9, !(dato2 & 01));  // NAND the rightmost bit with a 1
    dato2>>=1;
   }
 
   delay(Delay);  // Hold light pattern for this many microseconds
}

void loop()
{

/*
 * Show - Just light up the blue center ball
 */
 

for (i=1;i<50;i++) {
  DefaultDelay=i/5; 
  LightUp(B00111111, B11111, DefaultDelay);
  LightUp(B00000000, B11111, DefaultDelay);
  LightUp(B00000001, B01111, DefaultDelay);
  LightUp(B00000010, B10111, DefaultDelay);
  LightUp(B00000101, B11111, DefaultDelay);
  LightUp(B00000000, B01011, DefaultDelay);
  LightUp(B00001011, B11111, DefaultDelay);
  LightUp(B00000000, B10111, DefaultDelay);
  LightUp(B00000101, B01111, DefaultDelay);
  LightUp(B00000010, B11111, DefaultDelay);
  LightUp(B00000001, B11111, DefaultDelay);
  LightUp(B00000000, B00001, DefaultDelay);
  LightUp(B00011111, B11111, DefaultDelay);
  LightUp(B00000000, B11111, DefaultDelay);
  LightUp(B00000001, B01111, DefaultDelay);
  LightUp(B00000010, B10111, DefaultDelay);
  LightUp(B00000101, B11111, DefaultDelay);
  LightUp(B00000000, B01011, DefaultDelay);
  LightUp(B00001011, B11111, DefaultDelay);
  LightUp(B00000000, B10111, DefaultDelay);
  LightUp(B00000101, B01111, DefaultDelay);
  LightUp(B00000010, B11111, DefaultDelay);
  LightUp(B00000001, B11111, DefaultDelay);
  LightUp(B00000000, B00001, DefaultDelay);
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

