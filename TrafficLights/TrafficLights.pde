/*
 Traffic light control
 
 Copyright 2010 SparkFun Electronics
 Nathan Seidle
 
 Red, green, and yellow LEDs are controlled via MOSFETs depending on what is heard over the serial stream.
 
 There is a master and a slave. If master sends R, slave goes to Red. If master sends Y, slave goes to Yellow, etc.

 If slave hears nothing for more than 10 seconds, than go to normal light mode.
 If either master or slave hears disco mode command then go to special disco mode.
 
 Stay on Green for 8 seconds
 Stay on Yellow for 2 seconds
 Stay on Red for 10 seconds
 
 If we get a disco button hit then:
 Master transmits #D$, then immediately goes Red, Yellow, Green with quick delays on each, then goes back to normal function.
 If Slave sees #D$ then go to disco mode then return to normal function.

 If slave doesn't hear anything for 14 seconds, do your own thing (wireless is lost, go to independent mode).
 */

int Red = 11;
int Yellow = 10;
int Green = 9;
int StatusLED = 13;

int DiscoButton = 12;
int LightLevelKnob = A1; //Analog 1

unsigned long OldTime;
unsigned long NewTime;
unsigned long TimeDiff;
int timeout;
int timeoutMax = 14; //If we don't hear anything for 14 seconds, then begin autonomous mode
char currentMode;

#define LIGHTPAUSE  50 //50ms delay to switch from one light to next
#define REDTIME  9 //Spend 9 seconds on Red
#define YELLOWTIME  2
#define SLAVEYELLOWTIME  2 //Spend 2 seconds on the remote yellow mode
#define GREENTIME  6

//Depending on the unit, we need to run slightly different code
int I_AM_MASTER = 1; //Set to 1 if this code is meant for a master unit
int I_AM_SLAVE = 0; //Set to 1 if this code is meant for a slave unit

void setup()  { 
  pinMode(Red, OUTPUT);
  pinMode(Yellow, OUTPUT);
  pinMode(Green, OUTPUT);

  //Turn off all lights
  digitalWrite(Red, LOW);
  digitalWrite(Yellow, LOW);
  digitalWrite(Green, LOW);

  digitalWrite(DiscoButton, HIGH); //Enable internal pull-up on this pin
  pinMode(DiscoButton, INPUT);
  pinMode(LightLevelKnob, INPUT);

  //This sets the pins on the ends of the trimpot to high/low
  pinMode(A0, OUTPUT);
  pinMode(A2, OUTPUT);

  digitalWrite(A0, LOW);
  digitalWrite(A2, HIGH);

  OldTime = 0;
  NewTime = 0;
  timeout = 0;

  Serial.begin(9600);

  //Light up green in a fancy way
  if(I_AM_MASTER){
    currentMode = 'G';
    pulse_light(0, Green);
  }
  else if (I_AM_SLAVE){
    currentMode = 'R';
    pulse_light(0, Red);
  }


  //Blink test
  /*while(1)
   {
   Serial.print("Hi!:");
   Serial.println(analogRead(LightLevelKnob));
   
   digitalWrite(13, HIGH);
   digitalWrite(Yellow, HIGH);
   //digitalWrite(Green, HIGH);
   //digitalWrite(Red, HIGH);
   delay(3000);
   digitalWrite(13, LOW);
   digitalWrite(Yellow, LOW);
   //digitalWrite(Green, LOW);
   //digitalWrite(Red, LOW);
   delay(3000);
   }*/

  //Pulse test
  /*while(1)
   {
   if(I_AM_MASTER)
   Serial.print("Master: ");
   if (I_AM_SLAVE)
   Serial.print("Slave: ");
   
   Serial.println(knob());
   
   pulse_light(Yellow, Yellow);
   //pulse_light(Green, Green);
   }*/

} 

