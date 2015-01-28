//#include <TimerOne.h>
//#include "Button.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

//
//  Hedgehog! - 50 Lights. This version does not use a keypad
//
//  Hooked up an xBee to receive color, timing, and show information
//
//  Current approach only lets Hedgehog receive information
//
/*****************************************************************************/
//
// Set Up Pins
//
// 2 pins for clock and data of lights. Reversed from usual!
// 
#define dataPin 9       // 'yellow' wire
#define clockPin 8      // 'green' wire

#define numLights 50      // number of spikes
// framebuffers
uint32_t current_frame[numLights];
uint32_t next_frame[numLights];

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numLights, dataPin, clockPin);

// Hedgehog colors

int color1  = 85;   // Starting foreground color - Global!
int color2 = 170;  // Starting background color - Global!
#define MaxColor 255    // Colors go from 0 to 255

// Shows

int show=1;    // Show and number of initial show
int wait=6;    // index to keep track of delay time
#define MAX_SHOW 14    // Total number of shows
#define maxWait 12     // Number of stored delay times 

// Timing variables for xBee communication

unsigned long OldTime;
unsigned long NewTime;
boolean ALONE = true; // Flag for if controller has sent a message

#define MAX_AWAY 20     // Slave goes into Alone mode if no signal in this #sec
//#define SYNC_FREQ 500   // How often in msec that the master sends out data
#define NO_DATA 9999

// xBee language
#define COMMAND_PROMPT     '#'
#define COMMAND_WHITE      'W'
#define COMMAND_COLOR      'C'
#define COMMAND_DELAY      'D'
#define COMMAND_SHOW       'S'
#define COMMAND_COLORSENSE 'X'
#define COMMAND_END        '$'

#define COST_NUM  '1'    // Which number costume this is. Quite important!

#define NO_CHANGE    0
#define MINOR_CHANGE 1  // Colors have changed
#define MAJOR_CHANGE 2  // Show has changed

// Order of spines
byte SpineOrder[50] = {
      8,
    9,13,7,
   10,12,14,6,
    11,15,5,
   17,16,3,4,
    18,2,0,
     19,1,
    20,22,24,
     21,23,
    40,25,27,
   41,39,26,28,
    42,38,29,
   43,37,31,30,
    44,36,32,
   45,47,35,33,
    46,48,34,
      49,
};

// Centering: creates a center bright ring
byte centering[50] = {
      0,
    0,1,0,
   0,1,1,0,
    0,2,0,
   0,2,2,0,
    1,2,1,
     2,2,
    2,3,2,
     3,3,
    2,3,2,
   2,3,3,2,
    2,3,2,
   1,2,2,1,
    0,2,0,
   0,1,1,0,
    0,1,0,
      0,
};

// chevpattern: pattern of chevrons
byte chevpattern[50] = {
      9,
    9,8,9,
   9,8,8,9,
    8,7,8,
   8,7,7,8,
    7,6,7,
     6,6,
    6,5,6,
     5,5,
    5,4,5,
   5,4,4,5,
    4,3,4,
   4,3,3,4,
    3,2,3,
   3,2,2,3,
    2,1,2,
      0,
};

// starburstpattern: from center to edge
byte starburstpattern[50] = {
      9,
    9,8,9,
   8,7,7,8,
    6,5,6,
   5,4,4,5,
    3,2,3,
     1,1,
    2,0,2,
     1,1,
    2,1,2,
   4,3,3,4,
    5,4,5,
   6,5,5,6,
    7,6,7,
   8,7,7,8,
    9,8,9,
      9,
};

// centerstripe: a center stripe
byte centerstripe[50] = {
      1,
    0,1,0,
   0,0,1,0,
    0,1,0,
   0,1,0,0,
    0,1,0,
     0,1,
    0,1,0,
     1,0,
    0,1,0,
   0,0,1,0,
    0,1,0,
   0,1,0,0,
    0,1,0,
   0,0,1,0,
    0,1,0,
      1,
}; 

