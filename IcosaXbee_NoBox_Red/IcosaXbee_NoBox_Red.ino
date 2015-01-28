/******************
 *
 *   Platonic Solids
 *
 *   12/7/2014
 *
 *   XBee Communication, but no control box
 *
 *   Colors: full, morphing, no HSV correction
 *
 ******************/

#define datapin   5 // DI
#define latchpin  6 // LI
#define enablepin 7 // EI
#define clockpin  8 // CI

#define NumLEDs    20        // Sides
#define NumNeigh    3        // Neighbors
#define Solid_ID    5        // 1 = tet, 2 = cube, 3 = oct, 4 = dodec, 5 = icosa
#define MAX_SOLID   5

// framebuffers
uint32_t current_frame[NumLEDs];
uint32_t next_frame[NumLEDs];

// For random-fill show (nasty global)
byte shuffle[NumLEDs];  // Will contain a shuffle of lights
byte pointLED;          // For opposites show

int LEDChannels[NumLEDs][3] = {0};    // 8 for each octobar. Not all channels are used!

int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated

#define MAX_CLOCK   6000      // Total length of the story
#define UPDATE_FREQ 1000      // How often the master sends out data in ms

// Delays
int wait = 6;
#define MAX_WAIT 12 // Number of stored delay times 

// Shows
int show = 0;       // Which show we are on
#define MAX_SHOW  6  // Total number of shows

// Colors
int foreColor = 1000;   // Foreground color
int backColor = 2000;   // Background color
#define MaxColor 6144   // Number of colors: 1024 * 6

// Timing variables for xBee communication
unsigned long OldTime;
unsigned long NewTime;

#define MAX_AWAY 20     // Slave goes into Alone mode if no signal in this #sec
#define NO_DATA 9999

// xBee language
#define COMMAND_PROMPT     '#'
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define NO_CHAR            'X'
#define MAX_MESSAGE        30
#define MAX_NUM             5

//Depending on the unit, we need to run slightly different code
boolean I_AM_MASTER = true; // true if master; false if slave
boolean Alone = true; // Flag for slave; turns false if master sends information

/*
// Tetrahedron

byte neighbors[NumLEDs][NumNeigh] = {
  // Remember to -1 to these values
  { 2,3,4 },  // 1
  { 4,3,1 },  // 2
  { 1,2,4 },  // 3
  { 3,1,2 },  // 4
};
*/

/*
// Cube

byte neighbors[NumLEDs][NumNeigh] = {
  // Remember to -1 to these values
  { 2,3,4,5 },  // 1
  { 3,4,6,1 },  // 2
  { 3,5,6,2 },  // 3
  { 5,6,1,2 },  // 4
  { 6,1,3,4 },  // 5
  { 2,3,4,5 },  // 6
};
*/

/*
// Octahedron

byte neighbors[NumLEDs][NumNeigh] = {
  // Remember to -1 to these values
  { 3,7,4 },  // 1
  { 4,8,3 },  // 2
  { 5,1,2 },  // 3
  { 6,2,1 },  // 4
  { 7,3,8 },  // 5
  { 8,4,7 },  // 6
  { 1,5,6 },  // 7
  { 2,6,5 }   // 8
};
*/
/*
//  Dodecahedron

byte neighbors[NumLEDs][NumNeigh] = {
  // Remember to -1 to these values
  { 2,4,6,5,10 },  // 1
  { 7,8,4,1,10 },  // 2
  { 12,11,6,4,8 },  // 3
  { 8,3,6,1,2 },  // 4
  { 1,6,11,9,10 },  // 5
  { 3,11,5,1,4 },  // 6
  { 9,12,8,2,10 },  // 7
  { 12,3,4,2,7 },  // 8
  { 5,11,12,7,10 },  // 9
  { 2,1,5,9,7 },  // 10
  { 9,5,6,3,12 },   // 11
  { 7,9,11,3,8 }   // 12
};
*/

// Icosahedron

byte neighbors[NumLEDs][NumNeigh] = {
  // Remember to -1 to these values
  { 7,19,13 },  // 1
  { 12,18,20 },  // 2
  { 17,16,19 },  // 3
  { 18,11,14 },  // 4
  { 18,15,13 },  // 5
  { 9,16,14 },  // 6
  { 15,17,1 },  // 7
  { 16,10,20 },  // 8
  { 6,11,19 },  // 9
  { 17,12,8 },  // 10
  { 9,4,13 },  // 11
  { 10,15,2 },  // 12
  { 11,5,1 },  // 13
  { 4,6,29 },  // 14
  { 5,12,7 },  // 15
  { 6,3,8 },  // 16
  { 10,3,7 },  // 17
  { 5,4,2 },  // 18
  { 3,9,1 },  // 19
  { 2,14,8 },  // 20
};


