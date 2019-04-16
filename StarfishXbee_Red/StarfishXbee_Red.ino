//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

//  8/5/14
//
//  Starfish
//
//  5-armed starfish with 6 lights per arm = 30 lights
//  Big addition to this program is 2D lighting [ARM][LIGHT]
//
//  Modern 8-bit color lights with HSV morphing
//
//  Morphing is HSV-corrected (thanks, Greg)
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights.
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numArms 5       // 5-armed starfish
#define armLen  6       // 6 lights per arm
#define numLights 30    // Number of spike lights
#define Solid_ID    5        // 1 = tet, 2 = cube, 3 = oct, 4 = dodec, 5 = icosa

// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// For random-fill show (nasty global)
byte shuffle[numLights];  // Will contain a shuffle of lights

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Light colors

int foreColor =  0;    // Starting foreground color
int backColor = 500;   // Starting background color
float brightness = 1;  // Brightness of the overall piece
#define MaxColor 1530   // Colors are 6 * 255

// Shows

int show = 0;       // Starting show
#define MAX_SHOW 17  // Total number of shows

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated
boolean active = true;  // flag tells whether lights should be off

#define MAX_CLOCK   6000      // Total length of the story
#define UPDATE_FREQ 1000      // How often the master sends out data in ms

// Delays
int wait = 6;
#define MAX_WAIT 12 // Number of stored delay times 

// Timing variables for xBee communication

unsigned long OldTime;
unsigned long NewTime;
boolean ALONE = true; // Flag for if controller has sent a message

#define MAX_AWAY 20     // Slave goes into Alone mode if no signal in this #sec
#define NO_DATA 9999

// xBee language
#define COMMAND_PROMPT     '#'
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_BRIGHTNESS 'R'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define NO_CHAR            'X'
#define MAX_MESSAGE        30
#define MAX_NUM             5

//Depending on the unit, we need to run slightly different code
boolean I_AM_MASTER = false; // MUST BE FALSE - HAVE NOT PROGRAMMED SPEAKING
boolean Alone = true; // Flag for slave; turns false if master sends information

// Lookup table for position of lights on the arms
// startable[arm number][position on arm]
// for position: 0 = center, 5 = tail

int startable[5][6] = {
  { 5, 4, 3, 2, 1, 0},
  { 6, 7, 8, 9,10,11},
  {17,16,15,14,13,12},
  {18,19,20,21,22,23},
  {24,25,26,27,28,29}
};

//
// Setup
//

void setup() {
  
  randomSeed(analogRead(0));
  
  // xBee communication
  
  OldTime = 0;
  NewTime = 0;
  
  Serial.begin(9600);
  //Serial.println("Start");
    
  // Start up the LED counter
  strip.begin();
  
  // Initialize the strip, to start they are all 'off'
  
  for (int i=0; i<numLights; i++) {
    current_frame[i] = Color(0,0,0);        // Black
    next_frame[i] = Color(0,0,0);           // Black
    strip.setPixelColor(i, Color(0,0,0));
  }
  strip.show();
}

