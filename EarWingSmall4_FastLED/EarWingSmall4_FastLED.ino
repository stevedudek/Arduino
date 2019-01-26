#include <Noise.h>
#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Linear Lights with FastLED
//
//  Leds from Library
//  Noise from Library
//  Shows from Library
//    pushed variables into Shows.h
//
//  6/20/2018
//
#define NUM_LEDS 28

#define BRIGHTNESS  64 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library

// Palettes
#define CAN_CHANGE_PALETTES true
#define PALETTE_DURATION 300  // seconds between palettes

// Shows - now enumerated
#define NUM_SHOWS 9
uint8_t current_show = 0;

// Clocks and time
#define SHOW_DURATION 20  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 2   // seconds to fade out
#define FADE_IN_TIME 2    // seconds to fade out
#define WAIT_DURATION 20 // second between increasing wait time

// noise
#define HAVE_NOISE false    // set to false to suppress noise
Noise noise = Noise(NUM_LEDS);

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  if (HAVE_NOISE) {
    noise.turnNoiseOn();
  } else {
    noise.turnNoiseOff();
  }

  if (CAN_CHANGE_PALETTES) {
    led.setPalette();  // turns on palettes with default valets
  }

  // Set up the various mappings here (1D lists in PROGMEM)
  //   led.setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  
  set_all_black();
}

void loop() {
  
  switch (current_show) {
  
    case 0:
      shows.allOn();
      break;
    case 1:
      shows.morphChain();
      break;
    case 2:
      shows.randomFill();
      break;
    case 3:
      shows.randomColors();
      break;
    case 4:
      shows.twoColor();
      break;
    case 5:
      shows.lightWave();
      break;
    case 6:
      shows.sawTooth();
      break;
    case 7:
      noisyshow();
      break;
    default:
      shows.bands();
      break;
  }

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay

  noise.fillNoise();
  
  shows.advanceClock();
  
  if (shows.getSmallCycle() >= MAX_SMALL_CYCLE) { 
    next_show(); 
  }
  
  change_it_up();  // change palettes and wait times
}

//
// change_it_up
//
void change_it_up() {
  EVERY_N_SECONDS(PALETTE_DURATION) { 
    led.randomizePalette(); 
  }
  EVERY_N_SECONDS(WAIT_DURATION) { 
    shows.tweakWait();
  }
}

//
// next_show
//
void next_show() {
  current_show = random8(NUM_SHOWS);
  shows.resetAllClocks();
  noise.setRandomNoiseParams();
//  set_all_black();
  shows.tweakColorSpeeds();
}

//
// set_all_black - turn all to black and update leds
//
void set_all_black() {
  led.fillBlack();
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV( 0, 0, 0);
  }
  FastLED.show();
  led.push_frame();
}

//
// noisyshow
//
void noisyshow() {
  // The Shows.h library does not handle Noise, so it is handled here
  if (shows.isShowStart()) { 
    noise.makeVeryNoisy(); 
  }
  shows.allOn();
}
    
//
// morph_frame
//
void morph_frame() {
  shows.morphFrame();  // 1. calculate interp_frame 2. adjust palette
  add_noise();   // Use the noise library
  update_leds();  // push the interp_frame on to the leds
  check_fades();  // Fade start and end of shows
  FastLED.show();  // Update the display 
}

//
// add_noise - from library - uses led.library getters and setters
//
void add_noise() {
  if (HAVE_NOISE) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      led.setInterpFrameHue(i, noise.addNoiseAtoValue(i, led.getInterpFrameHue(i))); 
      led.setInterpFrameSat(i, noise.addNoiseBtoValueNoWrap(i, led.getInterpFrameSat(i)));
    }
  }
}

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = led.getInterpFrameColor(i);
  }
}

//
// check_fades - check the fade-to-blacks at beginning and end of show
//
void check_fades() {
  uint8_t fade_amount = 0;
  uint32_t small_cycle = shows.getSmallCycle();
  
  if (small_cycle <= (FADE_IN_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map(small_cycle, 0, (FADE_IN_TIME * 1000 / DELAY_TIME), 255, 0);
  } else if ((MAX_SMALL_CYCLE - small_cycle) <= (FADE_OUT_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map((MAX_SMALL_CYCLE - small_cycle), 0, (FADE_OUT_TIME * 1000 / DELAY_TIME), 255, 0);
  }
  if (fade_amount > 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(fade_amount);
    }
  }
}
