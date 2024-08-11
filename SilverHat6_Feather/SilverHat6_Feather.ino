#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Linear Lights with FastLED
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  10/3/20
//
//  Modernized
//
#define NUM_LEDS 35  // Chance of memory shortage for large NUM_LEDS

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

#define MIN_VALUE 10

uint8_t show_speed = 196;  // (0 = fast, 255 = slow)

uint8_t DELAY_TIME = 30;  // in milliseconds (ArduinoBlue)
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 0  // 7
#define CLOCK_PIN 2  //  8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[NUM_LEDS];  // Hook for FastLED library

#define ONLY_BLUE true
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;
CHSV dim_blue = CHSV(160, 255, MIN_VALUE);

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define START_SHOW_CHANNEL_A  1  // Channels A starting show
#define START_SHOW_CHANNEL_B  0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 13

// wait times
uint8_t show_duration = 60;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 255; // 148;  // 0 = no fading, to 255 = always be fading
uint32_t max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // *2! 50% = all on, 50% = all off, and rise + decay on edges
uint32_t last_time = 0;

// Order of spines
const uint8_t SpineOrder[35] = {
        10,
   11,  20,   9,
     19,   21,
  12,18,23,22,8,
  13,17,24,25,7,
  14,16,27,26,6,
     15,28,5,
        29,
       30,4,
       31,3,
       32,2,
       33,1,
       34,0,
};

// Tail pattern
const uint8_t Tails[35] = {
         0,
    0,   0,   0,
      0,    0,
   0, 0, 0, 0,0,
   0, 0, 0, 0,0,
   0, 0, 0, 0,0,
      0, 0, 0,
        1,
       2,2,
       3,3,
       4,4,
       5,5,
       6,6,
};

// Hood Ring
const uint8_t Hood[35] = {
         1,
    2,   0,   2,
      0,    0,
   3, 0, 0, 0,3,
   4, 0, 0, 0,4,
   5, 0, 0, 0,5,
      6, 0, 6,
         0,
        0,0,
        0,0,
        0,0,
        0,0,
        0,0,
};

// Stripe Patten
const uint8_t Stripes[35] = {
         1,
    0,   2,   0,
      0,    0,
   0, 0, 3, 0,0,
   0, 0, 4, 0,0,
   0, 0, 5, 0,0,
      0, 6, 0,
         7,
        8,8,
        9,9,
        10,10,
        11,11,
        12,12,
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);  // Only 1 leds object
  FastLED.setBrightness( bright );

  // Set up the various mappings here (1D lists in PROGMEM)
  //  for (uint8_t i = 0; i < DUAL; i++) {
  //    led[i].setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  //    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
  //  }

  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);  // Start Channel B offset at halfway through show
  last_time = millis();
  
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();
  }

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  if (ONLY_BLUE) {  // (ArduinoBlue)
    hue_center = 190;
    hue_width = 80;
  }
}

void loop() {

  for (uint8_t i = 0; i < DUAL; i++) {
      
    switch (current_show[i]) {
    
      case 0:
        shows[i].allOn();
        break;
      case 1:
        shows[i].morphChain();
        break;
      case 2:
        shows[i].twoColor();
        break;
      case 3:
        shows[i].lightWave();
        break;
      case 4:
        shows[i].sawTooth();
        break;
      case 5:
        shows[i].lightRunUp();
        break;
      case 6:
        shows[i].sinelon_fastled();
        break;
      case 7:
        shows[i].bpm_fastled();
        break;
      case 8:
        shows[i].juggle_fastled();
        break;
      case 9:
        lighttails(i);
        break;
      case 10:
        lighthood(i);
        break;
      case 11:
        centergradient(i);
        break;
      default:
        centerpulse(i);
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
  fixed_delay();
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
    if (shows[i].getSmallCycle() >= max_small_cycle) { 
      next_show(i); 
    }
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  
  led[i].push_frame();
  
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(show_speed);
  shows[i].setColorSpeedMinMax(show_speed);
}

//
// Specialized Shows
//

//
// lighttails: turn on the tail lights to a gradient
// 
void lighttails(uint8_t c) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (Tails[i] == 0) {
      shows[c].setPixeltoForeBlack(SpineOrder[i]);
    } else {
      shows[c].setPixeltoColor(SpineOrder[i], led[c].wheel(shows[i].getForeColor() + (10 * Tails[i])));
    }
  }
}

//
// lighthood: ring the hood with one color; fill everything else with another color
//
void lighthood(uint8_t c) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (Hood[i] == 0) {
      shows[c].setPixeltoForeBlack(i);
    } else {
      shows[c].setPixeltoColor(i, led[c].wheel(shows[c].getForeColor() + (10 * Hood[i])));
    }
  }
}

//
// centergradient: turn on the central spine with a light gradient
//
 
void centergradient(uint8_t c) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (Stripes[i] == 0) {
      shows[c].setPixeltoForeBlack(i);
    } else {
      shows[c].setPixeltoColor(i, led[c].wheel(shows[c].getForeColor() + (10 * Stripes[i])));
    }
  }
}

//
// centerpulse - One pixel travelling down the center spine
//
 
void centerpulse(uint8_t c) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (Stripes[i] == (1 + (shows[c].getCycle() % 12))) {
      shows[c].setPixeltoForeColor(i);
    } else {
      shows[c].setPixeltoForeBlack(i);
    }
  }
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getInterpFrameColor(i);
    CHSV color_a = led[CHANNEL_A].getInterpFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    
    color = narrow_palette(color);

    if (color.v < MIN_VALUE) {
      color = dim_blue;
    }
    
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }

    leds[SpineOrder[i]] = color;
    led_buffer[i] = color;
  }
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();
  uint16_t fade_cycles =  (map8(fade_amount, 0, show_duration) * 1000 / DELAY_TIME);  // cycles to fade in + out

  if (small_cycle < fade_cycles) {
    intensity = map(small_cycle, 0, fade_cycles, 0, 255);  // rise
  } else if (small_cycle <= (max_small_cycle / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((max_small_cycle / 2) + fade_cycles)) {
    intensity = map(small_cycle - (max_small_cycle / 2), 0, fade_cycles, 255, 0);  // decay
  } else {
    intensity = 0;
  }

  return ease8InOutApprox(intensity);
}

//// End DUAL SHOW LOGIC

//
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  uint8_t h1 = (hue_center - (hue_width / 2)) % MAX_COLOR;
  uint8_t h2 = (hue_center + (hue_width / 2)) % MAX_COLOR;
  color.h = map8(color.h, h1, h2 );
  if (color.s != 0) {
    color.s = saturation;  // Reset saturation
  }
  return color;
}
