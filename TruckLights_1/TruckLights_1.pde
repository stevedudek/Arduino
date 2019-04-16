/* Rake Driver
 * --------------
 * Lights 13 LEDs in different patterns via a Lilypad microcontroller
*/

int showdelay = 0;
int i = 0;
int count = 0;

byte dato1 = 0;
byte dato2 = 0;

int numlights = 10;      // number of lights

void setup() {  for (i=1; i<(numlights+1); i++) pinMode(i, OUTPUT);  }// set up all the lights

void LightUp(byte dato1, byte dato2, int delaytime)
{ 
   for (count = 0; count < 8; count++) {
    digitalWrite(count+1, !(dato2 & 01));  // NAND the rightmost bit with a 1
    dato2>>=1;
   }
   
   for (count = 0; count < 2; count++) {
    digitalWrite(count+9, !(dato1 & 01));  // NAND the rightmost bit with a 1
    dato1>>=1;
   }
 
   delay(delaytime);  // Hold light pattern for this many microseconds
}

void loop()
{

/*
 * Show - Alternating half lights on
 */
showdelay = 10000;

for (i=0; i<10; i++) {
  LightUp(B10, B10101010, showdelay);
  LightUp(B01, B01010101, showdelay);
}

/*
 * Show - Cascading bands out from sides to center
 */
showdelay = 200;

for (i=0;i<15;i++) {
  LightUp(B10, B00000001, showdelay);
  LightUp(B01, B00000010, showdelay);
  LightUp(B00, B10000100, showdelay);
  LightUp(B00, B01001000, showdelay);
  LightUp(B00, B00110000, showdelay);
}

/*
 * Show - Wall bounce
 */
showdelay = 1000; 
 
for (i=0; i<6; i++) {
  LightUp(B11, B11100000, showdelay);
  LightUp(B00, B00011111, showdelay);
}

/*
 * Show - Lights go from dark to on
 */
showdelay = 10000;

LightUp(B00, B00000000, showdelay);
LightUp(B01, B00000000, showdelay);
LightUp(B01, B00010000, showdelay);
LightUp(B01, B00010100, showdelay);
LightUp(B11, B00010100, showdelay);
LightUp(B11, B00010101, showdelay);
LightUp(B11, B01010101, showdelay);
LightUp(B11, B01010111, showdelay);
LightUp(B11, B11010111, showdelay);
LightUp(B11, B11110111, showdelay);
LightUp(B11, B11111111, showdelay);

/*
 * Show - Only center lights
 */
showdelay = 1000; 
 
LightUp(B00, B00110000, showdelay);

/*
 * Show - Lights process around the room
 */
showdelay = 100;

for (i=0;i<15;i++) {
  LightUp(B10, B00000000, showdelay);
  LightUp(B01, B00000000, showdelay);
  LightUp(B00, B10000000, showdelay);
  LightUp(B00, B01000000, showdelay);
  LightUp(B00, B00100000, showdelay);
  LightUp(B00, B00010000, showdelay);
  LightUp(B00, B00001000, showdelay);
  LightUp(B00, B00000100, showdelay);
  LightUp(B00, B00000010, showdelay);
  LightUp(B00, B00000001, showdelay);
}
}

