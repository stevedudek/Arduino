/* Scarf Driver
 * --------------
 * Lights 12 LEDs in different patterns via a Lilypad microcontroller
*/

int button = 13;        // Button input

int i = 0;
int count = 0;
int ShowNum = 0;
int MaxShow = 7;

boolean ButtonState;
boolean ButtonPrev = LOW;
long time = 0;         // the last time the button was pushed
long debounce = 50;   // the debounce time, increase if the output flickers

byte dato1 = 0;
byte dato2 = 0;

int DefaultDelay = 200;  // default delay time <- controls speed of movement

int numlights = 12;      // number of lights

void setup() {
  for (i=1; i<(numlights+1); i++) pinMode(i, OUTPUT);   // set up all the lights
  pinMode(button, INPUT);
  
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
    digitalWrite(count+1, !(dato2 & 01));  // NAND the rightmost bit with a 1
    dato2>>=1;
   }
   
   for (count = 0; count < 4; count++) {
    digitalWrite(count+9, !(dato1 & 01));  // NAND the rightmost bit with a 1
    dato1>>=1;
   }
 
   delay(delaytime);  // Hold light pattern for this many microseconds
}

void loop()
{

/*
 * Show - All on
 */
 if (ShowNum == 1 or ShowNum == 0) {
    LightUp(B1111, B11111111, 0); if (ButtonPushed()) break;
 }

/*
 * Show - All off
 */
 if (ShowNum == 2 or ShowNum == 0) {
  LightUp(B0000, B00000000, 0); if (ButtonPushed()) break;
 }

/*
 * Show - one scrolling light back and forth
 */
 if (ShowNum == 3 or ShowNum == 0) {
  
  DefaultDelay = 100; 
    
  LightUp(B0000, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1000, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0100, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0010, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0001, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B10000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B01000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00100000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00010000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00001000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00000100, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00000010, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00000001, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00000010, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00000100, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00001000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00010000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00100000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B01000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B10000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0001, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0010, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0100, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1000, B00000000, DefaultDelay); if (ButtonPushed()) break; 
}

/*
 * Show - lights fill up; empty down
 */
while (HIGH) {

  DefaultDelay = 50; 
  
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
while (HIGH) {

  DefaultDelay = 200; 
    
  LightUp(B0101, B01010101, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1010, B10101010, DefaultDelay); if (ButtonPushed()) break;
}

/*
 * Show - Push out from the middle
 */
while (HIGH) {

  DefaultDelay = 200; 
    
  LightUp(B0000, B01100000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B10010000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0001, B00001000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0010, B00000100, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0100, B00000010, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1000, B00000001, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00000000, DefaultDelay); if (ButtonPushed()) break;
  
  delay(1000);  
}

/*
 * Show - Random On - Flash - Random Off
 */
while (HIGH) {

  DefaultDelay = 1000; 
    
  LightUp(B0000, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0100, B00000000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0100, B00000010, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0100, B01000010, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0110, B01000010, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0110, B01010010, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0110, B01010011, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0110, B11010011, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0110, B11011011, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1110, B11011011, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1110, B11111011, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1110, B11111111, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11111111, DefaultDelay); if (ButtonPushed()) break;
  
  LightUp(B0000, B00000000, DefaultDelay); if (ButtonPushed()) break;
  
  LightUp(B1111, B11111111, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1111, B11111101, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1110, B11111101, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1110, B11101101, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1010, B11101101, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1010, B01101101, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B1010, B01101100, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0010, B01101100, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0010, B01101000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0010, B00101000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00101000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00001000, DefaultDelay); if (ButtonPushed()) break;
  LightUp(B0000, B00000000, DefaultDelay); if (ButtonPushed()) break;  
}

// Start shows at beginning
}
