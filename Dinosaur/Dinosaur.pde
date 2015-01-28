/* Dinosaur Hat Driver
 * --------------
 * Lights 13 LEDs in different patterns via a Lilypad microcontroller
 *
 * A button causes all the lights to turn on
 *
 */

int i = 0;
int count = 0;
int delaytime = 0;

byte dato1 = 0;
byte dato2 = 0;

int numlights = 13;      // number of lights
int buttonPin = 5;       // button is on Analog Pin 5

void setup() { for (i=1; i<(numlights+1); i++) pinMode(i, OUTPUT); } // set up all the lights

void LightUp(byte dato1, byte dato2, int delaytime)
{ 
   if (analogRead(buttonPin) == 0) {  // Button pushed (Don't know why this works)
     dato1 = B11111;                  // Turn all lights on
     dato2 = B11111111;
     delaytime = 50;
   }
   
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

void loop()
{

/*
 * Show - Lights fill from tail to head
 */
for (i=0;i<30;i++) { 
  delaytime = 80;
  LightUp(B10000, B00000000, delaytime);
  LightUp(B11000, B00000000, delaytime);
  LightUp(B11100, B00000000, delaytime);
  LightUp(B11110, B00000000, delaytime);
  LightUp(B11111, B00000000, delaytime);
  LightUp(B11111, B10000000, delaytime);
  LightUp(B11111, B11000000, delaytime);
  LightUp(B11111, B11100000, delaytime);
  LightUp(B11111, B11110000, delaytime);
  LightUp(B11111, B11111000, delaytime);
  LightUp(B11111, B11111100, delaytime);
  LightUp(B11111, B11111110, delaytime);
  LightUp(B11111, B11111111, delaytime);
  LightUp(B11111, B11111110, delaytime);
  LightUp(B11111, B11111100, delaytime);
  LightUp(B11111, B11111000, delaytime);
  LightUp(B11111, B11110000, delaytime);
  LightUp(B11111, B11100000, delaytime);
  LightUp(B11111, B11000000, delaytime);
  LightUp(B11111, B10000000, delaytime);
  LightUp(B11111, B00000000, delaytime);
  LightUp(B11110, B00000000, delaytime);
  LightUp(B11100, B00000000, delaytime);
  LightUp(B11000, B00000000, delaytime);
  LightUp(B10000, B00000000, delaytime);
}

/*
 * Show - All lights off for a long time
 */
for (i=0;i<100;i++) {
  delaytime = 100;
  LightUp(B00000, B00000000, delaytime);
}

/*
 * Show - A single light travels from tail to head progressively faster
 */
for (i=0;i<21;i++) {
  delaytime = 200-i*10;
  LightUp(B00000, B00000001, delaytime);
  LightUp(B00000, B00000010, delaytime);
  LightUp(B00000, B00000100, delaytime);
  LightUp(B00000, B00001000, delaytime);
  LightUp(B00000, B00010000, delaytime);
  LightUp(B00000, B00100000, delaytime);
  LightUp(B00000, B01000000, delaytime);
  LightUp(B00000, B10000000, delaytime);
  LightUp(B00001, B00000000, delaytime);
  LightUp(B00010, B00000000, delaytime);
  LightUp(B00100, B00000000, delaytime);
  LightUp(B01000, B00000000, delaytime);
  LightUp(B10000, B00000000, delaytime);
}

/*
 * Show - All lights off for a long time
 */
for (i=0;i<100;i++) {
  delaytime = 100;
  LightUp(B00000, B00000000, delaytime);
}

/*
 * Show - Lights alternate
 */
for (i=0;i<30;i++) {
  delaytime = 200;
  LightUp(B01010, B10101010, delaytime);
  LightUp(B10101, B01010101, delaytime);
}

/*
 * Show - All lights off for a long time
 */
for (i=0;i<100;i++) {
  delaytime = 100;
  LightUp(B00000, B00000000, delaytime);
}

/*
 * Show - Cascading bands out from center
 */
for (i=0;i<30;i++) {
  delaytime = 100;
  LightUp(B00000, B01000000, delaytime);
  LightUp(B00000, B11100000, delaytime);
  LightUp(B00001, B11110000, delaytime);  
  LightUp(B00011, B11111000, delaytime);
  LightUp(B00111, B11111100, delaytime);
  LightUp(B01111, B11111110, delaytime);
  LightUp(B11111, B11111111, delaytime);
  LightUp(B11111, B10111111, delaytime);
  LightUp(B11111, B00011111, delaytime);
  LightUp(B11110, B00001111, delaytime);
  LightUp(B11100, B00000111, delaytime);
  LightUp(B11000, B00000011, delaytime);
  LightUp(B10000, B00000001, delaytime);
  LightUp(B00000, B00000000, delaytime);
}

/*
 * Show - All lights off for a long time
 */
for (i=0;i<100;i++) {
  delaytime = 100;
  LightUp(B00000, B00000000, delaytime);
}

/*
 * Show - Only the top horn is lit
 */
for (i=0;i<100;i++) {
  delaytime = 100;
  LightUp(B00000, B00000001, delaytime);
}

}