void setup() {
  
  // xBee communication
  
  OldTime = 0;
  NewTime = 0;
  
  Serial.begin(9600);
  
  //not sure if this is necessary but try it anyway
  randomSeed(analogRead(0));
  
  for (int i=0; i < numLights; i++) current_frame[i] = 0; // clear display
  
  // Start up the LED counter
  strip.begin();

  // Update the strip, to start they are all 'off'
  clear();
  strip.show();
}

void loop() {
   
  // Random show if Master or Slave in Alone mode
  if (ALONE) show = random(1,MAX_SHOW);  // Show 0 is dark
  
  switch(show) {
    case 0:
      clearwithoutfade(random(1,4)*10000);
      break;
    case 1:    
      allOn(random(20,100));
      break;
    case 2:
      randomfill(random(3,10)); 
      break;
    case 3:
      randomcolors(random(5,30)*10000);
      break;
    case 4:
      twocolor(random(1,30)*10000);
      break;
    case 5:
      rainbowshow(random(100,500));
      break;
    case 6:
      lightwave(random(2,10));
      break;
    case 7:
      chevrons(random(50,200));
      break;
    case 8:
      hogshell(random(10,30));
      break;
    case 9:
      bullseye(random(1,30)*10000);
      break;
    case 10:
      chevronrainbow(random(1,30)*10000);
      break;
    case 11:
      stripe(random(20,100));
      break;
    case 12:
      chevronfill(random(25,100));
      break;
    case 13:
      starburst(random(50,200));
      break;
    default:
      allOn(random(20,100));
      break;
  }
  // Then a break with all the lights off
  if (ALONE)  {
    show = 0;
    clearwithoutfade(random(1,4)*10000);  // Hopefully 10-1200 seconds
  }
}

/*
 * set all cells to black but don't call show yet
 * ignores buffering
 */
void clear() {
  for (int i=0; i<numLights; i++) {
    strip.setPixelColor(i, Color(0,0,0));
    setPixelColor(i, Color(0,0,0));
  }
}

/*
 * set all cells to black and calls the show
 * no morphing
 */
void clearwithoutfade(int darktime) {
  clear();
  strip.show();
  talkdelay(darktime);
}

/*
 * Fades lights to black over fade time
 */
void clearWithFade(int fadeTime) {
  for (int i=0; i<numLights; i++) {
    setPixelColor(i, Color(0,0,0));
  }
  morph_frame(fadeTime);
}

/*
 * All On
 *
 * Simply turns all the pixes on to a random color and then cycles
 *
 */
 
void allOn(int cycles) {
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      setPixelColor(i, Wheel(color1));
    }
    if(morph_frame(GetDelayTime(wait))==MAJOR_CHANGE) return;  // Slave must recalibrate
  
    if (!random(100)) { if (wait++ > maxWait) wait = 0; }
    if (color1+=5 > MaxColor) color1 = color1 - MaxColor;
  }
  clearWithFade(1000);
}

/*
 * Stripe
 *
 * Put a stripe down the center Slowly color change
 *
 */
 
void stripe(int cycles) {
  for (int count = 0; count < cycles; count++) {
   for (int i=0; i < numLights; i++) {
     if (centerstripe[i]) { setPixelColor(SpineOrder[i], Wheel(color1));
     } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
   }
   if(morph_frame(GetDelayTime(wait))==MAJOR_CHANGE) return;  // Slave must recalibrate
  
   if (!random(100)) { if (wait++ > maxWait) wait = 0; }
   if (color1+=10 > MaxColor) color1 = color1 - MaxColor;    
  }
  clearWithFade(1000);
}

/*
 * randomcolors: turns each pixel on to a random color
 */

void randomcolors(long time) {
  for (int i=0; i < numLights; i++) setPixelColor(i, Wheel(random(MaxColor)));
  if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;  // Slave must recalibrate
  if(talkdelay(time) == MAJOR_CHANGE) return;
  clearWithFade(1000);
}

/*
 * twocolor: turns each pixel on to either of two random colors
 */

