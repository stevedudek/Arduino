/* Chandelier
 * --------------
 * Lights 24 LEDs via 2 shift registers
 */


int data = 9;
int strob = 8;
int clock = 10;
int oe = 11;

int count = 0;
int i = 0;

byte dato1 = 0;
byte dato2 = 0;
byte dato3 = 0;

int strob1Delay = 20;
int strob2Delay = 20;
int oeDelay = 20;
int ClockDelay = 20;
int DDelay;        // default delay time for auto mode - different for each show
float delaytime;     // variable delay time
int ShowRepeat;    // number of times a show repeats on automode

int numlights = 24;  // number of LEDs

void setup() {
  //beginSerial(9600);
  pinMode(data, OUTPUT);
  pinMode(clock, OUTPUT);
  pinMode(strob, OUTPUT);
  pinMode(oe, OUTPUT);
}

void PulseClock(void) {
    digitalWrite(clock, LOW);
    delayMicroseconds(ClockDelay);
    digitalWrite(clock, HIGH);
    delayMicroseconds(ClockDelay*2);
    digitalWrite(clock, LOW);
    delayMicroseconds(ClockDelay);
    digitalWrite(oe, HIGH);
    delayMicroseconds(oeDelay);
}


void LightUp(byte dato1, byte dato2, byte dato3, int delaytime)
{  
   for (count = 0; count < 8; count++) {
    digitalWrite(data, dato3 & 01);
    dato3>>=1;
    PulseClock();
   }
   
   for (count = 0; count < 8; count++) {
    digitalWrite(data, dato2 & 01);
    dato2>>=1;
    PulseClock();
   }
   
   for (count = 0; count < 8; count++) {
    digitalWrite(data, dato1 & 01);
    dato1>>=1;
    
    if (count == 7) {
      digitalWrite(oe, LOW);
      delayMicroseconds(strob1Delay);
      digitalWrite(strob, HIGH);
    }
    PulseClock();
   }

    delayMicroseconds(strob2Delay);
    digitalWrite(strob, LOW);
    delay(delaytime);  // Hold light pattern for this many microseconds
}