void loop() { 
   
  delay(20);   // The only delay!
  
  // xBee Communication
  
  if (I_AM_MASTER) {     // Master
    SpeakSignal();
  } else {  
    if (HearSignal()) {  // Slave
      update = true;
    }
  } 
 
  // Check if the lights need updating
  
  if (update) {
    
    update = false;
    
    switch(show) {
    
      case 0:    
        allOn();
        break;
      case 1:
        twocolor();
        break;
      case 2:
        randomfill();
        break;
      case 3:
        randomcolors();
        break;
      case 4:
        morphArm();
        break;
      case 5:
        lightarmwave();
        break;
      case 6:
        lightarmcolorwave();
        break;
      case 7:
        lightarmrunup();
        break;
      case 8:
        lightarmcolorrunup();
        break;
      case 9:
        armspin();
        break;
      case 10:
        armspinwave();
        break;
      case 11:
        armspinrunup();
        break;
      case 12:
        armsawtooth();
        break;
      case 13:
        randomarmcolors();
        break;
      case 14:
        randomradialcolors();
        break;
      case 15:
        movingarmrunup();
        break;
      case 16:
        clear();
        break;
    }
  }
  
  // Morph the display
  
  morph_frame();
  
  // Advance the clock
   
  if (morph++ >= GetDelayTime(wait)) {  // Finished morphing
    
    update = true;  // Force an update
    
    morph = 0;  // Reset morphClock
    
    // Advance the cycle clock
    
    if(cycle++ > 1000) cycle = 0;  
    
    // Update the frame display
  
    for (int i = 0; i < numLights; i++) {
      current_frame[i] = next_frame[i];
    }
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(10)) {
    foreColor = (foreColor + 5) % MaxColor;
    update = true;
  }
  
  if (!random(10)) {
    backColor -= 10;
    if (backColor < 0) backColor += MaxColor;
    update = true;
  }
  
  if (!random(10000)) {  // Change the show
      
    morph = 0;
    cycle = 0;
    clearWithFade();
  
    if (show == 16) {
      show = random(MAX_SHOW - 1);
    } else {
      show = 16;
    }
  }
  
  if (!random(1000)) {
    wait++;
    if (wait >= MAX_WAIT) {
      wait = 0;
      morph = 0;
    }
  }
  
}

//
// SpeakSignal
//
// For Master controller
// Send out signals
//
void SpeakSignal() {
}

// clear
//
// set all cells to black but don't call show yet
// ignores buffering
// 
void clear() {
  for (int i=0; i<numLights; i++) {
    strip.setPixelColor(i, Color(0,0,0));
    setPixelColor(i, Color(0,0,0));
  }
}

// clear with fade
//
// Fades lights to black
//
void clearWithFade() {
  for (int i=0; i<numLights; i++) {
    setPixelColor(i, Color(0,0,0));
  }
}

//
// All On
//
// Simply turns all the pixels on to one color
// 
void allOn() {
   for (int i=0; i < strip.numPixels(); i++) {
     setPixelColor(i, Wheel(foreColor));
   }
}

// random fill
//
// randomfill: randomly fills in pixels from blank to all on
// then takes them away random until blank
//
 
void randomfill() {
  int i, j, save, pos;
  
  pos = cycle % (numLights*2);  // Where we are in the show
  
  if (pos == 0) {  // Start of show
  
    // Shuffle sort to determine order to turn on lights
    for (i=0; i < numLights; i++) shuffle[i] = i; // before shuffle
    for (i=0; i < numLights; i++) {  // here's position
      j = random(numLights);         // there's position
      save = shuffle[i];
      shuffle[i] = shuffle[j];       // first swap
      shuffle[j] = save;             // second swap
    }
  }
  
  for (i=0; i < numLights; i++) {
    if (i < pos) {  
      setPixelColor(shuffle[i], Wheel(foreColor));  // Turning on lights one at a time
    } else { 
      setPixelColor(shuffle[numLights-(i % numLights)-1], Color(0,0,0));  // Turning off lights one at a time
    }
  }
}  

// random colors
//
// randomcolors: turns each pixel on to a random color
//
void randomcolors() {
  int i;
  
  if (cycle == 0) {  // Start of show: assign lights to random colors
    for (i=0; i < numLights; i++) shuffle[i] = random(MaxColor);
  }
  
  // Otherwise, fill lights with their color
  for (i=0; i < numLights; i++) {
    setPixelColor(i, Wheel(shuffle[i]));
  }
}

// randomarmcolors
//
// randomarmcolors: turns each arm on to a random color
//
void randomarmcolors() {
  int i;
  int randomcolor;
  
  if (cycle == 0) {  // Start of show: assign each arm to a random colors
    for (i=0; i < numArms; i++) {
      randomcolor = random(MaxColor);
      for (int j=0; j < armLen; j++) {
        shuffle[startable[i][j]] = randomcolor;
      }
    }
  }
  
  // Otherwise, fill lights with their color
  for (i=0; i < numLights; i++) {
    setPixelColor(i, Wheel(shuffle[i]));
  }
}