void twocolor(long time) {
  
  for (int i=0; i < numLights; i++) {
    if (random(2)) { setPixelColor(i, Wheel(color1)); }
    else { setPixelColor(i, Wheel(color2)); }
  }
  if(morph_frame(1000)==MAJOR_CHANGE) return;  // Slave must recalibrate
  if(talkdelay(time) == MAJOR_CHANGE) return;
  clearWithFade(1000);
}

/*
 * bullseye: lights up the centering image with 4 colors
 */

void bullseye(long time) {
  int colors[4];
  int tempcolor;
  int colorspacing;
  
  tempcolor = color1;
  colorspacing = random(5, MaxColor/5);
  
  for (int i=0; i<4; i++) {
    colors[i] = tempcolor;
    tempcolor = tempcolor + colorspacing;
    if (tempcolor > MaxColor) tempcolor = tempcolor - MaxColor;
  }
  
  for (int j=0; j < strip.numPixels(); j++) {
      setPixelColor(SpineOrder[j], Wheel(colors[centering[j]]));
  }
  if(morph_frame(1000)==MAJOR_CHANGE) return;  // Slave must recalibrate
  if(talkdelay(time) == MAJOR_CHANGE) return;
  clearWithFade(1000);
}

/*
 * chevronrainbow: lights up the chevron with a rainbow pattern with 10 colors
 */

void chevronrainbow(long time) {
  int colors[10];
  int colorspacing;
  
  colorspacing = random(2, MaxColor/11);
  
  for (int i=0; i<10; i++) {
    colors[i] = ((i*colorspacing) + color1) % MaxColor;
  }
  
  for (int j=0; j < strip.numPixels(); j++) {
      setPixelColor(SpineOrder[j], Wheel(colors[chevpattern[j]]));
  }
  if(morph_frame(1000)==MAJOR_CHANGE) return;  // Slave must recalibrate
  if(talkdelay(time) == MAJOR_CHANGE) return;
  clearWithFade(1000);
}

/*
 * randomfill: randomly fills in pixels from blank to all on
 * then takes them away random until blank
 */
 
void randomfill(int cycles) {
  int i, j, save;
  byte shuffle[numLights];  // Will contain a shuffle of 0 to totPixels
  
  for (int count = 0; count < cycles; count++) {
    
    // Shuffle sort to determine order to turn on lights
    for (i=0; i < numLights; i++) shuffle[i] = i; // before shuffle
    for (i=0; i < numLights; i++) {  // here's position
      j = random(numLights);       // there's position
      save = shuffle[i];
      shuffle[i] = shuffle[j];     // first swap
      shuffle[j] = save;           // second swap
    }
    // Blank the screen
    clear();
    strip.show();
    if(talkdelay(GetDelayTime(wait)) == MAJOR_CHANGE) return;
    
    // Turn on lights one at a time
    for (i=0; i < numLights; i++) {
      setPixelColor(shuffle[i], Wheel(color1));
      if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
    }
    // Turn off lights one at a time
    for (i=0; i < numLights; i++) {
      setPixelColor(shuffle[numLights-i-1], Color(0,0,0));
      if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
    }
    // Possibly new delay
    if (!random(2)) { wait = random(0,maxWait); }
  }
}  

/*
 * rainbowshow: fills the spines with a gradient rainbow
 * over time, the starting color changes
 */
 
void rainbowshow(int cycles) {
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
      strip.setPixelColor(SpineOrder[i], Wheel(((i*256 / strip.numPixels()) + color1) % 256) );
    } 
    strip.show();   // write all the pixels out
    if( talkdelay(50) == MAJOR_CHANGE) return;
    if (color1+=15 > MaxColor) color1 = color1 - MaxColor;
  }
}

/*
 * starburst: burst from center to outer ring. Code is identical to chevrons
 */
 