void loop() { 

  char incoming1, incoming2, incoming3, incoming4;

  //Blink the status LED
  if(digitalRead(StatusLED) == 0) digitalWrite(StatusLED, HIGH);
  else digitalWrite(StatusLED, LOW);

  delay(50);

  //Master
  NewTime = millis();
  TimeDiff = NewTime - OldTime;
  TimeDiff /= 1000; //Convert to seconds

  if(TimeDiff > timeout) timeout = TimeDiff;

  //Debug info
  /*if(I_AM_MASTER)  Serial.print("Master:");
   if(I_AM_SLAVE)  Serial.print("Slave:");
   if(currentMode == 'R') Serial.print("R");
   if(currentMode == 'Y') Serial.print("Y");
   if(currentMode == 'G') Serial.print("G");
   
   Serial.print(" Time:");
   Serial.print(TimeDiff);
   
   Serial.print(" Out:");
   Serial.println(timeout);*/

  //Check to see if someone hit the walk button
  if(digitalRead(DiscoButton) == 0)
  {
    //Broadcast to the other costume
    if(I_AM_MASTER) Serial.println("#MD$"); //Tell slave to go to Disco mode
    if(I_AM_SLAVE) Serial.println("#SD$"); //Tell slave to go to Disco mode

    play_that_disco();
  }


  //Start deciding what light to lite up
  if(I_AM_MASTER){  
    if(currentMode == 'R' && TimeDiff >= REDTIME)
    {
      currentMode = 'S'; //Goto mode where slave is yellow
      OldTime = NewTime;
      //Don't change a light
      Serial.println("#MY$"); //Tell slave to go to Yellow
    }
    //While we are sitting on red, we need to tell slave to go to yellow after X seconds
    else if(currentMode == 'S' && TimeDiff >= SLAVEYELLOWTIME)
    {
      currentMode = 'G'; //Goto green
      OldTime = NewTime;
      Serial.println("#MR$"); //Tell slave to go to Red
      pulse_light(Red, Green); //Fade from one light to the next
    }
    else if(currentMode == 'G' && TimeDiff >= GREENTIME)
    {
      currentMode = 'Y'; //Goto yellow
      OldTime = NewTime;
      pulse_light(Green, Yellow);
      //Don't tell slave to anything (stay on red)
    }
    else if(currentMode == 'Y' && TimeDiff >= YELLOWTIME)
    {
      currentMode = 'R'; //Goto red
      OldTime = NewTime;
      Serial.println("#MG$"); //Tell slave to go to Green
      pulse_light(Yellow, Red);
    }
  }
  if(I_AM_SLAVE){ //Slave changes lights based on incoming commands, not the timer
    //We are waiting for the master to tell us the next command. Check here for timeout scenario
    if(timeout > timeoutMax){ //Uh oh, we haven't heard from the master in too long
      if(currentMode == 'R' && TimeDiff >= REDTIME)
      {
        currentMode = 'G'; //Goto green mode
        OldTime = NewTime;
        Serial.println("#Sg$"); //Notify partner costume of new mode
        pulse_light(Red, Green); //Fade from one light to the next
      }
      else if(currentMode == 'Y' && TimeDiff >= SLAVEYELLOWTIME)
      {
        currentMode = 'R'; //Goto red
        OldTime = NewTime;
        Serial.println("#Sr$"); //Notify partner costume
        pulse_light(Yellow, Red);
      }
      else if(currentMode == 'G' && TimeDiff >= GREENTIME)
      {
        currentMode = 'Y'; //Goto yellow
        OldTime = NewTime;
        Serial.println("#Sy$"); //Notify partner costume
        pulse_light(Green, Yellow);
      }
    }
  }

  //These are here so that we update the LEDs whenver we twist the knob. It's more interactive this way
  if(currentMode == 'R') analogWrite(Red, knob());
  if(currentMode == 'Y') analogWrite(Yellow, knob());
  if(currentMode == 'G') analogWrite(Green, knob());

  //What happens if we don't hear from the master? We need to timeout and do our own thing (don't stop the normal traffic light cycle)
  //When we do hear from the master, reset the time mode
  if(Serial.available())
  {
    incoming1 = Serial.read();
    if(incoming1 == '#') //Good!
    {
      int counter;
      incoming2 = 0;
      incoming3 = 0;
      incoming4 = 0;

#define WAITLIMIT  100

        for(counter = 0 ; counter < WAITLIMIT ; counter++)
      {
        delay(1);
        if(Serial.available()) break;//Wait for the next character
      }
      if(counter < WAITLIMIT) incoming2 = Serial.read(); 

      for(counter = 0 ; counter < WAITLIMIT ; counter++)
      {
        delay(1);
        if(Serial.available()) break;//Wait for the next character
      }
      if(counter < WAITLIMIT) incoming3 = Serial.read(); 

      for(counter = 0 ; counter < WAITLIMIT ; counter++)
      {
        delay(1);
        if(Serial.available()) break;//Wait for the next character
      }
      if(counter < WAITLIMIT) incoming4 = Serial.read(); 

      if(incoming4 =='$'){
        //We have a good response character
        if(I_AM_MASTER){
          //Do nothing but check for button/disco mode
          if(incoming3 == 'D') play_that_disco(); //Yay! Go to Disco mode!
          timeout = 0; //Reset the timeout mode
        }
        if(I_AM_SLAVE){
          if(incoming3 == 'D') play_that_disco(); //Yay! Go to Disco mode!

          if(incoming2 == 'M'){ //Then we have a command from master
            if(incoming3 == 'R'){ //Master says goto Red
              currentMode = 'R';
              OldTime = NewTime;
              Serial.println("#SR$"); //Notify partner costume of new mode
              pulse_light(Yellow, Red); //Fade from one light to the next
              timeout = 0; //Reset the timeout mode
            }
            if(incoming3 == 'Y'){ //Master says goto Yellow
              currentMode = 'Y';
              OldTime = NewTime;
              Serial.println("#SY$"); //Notify partner costume
              pulse_light(Green, Yellow);
              timeout = 0; //Reset the timeout mode
            }
            if(incoming3 == 'G'){ //Master says goto Green
              currentMode = 'G';
              OldTime = NewTime;
              Serial.println("#SG$"); //Notify partner costume
              pulse_light(Red, Green);
              timeout = 0; //Reset the timeout mode
            }
          }
        }
      }
    } //End incoming1 check
  }
} //End loop

