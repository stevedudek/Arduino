/* Clock Driver
 * --------------
 * Lights a ring of 8 LEDs strands in different patterns via a Lilypad microcontroller
*/

int i = 0;
int j = 0;
int count = 0;
int delayshow = 0;

byte dato1 = 0;

int numlights = 8;      // number of lights

void setup() {  for (i=0; i<numlights; i++) pinMode(i, OUTPUT);  }// set up all the lights

void LightUp(byte dato1, int delaytime)
{ 
   for (count = 0; count < 8; count++) {
    digitalWrite(count, !(dato1 & 01));  // NAND the rightmost bit with a 1
    dato1>>=1;
   }
   
   delay(delaytime);  // Hold light pattern for this many microseconds
}


void loop()
{

LightUp(B11111111, 10000);  

/*
 * Show - Lights fill up and then down
 */
 
delayshow = 150;

for (j=0;j<20;j++) {
  LightUp(B10000000, delayshow);
  LightUp(B01000000, delayshow);
  LightUp(B00100000, delayshow);
  LightUp(B00010000, delayshow);
  LightUp(B00001000, delayshow);
  LightUp(B00000100, delayshow);
  LightUp(B00000010, delayshow);
  LightUp(B00000001, delayshow);
}

LightUp(B11111111, 10000); 

/*
 * Show - Half Circle Progresses
 */

delayshow = 300;

for (j=0;j<30;j++) {
  LightUp(B11110000, delayshow);
  LightUp(B01111000, delayshow);
  LightUp(B00111100, delayshow);
  LightUp(B00011110, delayshow);
  LightUp(B00001111, delayshow);
  LightUp(B10000111, delayshow);
  LightUp(B11000011, delayshow);
  LightUp(B11100001, delayshow);
}

LightUp(B11111111, 10000); 

/*
 * Show - Lights alternate
 */

delayshow = 3000;

for (j=0;j<20;j++) {
  LightUp(B10101010, delayshow);
  LightUp(B01010101, delayshow);
}

LightUp(B11111111, 10000);

/*
 * Show - Two lights travel
 */
 
delayshow = 400;

for (j=0;j<10;j++) {
  LightUp(B10000001, delayshow);
  LightUp(B01000010, delayshow);
  LightUp(B00100100, delayshow);
  LightUp(B00011000, delayshow);
  LightUp(B00100100, delayshow);
  LightUp(B01000010, delayshow);
}

LightUp(B11111111, 10000);

/*
 * Show - lights fill up, fall back
 */

delayshow = 250;

for (j=0;j<10;j++) {
  LightUp(B00000000, delayshow);
  LightUp(B00000001, delayshow);
  LightUp(B00000011, delayshow);
  LightUp(B00000111, delayshow);
  LightUp(B00001111, delayshow);
  LightUp(B00011111, delayshow);
  LightUp(B00111111, delayshow);
  LightUp(B01111111, delayshow);
  LightUp(B11111111, delayshow);
  LightUp(B11111110, delayshow);
  LightUp(B11111100, delayshow);
  LightUp(B11111000, delayshow);
  LightUp(B11110000, delayshow);
  LightUp(B11100000, delayshow);
  LightUp(B11000000, delayshow);
  LightUp(B10000000, delayshow);
}

}
