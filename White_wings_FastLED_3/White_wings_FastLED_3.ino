#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Linear Lights with FastLED
//
//  Leds from Library
//  Shows from Library
//
//  4/15/2019
//
#define NUM_LEDS 84
#define SYM_LEDS (NUM_LEDS / 2)

#define BRIGHTNESS  48 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

Led led = Led(SYM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library

#define CAN_CHANGE_PALETTES true  // Palettes

// Shows
#define NUM_SHOWS 14
uint8_t current_show = 0;

// Clocks and time
#define SHOW_DURATION 30  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 1   // seconds to fade out
#define FADE_IN_TIME 1    // seconds to fade out

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
  
  if (CAN_CHANGE_PALETTES) {
    led.setPalette();  // turns on palettes with default valets
  }

  // Set up the various mappings here (1D lists in PROGMEM)
  //  led.setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  shows = Shows(&led);  // Show library - reinitialized for led mappings
  
  led.fillBlack();
  led.push_frame();
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
      shows.lightRunUp();
      break;
    case 8:
      shows.packets();
      break;
    case 9:
      shows.packets_two();
      break;
    case 10:
      shows.sinelon_fastled();
      break;
    case 11:
      shows.bpm_fastled();
      break;
    case 12:
      shows.juggle_fastled();
      break;
    default:
      shows.bands();
      break;
  }

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay

  shows.advanceClock();
  
  if (shows.getSmallCycle() >= MAX_SMALL_CYCLE) { 
    next_show(); 
  }
}

//
// next_show
//
void next_show() {
  current_show = random8(NUM_SHOWS);
  led.fillBlack();
  led.push_frame();
  shows.resetAllClocks();
  shows.pickRandomColorSpeeds();
  shows.pickRandomWait();
  led.randomizePalette(); 
}
    
//
// morph_frame
//
void morph_frame() {
  shows.morphFrame();  // 1. calculate interp_frame 2. adjust palette
  update_leds();  // push the interp_frame on to the leds
  check_fades();  // Fade start and end of shows
  FastLED.show();  // Update the display 
}

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (int i = 0; i < SYM_LEDS; i++) {
    CHSV color = led.getInterpFrameColor(i);
    leds[i] = color;
    leds[NUM_LEDS - i - 1] = color;
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
