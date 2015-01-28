/******************
 *
 *   Color Box - Battery-operated black box with
 *
 *   3 knobs and 3 buttons
 *
 *   and an Arduino and xBee
 *
 *   use controller to operate remotely light-up outfits
 *
 ******************/

// Digital pins for buttons 1, 2, 3
char buttonpins[3] = { 0,1,2 };

// Current and previous status of buttons
boolean buttoncurr[3];
boolean buttonprev[3];
long buttontime[3];

// Analog pins for dials 1, 2, 3
char dialpins[3] = { 0,1,2 };

// Current position of dials
int dialval[3];
#define MinDialChange 5 // Minimum change in a dial that will register. Prevents flickering

int costnum;        // Which costume is dial 0 set to
#define MaxCost 4   // 0=No control, 1-2-3-etc=specific costume, 9=All costumes 

int show = 0;       // Which show we are on
#define MaxShow  6  // Total number of shows

int pacing;
#define MinDelay 300  // Minimum delay
#define MaxDelay 3000 // Maximum delay

byte color;
#define MaxColor 255     // Number of color options

void setup() {
   for (int p=0; p<3; p++) {
     pinMode(buttonpins[p], INPUT);
     buttonprev[p] = false;
     buttoncurr[p] = digitalRead(buttonpins[p]);
     buttontime[p] = 0;
  
     dialval[p] = GetDial(p);
   }
   costnum = CalcCostume(dialval[0]);   
   //Serial.begin(9600);
}

void loop() {
  CheckBox();
}



//
// Check Box
//
// Goes through the 3 buttons and 3 dials and see whether they have changed position

void CheckBox() {
  
  // Start with the dials
  
  // Dial #0 - The costume dial
  if (abs(GetDial(0) - dialval[0]) > MinDialChange) { // Has dial #0 budged?
    dialval[0] = GetDial(0);  // Update dial
    int newCost = CalcCostume(dialval[0]);
    if (newCost != costnum) {
      costnum = newCost;
      // Make new costumes glow
    }
  }
  
  // Dial #1 - The color dial
  if (abs(GetDial(1) - dialval[1]) > MinDialChange) { // Has dial #1 budged?
    dialval[1] = GetDial(1);  // Update dial
    color = dialval[1];       // Set new color
    //Send out new color by xBee
  }
  
   // Dial #2 - The speed dial
  if (abs(GetDial(2) - dialval[2]) > MinDialChange) { // Has dial #2 budged?
    dialval[2] = GetDial(2);  // Update dial
    pacing = (dialval[2]*(MaxDelay-MinDelay)/255) + MinDelay;
    //Send out new pacing by xBee
  }
  
  // Look next at the buttons
  
  // Button #0 - Turns the particular outfit white
  if (digitalRead(buttonpins[0])) { // Button #0 is pressed. No debouncing here
    // Send out message to costume(s) to turn white
  }
 
  // Button #1 - The color picker
  if (ButtonPushed(1)) { // Button #1 is pressed
    // Pick a color 
    // Send out new color by xBee
  }

  // Button #2 - The show button
  if (ButtonPushed(2)) { // Button #2 is pressed
    show = show++ % MaxShow;
    // Send out new show
  }
}

//
// Get Dial
//
// Returns the position of the dial pot
// The dial input needs to be 0-2

int GetDial(char dial) {
  return(analogRead(dial));
}

// CalcCostume
//
// Divides the 0-255 costume dial into even portions of MaxCost+2
// Returns a key code: 0=No control, 1-2-3-etc=specific costume, 9=All costumes

int CalcCostume(int potvalue) {
  int partition = potvalue/(255/(MaxCost+2));
  if (partition > MaxCost) partition = 9;
  return (partition);
}

//
// ButtonPushed
//
// Returns true if the button has been pushed
// Includes a timer to prevent flickering

boolean ButtonPushed(char b) {  
  //return(false);
  
  boolean bstate = digitalRead(buttonpins[b]);
  
  if (bstate == HIGH && buttonprev[b] == LOW && millis() - buttontime[b] > 500) { // Has button been pushed?
      buttoncurr[b] = LOW;                                                             // Yes, reset the button
      buttonprev[b] == HIGH;
      buttontime[b] = millis();      // ... and remember when the last button press was    
      return true;          // Button pushed
  } else {                  // Button not pushed
      buttonprev[b] = bstate;
      return false;           
  }
}


