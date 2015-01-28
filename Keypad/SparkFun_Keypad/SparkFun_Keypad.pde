/* SparkFun Keypad Example Code
  by: Jim Lindblom
  SparkFun Electronics
  date: 4/26/11
  license: Creative Commons Share-Alike attribution v3.0

  This example code will quickly get you up and running with the 
  SparkFun U/D/L/R/Flame Keypad. Pressing the flame button will
  turn on/off the red LED. Up and down adjust the brightness of
  the LED. And left and right are just there to look pretty will
  output their status via the serial monitor.
  
  Multiple touches are not supported in this code.  
  
  The circuit:
  SparkFun Keypad                Arduino
  -----------------------------------------
    Wire 1 (NC) ------------- No connection
   Wire 2 (LEDR) ----------------- D3
    Wire 3 (GND) ------------------GND
   Wire 4 (P5.1) ----------------- D5
   Wire 5 (P5.3) ----------------- D6
   Wire 6 (P5.2) ----------------- D7
*/

// Pin definitions
int LEDpin = 3;
int p51 = 5;
int p53 = 6;
int p52 = 7;

// Global variables
int button = 0;
int ledLevel = 127;
int onstatus = 1;

void setup()
{
  // Initially set up the pins
  pinMode(LEDpin, OUTPUT);
  pinMode(p51, INPUT);
  pinMode(p53, INPUT);
  pinMode(p52, INPUT);
  digitalWrite(p51, HIGH);
  digitalWrite(p52, HIGH);
  digitalWrite(p53, HIGH);
  analogWrite(LEDpin, ledLevel);
  
  Serial.begin(9600);
}

void loop()
{
  button = getButtonState();  // Get button status
  if (button == 0x04)  // FLAME
  {
    onstatus = onstatus ^ 1;  // flip on/off status of LED
    delay(1);  // "debounce"
    while (getButtonState() == 0x04)
      ;  // Wait for them to stop pressing flame
  }
  else if (button == 0x02)  // UP
  {
    ledLevel++;  // Turn LED level up
    delay(3);  // delay to make the effect visible
  }
  else if (button == 0x01)  // DOWN
  {
    ledLevel--;
    delay(3);  // delay to make the effect visible
  }
  else if (button == 0x08)  // RIGHT
  {
    Serial.println("right");
  }
  else if (button == 0x10)  // LEFT
  {
    Serial.println("left");
  }
    
  // Keep LED levels in range
  if (ledLevel >= 256)
    ledLevel = 255;
  if (ledLevel <= 0)
    ledLevel = 0;
  
  // To turn the LED on or not to turn the LED on, that's the question
  if (onstatus)
    analogWrite(LEDpin, ledLevel);
  else
    digitalWrite(LEDpin, 0);
}

/* getButtonState() will return a uint8_t representing the status
  of the SparkFun button pad. The meaning of the return value is:
  0x01: Down
  0x02: Up
  0x04: Flame
  0x08: Right
  0x10: Left
*/
uint8_t getButtonState()
{
  // Initially set all buttons as inputs, and pull them up
  pinMode(p52, INPUT);
  digitalWrite(p52, HIGH);
  pinMode(p51, INPUT);
  digitalWrite(p51, HIGH);
  pinMode(p53, INPUT);
  digitalWrite(p53, HIGH);
  
  // Read the d/u/flame buttons
  if (!digitalRead(p53))
    return 0x01;  // Down
  if (!digitalRead(p52))
    return 0x02;  // Up
  if (!digitalRead(p51))
    return 0x04;  // Flame
    
  // Read right button
  pinMode(p52, OUTPUT);  // set p52 to output, set low
  digitalWrite(p52, LOW);
  if (!digitalRead(p53))
    return 0x08;  // Right
  pinMode(p52, INPUT);  // set p52 back to input and pull-up
  digitalWrite(p52, HIGH);
  
  // Read left button
  pinMode(p51, OUTPUT);  // Set p51 to output and low
  digitalWrite(p51, LOW);
  if (!digitalRead(p53))
    return 0x10;  // Left
  pinMode(p51, INPUT);  // Set p51 back to input and pull-up
  pinMode(p51, HIGH);
  
  return 0;
}
