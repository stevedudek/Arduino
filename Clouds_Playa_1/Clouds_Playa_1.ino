//#include "SPI.h"
//#include <Wire.h>
//#include "Adafruit_TCS34725.h"

/******************
 *
 *   7/10/17
 *
 *   64 Clouds controlled by a black box
 *
 *   3 knobs, 1 buttons, and an Arduino (no xBee)
 *
 ******************/

#define numClouds 64

#define ROOM_WIDTH 30
#define ROOM_LENGTH 40

float brightness = 1;  // A fraction

// Colors
int foreColor = 1000;    // Starting foreground color
int backColor = 2000;    // Starting background color
#define MAX_COLOR 6144    // Colors are 6 * 1024
uint32_t BLACK = Color(0,0,0);
int constrain_color = 0;  // constrain the color palette. 0 = no_constraint.
#define COLOR_WINDOW 0.3  // 0 = one color; 1 = all colors (#define)
#define DIM_INTENSITY 0.2  // Brightness of a dimmed cloud

// Shows
int curr_show = 12;  // Starting show
#define MAX_SHOW 13  // Total number of shows

// Morphing
int morph = 0;      // Keeps track of morphing steps (morph clock - minute hand)
int cycle = 0;      // Keeps track of animation (cycle clock - hour hand)
boolean update = true; // flag that tells whether lights need to be recalculated

// LED Parameters
#define datapin   5 // DI
#define latchpin  6 // LI
#define enablepin 7 // EI
#define clockpin  8 // CI

unsigned long SB_CommandPacket;
int SB_CommandMode;
int SB_BlueCommand;
int SB_RedCommand;
int SB_GreenCommand;

int LEDChannels[numClouds][3] = {0};    // 8 for each octobar. Not all channels are used!

// Delays
int wait = 0;
#define MAX_WAIT 12 // Number of stored delay times

// Frame buffers
uint32_t current_frame[numClouds];
uint32_t next_frame[numClouds];

int shuffle_order[numClouds];  // Need int type as this stores wheel colors

#define MAX_MAP 100  // max values for cloud_map

byte cloud_map[numClouds][2] = {
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4
  
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4
  
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4
  
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4

  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4

  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, // 0 - 4

  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
};

//
// Box stuff
//
#define NUM_BUTTONS 1
#define NEXT_BUTTON 0
byte buttonpins[NUM_BUTTONS] = { 8 };  // Digital pins for buttons 1, 2, 3
boolean buttoncurr[NUM_BUTTONS];  // Current and previous status of buttons
boolean buttonprev[NUM_BUTTONS];
long buttontime[NUM_BUTTONS];

#define NUM_DIALS 3
#define BRIGHTNESS_DIAL 0
#define SPEED_DIAL      1
#define COLOR_DIAL      2
byte dialpins[NUM_DIALS] = { 0,1,2 }; // Analog pins for dials 1, 2, 3
int dialval[3]; // Current position of dials
#define MIN_CHANGE 5 // Minimum change in a dial that will register. Prevents flickering

byte solid = 0;        // Which solid is dial 0 set to
#define MAX_SOLID  6   // 0=All, 1-2-3-etc=specific solid 


void setup() {
   
   Serial.begin(9600);
   Serial.println("Start");

   randomSeed(analogRead(0));

   pinMode(datapin, OUTPUT);
   pinMode(latchpin, OUTPUT);
   pinMode(enablepin, OUTPUT);
   pinMode(clockpin, OUTPUT);

   digitalWrite(latchpin, LOW);
   digitalWrite(enablepin, LOW);
   
   for (int p = 0; p < NUM_BUTTONS; p++) {
     pinMode(buttonpins[p], INPUT);
     buttonprev[p] = false;
     buttoncurr[p] = digitalRead(buttonpins[p]);
     buttontime[p] = 0;
   }

   for (int d = 0; d < NUM_DIALS; d++) {
     dialval[d] = GetDial(d);
   }   

   shuffle_clouds();
   initializeColorBuffers();  // Stuff with zeros (all black)
}

