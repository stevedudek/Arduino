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
//
//  9/10/2018
//
#define NUM_LEDS 55

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library

#define CAN_CHANGE_PALETTES true  // Palettes

// Shows
#define NUM_SHOWS 11
uint8_t current_show = 7;

// Clocks and time
#define SHOW_DURATION 30  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 1   // seconds to fade out
#define FADE_IN_TIME 1    // seconds to fade out

// noise
#define HAVE_NOISE false    // set to false to suppress noise
Noise noise = Noise(NUM_LEDS);

// symmetries - 6 arms - 9 leds per arm - center is +1
#define NUM_SYMMETRIES 5
uint8_t symmetry = 4;  // 0-4
boolean invert = false;  // Whether to invert odd arms
const uint8_t symmetries[] = { 1, 2, 3, 6, 6 };
const uint8_t numSymLeds[] = { 55, 28, 19, 10, 7 };  // (54 / symmmetries) + 1

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
  
  if (HAVE_NOISE) {
    noise.turnNoiseOn();
  } else {
    noise.turnNoiseOff();
  }

  if (CAN_CHANGE_PALETTES) {
    led.setPalette();  // turns on palettes with default valets
  }

  // Set up the various mappings here (1D lists in PROGMEM)
  //  led.setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  shows = Shows(&led);  // Show library - reinitialized for led mappings
  
  led.fillBlack();
  led.push_frame();

  set_symmetry();
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
}

//
// next_show
//
void next_show() {
  set_symmetry();
  current_show = random8(NUM_SHOWS);
  led.fillBlack();
  led.push_frame();
  shows.resetAllClocks();
  noise.setRandomNoiseParams();
  shows.pickRandomColorSpeeds();
  shows.pickRandomWait();
  led.randomizePalette(); 
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
// set_symmetry - pick a random symmetry for the duration of the show
//
void set_symmetry() {
  symmetry = random8(NUM_SYMMETRIES); // 0-4
  shows.resetNumLeds(numSymLeds[symmetry]);  // resets the virtual number of LEDs
  invert = (random8(2)) ? true : false;
}

//
// update_leds - push the interp_frame on to the leds - adjust for SYMMETRY!
//
void update_leds() {
  draw_center();
  draw_hub();
  draw_spokes();
}

//
// draw_center - fill in the center circle - pixel 54 maps to virtual led 0
//
void draw_center() {
  leds[54] = led.getInterpFrameColor(0);
}

//
// draw_hub - fill in the 6 petals around the center (48-53) with virtual leds (1+)
//
void draw_hub() {
  uint8_t pitch = 6 / symmetries[symmetry];
  for (uint8_t i = 0; i < pitch; i++) {
    for (uint8_t j = 0; j < symmetries[symmetry]; j++) {
      leds[48 + i + (j * pitch)] = led.getInterpFrameColor(1 + i);
    }
  }
}

//
// draw_spokes - draw the six 8-pixel spokes
//
// spoke virtual leds start at 7
// actual leds are 0-47
//
void draw_spokes() {
  uint8_t adj_i, pitch;
  
  if (symmetry < 4) {
    pitch = 48 / symmetries[symmetry];
    
    for (uint8_t i = 0; i < pitch; i++) {      
      for (uint8_t j = 0; j < symmetries[symmetry]; j++) {
        adj_i = (invert && (j % 2) && (symmetry == 2 || symmetry == 6)) ? ((pitch / 2) - i) % pitch : i;
        leds[shift_one(adj_i + (j * pitch))] = led.getInterpFrameColor(7 + i);
      }
    }
  } else {  // For 12-fold symmetry
    for (uint8_t i = 0; i < 5; i++) {
      for (uint8_t spoke = 0; spoke < 6; spoke++) {
        leds[shift_one((8 - i) + (spoke * 8))] = led.getInterpFrameColor(7 + i);
        leds[shift_one(i + (spoke * 8))] = led.getInterpFrameColor(7 + i);        
      }
    }
  }
}

//
// shift_one - shift the LED one over to fix in software the hardware layout
//
uint8_t shift_one(uint8_t i) {
  return (i + 47) % 48;
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
