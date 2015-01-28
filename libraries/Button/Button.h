#include <WProgram.h>

/*
 * Button - Arduino driver for using buttons
 *
 */

#ifndef Button_h
#define Button_h

class Button
{
  private:
    boolean butState;     // current button state
    int butPin;           // Arduino pin 
    long butTime;         // How long since button was pushed
    long butMaxTime;      // Time until registering next push (debounce)
  
  public:
    Button(boolean state, int pin, long time, long maxTime);
    boolean isOn();
    boolean isPushed();
};

#endif