//
// Main loop
//
void loop() {
  
  check_box();
  
  if (update) {
    
    update = false;
    
    switch(curr_show) {
    
      case 0:    
        allOn();
        break;
      case 1:
        moving_center();
        break;
      case 2:
        sinewave();
        break;
      case 3:
        colorwave();
        break;
      case 4:
        twinkle();
        break;
      case 5:
        radar();
        break;
      case 6:
        all_flux();
        break;
      case 7:
        snake();
        break;
      case 8:
        two_color();
        break;
      case 9:
        random_color();
        break;
      case 10:
        twinkle_one_color();
        break;
      case 11:
        moving_color();
        break;
      case 12:
        radar_color();
        break;
      case 13:
        explosion();
        break;
    }
  }
  
  // Morph the display
  
  drawClouds((float)morph/GetDelayTime(wait));
  
  // Advance the clock
   
  if (morph++ >= GetDelayTime(wait)) {  // Finished morphing 
    update = true;  // Force an update
    push_frame();
    morph = 0;  // Reset morphClock
    
    if (cycle++ > 1000) {
      cycle = 0;  // Advance the cycle clock
      shuffle_clouds();
    }
  }
  
  // Randomly change foreColor, backColor, show, and timing
  
  if (!random(10)) {
    foreColor = IncColor(foreColor, 10);
  }
  
  if (!random(10)) {
    backColor = IncColor(backColor, -5);
  }
  
  if (!random(10000)) {
    curr_show = random(MAX_SHOW);
    morph = 0;
    cycle = 0;
    shuffle_clouds();
  }
  
  if (!random(1000) && digitalRead(SPEED_DIAL) == 0) {
    wait++;
    if (wait == MAX_WAIT) {
      wait = 0;
      morph = 0;
    }
  }
}

/***********
 *  
 *  Shows - lifted directly from Processing
 *  
 */
 
//
// All On - Simply turns all the pixels on to one color
// 
void allOn() {
  fill_color(Wheel(foreColor));
}

//
// two_color - set half of the clouds one color, the other half a second color
//
void two_color() {
  for (int i=0; i < numClouds; i++) {
    if (i % 2 == 0) {
      setPixelColor(shuffle_order[i], Wheel(foreColor));
    } else {
      setPixelColor(shuffle_order[i], Wheel(backColor));
    }
  }
  foreColor = IncColor(foreColor, 10);
}

//
// twinkle - set all to foreColor, but have n-clouds twinkle backColor
//
void twinkle() {
  fill_color(Wheel(foreColor));
  int num_twinkle = random(1,6);
  for (int i=0; i < num_twinkle; i++) {
    setPixelColor(get_random_cloud(), Wheel(backColor));
  }
}

//
// twinkle - set all to dim color, but have n-clouds brighten
//
void twinkle_one_color() {
  fill_color(Gradient_Wheel(foreColor, DIM_INTENSITY));
  int num_twinkle = random(2,8);
  for (int i=0; i < num_twinkle; i++) {
    setPixelColor(get_random_cloud(), Gradient_Wheel(foreColor, 1.0));
  }
}

//
// random_color
//
void random_color() {
  if (cycle == 1) {  // Start of show: assign lights to random colors
    for (int i = 0; i < numClouds; i++) {
      shuffle_order[i] = random(MAX_COLOR);
    }
  }
  for (int i = 0; i < numClouds; i++) {
    setPixelColor(i, Wheel(shuffle_order[i]));
    shuffle_order[i] = IncColor(shuffle_order[i], 100);
  }
}

//
// explosion - center to edge motion
//
void explosion() {
  byte explosion_speed = 10;  // higher is slower
  float best_distance = (cycle % explosion_speed) / (float)(explosion_speed - 1);
  float distance, distance_diff;
  
  for (int i=0; i < numClouds; i++) {
    // 70.7 is the max distance possible between corner and center
    distance = get_distance((byte)(MAX_MAP / 2), (byte)(MAX_MAP / 2), cloud_map[i][0], cloud_map[i][1]);
    distance = distance / 70.7;  // Change to 0-1
    distance_diff = abs(best_distance - distance);
    if (distance_diff > 0.5) {
      distance_diff = 1.0 - distance_diff;
    }
    
    setPixelColor(i, Gradient_Wheel(foreColor, distance_diff * 2));
  }
}

