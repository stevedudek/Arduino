/*
 * This is a program for the SparkFun EL Sequencer that demonstrates how to light
 * up each EL segment A-H for a specified time.
 *
 * by Diana Eng for Makezine.com - "Programming EL Wire Fashion"
 * http://blog.makezine.com/archive/2010/04/programming_el_wire_fashion.html
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// turn a given EL wire segment on or off. 'num' is between 0 and 7, corresponding
// to EL segments 'A' through 'H'.  if 'value' is true, the segment will be lit.
// if value is false, the segment will be dark.
void elSegment(byte num, boolean value) {
  digitalWrite(num + 2, value ? HIGH : LOW);
}

void setup() {
  // set up the EL wire segment pins
  byte i;
  for (i = 2; i < 10; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
}

// control how long each segment is turned on by changing the delay
// delete the code that is not being used EL wire segments 
void loop() {
  elSegment(0, true);
  delay(200);
  elSegment(0, false);
  
  elSegment(1, true);
  delay(200);
  elSegment(1, false);
  
/*  
  elSegment(2, true);
  delay(200);
  elSegment(2, false);
  
  elSegment(3, true);
  delay(200);
  elSegment(3, false);
  
  elSegment(4, true);
  delay(200);
  elSegment(4, false);
  
  elSegment(5, true);
  delay(200);
  elSegment(5, false);
  
  elSegment(6, true);
  delay(200);
  elSegment(6, false);
  
  elSegment(7, true);
  delay(200);
  elSegment(7, false);
*/

}
