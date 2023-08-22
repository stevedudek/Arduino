#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

#define FASTLED_ESP32_FLASH_LOCK 1

//
//  Triceratops - 45 cones on a coat
//
//  Modern Software - a lot to test
//
//  6/5/23
//

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

#define DELAY_TIME  15  // in milliseconds

// Smoothing constants - lower is slower smoothing
#define SMOOTHING_SHOWS_HUE    20   // Fastest full rainbow = DELAY_TIME * (255 / this value) = 150 ms
#define SMOOTHING_SHOWS_VALUE  40   // Fastest turn off/on = DELAY_TIME * (255 / this value) = 150 ms

#define DATA_PIN 0  //  11 // 7 // data = 7 (Teensy) or 0 (Feather)
#define CLOCK_PIN 2  //  13 // 8 // clock = 8 (Teensy) or 2 (Feather)

#define NUM_LEDS 45

#define XX  255

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define MAX_XCOORD  4
#define MAX_YCOORD  17

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A], CHANNEL_A), 
                  Shows(&led[CHANNEL_B], CHANNEL_B)  };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // Required for smoothing
CRGB leds[NUM_LEDS];  // The Leds themselves

#define ONLY_RED false
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
#define BLACK  CHSV(0, 0, 0)

// Shows
#define NUM_SHOWS 25
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

// wait times
uint8_t show_duration = 40;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 128;  // 0 = no fading, to 255 = always be fading

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

// Lookup tables

uint8_t ConeLookUp[] PROGMEM = {
      44,
    42, 43,
  41, 40, 32,
    39, 33,
  38, 34, 31,
    35, 30,
  36, 29, 20,
37, 28, 21, 19,
  27, 22, 18,
26, 23, 17,  9,
  24, 16,  8,
25, 15,  7, 10,
  14,  6, 11,
     5, 12,
   4, 13,  0,
     3,  1,
       2
};

uint8_t ConeGrid[] PROGMEM = {
 XX,    44,   XX,XX,
 XX,  42, 43,    XX,
    41, 40, 32,  XX,
 XX,  39, 33,    XX,
    38, 34, 31,  XX,
 XX,  35, 30,    XX,
    36, 29, 20,  XX,
  37, 28, 21, 19,  
    27, 22, 18,  XX,
  26, 23, 17,  9,  
    24, 16,  8,  XX,
  25, 15,  7, 10,  
    14,  6, 11,  XX,
 XX,   5, 12,    XX,
     4, 13,  0,  XX,
 XX,   3,  1,    XX,
 XX,     2,   XX,XX,
};

uint8_t neighbors[] PROGMEM = {
  XX,XX,XX,1,13,12, // 0
  0,XX,XX,2,3,13,
  1,XX,XX,XX,XX,3,
  13,1,2,XX,XX,4,
  5,13,3,XX,XX,XX, // 4
  6,12,13,4,XX,14,
  7,11,12,5,14,15,
  8,10,11,6,15,16,
  9,XX,10,7,16,17, // 8
  XX,XX,XX,8,17,18,
  XX,XX,XX,11,7,8,
  10,XX,XX,12,6,7,
  11,XX,0,13,5,6, // 12
  12,0,1,3,4,5,
  15,6,5,XX,XX,25,
  16,7,6,14,25,24,
  17,8,7,15,24,23, // 16
  18,9,8,16,23,22,
  19,XX,9,17,22,21,
  XX,XX,XX,18,21,20,
  XX,XX,19,21,29,30, // 20
  20,19,18,22,28,29,
  21,18,17,23,27,28,
  22,17,16,24,26,27,
  23,16,15,25,XX,26, // 24
  24,15,14,XX,XX,XX,
  27,23,24,XX,XX,XX,
  28,22,23,26,XX,37,
  29,21,22,27,37,36, // 28
  30,20,21,28,36,35,
  31,XX,20,29,35,34,
  XX,XX,XX,30,34,33,
  XX,XX,XX,33,40,43, // 32
  32,XX,31,34,39,40,
  33,31,30,35,38,39,
  34,30,29,36,XX,38,
  35,29,28,37,XX,XX, // 36
  36,28,27,XX,XX,XX,
  39,34,35,XX,XX,XX,
  40,33,34,38,XX,41,
  43,32,33,39,41,42, // 40
  42,40,39,XX,XX,XX,
  44,43,40,41,XX,XX,
  XX,XX,32,40,42,44,
  XX,XX,43,42,XX,XX, // 44
};

const uint8_t PatternLookUp[45] = { 41,43,44,42,39,37,35,32,29,26,
                           33,36,38,40,34,31,28,25,22,19,
                           15,18,21,24,27,30,23,20,17,14,
                           12,10,5,7,9,11,13,16,8,6,
                           4,3,2,1,0 };

const uint8_t Stripe_Pattern[45] PROGMEM = {
       0,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
       0
};