//
// moving_center - bright center that moves around the room
//
void moving_center() {
  float center_x_norm = (sin(TWO_PI * backColor / MAX_COLOR) + 1) / 2.0; // 0 to 1
  float center_y_norm = (sin(TWO_PI * foreColor / MAX_COLOR) + 1) / 2.0; // 0 to 1
  byte center_x = (byte)(MAX_MAP * center_x_norm);
  byte center_y = (byte)(MAX_MAP * center_y_norm);
  float attenuation;
  
  for (int i=0; i < numClouds; i++) {
    // 141.2 is the max distance possible between (0,0) and (100,100)
    attenuation = (141 - get_distance(center_x, center_y, cloud_map[i][0], cloud_map[i][1])) / 141.2;
    attenuation = attenuation * attenuation;  // Square to magnify signal
    setPixelColor(i, Gradient_Wheel(foreColor, attenuation));
  }
}

//
// moving_color - one color in the center; different colors at edges
//
void moving_color() {
  float center_x_norm = (sin(TWO_PI * backColor / MAX_COLOR) + 1) / 2.0; // 0 to 1
  float center_y_norm = (sin(TWO_PI * foreColor / MAX_COLOR) + 1) / 2.0; // 0 to 1
  byte center_x = (byte)(MAX_MAP * center_x_norm);
  byte center_y = (byte)(MAX_MAP * center_y_norm);
  int calc_color;
  float attenuation;
  
  for (int i=0; i < numClouds; i++) {
    // 141.2 is the max distance possible between (0,0) and (100,100)
    attenuation = get_distance(center_x, center_y, cloud_map[i][0], cloud_map[i][1]) / 141.2;
    calc_color = IncColor(foreColor, (int)(attenuation * MAX_COLOR));
    setPixelColor(i, Wheel(calc_color));
  }
}

//
// radar
//
void radar() {
  byte radar_speed = 10;  // higher is slower
  float angle, angle_diff;
  float best_angle = (cycle % radar_speed) / (float)(radar_speed - 1);
  
  for (int i=0; i < numClouds; i++) {
    angle = atan((cloud_map[i][1] - (MAX_MAP / 2.0)) / (0.1+(cloud_map[i][0] - (MAX_MAP / 2.0))));
    angle = (angle + (PI/2)) / PI;  // atan range is -PI/2 to PI/2; change to 0 to 1
    
    angle_diff = abs(best_angle - angle);
    if (angle_diff > 0.5) {
      angle_diff = 1.0 - angle_diff;
    }
    setPixelColor(i, Gradient_Wheel(foreColor, angle_diff * 2));
  }
}

//
// radar_color
//
void radar_color() {
  byte radar_speed = 10;  // higher is slower
  float best_angle = (cycle % radar_speed) / (float)(radar_speed - 1);
  float angle, angle_diff;
  
  for (int i=0; i < numClouds; i++) {
    angle = atan((cloud_map[i][1] - (MAX_MAP / 2.0)) / (0.1+(cloud_map[i][0] - (MAX_MAP / 2.0))));
    angle = (angle + (PI/2)) / PI;  // atan range is -PI/2 to PI/2; change to 0 to 1
    
    angle_diff = abs(best_angle - angle);
    if (angle_diff > 0.5) {
      angle_diff = 1.0 - angle_diff;
    }
    setPixelColor(i, Wheel(IncColor(foreColor, (int)(MAX_COLOR * (1 - (angle_diff * 2))))));
  }
}

//
// all_flux - all lights go through a sinewave of intensity
//
void all_flux() {
  byte flux_speed = 10;  // Higher = slower
  byte best_cloud = (byte)(cycle % flux_speed);
  byte curr_cloud;
  float attenuation;
  
  for (int i=0; i < numClouds; i++) {
    curr_cloud = (byte)(shuffle_order[i] % flux_speed);
    attenuation = abs(curr_cloud - best_cloud) / (float)flux_speed;
    if (attenuation > 0.5) {
      attenuation = 1.0 - attenuation;
    }   
    setPixelColor(i, Gradient_Wheel(foreColor, attenuation * 2));
  }
}

//
// snake - all lights go through a sinewave of intensity; neighbors are similar
//
void snake() {
  byte flux_speed = 20;  // Higher = slower
  byte best_cloud = (byte)(cycle % flux_speed);
  byte curr_cloud;
  float attenuation;
  
  for (int i=0; i < numClouds; i++) {
    curr_cloud = (byte)(i % flux_speed);
    attenuation = abs(curr_cloud - best_cloud) / (float)flux_speed;
    if (attenuation > 0.5) {
      attenuation = 1.0 - attenuation;
    }   
    setPixelColor(i, Gradient_Wheel(foreColor, attenuation * 2));
  }
}