unsigned long SB_CommandPacket;
int SB_CommandMode;
int SB_BlueCommand;
int SB_RedCommand;
int SB_GreenCommand;

void setup() {
   pinMode(datapin, OUTPUT);
   pinMode(latchpin, OUTPUT);
   pinMode(enablepin, OUTPUT);
   pinMode(clockpin, OUTPUT);

   digitalWrite(latchpin, LOW);
   digitalWrite(enablepin, LOW);
      
   // Initialize the cones, to start they are all 'off'
  
   BlackCones();
   ShuffleLights();
  
   pointLED = random(NumLEDs);
  
   // xBee communication
  
   OldTime = 0;
   NewTime = 0;
  
   Serial.begin(9600);
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
        AllOn();
        break;
      case 1:
        twocolor();
        break;
      case 2:
        RandomFill();
        break;
      case 3:
        RandomColor();
        break;
      case 4:
        LightOpposites();
        break;
      case 5:
        Pulse();
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
    
    if (cycle++ > MAX_CLOCK) cycle = 0;
    
    // Update the frame display
  
    for (int i = 0; i < NumLEDs; i++) {
      current_frame[i] = next_frame[i];
    }
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (I_AM_MASTER || Alone) {
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
      show = random(MAX_SHOW);
     
      morph = 0;
      clearLights();
    }
    
    if (!random(1000)) {
      wait++;
      if (wait >= MAX_WAIT) {
        wait = 0;
        morph = 0;
      }
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
  byte chapter = cycle / (MAX_CLOCK / 2); // Hopefully 0 or 1

  
  switch(chapter) {
        
    case 0:    // 1st chapter - All solids do their own thing
      break;
      
    default:    // 2d chapter - All solids together
      NewTime = millis();  // Take a time point
      if (NewTime - OldTime > UPDATE_FREQ) {
        OldTime = NewTime;    // Reset the clock
        SendAllData(0);  // Send out all color + time data
      }
      break;
  }
}

//
// SendAllData
//
// For Master controller
// Dumps all color + time data into the serial buffer
//
void SendAllData(byte solid) {
  SendMessage(COMMAND_PROMPT, solid);
  SendMessage(COMMAND_FORE, foreColor);
  SendMessage(COMMAND_BACK, backColor);
  SendMessage(COMMAND_WAIT, wait);
  SendMessage(COMMAND_SHOW, show);
  Serial.print(COMMAND_PERIOD);
}

//
// Send Message
//
// Sends out the command (one letter)
// Sends out the value
// Sends out the end prompt ','

void SendMessage(char command, int value) {
  Serial.print(command);
  Serial.print(value);
  Serial.print(COMMAND_COMMA);
}

//
// All On - Boring! Turn all the lights on to color1
//

void AllOn() {
  for (int i=0; i < NumLEDs; i++) {
    LightCone(i, Wheel(foreColor));
  }
}

// two color
//
// lights half of the cones one color, the other half the other color
//
void twocolor() {
  
  if ((cycle % 100) == 0) ShuffleLights();
  
  for (int i=0; i < NumLEDs; i++) {
    if (shuffle[i] % 2) LightCone(i, Wheel(foreColor));
    else LightCone(i, Wheel(backColor));
  }
}

// RandomColor
//
// lights the cones with a random gradient rainbow
//
void RandomColor() {
  int currColor;
  
  if ((cycle % 100) == 0) ShuffleLights();
  
  for (int i=0; i < NumLEDs; i++) {
    currColor = (foreColor + (shuffle[i] * MaxColor / NumLEDs)) % MaxColor;
    LightCone(i, Wheel(currColor));
  }
}

//
// Unit Test
//
// Debugging for the lights
void Unit_Test() {
  for (int i = 0; i < NumLEDs; i++) {
    if (i == cycle % NumLEDs) { 
      if (i == 0) { LightCone(i, Color(255,0,0)); }
      else { LightCone(i, Color(0,0,255)); }
    } else {
      LightCone(i, Color(0,0,0));
    }
  }
}
//
// Pulse
//
void Pulse() {
  
  if ((cycle % 100) == 0) ShuffleLights();
  
  float intense = (sin( (cycle % 360) / (2 * 3.14159) ) + 1) / 2.0;
  
  for (int i=0; i < NumLEDs; i++) {
    if (shuffle[i] % 2) { LightCone(i, Gradient_Wheel(backColor, intense)); }
    else                { LightCone(i, Gradient_Wheel(backColor, 1.0 - intense)); }
  }
}

// Random Fill
//
// randomfill: randomly fills in pixels from blank to all on
// then takes them away random until blank
//
 
void RandomFill() {
  
  int pos = cycle % (NumLEDs*2);  // Where we are in the show
  
  if (pos == 0) ShuffleLights();
  
  if (pos < NumLEDs) {  
    LightCone(shuffle[pos], Wheel(foreColor));  // Turning on lights one at a time
  } else { 
    LightCone(shuffle[(NumLEDs*2)-pos-1], Color(0,0,0));  // Turning off lights one at a time
  }
}

//
// Shuffle Lights
//
// Shuffles the global shuffle[] array with the order of lights
// Shuffle sort to determine order to turn on lights

void ShuffleLights() {
  int i,j,save;
  
  for (i=0; i < NumLEDs; i++) shuffle[i] = i; // before shuffle
  for (i=0; i < NumLEDs; i++) {  // here's position
    j = random(NumLEDs);         // there's position
    save = shuffle[i];
    shuffle[i] = shuffle[j];       // first swap
    shuffle[j] = save;             // second swap
  }
}

//
// clear
// 
// set all lights to black
//
void clearLights() {
  for (int i=0; i<NumLEDs; i++) {
    LightCone(i, Color(0,0,0));    // Black
  }
}

//
// Light Opposites
//
// Light a cone and its opposite the forecolor
// Light all the other cones the backcolor

void LightOpposites() {
  
  for (int i=0; i < NumLEDs; i++) {
    if (i == pointLED || i == (NumLEDs - pointLED - 1)) {
      LightCone(i, Wheel(foreColor));
    } else {
      LightCone(i, Wheel(backColor));
    }
  }
  
  // Pick an adjacent cone
  pointLED = neighbors[pointLED][random(NumNeigh)] - 1;  // Remember -1 offset!
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
  
  if (solid == 0 || solid == Solid_ID) {  // Command is for me
    return (ProcessOrder(message));  // Figure out the next command(s)
  } else {  // Command not for me
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
      Serial.println("");
      if (show != value) {  // Slave has different show
        show = value;
        return (true);
      } else {
         return (false);
      }
    
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
         
     case 'B':  // Show
       if (backColor != value) {  // Slave has different color
         backColor = value;
         return (true);
      } else {
         return (false);
      }
         
    default:
      return (false);
  }
}

//
// Light Cone
//
// Put color into a particular numbered cone
// Values go into the next_frame array
// Does not light up the array
//

void LightCone(int cone, uint32_t color) {
  next_frame[cone] = color;
}

// Black Cones
//
// Immediately turns off all cones
//

void BlackCones() {
  for (int i=0; i<NumLEDs; i++) {
    current_frame[i] = Color(0,0,0);        // Black
    next_frame[i] = Color(0,0,0);           // Black
    UpdateCone(i, Color(0,0,0));    // Black
  }
  WriteCones();  // Immediately turn the cones off
}

//
// Update Cone
//
// Decompresses 30-bit color into 10-bit r,g,b
// Updates the r,g,b values into the LED array
// Does not call the array; WriteCones() does that


void UpdateCone(int cone, uint32_t color) {
  for (int j = 0; j < 3; j++) {
    LEDChannels[cone][j] = (color >> (j * 10)) & 0x3ff;
  }
}

//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code initially from Greg and Robie
//
// RGBinter30 interpolates between current and next frames
//

void morph_frame() {
   for (int i = 0; i < NumLEDs; i++) {
     UpdateCone(i, RGBinter30(current_frame[i], next_frame[i], (float)morph/GetDelayTime(wait)));
   }
   WriteCones();  // Update the display
}

// Create a 30-bit color value from 10-bit R,G,B values
uint32_t Color(uint32_t r, uint32_t g, uint32_t b)
{ 
  // uint32_t colorbit;
  // int32_t result = (g & 0x3FF)<<20 | (b & 0x3FF)<<10 | r & 0x3FF;
  // colorbit = (result >> 10) & 0x3ff;

  //Take the lowest 10 bits of each value and append them end to end
  //return( ((unsigned int)g & 0x3FF )<<20 | ((unsigned int)b & 0x3FF)<<10 | (unsigned int)r & 0x3FF);
  return((g & 0x3FF)<<20 | (b & 0x3FF)<<10 | r & 0x3FF);
}
  
//Input a value 0 to 6144 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(int color)
{
  return Gradient_Wheel(color, 1);  // Intensity = 1
}

//Input a value 0 to 6144 to get a color value.
//The colours are a transition r - g -b - back to r
//Intensity must be 0 <= intensity <= 1
uint32_t Gradient_Wheel(int color, float intensity)
{
  int r,g,b;
  int channel, value;
  
  color = color % MaxColor;  // Keep colors within bounds
  
  // Red party
  color = color * 2 / 3;  // Break color into 4 (not 6) channels
  
  channel = color / 1024;
  value = color % 1024;
  
  if (intensity > 1) intensity = 1;
  if (intensity < 0) intensity = 0;
  
  // Red party - These values are different
  
  switch(channel)
  {
    case 0:
      r = 1023;
      g = value;
      b = 0;        
      break;
    case 1:
      r = 1023;
      g = 1023 - value;
      b = 0;        
      break;
    case 2:
      r = 1023;
      g = 0;
      b = value;        
      break;
    default:
      r = 1023;
      g = 0;
      b = 1023 - value;        
      break; 
   }
  
  /*  This is the usual wheel
  
  switch(channel)
  {
    case 0:
      r = 1023;
      g = value;
      b = 0;        
      break;
    case 1:
      r = 1023 - value;
      g = 1023;
      b = 0;        
      break;
    case 2:
      r = 0;
      g = 1023;
      b = value;        
      break;
    case 3:
      r = 0;
      g = 1023 - value;
      b = 1023;        
      break;
    case 4:
      r = value;
      g = 0;
      b = 1023;        
      break;
    default:
      r = 1023;
      g = 0;
      b = 1023 - value;        
      break; 
   }
   */
   
  return(Color(r * intensity, g * intensity, b * intensity));
}

//
// WriteCones
//
// Push the color data into the lights
// From MaceTech website: http://docs.macetech.com/doku.php/shiftbrite#command_format
//
//
 
void WriteCones() {
 
    SB_CommandMode = B00; // Write to PWM control registers
    for (int h = 0; h < NumLEDs; h++) {
      
      	  SB_RedCommand = LEDChannels[h][0];
	  SB_GreenCommand = LEDChannels[h][1];
	  SB_BlueCommand = LEDChannels[h][2];
	  SB_SendPacket();
    }
 
    delayMicroseconds(15);
    digitalWrite(latchpin,HIGH); // latch data into registers
    delayMicroseconds(15);
    digitalWrite(latchpin,LOW);
 
    SB_CommandMode = B01; // Write to current control registers
    for (int z = 0; z < NumLEDs; z++) SB_SendPacket();
    delayMicroseconds(15);
    digitalWrite(latchpin,HIGH); // latch data into registers
    delayMicroseconds(15);
    digitalWrite(latchpin,LOW);
 
}

// SB_SendPacket()
//
// From MaceTech website: http://docs.macetech.com/doku.php/shiftbrite#command_format

void SB_SendPacket() {
 
   if (SB_CommandMode == B01) {
     SB_RedCommand = 127;
     SB_GreenCommand = 127;
     SB_BlueCommand = 127;
   }
   
   SB_CommandPacket = SB_CommandMode & B11;
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_BlueCommand & 1023);
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_RedCommand & 1023);
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_GreenCommand & 1023);

   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 24);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 16);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 8);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket); 
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
//  RGB Interpolate Wheel
//
//  Wrapper for RGB Interpolate RGB below
//  start and end colors are 0-255 wheel colors