// randomradialcolors
//
// randomradialcolors: turns each radius on to a random color
//
void randomradialcolors() {
  int i;
  int randomcolor;
  
  if (cycle == 0) {  // Start of show: assign each arm to a random colors
    for (int j=0; j < armLen; j++) {
      randomcolor = random(MaxColor);
      for (i=0; i < numArms; i++) {
        shuffle[startable[i][j]] = randomcolor;
      }
    }
  }
  
  // Otherwise, fill lights with their color
  for (i=0; i < numLights; i++) {
    setPixelColor(i, Wheel(shuffle[i]));
  }
}

// two color
//
// alternates the color of pixels between two colors
//
void twocolor() {
  for (int i=0; i < numLights; i++) {
    if (i%2) setPixelColor(i, Wheel(foreColor));
    else setPixelColor(i, Wheel(backColor));
  }
}

//
// Morph Chain
//
// Morphs color 1 from position x to
// color 2 at position x+n

void morphChain() {
  float attenuation;
  
  for (int i=0; i < numLights; i++) {
    attenuation = ((i+(cycle%numLights)) % numLights)/(float)(numLights-1);
    setPixelColor(numLights-i-1, HSVinter24(Wheel(foreColor),Wheel(backColor), attenuation));
  }
}

//
// Morph Arm
//
// Morphs color 1 from position x to
// color 2 at position x+n
// but only along an arm
// All arms react symmetrically

void morphArm() {
  float attenuation;
  
  for (int i=0; i < armLen; i++) {
    attenuation = ((i+(cycle%armLen)) % armLen)/(float)(armLen-1);
    setRadialColor(armLen-i-1, HSVinter24(Wheel(foreColor),Wheel(backColor), attenuation));
  }
}

//
// Arm Saw tooth
//
// Fills in pixels with a sawtooth of intensity
//
// Peak of the sawtooth moves with the cycle
//

void armsawtooth() {
  float attenuation;
  
  for (int i=0; i < armLen; i++) {
    attenuation = 2*(((i+(cycle%armLen)) % armLen)/(float)(armLen-1));
    if (attenuation > 1) attenuation = 2 - attenuation;  // the '2 -' creates the sawtooth
    attenuation = attenuation * attenuation;  // magnify effect - color brightness is not linear
    // "i" will have pattern move up; "numLights-i-1'' will have pattern move down
    setRadialColor(armLen-i-1, HSVinter24(Color(0,0,0), Wheel(foreColor), attenuation));
  }
}

//
// rainbowshow: fills the spines with a gradient rainbow
// over time, the starting color changes
//
 
void rainbowshow(int cycles) {
  int diff = abs(foreColor - backColor);
  int j;
  
  for (int i=0; i < numLights; i++) {
    j = (i+(cycle%numLights)) % numLights;
    setPixelColor(numLights-j-1, Wheel(((i*diff / numLights)+foreColor) % diff));
  }
}

//
// lightarmwave
//
// Just one pixel traveling from center to edge of ALL arms
 
void lightarmwave() {
 
  for (int i=0; i < armLen; i++) {
     if (i == cycle % armLen) setRadialColor(i, Wheel(foreColor));
     else setRadialColor(i, Color(0,0,0));
  }
}

//
// armspinwave
//
// Just one pixel traveling from center to edge 
// Pixel gets shifted around all the arms
 
void armspinwave() {
 
  for (int j=0; j < numArms; j++) {
    for (int i=0; i < armLen; i++) {
       if (i == (cycle+j) % armLen) setPixelColor(startable[j][i], Wheel(foreColor));
       else setPixelColor(startable[j][i], Color(0,0,0));
    }
  }
}

//
// lightarmcolorwave
//
// Just one pixel traveling from center to edge of ALL arms
// Pixels not illuminated get backColor
// To be different, pixels go from edge to center
 