const uint8_t Section_Pattern[45] PROGMEM = {
       2,
     2,  2,
   0,  0,  0,
     0,  0,
   0,  0,  0,
     0,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
     0,  0,
   2,  0,  2,
     2,  2,
       2
};

const uint8_t Explode_Pattern[45] PROGMEM = {
       5,
     5,  5,
   5,  4,  5,
     4,  4,
   4,  3,  4,
     3,  3,
   3,  2,  3,
 3,  1,  1,  3,
   2,  0,  2,
 3,  1,  1,  3,
   3,  2,  3,
 4,  3,  3,  4,
   4,  3,  4,
     4,  4,
   5,  4,  5,
     5,  5,
       5
};

const uint8_t Alternate_Pattern[45] PROGMEM = {
       0,
     1,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
 0,  0,  1,  0,
   1,  0,  0,
 1,  0,  1,  0,
   1,  0,  0,
 0,  0,  0,  1,
   0,  1,  0,
     0,  0,
   1,  0,  1,
     0,  0,
       1
};

const uint8_t SideSide_Pattern[45] PROGMEM = {
       3,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
       3
};

const uint8_t Diag_Pattern[45] PROGMEM = {
       0,
     1,  0,
   2,  1,  0,
     2,  1,
   3,  2,  1,
     3,  2,
   4,  3,  2,
 5,  4,  3,  2,
   5,  4,  3,
 6,  5,  4,  3,
   6,  5,  4,
 7,  6,  5,  4,
   7,  6,  5,
     7,  6,
   8,  7,  6,
     8,  7,
       8
};

const uint8_t ConeSize[45]  = { 5,5,5,5,5,3,5,1,3,5,1,1,5,4,3,4,5,5,6,3,6,2,1,6,6,4,2,6,2,1,3,4,2,4,4,5,3,2,2,3,1,6,3,3,1 };


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setLedMap(ConeLookUp);  // turned off mapping - it's handled explicitly here in update_leds()
    led[i].setCoordMap(MAX_YCOORD, ConeGrid);  // x,y grid of cones
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    shows[i] = Shows(&led[i], i);  // Show library - reinitialized for led mappings
    shows[i].setAsHexagon();
    shows[i].setShowDuration(show_duration);
    shows[i].setShowSpeed(show_speed);
    shows[i].setDelayTime(DELAY_TIME);
    shows[i].setFadeAmount(fade_amount);
    shows[i].setColorSpeedMinMax();
    shows[i].pickCycleDuration();
    shows[i].fillForeBlack();
    led[i].push_frame();
  }
  
  shows[CHANNEL_B].setSmallCycleHalfway();  // Start Channel B offset at halfway through show

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_center = 0;
    hue_width = 124;
  }

  userScheduler.addTask(taskUpdateLeds);
  taskUpdateLeds.enable();
}

//
// loop
//
void loop() {
  userScheduler.execute();  // Essential!
}

//
// update leds - Moved to a task-scheduled event
//
void updateLeds() {
  // Moved to a task-scheduled event

  for (uint8_t i = 0; i < DUAL; i++) {

    switch (current_show[i]) {
  
      case 0:
        shows[i].allOn();
        break;
      case 1:
        shows[i].twoColor();
        break;
      case 2:
        shows[i].lightRunUp();
        break;
      case 3:
        colorsize(i);
        break;
      case 4:
        brightsize(i);
        break;
      case 5:
        stripe(i);
        break;
      case 6:
        alternate(i);
        break;
      case 7:
        diagcolor(i);
        break;
      case 8:
        sidesidecolor(i);
        break;
      case 9:
        explodecolor(i);
        break;
      case 10:
        diagbright(i);
        break;
      case 11:
        sidesidebright(i);
        break;
      case 12:
        explodebright(i);
        break;
      case 13:
        sectioncolor(i);
        break;
      case 14:
        shows[i].morphChain();  // works — lots of flicker
        break;
      case 15:
        shows[i].lightWave();
        break;
      case 16:
        shows[i].randomFlip();  // works
        break;
      case 17:
        shows[i].sawTooth();  // works
        break;
      case 18:
        shows[i].sinelon_fastled();  // works
        break;
      case 19:
        shows[i].bpm_fastled();  // works — fast!
        break;
      case 20:
        shows[i].juggle_fastled();  // works
        break;
      case 21:
        shows[i].bounce();  // fast! dim hue?
        break;
      case 22:
        shows[i].bounceGlowing();  // does not work
        break;
      case 23:
        shows[i].plinko(40);  // works
        break;
      default:
        shows[i].confetti();  // more confetti - longer dim
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }
  morph_channels();  // morph together the 2 leds channels and deposit on to Channel_A
  advance_clocks();  // advance the cycle clocks and check for next show
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].checkCycleClock();
    if (shows[i].hasShowFinished()) {
      next_show(i);
      pick_next_show(i);
    }
  }
}

