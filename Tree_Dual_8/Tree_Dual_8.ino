#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>

//
//  Linear Lights with FastLED
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  Removed: Noise, palettes, 2D shows (need to do this on Show repository)
//
//  10/27/2020
//
//  NOTE: YOU WILL NEED TO CREATE 16-bit Led and Show Libraries to run this
//  uint8_t -> uint16_t all over the place
//
#define NUM_LEDS 1170  // A lot!
#define TOTAL_LEDS 2340   // even more!!

uint8_t BRIGHTNESS = 96;  // (0-255)

uint8_t DELAY_TIME = 40;  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
uint32_t timing = millis();  // last_time
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 7

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[TOTAL_LEDS];  // The Leds themselves
CHSV led_buffer[NUM_LEDS];  // For smoothing

// Shows
#define START_SHOW_CHANNEL_A  3  // Channels A starting show
#define START_SHOW_CHANNEL_B  1  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 12

// ArduinoBlue
#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;
#define MAX_COLOR 256   // Colors are 0-255

// Clocks and time
#define SHOW_DURATION 30  // seconds
uint8_t FADE_TIME = 15;  // seconds to fade in. If FADE_TIME = SHOW_DURATION, then Always Be Fading
#define MAX_SMALL_CYCLE  (SHOW_DURATION * 2 * (1000 / DELAY_TIME))  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// ArduinoBlue
ArduinoBlue phone(Serial2); // Blue Tx = pin 9; Blue Rx = pin 10
#define HUE_SLIDER        0
#define HUE_WIDTH_SLIDER  1
#define SPEED_SLIDER      2
#define BRIGHTNESS_SLIDER 3
#define FADE_TIME_SLIDER  4
#define BAM_BUTTON        0
#define BOLT_TIME        20


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");
  Serial2.begin(9600);  // Serial2: Bluetooth serial (ArduinoBlue)

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, TOTAL_LEDS);  // Only 1 leds object
  FastLED.setBrightness( BRIGHTNESS );
  
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].turnOnBlur();
    shows[i].fillForeBlack();
    led[i].push_frame();
  }
  
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);
  timing = millis();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 255, 0);
  }

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_start = 192;
    hue_width = 124;
  }
}

void loop() {

  for (uint8_t i = 0; i < DUAL; i++) {

    if (get_intensity(i) > 0) {
      
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
          shows[i].packets();
          break;
        case 7:
          shows[i].packets_two();
          break;
        case 8:
          shows[i].sinelon_fastled();
          break;
        case 9:
          shows[i].bpm_fastled();
          break;
        case 10:
          shows[i].juggle_fastled();
          break;
        default:
          shows[i].bands();
          break;
      }
      shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
    }
  }
  
  check_phone();  // Check the phone settings (ArduinoBlue)
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  fixed_delay();
  advance_clocks();  // advance the cycle clocks and check for next show 
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
// fixed_delay - make every cycle the same time
//
void fixed_delay() {
  long new_time = millis();
  long time_delta = new_time - timing;  // how much time has elapsed? Usually 3-5 milliseconds
  timing = new_time;  // update the counter
  if (time_delta < DELAY_TIME) {  // if we have excess time,
    delay(DELAY_TIME - time_delta);  // delay for the excess
  }
}


//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  
  led[i].fillBlack();
  led[i].push_frame();
  
  shows[i].resetAllClocks();
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color = led[CHANNEL_A].getInterpHSV(led[CHANNEL_B].getInterpFrameColor(i),
                                             led[CHANNEL_A].getInterpFrameColor(i),
                                             fract);  // interpolate a + b channels
    color = lightning(narrow_palette(color));  // (ArduinoBlue)

    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }
    led_buffer[i] = color;
    
    for (uint8_t x = 0; x < 2; x++) {
      uint16_t n = i + (NUM_LEDS * x);

      for (uint8_t m = 0; m < 15; m += 2) {
        if (n <= lookup_small_map(m)) {
          n += lookup_small_map(m+1);
//          Serial.println(n);
          break;
        }
      }
      leds[n] = color;
      leds[get_paired_led(n)] = color;
    }
  }
}

uint16_t lookup_small_map(uint8_t i) {
  uint16_t SMALL_MAP[] = { 141, 0, 161, 20, 209, 68, 229, 88, 315, 174, 335, 194, 383, 242, 403, 262 };
  return SMALL_MAP[i];
}

//
// get_paired_led
//
uint16_t get_paired_led(uint16_t n) {
  // rather expensive way to get the paired led
  uint16_t LARGE_MAP[] = { 0, 55, 752, 807, 56, 93, 366, 403,
                         94, 121, 202, 229, 122, 141, 142, 161,
                         162, 181, 182, 201, 230, 257, 338, 365,
                         258, 277, 278, 297, 298, 317, 318, 337,
                         404, 441, 714, 751, 442, 469, 550, 577,
                         470, 489, 490, 509, 510, 529, 530, 549,
                         578, 605, 686, 713, 606, 625, 626, 645,
                         646, 665, 666, 685 };
                         
  for (uint8_t m = 1; m < 60; m += 4) {
    if (n <= LARGE_MAP[m]) {
      return (LARGE_MAP[m] - n + LARGE_MAP[m+1]);
    }
  }
  return n;
}

//
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  color.h = map8(color.h, hue_start, (hue_start + hue_width) % MAX_COLOR );
  return color;
}

//
// lightning - ramp all pixels quickly up to white (down sat & up value) and back down
//
CHSV lightning(CHSV color) {  // (ArduinoBlue)
  if (curr_lightning > 0) {
    uint8_t increase = 255 - cos8( map(curr_lightning, 0, BOLT_TIME, 0, 255));
    color.s -= increase;
    color.v += increase;
  }
  return color;
}

//
// check_phone - poll the phone for updated values  (ArduinoBlue)
//
void check_phone() {
  int8_t sliderId = phone.getSliderId();  // ID of the slider moved
  int8_t buttonId = phone.getButton();  // ID of the button

  if (sliderId != -1) {
    int16_t sliderVal = phone.getSliderVal();  // Slider value goes from 0 to 200
    sliderVal = map(sliderVal, 0, 200, 0, 255);  // Recast to 0-255

    switch (sliderId) {
      case BRIGHTNESS_SLIDER:
        BRIGHTNESS = sliderVal;
        FastLED.setBrightness( BRIGHTNESS );
        break;
      case HUE_SLIDER:
        hue_start = sliderVal;
        break;
      case HUE_WIDTH_SLIDER:
        hue_width = sliderVal;
        break;
      case SPEED_SLIDER:
        DELAY_TIME = map8(sliderVal, 10, 100);
        break;
      case FADE_TIME_SLIDER:
        FADE_TIME = map8(sliderVal, 0, SHOW_DURATION);
        break;
    }
  }
  
  if (buttonId == BAM_BUTTON) { curr_lightning = BOLT_TIME; }
}    

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint16_t small_cycle = shows[i].getSmallCycle();
  uint8_t intensity;  // 0 = Off, 255 = full On

  if (small_cycle < FADE_CYCLES) {
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