void lightarmcolorwave() {
 
  for (int i=0; i < armLen; i++) {
     if (i == cycle % armLen) setRadialColor(armLen-i-1, Wheel(foreColor));
     else setRadialColor(armLen-i-1, Wheel(backColor));
  }
}

//
// lightarmrunup
//
// lights fill up one at time
// symmetric for ALL arms
// lights go from center to edge
// 
void lightarmrunup() {
  int pos = cycle % ((armLen+1)*2);  // Where we are in the show
  if (pos >= (armLen+1)) pos = ((armLen+1)*2) - pos - 1;
  
  for (int i=0; i < armLen; i++) {
    if (i < pos) {
      setRadialColor(i, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setRadialColor(i, Color(0,0,0));   // black
    }
  }
}

//
// movingarmrunup
//
// lights fill up one at time
// but only on one arm - pick a random arm
// lights go from center to edge
// 
void movingarmrunup() {
  int pos = cycle % ((armLen+1)*2);  // Where we are in the show
  if (pos >= (armLen+1)) pos = ((armLen+1)*2) - pos - 1;
  if (pos == 0) shuffle[0] = shuffle[0] + 2; // move to opposite arm
  
  for (int j=0; j < numArms; j++) {
    for (int i=0; i < armLen; i++) {
      if (i < pos && j == (shuffle[0] % numArms)) {
        setPixelColor(startable[j][i], Wheel(foreColor));  // Turning on lights one at a time
      } else {
        setPixelColor(startable[j][i], Color(0,0,0));   // black
      }
    }
  }
}
//
// armspinrunup
//
// lights fill up one at time
// but also move differently for each arm
// lights go from center to edge
// 
void armspinrunup() {
  for (int j=0; j < numArms; j++) {
    for (int i=0; i < armLen; i++) {
       if (i < (cycle+j) % (armLen+1)) setPixelColor(startable[j][i], Wheel(foreColor));
       else setPixelColor(startable[j][i], Color(0,0,0));
    }
  }
}

//
// lightarmcolorrunup
//
// lights fill up one at time. Lights not on are set to backColor
// symmetric for ALL arms
// lights go from edge to center (to be different)
// 
void lightarmcolorrunup() {
  int pos = cycle % ((armLen+1)*2);  // Where we are in the show
  if (pos >= (armLen+1)) pos = ((armLen+1)*2) - pos - 1;
  
  for (int i=0; i < armLen; i++) {
    if (i < pos) {
      setRadialColor(armLen-i-1, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setRadialColor(armLen-i-1, Wheel(backColor));   // Background color
    }
  }
}

//
// armspin
//
// Turns on one arm to foreColor
// Turns off all the other arms
//

void armspin() {
  for (int i=0; i < numArms; i++) {
    if (i == cycle % numArms) {
      setArmColor(i, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setArmColor(i, Color(0,0,0));   // Black
    }
  }
}
  
//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code initially from Greg and Robie
//

void morph_frame() {
   for (int i = 0; i < numLights; i++) {
     strip.setPixelColor(i, HSVinter24(current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait)));
   }
   strip.show();  // Update the display 
}

void setPixelColor(int pos, uint32_t color) {
  next_frame[pos] = color;
}

void setRadialColor(int radius, uint32_t color) {
  for (int i = 0; i < numArms; i++) {
    next_frame[startable[i][radius % armLen]] = color;
  }
}

void setArmColor(int arm, uint32_t color) {
  for (int i = 0; i < armLen; i++) {
    next_frame[startable[arm % numArms][i]] = color;
  }
}

//
// Get Delay Time
//
// Returns a delay time from an array
//
 
int GetDelayTime(int wait) {
  int DelayValues[MAX_WAIT] = { 2, 3, 4, 6, 8, 10, 15, 20, 30, 50, 75, 100 };
  return (DelayValues[wait % MAX_WAIT]);
}

//
// SLAVE HEARING
//
//
// Hear Signal
//
// For slaves, listen to the Serial port and decipher incoming signals
// Return true is a signal is heard. False otherwise.
// Flip the Alone flag if no signal for awhile

boolean HearSignal() {
  
  NewTime = millis();  // Take a time point
  
  if (!Serial.available()) {
    // No received signal. Check how long since last signal

    if((NewTime - OldTime) / 1000 > MAX_AWAY) {
      OldTime = NewTime;    // Reset the clock
      Alone = true;         // Lost contact with the Master
      active = true;        // Turn back on
      return (true);        // Force an update
      //Serial.println("Feeling very alone");
    }
    return(false);  // no update required yet
  
  } else {  // Heard a signal!
      
      char incoming = Serial.read();
      
      if (incoming != '#') {
        //Serial.print("Command not starting with # prompt: ");
        //Serial.println(incoming);
        return(false);  // Not a '#' signal prompt - Usually an empty
      
      } else {  // Heard the '#' prompt from the master
        OldTime = NewTime;    // Reset the clock
        Alone = false;        // We're in contact! 
        
        char message[MAX_MESSAGE];
        for (int i = 0; i < MAX_MESSAGE; i++) message[i] = 0; // Clear message
  
        GetMessage(message);  // Load by pointer
        //Serial.println(message);
        return (ProcessMessage(message));
      }
  }
}

//
// GetMessage
//
// Pulls the whole serial buffer from just after the command prompt
// to the period
//
// Returns the string that drops the command prompt but keeps the period
//

void GetMessage(char* message) {
  int i = 0;
  
  while (i < MAX_MESSAGE) {
    char tmp = HearChar();
    //Serial.print(tmp);
    message[i] = tmp;
    if (tmp == COMMAND_PERIOD || tmp == NO_CHAR) return;  // End of line
    i++;
  }
  //Serial.println("Message too long");  // Ran out of message space
}

//
// HearChar
//
// Listens for a single character on the serial port
// Return NO_CHAR if time runs out

char HearChar() {
  
  #define TIMEOUT_TRIES 100
  
  int waitTime = 0;  // Start counter
  
  while (waitTime < TIMEOUT_TRIES) {
   
    if (Serial.available()) {
      return (Serial.read());  // Got something off the serial port;
    } else {
      delay(1);    // Keep waiting
      waitTime++; 
    }
  } 
  //Serial.println("Waiting too long");
  return (NO_CHAR);  // ran out of time: bail and return NO_CHAR
}

//
// ProcessMessage
//
// Process an incoming message. First character is solid number:
//
// 0 = all
// 1,2,3,4,5 = particular solid

boolean ProcessMessage(char* message) {
  byte solid = message[0] - '0';  // Which solid = first character
  
  if (solid == 0 || solid == Solid_ID) {  // We should be active!
    active = true;  // Turn lights back on
    return (ProcessOrder(message));  // Figure out the next command(s)
  } else {  // Command not for me
    clear();  // Turn off all lights
    active = false; // Keep lights off
    return (false);
  }
}

//
// ProcessOrder
//
// Order is for the particular solid
// Translate and execute the order
//

boolean ProcessOrder(char* message) {
  int i = 2;  // 0 = solid#, 1 = comma, 2 = first message
  boolean change = false;  // flag for updates
  
  while (i < MAX_MESSAGE) {
    char command = message[i];
    if (command == COMMAND_PERIOD || command == NO_CHAR) return (change);  // end of message
    else {
      char number[MAX_NUM];
      for (int j = 0; j < MAX_NUM; j++) number[j] = 0;  // Clear number buffer
      
      int numsiz = HearNum(message, i+1, number);  // Read in the number
      i = i + 2 + numsiz; // 2 = leap beginning command and trailing comma
      if (ExecuteOrder(command, atoi(number))) change = true;
    }
  }
  return (change);
}

//
// HearNum
//
// Reads numbers in a string
//

int HearNum(char* msg, int place, char* number) {
  int i = 0; // Start of number
  char tmp;
  
  while (i < MAX_NUM) {
    tmp = msg[place];
    if (tmp == COMMAND_COMMA || tmp == COMMAND_PERIOD || tmp == NO_CHAR) {
      return (i);
    } else {
      number[i] = msg[place];
      i++;
      place++;
      if (place >= MAX_MESSAGE) break;  
    }
  }
  //Serial.println("Number too long");
  return (NO_DATA); // Number too long
}

//
// ExecuteOrder
//
// Given a letter command followed by a number
// Execute the order
//

boolean ExecuteOrder(char command, int value) {
  Serial.print(command);
  Serial.print(value);
      
  switch (command) {
    
    case 'S':  // Show
      /*
      if (show != value) {  // Slave has different show
        show = value;
        return (true);
      } else {
         return (false);
      }
      */
    
    case 'W':  // Wait
      if (wait != value) {  // Slave has different wait
        wait = value;
      }
      return (false);
    
    case 'F':  // Foreground color
      if (foreColor != value) {  // Slave has different color
        foreColor = value;
        return (true);
      } else {
         return (false);
      }
         
     case 'B':  // Background color
       if (backColor != value) {  // Slave has different color
         backColor = value;
         return (true);
      } else {
         return (false);
      }
      
      case 'R':  // Brightness
        brightness = value / 255.0;
        return (false);
         
    default:
      return (false);
  }
}

/* Helper functions */

// Color
//
// Create a 24-bit color value from R,G,B
//
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c = 0;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

// Extracts the red part of a 24-bit color
byte GetRed(uint32_t c)
{
  return ((c >> 16) & 0xff);
}

// Extracts the green part of a 24-bit color
byte GetGreen(uint32_t c)
{
  return ((c >> 8) & 0xff);
}

// Extracts the green part of a 24-bit color
byte GetBlue(uint32_t c)
{
  return (c & 0xff);
}

// If r=g=b=0, return true
boolean IsBlack(byte r, byte g, byte b)
{
  if(r=g=b=0) return (true);
  else return (false);
}

//Input a value 256 * 6 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(int color)
{
  return Gradient_Wheel(color, 1);  // Intensity = 1
}

//Input a value 255 * 6 to get a color value.
//The colours are a transition r - g -b - back to r
//Intensity must be 0 <= intensity <= 1
uint32_t Gradient_Wheel(int color, float intensity)
{
  int r,g,b;
  int channel, value;
  
  color = color % MaxColor;  // Keep colors within bounds
  
  // Red party
  color = color * 2 / 3;  // Break color into 4 (not 6) channels
  
  channel = color / 255;
  value = color % 255;
  
  if (intensity > 1) intensity = 1;
  if (intensity < 0) intensity = 0;
  
  // Red party - These values are different

  switch(channel)
  {
    case 0:
      r = 255;
      g = value;
      b = 0;        
      break;
    case 1:
      r = 255;
      g = 255 - value;
      b = 0;        
      break;
    case 2:
      r = 255;
      g = 0;
      b = value;        
      break;
    default:
      r = 255;
      g = 0;
      b = 255 - value;        
      break;
   }

/*  This is the usual wheel
  switch(channel)
  {
    case 0:
      r = 255;
      g = value;
      b = 0;        
      break;
    case 1:
      r = 255 - value;
      g = 255;
      b = 0;        
      break;
    case 2:
      r = 0;
      g = 255;
      b = value;        
      break;
    case 3:
      r = 0;
      g = 255 - value;
      b = 255;        
      break;
    case 4:
      r = value;
      g = 0;
      b = 255;        
      break;
    default:
      r = 255;
      g = 0;
      b = 255 - value;        
      break; 
   }
  return(Color(r * intensity, g * intensity, b * intensity));
  */
  return(Color(r * intensity * brightness,
               g * intensity * brightness,
               b * intensity * brightness));
}
 

// r,g,b values are from 0 to 255
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
// 
// code from http://www.cs.rit.edu/~ncs/color/t_convert.html

void RGBtoHSV( byte red, byte green, byte blue, float *h, float *s, float *v )
{
  float r = red/float(255);
  float g = green/float(255);
  float b = blue/float(255);
  
  float MIN = min(r, min(g,b));  // min(r,g,b)
  float MAX = max(r, max(g,b));  // max(r,g,b)
 
  *v = MAX;		        // v

  float delta = MAX - MIN;

  if (MAX != 0 ) *s = delta / MAX;	// s
  else { // r = g = b = 0		// s = 0, v is undefined
    *s = 0;
    *h = -1;
    return;
  }
  if( r == MAX ) *h = 60.0 * ( g - b ) / delta; // between yellow & magenta
  else {
    if( g == MAX ) {
      *h = 120.0 + 60.0 * ( b - r ) / delta; // between cyan & yellow
    } else {
      *h = 240.0 + 60.0 * ( r - g ) / delta;	// between magenta & cyan
    }
  }
  if( *h < 0 ) *h += 360;
}

// r,g,b values are from 0 to 255
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
//
// code from http://www.cs.rit.edu/~ncs/color/t_convert.html

void HSVtoRGB( byte *r, byte *g, byte *b, float h, float s, float v )
{
  int i;
  float f, p, q, t;
  
  if( s == 0 ) {
    // achromatic (grey)
    *r = *g = *b = (v*255);
    return;
  }
  
  h /= 60;			// sector 0 to 5
  i = floor( h );
  f = h - i;			// factorial part of h
  p = v * ( 1 - s );
  q = v * ( 1 - s * f );
  t = v * ( 1 - s * ( 1 - f ) );
  
  switch( i ) {
    case 0:
      *r = v * 255;
      *g = t * 255;
      *b = p * 255;
      break;
    case 1:
      *r = q * 255;
      *g = v * 255;
      *b = p * 255;
      break;
    case 2:
      *r = p * 255;
      *g = v * 255;
      *b = t * 255;
      break;
    case 3:
      *r = p * 255;
      *g = q * 255;
      *b = v * 255;
      break;
    case 4:
      *r = t * 255;
      *g = p * 255;
      *b = v * 255;
      break;
    default:		// case 5:
      *r = v * 255;
      *g = p * 255;
      *b = q * 255;
      break;
    }
}

//
//  HSV Interpolate Wheel
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 0-255 wheel colors

uint32_t HSVinterWheel(byte c1, byte c2, float fract)
{
  return(HSVinter24(Wheel(c1),Wheel(c2),fract));
}

//
//  HSV Interpolate 24-bit
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 24-bit colors

uint32_t HSVinter24(uint32_t c1, uint32_t c2, float fract)
{
  return(HSVinterRGB(GetRed(c1),GetGreen(c1),GetBlue(c1), GetRed(c2),GetGreen(c2),GetBlue(c2), fract));
}

//  HSV Interpolate RGB
//
//  Given a start rgb, an end rgb, and a fractional distance (0-1)
//  This function converts start and end colors to hsv
//  and interpolates between the two points
//  The function returns the properly interpolated rgb
//  as a 24-bit rgb color
//  Whew.

uint32_t HSVinterRGB(byte r1, byte g1, byte b1, byte r2, byte g2, byte b2, float fract)
{  
  //Serial.print("r1 = ");
  //Serial.print(r1);
  //Serial.print(", g1 = ");
  //Serial.print(g1);
  //Serial.print(", b1 = ");
  //Serial.print(b1);
  //Serial.print("r2 = ");
  //Serial.print(r2);
  //Serial.print(", g2 = ");
  //Serial.print(g2);
  //Serial.print(", b2 = ");
  //Serial.println(b2);
  
  if (fract < 0 || fract > 1) {
     Serial.print("fract = ");
     Serial.println(fract);
     return(Color(255,0,0));  // Fract is out of 0-1 bounds. Color defaults to red
  }
  // Check to see if either 1 or 2 are black. If black, just attenuate the other color.
   
  if (r1+g1+b1 == 0) return(Color(r2*fract, g2*fract, b2*fract));
  if (r2+g2+b2 == 0) return(Color(r1*(1-fract), g1*(1-fract), b1*(1-fract)));
   
  // Set up HSV1 and HSV2 variables and pointers
  
  float h1,s1,v1,h2,s2,v2,hi,si,vi;
  float* p_h1;
  float* p_s1;
  float* p_v1;
  p_h1 = &h1;
  p_s1 = &s1;
  p_v1 = &v1;
  float* p_h2;
  float* p_s2;
  float* p_v2;
  p_h2 = &h2;
  p_s2 = &s2;
  p_v2 = &v2;
  
  // Calculate HSV1 and HSV2
  
  RGBtoHSV(r1,g1,b1,p_h1,p_s1,p_v1);
  RGBtoHSV(r2,g2,b2,p_h2,p_s2,p_v2);
  
  // Calculate the interpolated HSVi
  
  hi = interpolateWrap(h1,h2,fract);
  si = interpolate(s1,s2,fract);
  vi = interpolate(v1,v2,fract);
  
  // Convert back to rgb via pointers
  
  byte r,g,b;
  byte* p_r;
  byte* p_g;
  byte* p_b;
  p_r = &r;
  p_g = &g;
  p_b = &b;
  
  HSVtoRGB(p_r,p_g,p_b,hi,si,vi);
  
  // Print test bed
  /*
  Serial.print("r1 = ");
  Serial.print(r1);
  Serial.print(", g1 = ");
  Serial.print(g1);
  Serial.print(", b1 = ");
  Serial.print(b1);
  Serial.print(", r = ");
  Serial.print(r);
  Serial.print(", g = ");
  Serial.print(g);
  Serial.print(", b = ");
  Serial.println(b);
  */
  // Return the proper 24-bit color
   
  return(Color(r,g,b));
}

//
//  RGB Interpolate Wheel
//
//  Wrapper for RGB Interpolate RGB below
//  start and end colors are 0-255 wheel colors

uint32_t RGBinterWheel(byte c1, byte c2, float fract)
{
  return(RGBinter24(Wheel(c1),Wheel(c2),fract));
}

//
//  RGB Interpolate 24-bit
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 24-bit colors

uint32_t RGBinter24(uint32_t c1, uint32_t c2, float fract)
{
  return(RGBinterRGB(GetRed(c1),GetGreen(c1),GetBlue(c1), GetRed(c2),GetGreen(c2),GetBlue(c2), fract));
}

//  RGB Interpolate RGB
//
//  Given a start rgb, an end rgb, and a fractional distance (0-1)
//  This function interpolates between the two points
//  and returns the properly interpolated rgb
//  as a 24-bit rgb color

uint32_t RGBinterRGB(byte r1, byte g1, byte b1, byte r2, byte g2, byte b2, float fract)
{
  if (fract < 0) return(Color(r1,g1,b1));
  if (fract > 1) return(Color(r2,g2,b2));
  return(Color(interpolate(r1,r2,fract), interpolate(g1,g2,fract), interpolate(b1,b2,fract) ));
}
//
// Interpolate
//
// Returns the fractional point from a to b

float interpolate(float a, float b, float fract)
{
  return(a + (fract*(b-a)));
}

//
// Interpolate Wrap
//
// Returns the fractional point from a to b
// checking both ways around a circle

float interpolateWrap(float a, float b, float fract)
{
  if(should_wrap(a,b)) {
    if(a<b) a += 360;
    else b += 360;
  }
  float result = interpolate(a,b,fract);
  if (result>360) result -= 360;
  return(result);
}

//
// should_wrap
//
// determines whether the negative way
// around the circle is shorter

boolean should_wrap(float a, float b) {
  if (a>b) {
    float temp = a;  // basic a <-> b swap
    a = b;
    b = temp;
  }
  return(abs(b-a) > abs((a+360) - b));
}
