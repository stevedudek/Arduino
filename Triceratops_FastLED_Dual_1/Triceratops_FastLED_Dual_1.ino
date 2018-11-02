#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Triceratops - 45 cones on a coat
//
//  10/28/18
//
//  Dual shows - Blend together 2 shows running at once
//
//  Try to save on memory. +1 giant Led object, +1 shows object.
//
//  Removed: Noise, Shuffle shows
//
//  AT THE EDGE OF FULL MEMORY
//  Adding in 2D Shows via (Shows:HAS_2D_SHOWS == true) may crash
//  Changed rgb_to_hsv from floats to uint8_t's. May help clear the stack.
//  Going forward, either:
//  1. Remove 2D Shows (USE Triceratops_FastLED_Dual_2)
//  2. Revert to non-dual show functionality (USE Triceratops_FastLED_17)
//  3. Switch hardware to the Teensy(USE Triceratops_FastLED_Dual_1)
//
#define BRIGHTNESS  255  // (0-255)

#define DELAY_TIME 40  // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 45

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define MAX_XCOORD  4
#define MAX_YCOORD  17

#define MAX_COLOR 128   // Colors are 0-255

#define CAN_CHANGE_PALETTES false  // Palettes don't work well with 2 channels

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // Hook for FastLED library

// Shows
#define NUM_SHOWS 21
#define START_SHOW_CHANNEL_A  16  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  17
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

// wait times
#define SHOW_DURATION 30  // seconds
#define FADE_TIME 10   // seconds to fade in + out. 1/3rd duty cycles of fade in, show, fade out.
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// Lookup tables

const uint8_t ConeLookUp[] PROGMEM = {
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

const int8_t ConeGrid[] PROGMEM = {
 -1,    44,   -1,-1,
 -1,  42, 43,    -1,
    41, 40, 32,  -1,
 -1,  39, 33,    -1,
    38, 34, 31,  -1,
 -1,  35, 30,    -1,
    36, 29, 20,  -1,
  37, 28, 21, 19,  
    27, 22, 18,  -1,
  26, 23, 17,  9,  
    24, 16,  8,  -1,
  25, 15,  7, 10,  
    14,  6, 11,  -1,
 -1,   5, 12,    -1,
     4, 13,  0,  -1,
 -1,   3,  1,    -1,
 -1,     2,   -1,-1,
};

const int8_t neighbors[] PROGMEM = {
  -1,-1,-1,1,13,12, // 0
  0,-1,-1,2,3,13,
  1,-1,-1,-1,-1,3,
  13,1,2,-1,-1,4,
  5,13,3,-1,-1,-1, // 4
  6,12,13,4,-1,14,
  7,11,12,5,14,15,
  8,10,11,6,15,16,
  9,-1,10,7,16,17, // 8
  -1,-1,-1,8,17,18,
  -1,-1,-1,11,7,8,
  10,-1,-1,12,6,7,
  11,-1,0,13,5,6, // 12
  12,0,1,3,4,5,
  15,6,5,-1,-1,25,
  16,7,6,14,25,24,
  17,8,7,15,24,23, // 16
  18,9,8,16,23,22,
  19,-1,9,17,22,21,
  -1,-1,-1,18,21,20,
  -1,-1,19,21,29,30, // 20
  20,19,18,22,28,29,
  21,18,17,23,27,28,
  22,17,16,24,26,27,
  23,16,15,25,-1,26, // 24
  24,15,14,-1,-1,-1,
  27,23,24,-1,-1,-1,
  28,22,23,26,-1,37,
  29,21,22,27,37,36, // 28
  30,20,21,28,36,35,
  31,-1,20,29,35,34,
  -1,-1,-1,30,34,33,
  -1,-1,-1,33,40,43, // 32
  32,-1,31,34,39,40,
  33,31,30,35,38,39,
  34,30,29,36,-1,38,
  35,29,28,37,-1,-1, // 36
  36,28,27,-1,-1,-1,
  39,34,35,-1,-1,-1,
  40,33,34,38,-1,41,
  43,32,33,39,41,42, // 40
  42,40,39,-1,-1,-1,
  44,43,40,41,-1,-1,
  -1,-1,32,40,42,44,
  -1,-1,43,42,-1,-1, // 44
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
    led[i].setCoordMap(MAX_YCOORD, ConeGrid);  // x,y grid of cones
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    led[i].fillBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
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
          shows[i].plinko(40);
          break;
        case 14:
          shows[i].bounce();
          break;
        case 15:
          shows[i].bounceGlowing();
          break;
        case 16:
          sectioncolor(i);
          break;
        case 17:
          shows[i].bands();
          break;
        case 18:
          shows[i].packets();
          break;
        case 19:
          shows[i].packets_two();
          break;
        default:
          shows[i].lightWave();
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
// colorsize - light each cone according to its cone size
//
void colorsize(uint8_t c) {  // c = channel
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(c, ConeSize[i]-1, 5, sin8_avr(shows[c].getBackColor()), shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

//
// brightsize - light just one cone size
//
void brightsize(uint8_t c) {  // c = channel
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(c, ConeSize[i]-1, 5, 255, shows[c].getForeColorSpeed());
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

//
// stripe
//
void stripe(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
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
  for (int i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Alternate_Pattern + PatternLookUp[i])) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoBackColor(i);
    }
  }
}

void diagcolor(uint8_t c) {  // c = channel
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(c, pgm_read_byte_near(Diag_Pattern + i), 9, 200, shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void sectioncolor(uint8_t c) {  // c = channel
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(c, pgm_read_byte_near(Section_Pattern + i), 3, 128, shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void sidesidecolor(uint8_t c) {  // c = channel
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(c, pgm_read_byte_near(SideSide_Pattern + i), 7, 256, shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void explodecolor(uint8_t c) {  // c = channel
  uint8_t change;
  for (int i=0; i < NUM_LEDS; i++) {
    change = calcIntensity(c, pgm_read_byte_near(Explode_Pattern + i), 6, 256, shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

void diagbright(uint8_t c) {  // c = channel
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(c, pgm_read_byte_near(Diag_Pattern + i), 9, 255, shows[c].getForeColorSpeed() * 3);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

void sidesidebright(uint8_t c) {  // c = channel
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(c, pgm_read_byte_near(SideSide_Pattern + i), 7, 255, shows[c].getForeColorSpeed() * 2);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

void explodebright(uint8_t c) {  // c = channel
  uint8_t intense;
  for (int i=0; i < NUM_LEDS; i++) {
    intense = calcIntensity(c, pgm_read_byte_near(Explode_Pattern + i), 6, 255, shows[c].getForeColorSpeed() * 2);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getForeColor(), intense));
  }
  setHead(c, shows[c].getForeColor());
}

//// End specialized shows

//
// lookup_leds - convert cones into LEDs - this is where mapping happens
//
int8_t lookup_leds(uint8_t i) {
  return pgm_read_byte_near(ConeLookUp + i);
}

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
