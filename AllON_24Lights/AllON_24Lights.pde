/* Chandelier
 * --------------
 * Lights 24 LEDs via 2 shift registers all on for those that don't like blinking
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
DDelay = 10000;
ShowRepeat = 5;

for (i=0; i<ShowRepeat; i++)
{  
  LightUp(B11111111, B11111111, B11111111, DDelay);
}

// Start shows at beginning
}