//
// sinewave - vertical sinewave
//
void sinewave() {
  float attenuation;
  int room_height;
  
  for (int i=0; i < numClouds; i++) {
    room_height = (cloud_map[i][1] + (cycle * (wait - MAX_WAIT))) % MAX_MAP;
    attenuation = sin(TWO_PI * room_height / MAX_MAP) + 0.1;
    attenuation = (attenuation + 1) / 2.0;
    setPixelColor(i, Gradient_Wheel(foreColor, attenuation));
  }
}

//
// colorwave - vertical sinewave
//
void colorwave() {
  float attenuation;
  int room_height;
  
  for (int i=0; i < numClouds; i++) {
    room_height = ((MAX_MAP - cloud_map[i][1]) + (cycle * (wait - MAX_WAIT))) % MAX_MAP;
    attenuation = sin(TWO_PI * room_height / MAX_MAP);
    setPixelColor(i, Wheel((int)(attenuation * MAX_COLOR)));
  }
}

//
// shuffle_clouds
//
void shuffle_clouds() {
  // Shuffle sort to determine order to turn on lights
  short i,j, save;
  
  for (i=0; i < numClouds; i++) {
    shuffle_order[i] = i; // before shuffle
  }
  for (i=0; i < numClouds; i++) {  // here's position
    j = (byte)get_random_cloud();         // there's position
    save = shuffle_order[i];
    shuffle_order[i] = shuffle_order[j];       // first swap
    shuffle_order[j] = save;             // second swap
  }
}

//
// get_distance - find the distance between two points
//
float get_distance(byte x1, byte y1, byte x2, byte y2) {
  return sqrt(sq(x2 - x1) + sq(y2 - y1));
} 

//
// get_angle - get the vector angle between 2 vectors (result will be an inv. cosine 0-1)
//
float get_angle(int ax, int ay, int bx, int by) {
  float a_length = sqrt((ax*ax) + (ay*ay));
  float b_length = sqrt((bx*bx) + (by*by)); 
  float unit_ax = ax / a_length;
  float unit_ay = ay / a_length;
  float unit_bx = bx / b_length;
  float unit_by = by / b_length;
  
  return cos((unit_ax * unit_ay) + (unit_bx * unit_by)); 
}

//
// get_random_cloud - pick a random cloud number
//
int get_random_cloud() {
  return random(numClouds);
}

//
// Get Delay Time - Returns a delay time from an array
//
int GetDelayTime(int wait) {
  int DelayValues[] = { 4, 8, 12, 20, 30, 40, 50, 60, 75, 100, 125, 150 };
  return (DelayValues[wait % MAX_WAIT]);
}

/***********
 *  
 *  Color Routines
 *  
 */

//
// fill
//
void fill_color(uint32_t color) {
  for (int i = 0; i < numClouds; i++) {
    setPixelColor(i, color);
  }
}

//
// clear_all: set all to black
//
void clear_all() {
  fill_color(BLACK);
}

//
// IncColor - Adds amount to color
//
int IncColor(int c, int amount) {
  int value = c + amount;
  while (value < 0) value += MAX_COLOR;
  while (value >= MAX_COLOR) value -= MAX_COLOR;
  return value;
}

//
// restrict_color - restrict color to certain wheel positions
//
int restrict_color(int c) {
  if (constrain_color != 0) { 
    c = ((int)((float)c * COLOR_WINDOW) + constrain_color) % MAX_COLOR;
  }
  return c;
}

void initializeColorBuffers() {
  for (int i = 0; i < numClouds; i++) {
    next_frame[i] = BLACK;
  }
  push_frame();
}

void push_frame() {
  for (int i = 0; i < numClouds; i++) {
    current_frame[i] = next_frame[i];
  }
}

//
// setPixelColor
//
void setPixelColor(int i, uint32_t color) {
  next_frame[i] = color;
}

//
// Draw Clouds
//
void drawClouds(float morph_ratio) {
  for (int i = 0; i < numClouds; i++) {
    update_cloud(i, RGBinter30(current_frame[i], next_frame[i], morph_ratio));  
  }
  write_clouds();  // Update the display
}

//
// update cloud - Decompresses 30-bit color into 10-bit r,g,b
//               Updates the r,g,b values into the LED array
//               Does not call the array; WriteCones() does that
//
void update_cloud(int i, uint32_t color) {
  for (int j = 0; j < 3; j++) {
    LEDChannels[i][j] = (color >> (j * 10)) & 0x3ff;
  }
}

