/* Scarf Driver
 * --------------
 * Lights 12 LEDs in different patterns via one shift register
 */


#include "WProgram.h"
void setup();
void PulseClock(void);
boolean ButtonPushed (void);
void LightUp(byte dato1, byte dato2, int delaytime);
void loop();
int data = 9;
int strob = 8;
int clock = 10;
int oe = 11;
int button = 7;        // May have a button

int count = 0;
int i = 0;

boolean ButtonState;
boolean ButtonPrev = LOW;
long time = 0;         // the last time the button was pushed
long debounce = 100;   // the debounce time, increase if the output flickers

byte dato1 = 0;
byte dato2 = 0;

int strob1Delay = 20;
int strob2Delay = 20;
int oeDelay = 20;
int ClockDelay = 20;
int DefaultDelay = 200;  // default delay time <- controls speed of movement

int numlights = 12;      // number of lights

void setup() {
  beginSerial(9600);
  pinMode(data, OUTPUT);
  pinMode(clock, OUTPUT);
  pinMode(strob, OUTPUT);
  pinMode(oe, OUTPUT);
  pinMode(button, INPUT);
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

boolean ButtonPushed (void) {  
  
  ButtonState = digitalRead(button);
  
  if (ButtonState == HIGH && ButtonPrev == LOW && millis() - time > debounce) {      // Has button been pushed?
      ButtonState = LOW;                                                             // Yes, reset the button
      ButtonPrev == HIGH;
      time = millis();      // ... and remember when the last button press was    
      return HIGH;          // Button pushed
  } else {                  // Button not pushed
      ButtonPrev = ButtonState;
      return LOW;           
  }
}

void LightUp(byte dato1, byte dato2, int delaytime)
{ 
   for (count = 0; count < 8; count++) {
    digitalWrite(data, dato2 & 01);
    dato2>>=1;
    PulseClock();
   }
   
   for (count = 0; count < 4; count++) {
    digitalWrite(data, dato1 & 01);
    dato1>>=1;
    
    if (count == 3) {
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
 * Show - one scrolling light; show gets progressively faster
 */
for (i=10; i>0; i--)
{  
  LightUp(B0000, B00000000, i*20); if (ButtonPushed()) break;
  LightUp(B1000, B00000000, i*20); if (ButtonPushed()) break;
  LightUp(B0100, B00000000, i*20); if (ButtonPushed()) break;
  LightUp(B0010, B00000000, i*20); if (ButtonPushed()) break;
  LightUp(B0001, B00000000, i*20); if (ButtonPushed()) break;
  LightUp(B0000, B10000000, i*20); if (ButtonPushed()) break;
  LightUp(B0000, B01000000, i*20); if (ButtonPushed()) break;
  LightUp(B0000, B00100000, i*20); if (ButtonPushed()) break;
  LightUp(B0000, B00010000, i*20); if (ButtonPushed()) break;
  LightUp(B0000, B00001000, i*20); if (ButtonPushed()) break;
  LightUp(B0000, B00000100, i*20); if (ButtonPushed()) break;
  LightUp(B0000, B00000010, i*20); if (ButtonPushed()) break;
  LightUp(B0000, B00000001, i*20); if (ButtonPushed()) break;
}

/*
 * Show - lights fill up; empty down
 */
for (i=0; i<5; i++)
{  
  LightUp(B0000, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1000, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1100, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1110, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B10000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11100000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11110000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11111000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11111100, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11111110, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11111111, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11111110, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11111100, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11111000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11110000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11100000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B10000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1110, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1100, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1000, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00000000, DefaultDelay); if (ButtonPushed()) break;
}

/*
 * Show - lights alternate
 */
for (i=0; i<20; i++)
{  
  LightUp(B0101, B01010101, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1010, B10101010, DefaultDelay); if (ButtonPushed()) break;
}

// Start shows at beginning
}

int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

