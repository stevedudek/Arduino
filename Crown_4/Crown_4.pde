/* Crown Driver
 * --------------
 * Lights 13 LEDs in different patterns via a Lilypad microcontroller
*/

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
 * Show - All Light
 */
void AllLight (void) { LightUp(B11111, B11111111, 5000); }

void loop()
{

AllLight();

/*
 * Show - Cascading bands out from center
 */
for (i=0;i<5;i++) {
  LightUp(B00000, B01000000, 150);
  LightUp(B00000, B11100000, 150);
  LightUp(B00001, B11110000, 150);  
  LightUp(B00011, B11111000, 150);
  LightUp(B00111, B11111100, 150);
  LightUp(B01111, B11111110, 150);
  LightUp(B11111, B11111111, 150);
  LightUp(B11111, B10111111, 150);
  LightUp(B11111, B00011111, 150);
  LightUp(B11110, B00001111, 150);
  LightUp(B11100, B00000111, 150);
  LightUp(B11000, B00000011, 150);
  LightUp(B10000, B00000001, 150);
  LightUp(B00000, B00000000, 150);
}

AllLight();

/*
 * Show - Band of3 lights travel
 */
for (i=0;i<20;i++) {
  LightUp(B11100, B00000000, 200);
  LightUp(B01110, B00000000, 200);
  LightUp(B00111, B00000000, 200);
  LightUp(B00011, B10000000, 200);
  LightUp(B00001, B11000000, 200);
  LightUp(B00000, B11100000, 200);
  LightUp(B00000, B01110000, 200);
  LightUp(B00000, B00111000, 200);
  LightUp(B00000, B00011100, 200);  
  LightUp(B00000, B00001110, 200); 
  LightUp(B00000, B00000111, 200);
  LightUp(B10000, B00000011, 200);
  LightUp(B11000, B00000001, 200);
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
  LightUp(B01111, B11111111, 100);
  LightUp(B00111, B11111111, 100);
  LightUp(B00011, B11111111, 100);
  LightUp(B00001, B11111111, 100);
  LightUp(B00000, B11111111, 100);
  LightUp(B00000, B01111111, 100);
  LightUp(B00000, B00111111, 100);
  LightUp(B00000, B00011111, 100);
  LightUp(B00000, B00001111, 100);
  LightUp(B00000, B00000111, 100);
  LightUp(B00000, B00000011, 100);
  LightUp(B00000, B00000001, 100);
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
 * Show - opposite sides revolve
 */
for (i=0;i<15;i++) {  
  LightUp(B10000, B01000000, 200);
  LightUp(B01000, B00100000, 200);
  LightUp(B00100, B00010000, 200);
  LightUp(B00010, B00001000, 200);
  LightUp(B00001, B00000100, 200);
  LightUp(B00000, B10000010, 200);
  LightUp(B00000, B01000001, 200);
}

}