//Reads the blue knob and returns a value 0 to 255
int knob()
{
  int trimValue = analogRead(LightLevelKnob); //Read the trimpot
  trimValue = map(trimValue, 0, 1024, 0, 255); //Adjust the max trimpot value to fadeValue which is 255
  return(trimValue);
}

//Slowly turns on light off, then delays, then slowly turns on the next light
void pulse_light(byte light_going_off, byte light_going_on)
{
  // fade out from max to min in increments of 5 points:
  for(int fadeValue = 255 ; fadeValue >= 0; fadeValue -=5) { 
    if(fadeValue < knob()) //Make sure we don't have a brightness above our knob setting
      analogWrite(light_going_off, fadeValue);
    else
      analogWrite(light_going_off, knob());

    delay(3); // wait for 30 milliseconds to see the dimming effect                            
  } 

  delay(LIGHTPAUSE); //Wait a bit between lights

  // fade in from min to max in increments of 5 points:
  for(int fadeValue = 0 ; fadeValue <= 255; fadeValue +=5) { 
    if(fadeValue < knob())  //Make sure we don't have a brightness above our knob setting
      analogWrite(light_going_on, fadeValue);
    else
      analogWrite(light_going_on, knob());

    delay(3); // wait for 30 milliseconds to see the dimming effect                            
  } 

}

void play_that_disco(void)
{
  digitalWrite(Green, LOW);
  digitalWrite(Yellow, LOW);
  digitalWrite(Red, LOW);

  delay(100);

  //Run down the three lights quickly, four times
  for(int x = 0 ; x < 4 ; x++)
  {
    digitalWrite(Red, LOW);
    digitalWrite(Green, HIGH);
    delay(120);
    digitalWrite(Green, LOW);
    digitalWrite(Yellow, HIGH);
    delay(120);
    digitalWrite(Yellow, LOW);
    digitalWrite(Red, HIGH);
    delay(120);
  }

  //Pulse all three lights at same time, three times
  for(int x = 0 ; x < 3 ; x++)
  {
    // fade in from min to max in increments of 5 points:
    for(int fadeValue = 0 ; fadeValue <= 255; fadeValue +=5) { 
      analogWrite(Red, fadeValue);
      analogWrite(Yellow, fadeValue);
      analogWrite(Green, fadeValue);
      delay(5); // wait for 30 milliseconds to see the dimming effect                            
    } 

    delay(LIGHTPAUSE); //Wait a bit between lights

    // fade out from max to min in increments of 5 points:
    for(int fadeValue = 255 ; fadeValue >= 0; fadeValue -=5) { 
      analogWrite(Red, fadeValue);
      analogWrite(Yellow, fadeValue);
      analogWrite(Green, fadeValue);

      delay(5); // wait for 30 milliseconds to see the dimming effect                            
    } 
  }

}
