void starburst(int cycles) {
  int chev, i;
    
  for (int count = 0; count < cycles; count++) {
    for (chev=0; chev < 11; chev++) {
      for (i=0; i < numLights; i++) {
        if (starburstpattern[i] == chev) {
          setPixelColor(SpineOrder[i], Wheel((color1+(starburstpattern[i]*3))%MaxColor));
        } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
    }
    if (!random(3)) { if (wait++ > maxWait) wait = 0; }
  }
}

/*
 * chevrons: upward chevrons
 */
 
void chevrons(int cycles) {
  int chev, i;
    
  for (int count = 0; count < cycles; count++) {
    for (chev = 0; chev < 11; chev++) {
      for (i=0; i < numLights; i++) {
        if (chevpattern[i] == chev) {
          setPixelColor(SpineOrder[i], Wheel(color1));
        } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
    }
    if (!random(3)) { if (wait++ > maxWait) wait = 0; }
  }
}

/*
 * chevronfill: upward chevrons that fill and fall back
 */
 
void chevronfill(int cycles) {
  int chev, i;
   
  for (int count = 0; count < cycles; count++) {
    for (chev=-1; chev < 10; chev++) {
      for (i=0; i < numLights; i++) {
        if (chev >= chevpattern[i]) {
          setPixelColor(SpineOrder[i], Wheel((color1+(chevpattern[i]*2))%MaxColor));
        } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
    }
    for (chev=8; chev >= 0; chev--) {
      for (i=0; i < numLights; i++) {
        if (chev >= chevpattern[i]) {
          setPixelColor(SpineOrder[i], Wheel((color1+(chevpattern[i]*2))%MaxColor));
        } else { setPixelColor(SpineOrder[i], Color(0,0,0)); }
      }
      if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
    }
    if (!random(3)) { if (wait++ > maxWait) wait = 0; }
  }
}

/*
 * visit each light in order and illuminate its neighbors
 * ignores buffering
 */
void unit_test() {
  
  for (int i=0; i<numLights; i++) {
    clear();
    strip.setPixelColor(SpineOrder[i], Color(255,0,0));
    strip.show();
    delay(500);    
  }
}

void hogshell(int cycles) {
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      setPixelColor(SpineOrder[i], GradientWheel(color1,centering[i]));
    }
    if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
    if (!random(100)) { if (wait++ > maxWait) wait = 0; }
    if (color1++ > MaxColor) color1 = color1 - MaxColor; 
  }
  clearWithFade(1000);
}

/*
 * lightwave - Just one pixel traveling from 0 to 49
 */
 
void lightwave(int cycles) {
  
  for (int count = 0; count < cycles; count++) {
    for (int i=0; i < strip.numPixels(); i++) {
      for (int j=0; j < strip.numPixels(); j++) {
        if (i == j) { setPixelColor(SpineOrder[j], Wheel(color1)); }
        else { setPixelColor(SpineOrder[j], Color(0,0,0)); }
      }
      if(morph_frame(GetDelayTime(wait)) == MAJOR_CHANGE) return;
      if (!random(100)) { if (wait++ > maxWait) wait = 0; }
      if (!random(10)) {
        if (color1+=5 > MaxColor) color1 = color1 - MaxColor;
      }
    }
  }
}

//
// Frame displays. Basically gradually changes image from previous frame to next frame.
// More pleasing to the eye.
//
// Code from Greg and Robey
//

void draw_frame(int delay_ms) {
  for (int i = 0; i < numLights; i++) {
    strip.setPixelColor(i, next_frame[i] & 0xffffff);
    current_frame[i] = next_frame[i];
  }
  strip.show();
  delay(delay_ms);
}

void draw_frame() {
  draw_frame(250);
}

#define FRAME_MSEC 100
#define MORPH_STEPS 25
#define STEP_LENGTH 10

byte morph_frame(int msec) {   // true if slave needs to update immediately
  byte flag = NO_CHANGE;
  int steps = msec / STEP_LENGTH;
  int deltas[numLights][3];
  for (int i = 0; i < numLights; i++) {
    for (int j = 0; j < 3; j++) {
      uint32_t old_color = (current_frame[i] >> (j * 8)) & 0xff; 
      uint32_t new_color = (next_frame[i] >> (j * 8)) & 0xff;
      deltas[i][j] = (int)new_color - (int)old_color;
    }
  }

  for (int t = 1; t < steps; t++) {
    for (int i = 0; i < numLights; i++) {
      uint32_t color = 0;
      for (int j = 0; j < 3; j++) {
        uint32_t old_color = (current_frame[i] >> (j * 8)) & 0xff;
        color |= ((old_color + (deltas[i][j] * t / steps)) & 0xff) << (8 * j);
      }
      strip.setPixelColor(i, color);
    }
    flag = HearBox();
    if (flag != NO_CHANGE) break;
    strip.show();
    delay(STEP_LENGTH);
  }
  for (int i = 0; i < numLights; i++) {
    current_frame[i] = next_frame[i];
    strip.setPixelColor(i, current_frame[i]);
  }
  strip.show();
  delay(STEP_LENGTH);
  flag = max(HearBox(), flag);
  return(flag);
}

void setPixelColor(int pos, uint32_t color) {
  next_frame[pos] = color;
}

void fill(uint32_t color) {
  for (int i = 0; i < numLights; i++) {
    setPixelColor(i, color);
  }
}

//
// talk delay
//
// Runs a normal timed delay
//
// Meanwhile, listen to the box every POLL_TIMEms


#define POLL_TIME 10

byte talkdelay(int delaytime) {
  int i;
  byte flag = NO_CHANGE;
  for(int i=delaytime; i>POLL_TIME; i-=POLL_TIME) {
    flag = max(flag, HearBox());
    if (flag == MAJOR_CHANGE) return(flag);  // Quit immediately!
    delay(POLL_TIME);
  }
  delay(i); // The remainder
  return(flag);
}

//
// HearBox
//
// Listen for the Box
//

byte HearBox() {
  char incoming;
  long value;
  byte flag = NO_CHANGE;
  
  NewTime = millis();  // Take a time point
  
  if (Serial.available()) {
    
    incoming = Serial.read();
    if (incoming == COMMAND_PROMPT) {  // Heard from the box
       incoming = Serial.read();       // For which number costume?
       if (incoming == COST_NUM || incoming == '9') {  // Talking to me!
           OldTime = NewTime;    // Reset the clock
           ALONE = false;        // We're in contact! 
       
           incoming = Serial.read();  // Get that one-letter command
           //Serial.print(incoming);
           value = HearNum();         // Get the number that follows the command
           //Serial.println(value);
           if (value == NO_DATA) {
              //Serial.println("No legitimate number read");
             return(flag); // Not getting a legit number
           }
           // Execute the command and return any flags from it
           return(ExecuteCommand(incoming, value));
       }
    }
  }
   
  // No received Serial.available or not talking to me
  //
  // Check how long since last hearing from the Master

  if((NewTime - OldTime)/1000 > MAX_AWAY) {
    OldTime = NewTime;    // Reset the clock (for some reason?)
    ALONE = true;         // Lost contact with the Master
    //Serial.println("Feeling very alone");
  }
  return(NO_CHANGE);  // no system update required
}

//
// HearNum
//
// Converts xBee characters into a number
// '$' character terminates the number string
//
// If no number, returns NO_DATA signifier

long HearNum() {
  #define BUFSIZE 20
  #define TIMEOUT_TRIES 100
  
  int waitTime = 0;  // Start counter
  int charplace = 0; // Start of number
  char buf[BUFSIZE];
  
  // Clear character buffer
  for (int i = 0; i <BUFSIZE; i++) buf[i] = 0;
  
  while (waitTime < TIMEOUT_TRIES) {
   
    if (Serial.available()) {
      char tmp = Serial.read();
      //Serial.print(tmp);
      if (tmp != '$') { // Hopefully 0-9 and '-'
        waitTime = 0;  // Reset wait counter
        buf[charplace] = tmp;
        if (charplace++ >= BUFSIZE) { return(NO_DATA); } // Number too long
       } else {      // Got a '$' to end the word
         if (charplace>0) return atoi(buf); else {  // successfully reached the end of a number 
           //Serial.println("i=0 number read");
           return(NO_DATA);  // Returning an empty word
         }
       }
      } else { 
        delay(1);
        waitTime++;
      }
    }
  //Serial.println("Out_of_Time");
  return(NO_DATA);  // ran out of time for next character: bail and return NO_DATA
}

//
// Execute Command
//
// Interprets a letter command and a number value
// Returns a flag alerting whether to break out of the current loop

byte ExecuteCommand(char command, long value) {
  int newDelay;
  
  switch(command) {
      case COMMAND_COLOR:
        color1 = value % MaxColor;
        //TurnAllOneColor(Wheel(color1), 200);
        return(MINOR_CHANGE);
        break;
      case COMMAND_WHITE:
        TurnAllWhite(value);
        return(NO_CHANGE);
        break;
      case COMMAND_DELAY:
        newDelay = GetDelayValue(value);
        if(wait != newDelay) {
          wait = newDelay;
          return(MINOR_CHANGE);
          //return(MAJOR_CHANGE);
        }
        break;
      case COMMAND_SHOW:
        if (show != value % MAX_SHOW) {
          show = value % MAX_SHOW;
          return(MAJOR_CHANGE);
        }
        break;
      case COMMAND_COLORSENSE:
        TurnAllOneColor(value, 4000);
        return(NO_CHANGE);
        break;
      default:
        return(NO_CHANGE);
  }
  return(NO_CHANGE);
}

/*
 * Get Delay Time - Returns a delay time from an array.
 */
 
int GetDelayTime(int wait) {
  int DelayValues[maxWait] = { 10, 20, 40, 60, 80, 100, 150, 200, 300, 500, 750, 1000 };
  if (wait < maxWait) { return (DelayValues[wait]); } else { return (DelayValues[maxWait]); }
}

//
// Get Delay Value - Returns the closest delay value to the time
//

int GetDelayValue(int delaytime) {
  int closest = 0;
  for (int i=1; i < maxWait; i++) {
    if (abs(delaytime-GetDelayTime(i)) < abs(delaytime-GetDelayTime(closest))) {
      closest = i;
    }
  }
  return(closest);
}
//
// TurnAllOneColor
//
// Turn all the lights to the given color
// The second value is the delay

void TurnAllOneColor(uint32_t c, int delaytime) {
  for (int i=0; i < strip.numPixels(); i++) {
     strip.setPixelColor(i, c);
  }
  strip.show();
  //
  talkdelay(delaytime);  // May hang in a recursive loop as this calls again a higher loop
}

void TurnAllWhite(int delaytime) {
  TurnAllOneColor(Color(255,255,255), delaytime);
}

void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 96 * 3; j++) {     // 3 cycles of all 96 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 96));
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t frame, uint8_t wait) {
  int i;
  for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // 24 and division is to get the repeat pattern just right
      // the % 96 is to make the wheel cycle around
     strip.setPixelColor(i, Wheel(((i*24 / strip.numPixels()) + frame) % 96) );
  }  
  strip.show();   // write all the pixels out
  delay(wait);
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint16_t c, uint8_t wait) {
  int i;
  
  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
  }  
  strip.show();
  delay(wait);
}


/* Helper functions */

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170; 
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
 
//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
//Furthermore, the colors are attenuated:
//3=full strength, 2=2/3rd strength, 1=1/3rd strength, 0=barely on
uint32_t GradientWheel(byte WheelPos, int strength)
{
  int r,g,b;
  int s[4] = { 2, 10, 50, 255 };  // This is the attenuation percent
  
  if (WheelPos < 85) {
   r = WheelPos * 3;
   g = 255 - WheelPos *3;
   b = 0;
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   r = 255 - WheelPos *3;
   g = 0;
   b = WheelPos * 3;
  } else {
   WheelPos -= 170; 
   r = 0;
   g = WheelPos * 3;
   b = 255 - WheelPos *3;
  }
  
  // Attenuate r,g,b
  r = r * s[strength] / 100;
  g = g * s[strength] / 100;
  b = b * s[strength] / 100;
  return Color(r,g,b);
}     