//
// next_show
//
void next_show(uint8_t i) {
  shows[i].resetAllClocks();
//  log_status(i);  // For debugging
}

//
// pick_next_show
//
void pick_next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
//  log_status(i);  // For debugging
}

boolean is_other_channel_show_zero(uint8_t c) {
  if (c == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

//// Start specialized shows

//
// calcIntensity - use sine wave + cycle + variable to calculate intensity
//
// x ranges from 0 to max_x-1
// output is 0 to max_y
// speed spd (1, 2, 3, etc.) determines the rate of color change
//
uint8_t calcIntensity(uint8_t c, uint8_t x, uint8_t max_x, uint8_t max_y, uint8_t spd) {
  spd = max(1, spd % 10);  // 1–10
  uint8_t intense = map8(sin8(map(x, 0, max_x, 0, 255) + (shows[c].getCycle() * spd)), 0, max_y);
  return intense;
}

//
// colorsize - light each cone according to its cone size
//
void colorsize(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, ConeSize[i]-1, 5, sin8(shows[c].getBackColor()), shows[c].getForeColorSpeed());
    shows[c].setPixeltoHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

//
// brightsize - light just one cone size
//
void brightsize(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, ConeSize[i]-1, 5, 255, shows[c].getForeColorSpeed());
    shows[c].setPixeltoColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

//
// stripe
//
void stripe(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Stripe_Pattern + PatternLookUp[i]) == 0) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoBackColor(i);
    }
  }
}

//
// alternate
//
void alternate(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Alternate_Pattern + PatternLookUp[i])) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoBackColor(i);
    }
  }
}

void diagcolor(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, pgm_read_byte_near(Diag_Pattern + i), 9, 200, shows[c].getForeColorSpeed());
    shows[c].setPixeltoHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void sectioncolor(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, pgm_read_byte_near(Section_Pattern + i), 3, 128, shows[c].getForeColorSpeed());
    shows[c].setPixeltoHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void sidesidecolor(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, pgm_read_byte_near(SideSide_Pattern + i), 7, 255, shows[c].getForeColorSpeed());
    shows[c].setPixeltoHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void explodecolor(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, pgm_read_byte_near(Explode_Pattern + i), 6, 255, shows[c].getForeColorSpeed());
    shows[c].setPixeltoHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void diagbright(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, pgm_read_byte_near(Diag_Pattern + i), 9, 255, shows[c].getForeColorSpeed() * 3);
    shows[c].setPixeltoColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

void sidesidebright(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, pgm_read_byte_near(SideSide_Pattern + i), 7, 255, shows[c].getForeColorSpeed() * 2);
    shows[c].setPixeltoColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

void explodebright(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, pgm_read_byte_near(Explode_Pattern + i), 6, 255, shows[c].getForeColorSpeed() * 2);
    shows[c].setPixeltoColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

//// End specialized shows

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels() {
  boolean update_leds = false;
  uint8_t fract = shows[CHANNEL_A].get_intensity();  // 0–255 between channel A and B

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getCurrFrameColor(i);   // New! Changed Interp to Next. No more Interp frames.
    CHSV color_a = led[CHANNEL_A].getCurrFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = led[CHANNEL_A].narrow_palette(color, hue_center, hue_width, saturation);
    
    // smoothing backstop. smooth constants should be large.
    color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING_SHOWS_HUE, SMOOTHING_SHOWS_VALUE);
    
    if (led_buffer[i].h == color.h && led_buffer[i].s == color.s && led_buffer[i].v == color.v) {
      continue;
    } else {
      leds[i] = color;
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
      update_leds = true;
    }
  }
  
  if (update_leds) { 
    FastLED.show();
  } // Update the display only if something changed
}

//// End DUAL SHOW LOGIC

void setHead(uint8_t channel, uint8_t hue) {
  led[channel].setPixelHue(0, hue);
  led[channel].setPixelHue(1, hue);
  led[channel].setPixelHue(2, hue);
}

//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}

//
// log status
//
void log_status(uint8_t i) {
  Serial.print(F("Channel: "));
  Serial.print(i);
  Serial.print(F(", Show: "));
  Serial.print(current_show[i]);
  Serial.print(F(", Cycle Duration: "));
  Serial.print(shows[i].getCycleDuration());
  Serial.print(F(", Cycle: "));
  Serial.print(shows[i].getCycle());
  Serial.print(F(", Sm Cycle: "));
  Serial.println(shows[i].getSmallCycle());
}

void log_shows() {
  Serial.print(current_show[0]);
  Serial.print(" ");
  Serial.print(shows[CHANNEL_A].get_intensity());
  Serial.print(" ");
  Serial.println(current_show[1]);
}

void log_light(uint8_t i) {
  Serial.print(i);
  Serial.print(F(": "));
  Serial.print(leds[i].r);
  Serial.print(F(" "));
  Serial.print(leds[i].g);
  Serial.print(F(" "));
  Serial.println(leds[i].b);
}
