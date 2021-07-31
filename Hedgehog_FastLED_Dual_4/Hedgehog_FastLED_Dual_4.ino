#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
//
//  Hedgehog - 50 Lights
//
//  10/23/20
//
//  Dual shows - Blend together 2 shows running at once
//
//  Try to save on memory. +1 giant Led object, +1 shows object.
//
//  Removed: Noise, Shuffle shows
//
//  Modernized
//
//  TOO BIG TO FIT ON A LILYPAD — USE A TEENSY

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40 // in milliseconds
long last_time;
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 7
#define CLOCK_PIN 8

#define NUM_LEDS 50

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // Hook for FastLED library
CHSV led_buffer[NUM_LEDS];  // For smoothing

// Shows
#define NUM_SHOWS 14
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

// wait times
#define SHOW_DURATION 30  // seconds
#define FADE_TIME 5   // seconds to fade in + out
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

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
  
  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setLedMap(ConeLookUp);  // turned off mapping - it's handled explicitly here in update_leds()
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].setWaitRange(4, 20, 16);
  }
  
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);
  last_time = millis();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }
}

void loop() { 
  
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
        shows[i].juggle_fastled();
        break;
      default:
        shows[i].bands();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
  delay(40);
//  fixed_delay();
}

//
// fixed_delay - make every cycle the same time
//
void fixed_delay() {
  long new_time = millis();
  long time_delta = new_time - last_time;  // how much time has elapsed? Usually 3-5 milliseconds
  last_time = new_time;  // update the counter
  if (time_delta < DELAY_TIME) {  // if we have excess time,
    delay(DELAY_TIME - time_delta);  // delay for the excess
  }
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
  
  led[i].push_frame();
  
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
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
  uint8_t intense = map8(sin8(map(x, 0, max_x, 0, 255) + (shows[channel].getCycle() * spd)), 0, max_y);
  return intense;
}

//
// stripe
//
void stripe(uint8_t c) {  // c = channel
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (get_centerstripe(i)) {
      shows[c].setPixeltoForeBlack(i);
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
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), sin8((shows[c].getBackColor() / 5) * get_centering(i))));
  }
}

//
// starburst
//
void starburst(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, get_starburstpattern(i), 10, 255, shows[c].getForeColorSpeed() * 3);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intense));
  }
}

//
// chevrons
//
void chevrons(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t intense = calcIntensity(c, get_chevpattern(i), 10, 255, shows[c].getForeColorSpeed() * 3);
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), intense));
  }
}

//
// chevronrainbow
//
void chevronrainbow(uint8_t c) {  // c = channel
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t change = calcIntensity(c, get_chevpattern(i), 10, 255, shows[c].getForeColorSpeed());
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), change));
  }
}

//
// chevronfill
//
void chevronfill(uint8_t c) {  // c = channel
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
  uint8_t s[4] = { 2, 10, 50, 255 };
  
  for (int i=0; i < NUM_LEDS; i++) {
    led[c].setPixelColor(i, CHSV(shows[c].getForeColor(), 255, s[get_centering(i)]));
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
void morph_channels(uint8_t fract) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getInterpFrameColor(i);
    CHSV color_a = led[CHANNEL_A].getInterpFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }

    leds[i] = color;
    led_buffer[i] = color;
  }
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();

  if (small_cycle <= FADE_CYCLES) {
    intensity = map(small_cycle, 0, FADE_CYCLES, 0, 255);  // rise
  } else if (small_cycle <= (MAX_SMALL_CYCLE / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((MAX_SMALL_CYCLE / 2) + FADE_CYCLES)) {
    intensity = map(small_cycle - (MAX_SMALL_CYCLE / 2), 0, FADE_CYCLES, 255, 0);  // decay
  } else {
    intensity = 0;
  }
  return ease8InOutQuad(intensity);
}
   
//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}
