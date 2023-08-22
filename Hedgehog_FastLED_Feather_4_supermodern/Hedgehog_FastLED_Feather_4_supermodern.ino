#include <FastLED.h>
#include <Led_Modern.h>
#include <Shows_Modern.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

#define FASTLED_ESP32_FLASH_LOCK 1

//
//  Hedgehog - 50 Lights
//
//  Modern Software - a lot to test
//
//  6/16/23
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

#define NUM_LEDS 50

#define XX  255

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

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

// Shows
#define NUM_SHOWS 18
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

// wait times
uint8_t show_duration = 40;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 196;  // 0 = no fading, to 255 = always be fading

Scheduler userScheduler; // to control your personal task
void updateLeds();
Task taskUpdateLeds(TASK_MILLISECOND * DELAY_TIME, TASK_FOREVER, &updateLeds);

// Lookup tables

uint8_t ConeLookUp[50] = {
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
      49
};

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
        shows[i].lightRunUp();  // Does not clear?
        break;
      case 3:
        chevrons(i);
        break;
      case 4:
        hogshell(i);  //slow
        break;
      case 5:
        bullseye(i);  // slow
        break;
      case 6:
        chevronrainbow(i);  // colors shift terribly
        break;
      case 7:
        chevronfill(i);  // fine
        break;
      case 8:
        stripe(i);  // empty?
        break;
      case 9:
        starburst(i);
        break;
      case 10:
        shows[i].morphChain();  // works — lots of flicker
        break;
      case 11:
        shows[i].lightWave();
        break;
      case 12:
        shows[i].randomFlip();
        break;
      case 13:
        shows[i].sawTooth();
        break;
      case 14:
        shows[i].sinelon_fastled();
        break;
      case 15:
        shows[i].bpm_fastled();
        break;
      case 16:
        shows[i].juggle_fastled();
        break;
      default:
        shows[i].confetti();  // more confetti - nothing!
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
//  current_show[i] = random8(NUM_SHOWS);
  shows[i].pickCycleDuration();  
  shows[i].pickRandomColorSpeeds();
  shows[i].fillBlack();
  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  log_status(i);  // For debugging
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
// stripe
//
void stripe(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (get_centerstripe(i)) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoForeBlack(i);
    }
  }
}

//
// bullseye
//
void bullseye(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    shows[c].setPixeltoHue(i, IncColor(shows[c].getForeColor(), sin8((shows[c].getBackColor() / 5) * get_centering(i))));
  }
}

//
// starburst
//
void starburst(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, get_starburstpattern(i), 10, 255, shows[c].getForeColorSpeed() * 3);
    shows[c].setPixeltoColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intense));
  }
}

//
// chevrons
//
void chevrons(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, get_chevpattern(i), 10, 255, shows[c].getForeColorSpeed() * 3);
    shows[c].setPixeltoColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intense));
  }
}

//
// chevronrainbow
//
void chevronrainbow(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (shows[c].isCycleStart()) {
    for (int i=0; i < NUM_LEDS; i++) {
      uint8_t change = calcIntensity(c, get_chevpattern(i), 10, 255, shows[c].getForeColorSpeed());
      shows[c].setPixeltoHue(i, IncColor(shows[c].getForeColor(), change));
    }
  }
}

//
// chevronfill
//
void chevronfill(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  uint8_t pos = shows[c].getCycle() % (11 * 2);  // Where we are in the show
  if (pos >= 11) {
    pos = (11 * 2) - pos;  // For a sawtooth effect
  }
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (get_chevpattern(i) < pos) {
      shows[c].setPixeltoForeBlack(i);
    } else {
      shows[c].setPixeltoForeColor(i);
    }
  }
}

//
// hogshell
//
void hogshell(uint8_t c) {  // c = channel
  if (shows[c].isShowStart()) {
    shows[c].turnOnMorphing();
  }

  if (!shows[c].isCycleStart()) { return; }  // Only draw at the start of a cycle
  
  uint8_t s[4] = { 2, 10, 50, 255 };
  
  for (int i=0; i < NUM_LEDS; i++) {
    shows[c].setPixeltoColor(i, CHSV(shows[c].getForeColor(), 255, s[get_centering(i)]));
  }
}


// centerstripe: a center stripe
uint8_t get_centerstripe(uint8_t i) {
  uint8_t centerstripe[50] = {
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
  return centerstripe[i % 50];
}

// starburstpattern: from center to edge
uint8_t get_starburstpattern(uint8_t i) {
  uint8_t starburstpattern[50] = {
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
  return starburstpattern[i % 50];
}

uint8_t get_chevpattern(uint8_t i) {
  uint8_t chevpattern[50] = {
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
  return chevpattern[i % 50];
}

// Centering: creates a center bright ring
uint8_t get_centering(uint8_t i) {
  uint8_t centering[50] = {
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
  return centering[i % 50];
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
      leds[i] = color;   // put HSV color on to LEDs
      led_buffer[i] = color;  // push new color into led_buffer (both HSV)
      update_leds = true;
    }
  }

  if (update_leds) { 
    FastLED.show();
  }
}

//// End DUAL SHOW LOGIC

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
