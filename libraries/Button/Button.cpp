#include "Button.h"

/*
 * Button - Arduino driver for using buttons
 *
 * Buttons have pull-up resistors so that open state is ++
 * and closed state is GND
 *
 */

Button::Button(boolean state, int pin, long time, long maxTime) {
  butState = state;
  butPin = pin;
  butTime = time;
  butMaxTime = maxTime;

  pinMode(butPin, INPUT);
}

boolean Button::isOn(void) {      // Returns current state of button
  return !digitalRead(butPin);    // Remember when pushed, signal is GND!
}

boolean Button::isPushed(void) { // Checks to see whether button was pushed
  boolean butRead;
  butRead = !digitalRead(butPin);  // Remember when pushed, signal is GND!
  
  if(!butRead) {       // Button is off
    butState = LOW;    // Record button in LOW state
    return false;    
   }

  else {                  // Button is on
    if(!butState || millis() - butTime > butMaxTime) { // Either off or has been on long enough
      butState = HIGH;    // Record button in HIGH state
      butTime = millis(); // Remember when button was pushed
      return true;       
    }
    else return false;    // Has not been long enough since last push
  }
}