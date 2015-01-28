/* Shift Out Data
 * --------------
 * http://arduino.cc/en/Tutorial/LEDDriver
 *
 * Shows a byte, stored in "dato" on a set of 8 LEDs
 *
 * (copyleft) 2005 K3, Malmo University
 * @author: David Cuartielles, Marcus Hannerstig
 * @hardware: David Cuartielles, Marcos Yarza
 * @project: made for SMEE - Experiential Vehicles
 */


int data = 9;
int strob = 8;
int clock = 10;
int oe = 11;
int button = 7;

int count = 0;

int ButtonState;
int ButtonPrev = LOW;
long time = 0;         // the last time the button was pushed
long debounce = 200;   // the debounce time, increase if the output flickers

byte dato1 = 0;
byte dato2 = 0;
byte dato3 = 0;
byte dato4 = 0;
byte dato5 = 0;
byte dato6 = 0;

int delaytime = 100;     // delay in microseconds
int numlights = 48;  // number of eyeballs

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
    delayMicroseconds(20);
    digitalWrite(clock, HIGH);
    delayMicroseconds(200);
    digitalWrite(clock, LOW);
}

boolean ButtonPushed (void) {
  ButtonState = digitalRead(button);
  
  if (ButtonState == HIGH && ButtonPrev == LOW && millis() - time > debounce) {      // Has button been pushed?
      ButtonState = LOW;                                                             // Yes, reset the button
      ButtonPrev == HIGH;
      time = millis();      // ... and remember when the last button press was
      return HIGH;          // Button pushed
  } else {
      ButtonPrev = ButtonState;
      return LOW;           // Button not pushed
  }
}
/*
void LightUpLong(long lights) 
{
  int endlight = numlights-1;
  
  for (count = 0; count < numlights; count++) {
    digitalWrite(data, lights & 01);
    lights>>=1;
    
    if (count == endlight) {
      digitalWrite(oe, LOW);
      digitalWrite(strob, HIGH);
     }
  
     PulseClock();
     digitalWrite(oe, HIGH);
    }

  delayMicroseconds(20);
  digitalWrite(strob, LOW);
}
*/

void LightUp(byte dato1, byte dato2, byte dato3, byte dato4, byte dato5, byte dato6, int delaytime)
{ 
  for (count = 0; count < 8; count++) {
    digitalWrite(data, dato6 & 01);
    dato6>>=1;
    PulseClock();
    digitalWrite(oe, HIGH);
   }
  
  for (count = 0; count < 8; count++) {
    digitalWrite(data, dato5 & 01);
    dato5>>=1;
    PulseClock();
    digitalWrite(oe, HIGH);
   } 
  
  for (count = 0; count < 8; count++) {
    digitalWrite(data, dato4 & 01);
    dato4>>=1;
    PulseClock();
    digitalWrite(oe, HIGH);
   }
   
   for (count = 0; count < 8; count++) {
    digitalWrite(data, dato3 & 01);
    dato3>>=1;
    PulseClock();
    digitalWrite(oe, HIGH);
   }
   
   for (count = 0; count < 8; count++) {
    digitalWrite(data, dato2 & 01);
    dato2>>=1;
    PulseClock();
    digitalWrite(oe, HIGH);
   }
   
   for (count = 0; count < 8; count++) {
    digitalWrite(data, dato1 & 01);
    dato1>>=1;
    
    if (count == 7) {
      digitalWrite(oe, LOW);
      digitalWrite(strob, HIGH);
     }
  
     PulseClock();
     digitalWrite(oe, HIGH);
    }

    delayMicroseconds(20);
    digitalWrite(strob, LOW);
    delay(delaytime);  // Hold light pattern for this many microseconds
}

void loop()
{
    
/*
 * Show - one light scrolls from left to right, bottom to top
 */

{
    LightUp(B10000000, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B01000000, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00100000, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00010000, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00001000, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000100, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000010, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000001, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B10000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B01000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00100000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00010000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00001000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000100, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000010, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000001, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B10000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B01000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00100000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00010000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00001000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000100, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000010, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000001, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B10000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B01000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00100000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00010000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00001000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000100, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000010, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000001, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B10000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B01000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00100000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00010000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00001000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000100, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000010, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000001, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B10000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B01000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00100000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00010000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00001000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000100, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000010, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000001, delaytime);
  }
    
  if (currshow == 1) {
    LightUp(B11111110, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000001, B11111000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000111, B11110000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00001111, B11000000, B00000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00111111, B10000000, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B01111110, B00000000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000001, B11110000, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00001100, delaytime); 
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000011, delaytime);
    }
    
  if (currshow == 2) {    
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime);
    LightUp(B11111110, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B11111111, B11111000, B00000000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B11111111, B11111111, B11110000, B00000000, B00000000, B00000000, delaytime); 
    LightUp(B11111111, B11111111, B11111111, B11000000, B00000000, B00000000, delaytime); 
    LightUp(B11111111, B11111111, B11111111, B11111111, B10000000, B00000000, delaytime); 
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111110, B00000000, delaytime); 
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11110000, delaytime); 
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111100, delaytime); 
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, delaytime);
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111100, delaytime);
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11110000, delaytime); 
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111110, B00000000, delaytime);
    LightUp(B11111111, B11111111, B11111111, B11111111, B10000000, B00000000, delaytime);
    LightUp(B11111111, B11111111, B11111111, B11000000, B00000000, B00000000, delaytime);
    LightUp(B11111111, B11111111, B11110000, B00000000, B00000000, B00000000, delaytime);
    LightUp(B11111111, B11111000, B00000000, B00000000, B00000000, B00000000, delaytime);
    LightUp(B11111110, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime);
    LightUp(B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, delaytime);   
    }
}