uint32_t RGBinterWheel(int c1, int c2, float fract)
{
  return(RGBinter30(Wheel(c1),Wheel(c2),fract));
}

//
//  RGB Interpolate 30-bit
//
//  Wrapper for HSV Interpolate RGB below
//  start and end colors are 30-bit colors

uint32_t RGBinter30(uint32_t c1, uint32_t c2, float fract)
{
  return(RGBinterRGB(GetRed(c1),GetGreen(c1),GetBlue(c1), GetRed(c2),GetGreen(c2),GetBlue(c2), fract));
}

//  RGB Interpolate RGB
//
//  Given a start rgb, an end rgb, and a fractional distance (0-1)
//  This function interpolates between the two points
//  and returns the properly interpolated rgb
//  as a 30-bit rgb color

uint32_t RGBinterRGB(int r1, int g1, int b1, int r2, int g2, int b2, float fract)
{
  if (fract <= 0) return(Color(r1,g1,b1));
  if (fract >= 1) return(Color(r2,g2,b2));
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

// Extracts the red part of a 30-bit color
int GetRed(uint32_t c)
{
  return (c & 0x3ff);  
}

// Extracts the green part of a 30-bit color
int GetGreen(uint32_t c)
{
  return ((c >> 10) & 0x3ff);
}

// Extracts the green part of a 30-bit color
int GetBlue(uint32_t c)
{
  return ((c >> 20) & 0x3ff);
}

// If r=g=b=0, return true
boolean IsBlack(int r, int g, int b)
{
  if(r=g=b=0) return (true);
  else return (false);
}

