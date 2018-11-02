/* Light Pants Driver
 * --------------
 * Lights 14 LEDs in different patterns via a Lilypad microcontroller
 * Pants have 7 LEDs down each leg
 *
 * 01111111 01111111
 *   Left    Right
 * Top  Btm Top  Btm
 *
*/

int i = 0;
int count = 0;
int StdDelay = 0;

byte dato1 = 0;
byte dato2 = 0;

int numlights = 14;      // number of lights

void setup() {  for (i=0; i<numlights; i++) pinMode(i, OUTPUT);  } // set up all the lights

void LightUp(byte dato1, byte dato2, int delaytime)
{ 
   for (count = 0; count < 7; count++) {
    digitalWrite(count,   !(dato2 & 01));  // NAND the rightmost bit2 with a 1
    digitalWrite(count+7, !(dato1 & 01));  // NAND the rightmost bit1 with a 1
    dato2>>=1;
    dato1>>=1;
   }
   delay(delaytime);  // Hold light pattern for this many microseconds
}

void loop()
{

/*
 * Show - Rise Up Slow
 */
for (i=0;i<1;i++) { 
  LightUp(B0000001, B0000001, 10000); 
  LightUp(B0000011, B0000011, 10000); 
  LightUp(B0000111, B0000111, 10000); 
  LightUp(B0001111, B0001111, 10000); 
  LightUp(B0011111, B0011111, 10000); 
  LightUp(B0111111, B0111111, 10000);
  LightUp(B1111111, B1111111, 10000); 
}

/*
 * Show - Rise Up Faster and Faster
 */
for (i=0;i<21;i++) {
  LightUp(B0000000, B0000000, 200-i*10); 
  LightUp(B0000001, B0000001, 200-i*10); 
  LightUp(B0000011, B0000011, 200-i*10); 
  LightUp(B0000111, B0000111, 200-i*10); 
  LightUp(B0001111, B0001111, 200-i*10); 
  LightUp(B0011111, B0011111, 200-i*10); 
  LightUp(B0111111, B0111111, 200-i*10);
  LightUp(B1111111, B1111111, 200-i*10); 
}

/*
 * Show - Up one leg and down the other
 */
for (i=0;i<20;i++) {
  StdDelay = 6;
  LightUp(B0000000, B0000001, 16*StdDelay); 
  LightUp(B0000000, B0000010, 14*StdDelay); 
  LightUp(B0000000, B0000100, 12*StdDelay); 
  LightUp(B0000000, B0001000, 10*StdDelay); 
  LightUp(B0000000, B0010000, 8*StdDelay); 
  LightUp(B0000000, B0100000, 6*StdDelay); 
  LightUp(B0000000, B1000000, 4*StdDelay); 
  LightUp(B1000000, B0000000, 4*StdDelay); 
  LightUp(B0100000, B0000000, 6*StdDelay); 
  LightUp(B0010000, B0000000, 8*StdDelay); 
  LightUp(B0001000, B0000000, 10*StdDelay); 
  LightUp(B0000100, B0000000, 12*StdDelay); 
  LightUp(B0000010, B0000000, 14*StdDelay); 
  LightUp(B0000001, B0000000, 16*StdDelay); 
  LightUp(B0000010, B0000000, 14*StdDelay); 
  LightUp(B0000100, B0000000, 12*StdDelay); 
  LightUp(B0001000, B0000000, 10*StdDelay); 
  LightUp(B0010000, B0000000, 8*StdDelay); 
  LightUp(B0100000, B0000000, 6*StdDelay); 
  LightUp(B1000000, B0000000, 4*StdDelay); 
  LightUp(B0000000, B1000000, 4*StdDelay); 
  LightUp(B0000000, B0100000, 6*StdDelay); 
  LightUp(B0000000, B0010000, 8*StdDelay); 
  LightUp(B0000000, B0001000, 10*StdDelay); 
  LightUp(B0000000, B0000100, 12*StdDelay); 
  LightUp(B0000000, B0000010, 14*StdDelay); 
  LightUp(B0000000, B0000001, 16*StdDelay);
}   

/*
 * Show - All on
 */
  LightUp(B0000000, B0000001, 700);

/*
 * Show - Alternate
 */
for (i=0;i<20;i++) {
  LightUp(B1010101, B0101010, 250); 
  LightUp(B0101010, B1010101, 250); 
}

/*
 * Show - Meet in Middle
 */
for (i=0;i<20;i++) {
  LightUp(B1000001, B1000001, 100); 
  LightUp(B0100010, B0100010, 100);
  LightUp(B0010100, B0010100, 100);
  LightUp(B0001000, B0001000, 100);
  LightUp(B0010100, B0010100, 100);
  LightUp(B0100010, B0100010, 100);
}

}
