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
int buttonstate;
int buttonprev = LOW;
long time = 0;         // the last time the button was pushed
long debounce = 200;   // the debounce time, increase if the output flickers

byte dato1 = 0;
byte dato2 = 0;
byte dato3 = 0;

byte dato4 = 0;
byte dato5 = 0;
byte dato6 = 0;

long lights = 0;  // current state of lights
int numlights = 48;  // number of eyeballs

int currshow = 0;
int totalshows = 2;

void setup()
{
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
    delayMicroseconds(50);
    digitalWrite(clock, LOW);
}

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
  
void LightUp(byte dato1, byte dato2, byte dato3, byte dato4, byte dato5, byte dato6)
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
}

void loop()
{
    buttonstate = digitalRead(button);
    if (buttonstate == HIGH && buttonprev == LOW && millis() - time > debounce) {
      if (buttonstate = LOW) buttonstate = LOW; else buttonstate = HIGH;             // reset the button
      currshow++;
      if (currshow == totalshows) currshow = 0;
      // ... and remember when the last button press was
      time = millis();    
    }
    buttonprev = buttonstate;
/*
 * Shows!
 */

/* 
if (currshow == 1) {  // Shift one light from #0 to #end
    long lights = 1;  // Turn all lights off but 1
    
    for (count = 0; count < numlights; count++) {
      LightUpLong(lights); 
      lights<<=1; 
      delay(1000);
    }
}
*/

if (currshow == 0) {
    LightUp(B11111111, B11111111, B11111111, B11111111, B11111111, B11111111); 
    delay(1000);
    LightUp(B01010101, B01010101, B01010101, B01010101, B01010101, B01010101);
    delay(1000);
    }
}
