/* Blinking LED
 * ------------
 * http://arduino.cc/en/Tutorial/Blink
 *
 * turns on and off a light emitting diode(LED) connected to a digital  
 * pin, in intervals of 2 seconds. Ideally we use pin 13 on the Arduino 
 * board because it has a resistor attached to it, needing only an LED

 *
 * Created 1 June 2005
 * copyleft 2005 DojoDave <http://www.0j0.org>
 * http://arduino.berlios.de
 *
 * based on an orginal by H. Barragan for the Wiring i/o board
 */

#include "WProgram.h"
void setup();
void loop();
int ledPin = 13;                 // LED connected to digital pin 13

void setup()
{
  pinMode(ledPin, OUTPUT);      // sets the digital pin as output
}

void loop()
{
  digitalWrite(ledPin, HIGH);   // sets the LED on
  delay(3000);                  // waits for a second
  digitalWrite(ledPin, LOW);    // sets the LED off
  delay(3000);                  // waits for a second
}

int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