void loop()
{

/*
 * Show - all lights on
 */
DDelay = 1000;
ShowRepeat = 5;

for (i=0; i<ShowRepeat; i++)
{  
  LightUp(B11111111, B11111111, B11111111, DDelay);
}
  
/*
 * Show - one light at a time
 */
DDelay = 100;
ShowRepeat = 10;

for (i=0; i<ShowRepeat; i++)
{  
  LightUp(B10000000, B00000000, B00000000, DDelay);
  LightUp(B01000000, B00000000, B00000000, DDelay);
  LightUp(B00100000, B00000000, B00000000, DDelay);
  LightUp(B00010000, B00000000, B00000000, DDelay);
  LightUp(B00001000, B00000000, B00000000, DDelay);
  LightUp(B00000100, B00000000, B00000000, DDelay);
  LightUp(B00000010, B00000000, B00000000, DDelay);
  LightUp(B00000001, B00000000, B00000000, DDelay);
  LightUp(B00000000, B10000000, B00000000, DDelay);
  LightUp(B00000000, B01000000, B00000000, DDelay);
  LightUp(B00000000, B00100000, B00000000, DDelay);
  LightUp(B00000000, B00010000, B00000000, DDelay);
  LightUp(B00000000, B00001000, B00000000, DDelay);
  LightUp(B00000000, B00000100, B00000000, DDelay);
  LightUp(B00000000, B00000010, B00000000, DDelay);
  LightUp(B00000000, B00000001, B00000000, DDelay);
  LightUp(B00000000, B00000000, B10000000, DDelay);
  LightUp(B00000000, B00000000, B01000000, DDelay);
  LightUp(B00000000, B00000000, B00100000, DDelay);
  LightUp(B00000000, B00000000, B00010000, DDelay);
  LightUp(B00000000, B00000000, B00001000, DDelay);
  LightUp(B00000000, B00000000, B00000100, DDelay);
  LightUp(B00000000, B00000000, B00000010, DDelay);
  LightUp(B00000000, B00000000, B00000001, DDelay);
}

/*
 * Show - lights descending on all strands until all full; single light rises back up
 */
DDelay = 200;
ShowRepeat = 10;

for (i=0; i<ShowRepeat; i++)
{  
  LightUp(B10000010, B00010000, B00100000, DDelay);
  LightUp(B11000011, B00011000, B00110000, DDelay);
  LightUp(B11100011, B10011100, B00111000, DDelay);
  LightUp(B11110011, B11011110, B00111100, DDelay);
  LightUp(B11111011, B11111111, B00111110, DDelay);
  LightUp(B11111111, B11111111, B10111111, DDelay);
  LightUp(B11111111, B11111111, B11111111, DDelay);
  LightUp(B01111101, B11101111, B11011111, DDelay);
  LightUp(B00111100, B11100111, B11001111, DDelay);
  LightUp(B00011100, B01100011, B11000111, DDelay);
  LightUp(B00001100, B00100001, B11000011, DDelay);
  LightUp(B00000100, B00000000, B11000001, DDelay);
  LightUp(B00000000, B00000000, B01000000, DDelay);
  LightUp(B00000000, B00000000, B00000000, DDelay*5);
  LightUp(B00000000, B00000000, B01000000, DDelay/2);
  LightUp(B00000000, B00000000, B10000000, DDelay/2);
  LightUp(B00000000, B00000001, B00000000, DDelay/2);
  LightUp(B00000000, B00000010, B00000000, DDelay/2);
  LightUp(B00000000, B00000100, B00000000, DDelay/2);
  LightUp(B00000000, B00001000, B00000000, DDelay/2);
  LightUp(B00000000, B00010000, B00000000, DDelay/2);
  LightUp(B00000000, B00000000, B00000000, DDelay/2);
}

/*
 * Show - one light per strand descending
 */
DDelay = 50;
ShowRepeat = 15;

for (i=0; i<ShowRepeat; i++)
{  
  LightUp(B10000010, B00010000, B00100000, DDelay);
  LightUp(B01000001, B00001000, B00010000, DDelay);
  LightUp(B00100000, B10000100, B00001000, DDelay);
  LightUp(B00010000, B01000010, B00000100, DDelay);
  LightUp(B00001000, B00100001, B00000010, DDelay);
  LightUp(B00000100, B00000000, B10000001, DDelay);
  LightUp(B00000000, B00000000, B01000000, DDelay);
}

/*
 * Show - inverse strands filling
 */
DDelay = 200;
ShowRepeat = 10;

for (i=0; i<ShowRepeat; i++)
{  
  LightUp(B10000000, B00110000, B00000001, DDelay);
  LightUp(B11000000, B01111000, B00000011, DDelay);
  LightUp(B11100000, B11111100, B00000111, DDelay);
  LightUp(B11110001, B11111110, B00001111, DDelay);
  LightUp(B11111011, B11111111, B00011111, DDelay);
  LightUp(B11111111, B11111111, B10111111, DDelay);
  LightUp(B11111111, B11111111, B11111111, DDelay);
  LightUp(B01111111, B11001111, B11111110, DDelay);
  LightUp(B00111111, B10000111, B11111100, DDelay);
  LightUp(B00011111, B00000011, B11111000, DDelay);
  LightUp(B00001110, B00000001, B11110000, DDelay);
  LightUp(B00000100, B00000000, B11100000, DDelay);
  LightUp(B00000000, B00000000, B01000000, DDelay);
  LightUp(B00000000, B00000000, B00000000, DDelay);
}

/*
 * Show - blinking
 */
DDelay = 1000;
ShowRepeat = 5;

for (i=0; i<ShowRepeat; i++)
{  
  LightUp(B11111111, B11111111, B11111111, DDelay);
  LightUp(B11011111, B11111111, B11111111, DDelay);
  LightUp(B11111111, B11111101, B11111111, DDelay);
  LightUp(B11111111, B11111111, B11111011, DDelay);
  LightUp(B11111111, B01111111, B11111111, DDelay);
  LightUp(B10111111, B11111111, B11111111, DDelay);
  LightUp(B11111111, B10111111, B11111111, DDelay);
  LightUp(B11111111, B11111111, B11011111, DDelay);
  LightUp(B11111111, B11111111, B11111110, DDelay);
  LightUp(B11111111, B11111011, B11111111, DDelay);
}

/*
 * Show - spiral down
 */
DDelay = 100;
ShowRepeat = 20;

for (i=0; i<ShowRepeat; i++)
{  
  LightUp(B10000000, B00000000, B00000000, DDelay);
  LightUp(B00000010, B00000000, B00000000, DDelay);
  LightUp(B00000000, B00010000, B00000000, DDelay);
  LightUp(B00000000, B00000000, B00100000, DDelay);
  LightUp(B01000000, B00000000, B00000000, DDelay);
  LightUp(B00000001, B00000000, B00000000, DDelay);
  LightUp(B00000000, B00001000, B00000000, DDelay);
  LightUp(B00000000, B00000000, B00010000, DDelay);
  LightUp(B00100000, B00000000, B00000000, DDelay);
  LightUp(B00000000, B10000000, B00000000, DDelay);
  LightUp(B00000000, B00000100, B00000000, DDelay);
  LightUp(B00000000, B00000000, B00001000, DDelay);
  LightUp(B00010000, B00000000, B00000000, DDelay);
  LightUp(B00000000, B01000000, B00000000, DDelay);
  LightUp(B00000000, B00000010, B00000000, DDelay);
  LightUp(B00000000, B00000000, B00000100, DDelay);
  LightUp(B00001000, B00000000, B00000000, DDelay);
  LightUp(B00000000, B00100000, B00000000, DDelay);
  LightUp(B00000000, B00000001, B00000000, DDelay);
  LightUp(B00000000, B00000000, B00000010, DDelay);
  LightUp(B00000100, B00000000, B00000000, DDelay);
  LightUp(B00000000, B00000000, B10000000, DDelay);
  LightUp(B00000000, B00000000, B00000001, DDelay);
  LightUp(B00000000, B00000000, B01000000, DDelay);
}
  
/*
 * Show -lights alternate
 */
DDelay = 200;
ShowRepeat = 10;

for (i=0; i<ShowRepeat; i++)
{  
  LightUp(B10101010, B10101010, B10101010, DDelay);
  LightUp(B01010101, B01010101, B01010101, DDelay);
}

// Start shows at beginning
}
