#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
//
//  Hedgehog - 50 Lights
//
//  10/25/18
//
//  Dual shows - Blend together 2 shows running at once
//
//  Try to save on memory. +1 giant Led object, +1 shows object.
//
//  Removed: Noise, Shuffle shows
//
#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 50

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // Hook for FastLED library

// Shows
#define NUM_SHOWS 16
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

// Palettes
#define CAN_CHANGE_PALETTES false
#define PALETTE_DURATION 300  // seconds between palettes

// wait times
#define SHOW_DURATION 30  // seconds
#define FADE_TIME 5   // seconds to fade in + out
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// Lookup tables

const uint8_t ConeLookUp[50] PROGMEM= {
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

// Centering: creates a center bright ring
const uint8_t centering[50] PROGMEM = {
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
const uint8_t chevpattern[50] PROGMEM = {
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
const uint8_t starburstpattern[50] PROGMEM = {
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
const uint8_t centerstripe[50] PROGMEM = {
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

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  if (CAN_CHANGE_PALETTES) {
    for (uint8_t i = 0; i < DUAL; i++) {
      led[i].setPalette();  // turns on palettes with default values
    }
  }
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setLedMap(ConeLookUp);  // turned off mapping - it's handled explicitly here in update_leds()
    led[i].fillBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].setWaitRange(4, 20, 16);
  }
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  
}

void loop() { 
  
  for (uint8_t i = 0; i < DUAL; i++) {

    if (get_intensity(i) > 0) {
      
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
          chevrons(i);
          break;
        case 4:
          hogshell(i);
          break;
        case 5:
          bullseye(i);
          break;
        case 6:
          chevronrainbow(i);
          break;
        case 7:
          stripe(i);
          break;
        case 8:
          chevronfill(i);
          break;
        case 9:
          starburst(i);
          break;
        case 10:
          shows[i].morphChain();
          break;
        case 11:
          shows[i].lightWave();
          break;
        case 12:
          shows[i].sinelon_fastled();
          break;
        case 13:
          shows[i].bpm_fastled();
          break;
        case 14:
          shows[i].juggle_fastled();
          break;
        default:
          shows[i].bands();
          break;
      }
  
      shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
    }
  }

  update_leds();  // morph together the 2 chanels & push the interp_frame on to the leds
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
  
  delay(DELAY_TIME); // The only delay  
}


//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { 
      next_show(i); 
    }
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  // current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For testing
  
  led[i].fillBlack();
  led[i].push_frame();
  led[i].randomizePalette();
  
  shows[i].resetAllClocks();
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
}

//// Start specialized shows

//
// calcIntensity - use sine wave + cycle + variable to calculate intensity
//
// x ranges from 0 to max_x-1
// output is 0 to max_y
// speed spd (1, 2, 3, etc.) determines the rate of color change
//
uint8_t calcIntensity(uint8_t channel, uint8_t x, uint8_t max_x, uint8_t max_y, uint8_t spd) {
  uint8_t intense = map8(sin8_avr(map(x, 0, max_x, 0, 255) + (shows[channel].getCycle() * spd)), 0, max_y);
  return intense;
}

//
// stripe
//
void stripe(uint8_t c) {  // c = channel
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(centerstripe + i)) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoBlack(i);
    }
  }
}

//
// bullseye
//
void bullseye(uint8_t c) {  // c = channel
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), sin8((shows[c].getBackColor() / 5) * pgm_read_byte_near(centering + i))));
  }
}

//
// starburst
//
void starburst(uint8_t c) {  // c = channel
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(c, pgm_read_byte_near(starburstpattern + i), 10, 255, shows[c].getForeColorSpeed() * 3);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intense));
  }
}

//
// chevrons
//
void chevrons(uint8_t c) {  // c = channel
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(c, pgm_read_byte_near(chevpattern + i), 10, 255, shows[c].getForeColorSpeed() * 3);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intense));
  }
}

//
// chevronrainbow
//
void chevronrainbow(uint8_t c) {  // c = channel
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(c, pgm_read_byte_near(chevpattern + i), 10, 255, shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

//
// chevronfill
//
void chevronfill(uint8_t c) {  // c = channel
  uint8_t pos, x;
  pos = shows[c].getCycle() % (11 * 2);  // Where we are in the show
  if (pos >= 11) {
    pos = (11 * 2) - pos;  // For a sawtooth effect
  }
  
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    x = pgm_read_byte_near(chevpattern + i);
    if (x < pos) {
      shows[c].setPixeltoBlack(i);
    } else {
      shows[c].setPixeltoForeColor(i);
    }
  }
}

//
// hogshell
//
void hogshell(uint8_t c) {  // c = channel
  uint8_t s[4] = { 2, 10, 50, 255 };
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t x = pgm_read_byte_near(centering + i);
    led[c].setPixelColor(i, CHSV(shows[c].getForeColor(), 255, s[x]));
  }
}

//// End specialized shows

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  uint8_t intensity_a = get_intensity(CHANNEL_A);
  uint8_t intensity_b = get_intensity(CHANNEL_B);
  
  if (intensity_a == 0) {
    update_leds_one_channel(CHANNEL_B);  // Channel A is off. Use only Channel B.
  } else if (intensity_b == 0) {
    update_leds_one_channel(CHANNEL_A);  // Channel B is off. Use only Channel A.
  } else {
    morph_channels(ease8InOutQuad(intensity_a));  // Interpolate channels
  }
}

//
// update_leds_one_channel - push the a single channel's interp_frame on to the leds
//
void update_leds_one_channel(uint8_t channel) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = led[channel].getInterpFrameColor(i);
  }
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = led[CHANNEL_A].getInterpHSV(led[CHANNEL_B].getInterpFrameColor(i), 
                                          led[CHANNEL_A].getInterpFrameColor(i), 
                                          fract);  // interpolate a + b channels
  }
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();

  // Similar logic to check_fades (deprecated)
  if (small_cycle <= FADE_CYCLES) {
    intensity = map(small_cycle, 0, FADE_CYCLES, 0, 255);  // rise
  } else if (small_cycle <= (MAX_SMALL_CYCLE / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((MAX_SMALL_CYCLE / 2) + FADE_CYCLES)) {
    intensity = map(small_cycle - (MAX_SMALL_CYCLE / 2), 0, FADE_CYCLES, 255, 0);  // decay
  } else {
    intensity = 0;
  }
  return intensity;
}
   
//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}