//
// Wheel - Input a value 0 to 6144 to get a color value.
//         The colours are a transition r - g -b - back to r
//
uint32_t Wheel(int color)
{
  return Gradient_Wheel(color, 1);  // Intensity = 1
}

//
// Gradient Wheel- Input a value 0 to 6144 to get a color value.
//                 The colours are a transition r - g -b - back to r
//
uint32_t Gradient_Wheel(int color, float intensity)
{
  int r,g,b;
  int channel, value;
  
  color = color % MAX_COLOR;  // Keep colors within bounds
  
  channel = color / 1024;
  value = color % 1024;

  intensity *= brightness;
  
  if (intensity > 1) intensity = 1;
  if (intensity < 0) intensity = 0;
  
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
   
  return (Color(r * intensity, g * intensity, b * intensity));
}

//
// Color - create a 30-bit color value from 10-bit R,G,B values
//
uint32_t Color(uint32_t r, uint32_t g, uint32_t b)
{ 
  // if this isn't working properly - check the Icosahedron code
  return ((g & 0x3FF)<<20 | (b & 0x3FF)<<10 | r & 0x3FF);
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


/***********
 *  
 *  Box Stuff
 *  
 */

//
// Check Box
//
// Goes through the 3 buttons and 3 dials and see whether they have changed position

void check_box() {
  
  int dial;
  
  // Start with the dials
  
  // SPEED_DIAL
  dial = GetDial(SPEED_DIAL);
  if (abs(dial - dialval[SPEED_DIAL]) > MIN_CHANGE) { // Has dial #0 budged?
    dialval[SPEED_DIAL] = dial;      // Update dial
    wait = dial * (MAX_WAIT - 1) / 1024;  
  }
  
  // COLOR_DIAL
  dial = GetDial(COLOR_DIAL);
  if (abs(dial - dialval[COLOR_DIAL]) > MIN_CHANGE) { // Has dial #1 budged?
    dialval[COLOR_DIAL] = dial;      // Update dial
    constrain_color = dial * MAX_COLOR / 1024;  
  }
  
  // BRIGHTNESS_DIAL
  dial = GetDial(BRIGHTNESS_DIAL);
  if (abs(dial - dialval[BRIGHTNESS_DIAL]) > MIN_CHANGE) { // Has dial #2 budged?
    dialval[BRIGHTNESS_DIAL] = dial;      // Update dial
    brightness = dial / 1024.0;
  }
  
  // Look next at the button
  
  // NEXT_BUTTON
  if (ButtonPushed(NEXT_BUTTON)) {
    curr_show = (curr_show + 1) % MAX_SHOW;
    morph = 0;
    cycle = 0;
    shuffle_clouds();
  }
}

//
// Get Dial - returns the position of the dial pot
//
int GetDial(byte dial) {
  return(analogRead(dialpins[dial]));
}

//
// ButtonPushed - returns true if the button has been pushed
// Includes a timer to prevent flickering
//
boolean ButtonPushed(byte b) {  
  
  boolean bstate = digitalRead(buttonpins[b]);
  
  if (bstate == HIGH && buttonprev[b] == LOW && millis() - buttontime[b] > 100) { // Has button been pushed?
      buttoncurr[b] = LOW;                                                        // Yes, reset the button
      buttonprev[b] = HIGH;
      buttontime[b] = millis();      // ... and remember when the last button press was    
      return true;          // Button pushed
  } else {                  // Button not pushed
      buttonprev[b] = bstate;
      return false;           
  }
}

/***********
 *  
 *  Low-level LED setting
 *  
 */
 
//
// WriteCones
//
// Push the color data into the lights
// From MaceTech website: http://docs.macetech.com/doku.php/shiftbrite#command_format
//
void write_clouds() {
 
    SB_CommandMode = B00; // Write to PWM control registers
    
    for (int h = 0; h < numClouds; h++) {
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
    for (int z = 0; z < numClouds; z++) SB_SendPacket();
    delayMicroseconds(15);
    digitalWrite(latchpin,HIGH); // latch data into registers
    delayMicroseconds(15);
    digitalWrite(latchpin,LOW);
 
}

//
// SB_SendPacket()
//
// From MaceTech website: http://docs.macetech.com/doku.php/shiftbrite#command_format
//
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

