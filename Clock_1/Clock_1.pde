/* Clock Driver
 * --------------
 * Lights a ring of 12 LEDs in different patterns via a Lilypad microcontroller
*/

int i = 0;
int j = 0;
int count = 0;
int count2 = 0;

int data = 0;        // remember, an int is two bytes
int data2 = 0;
int data3 = 0;
int hold = 0;
int clock = 0;

byte dato1 = 0;             
byte dato2 = 0;
boolean carry = false;

int numlights = 12;      // number of lights

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

void LightUpInt(int data, int delaytime)
{
   for (count = 0; count < 12; count++) {
      digitalWrite(count+1, !(data & 01));  // NAND the rightmost bit with a 1
      data>>=1;
   }
   delay(delaytime);  // Hold light pattern for this many microseconds
}

int ShiftLeft(int Shifter) {
  Shifter<<=1;                           // Shift all bits left one
  if (Shifter>>12) Shifter=Shifter|1;    // Tack the high bit back on to the 1st light
  return Shifter;                        // Return the shifted bits
} 

void CycleLights(int data2, int delaytime) {   // Takes the pattern in data2
  for (i=0;i<12;i++) {                         // and cycles it around the ring
    LightUpInt(data2, delaytime);              // at a specified delay time
    data2 = ShiftLeft(data2);
  }
}

void CycleLightsWithHold(int hold, int data2, int delaytime) { // Takes the pattern in data2
  for (i=0;i<12;i++) {                // and cycles it around the ring BUT keeps the pattern in hold constant
    LightUpInt(data2|hold, delaytime);     // at a specified delay time
    data2 = ShiftLeft(data2);
  }
} 

void Clock() {   // runs a "clock" cycle of 12 hours
  hold = 1;                    // Hour hand. Starts at 12 o'clock
  for (j=0;j<12;j++) {         // Hour's cycle of 12 hours
    data2 = 1;                 // Start minute hand at 12 o'clock
    CycleLightsWithHold(hold, data2, 150);      // Run 1 hour
    hold = ShiftLeft(hold);      // Advance the hour
  }
}

void loop()   // Run the clock; then run a show; then run the clock; etc.
{

Clock();
/*
 * Show - One light scrolls across progressively faster
 */
for (j=0;j<10;j++) { CycleLights(1, 1000-100*j); }

Clock();
/*
 * Show - Lights cycle and fill up by one for each cycle
 */
data3 = 1;
for (j=0;j<12;j++) {
  CycleLights(data3, 150);
  data3 = ShiftLeft(data3);
  data3 = data3 | 1;
}

/*
 * Show - Lights cycle and decrease up by one for each cycle
 */
data3 = 65535;
for (j=0;j<12;j++) {
  CycleLights(data3, 200);
  data3 = ShiftLeft(data3);
  data3 = data3 ^ 1;
}

Clock();
/*
 * Show - Lights fill up and then down
 */
for (j=0;j<10;j++) {
  LightUp(B00000000, B00000000, 100);
  LightUp(B00000000, B00000001, 100);
  LightUp(B00000000, B00000011, 100);
  LightUp(B00000000, B00000111, 100);
  LightUp(B00000000, B00001111, 100);
  LightUp(B00000000, B00011111, 100);
  LightUp(B00000000, B00111111, 100);
  LightUp(B00000000, B01111111, 100);
  LightUp(B00000000, B11111111, 100);
  LightUp(B00000001, B11111111, 100);
  LightUp(B00000011, B11111111, 100);
  LightUp(B00000111, B11111111, 100);
  LightUp(B00001111, B11111111, 100);
  LightUp(B00001111, B11111110, 100);
  LightUp(B00001111, B11111100, 100);
  LightUp(B00001111, B11111000, 100);
  LightUp(B00001111, B11110000, 100);
  LightUp(B00001111, B11100000, 100);
  LightUp(B00001111, B11000000, 100);
  LightUp(B00001111, B10000000, 100);
  LightUp(B00001111, B00000000, 100);
  LightUp(B00001110, B00000000, 100);
  LightUp(B00001100, B00000000, 100);
  LightUp(B00001000, B00000000, 100);
}

Clock();
/*
 * Show - Lights alternate
 */
for (j=0;j<20;j++) {
  LightUp(B00001010, B10101010, 1000);
  LightUp(B00000101, B01010101, 1000);
}

Clock();
/*
 * Show - Two lights travel
 */
for (j=0;j<10;j++) {
  LightUp(B00000000, B00000001, 500);
  LightUp(B00001000, B00000010, 400);
  LightUp(B00000100, B00000100, 300);
  LightUp(B00000010, B00001000, 200);
  LightUp(B00000001, B00010000, 100);
  LightUp(B00000000, B10100000, 50);
  LightUp(B00000000, B01000000, 50);
  LightUp(B00000000, B10100000, 100);
  LightUp(B00000001, B00010000, 200);
  LightUp(B00000010, B00001000, 300);
  LightUp(B00000100, B00000100, 400);
  LightUp(B00001000, B00000010, 500);
}

Clock();
/*
 * Show - Two lights fill up, fall back
 */
for (j=0;j<10;j++) {
  LightUp(B00000000, B00000001, 200-10*j);
  LightUp(B00001000, B00000011, 200-10*j);
  LightUp(B00001100, B00000111, 200-10*j);
  LightUp(B00001110, B00001111, 200-10*j);
  LightUp(B00001111, B00011111, 200-10*j);
  LightUp(B00001111, B10111111, 200-10*j);
  LightUp(B00001111, B11111111, 200-10*j);
  LightUp(B00000111, B11111110, 200-10*j);
  LightUp(B00000011, B11111100, 200-10*j);
  LightUp(B00000001, B11111000, 200-10*j);
  LightUp(B00000000, B11110000, 200-10*j);
  LightUp(B00000000, B01100000, 200-10*j);
  LightUp(B00000000, B00000000, 200-10*j);
}

}
