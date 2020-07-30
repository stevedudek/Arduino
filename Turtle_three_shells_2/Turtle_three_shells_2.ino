#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>  // (ArduinoBlue)
//
//  Turtle! - 29 Lights in a hexagonal grid
//
//  3 Turtle Shells with a different wiring layout
//
//  6/14/20
//
//  Dual shows - Blend together 2 shows running at once
//

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 30;  // in milliseconds (ArduinoBlue)

#define DATA_PIN 7  // Changed!
#define CLOCK_PIN 8

#define NUM_LEDS 29
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number

#define ACTUAL_LEDS 37  // There are 8 dummy spacer LEDs

#define XX  255

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 11
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

uint8_t current_pattern[] = { 0, 0 };

uint8_t spacer_leds[] = { 11, 12, 13, 14, 28, 29, 31, 34 };

// wait times
#define SHOW_DURATION 15  // seconds
uint8_t FADE_TIME = 2;  // seconds to fade in + out (Arduino Blue)
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
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

// Lookup tables

const uint8_t rewire[] PROGMEM = {
  30,26,27,0,1,  // Should be 30,26,27,0,1 (notice the switch of the first two)
  3,2,25,24,32,
  33,22,23,4,5,
  7,6,21,20,35,
  36,18,19,8,9,
  10,17,16,15
};

const uint8_t neighbors[] PROGMEM = {
  XX,1,8,9,XX,XX,
  XX,2,7,8,0,XX,
  XX,XX,3,7,1,XX,
  XX,XX,4,6,7,2,
  XX,XX,XX,5,6,3, // 4
  4,XX,XX,14,13,6,
  3,4,5,13,12,7,
  2,3,6,12,8,1,
  1,7,12,11,9,0,
  0,8,11,10,XX,XX,
  9,11,18,19,XX,XX,
  8,12,17,18,10,9,
  7,6,13,17,11,8, // 12
  6,5,14,16,17,12,
  5,XX,XX,15,16,13,
  14,XX,XX,24,23,16,
  13,14,15,23,22,17, // 16
  12,13,16,22,18,11,
  11,17,22,21,19,10,
  10,18,21,20,XX,XX,
  19,21,27,XX,XX,XX,
  18,22,26,27,20,19,
  17,16,23,26,21,18, // 22
  16,15,24,25,26,22,
  15,XX,XX,XX,25,23,
  23,24,XX,XX,28,26,
  22,23,25,28,27,21,
  21,26,28,XX,XX,20,
  26,25,XX,XX,XX,27 // 28
};

const uint8_t rings[] PROGMEM = {
  17, XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
  12, 13, 16, 22, 18, 11, XX,XX,XX,XX,XX,XX,
  7, 6, 5, 14, 15, 23, 26, 21, 19, 10, 9, 8,
  0, 1, 2, 3, 4, 24, 25, 28, 27, 20, XX, XX 
};

#define NUM_PATTERNS 8   // Total number of patterns

const uint8_t PatternMatrix[] PROGMEM = {
    1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,
    1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,1,
    1,1,1,1,1,1,2,2,2,1,1,2,1,2,1,1,2,2,2,1,1,2,1,2,1,1,2,1,1,
    1,1,1,1,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,1,1,
    1,1,1,1,1,2,2,2,2,2,2,1,1,1,2,2,1,1,1,2,1,2,1,2,1,1,2,1,1,
    2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,2,
    1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,1,1,1,2,1,2,1,2,1,1,2,1,1,
    1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,2,1,2,1 
};   

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");
  Serial2.begin(9600);  // Serial2: Bluetooth serial (ArduinoBlue)

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS);  // only use of ACTUAL_LEDS
  FastLED.setBrightness( BRIGHTNESS );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    led[i].fillBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    
    shows[i].setColorSpeedMinMax(1,10);  // Make colors change faster (lower = faster)
    shows[i].setWaitRange(2, 20, 18);
    shows[i].setBandsBpm(10, 30);
  }
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_start = 192;
    hue_width = 124;
  }
}

//
// loop
//
void loop() { 
                   
  for (uint8_t i = 0; i < DUAL; i++) {

    switch (current_show[i]) {
  
      case 0:
        patterns(i);
        break;
      case 1:
        warp1(i);
        break;
      case 2:
        warp2(i);
        break;
      case 3:
        rainbow_show(i);
        break;
      case 4:
        shows[i].morphChain();
        break;
      case 5:
        shows[i].bounce();
        break;
      case 6:
        shows[i].bounceGlowing();
        break;
      case 7:
        shows[i].plinko(2);
        break;
      case 8:
        shows[i].packets();
        break;
      case 9:
        shows[i].packets_two();
        break;
      default:
        shows[i].bands();
        break;
    }
    
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  check_phone();  // Check the phone settings (ArduinoBlue)
  update_leds();  // push the interp_frame on to the leds_buffer
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
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
      pick_next_show(i);
      next_show(i);
    }
  }
  
  if (curr_lightning > 0 ) {  // (ArduinoBlue)
    curr_lightning--; // Reduce the current bolt
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  led[i].fillBlack();
  led[i].push_frame();
  
  shows[i].resetAllClocks();
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  current_show[i] = is_other_channel_show_zero(i) ? random8(1, NUM_SHOWS) : 0 ;
  current_pattern[i] = random8(NUM_PATTERNS);
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();

//  log_status(i);  // For debugging
}

boolean is_other_channel_show_zero(uint8_t channel) {
  if (channel == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

//
// patterns shows
//
void patterns(uint8_t c) {
  uint8_t pattern;
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    pattern = pgm_read_byte_near(PatternMatrix + (current_pattern[c] * 29) + i);
 
    switch (pattern) {
      case 0: {        // Off
        led[c].setPixelBlack(i);
        break;
      }
      case 1: {        // always green
        led[c].setPixelColor(i, CHSV(96, 255, 255));
        break;
      }
      case 2: {        // the other color
        led[c].setPixelHue(i, shows[c].getForeColor());
        break;
      }
    }
  }
}

//
// draw_ring
//
void draw_ring(uint8_t i, CHSV color, uint8_t c) {
  uint8_t r;
  for (int j = 0; j < 12; j++) {
    r = pgm_read_byte_near(rings + (i * 12) + j);
    if (r != XX) {
      led[c].setPixelColor(r, color);
    }
  }
}

//
// tunnel vision
//
// Colored ring animating outwards
// color1 is the primary color, colo22 is a trail color
// background is the background color
//
void tunnelvision(CHSV color1, CHSV color2, CHSV background, uint8_t c) {  
  int i = shows[c].getCycle() % 5;
  if (i < 4) { draw_ring(i, color1, c); }      
  if (i != 0) { draw_ring(i-1, color2, c); }
}

//
// warp1 - colors on a black field
// 
void warp1(uint8_t c) {
  switch ((shows[c].getCycle() / 5) % 6) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0, 255, 0)), rgb_to_hsv(CRGB(0,40,0)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,0,255)), rgb_to_hsv(CRGB(0,0,40)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,255,255)), rgb_to_hsv(CRGB(0,40,40)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(40,40,0)), rgb_to_hsv(CRGB(0,0,0)), c);  
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,0,0)), rgb_to_hsv(CRGB(40,0,0)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(255,0,255)), rgb_to_hsv(CRGB(40,0,40)), rgb_to_hsv(CRGB(0,0,0)), c);  
      break;
  }
}

//
// warp2 - colors on a green field
//
void warp2(uint8_t c) {
  switch ((shows[c].getCycle() / 5) % 8) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0,255,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,200,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,150,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(0,100,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 5:
      tunnelvision(rgb_to_hsv(CRGB(200,200,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 6:
      tunnelvision(rgb_to_hsv(CRGB(150,150,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(100,100,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);  
      break;
  }
}

//
// rainbow show - distribute a rainbow wheel equally distributed along the chain
//
void rainbow_show(uint8_t c) {
  for (uint8_t i = 0; i < HALF_LEDS; i++) {
    uint8_t hue = ((shows[c].getForeColorSpeed() * i) + shows[c].getBackColorSpeed() + (3 * shows[c].getCycle())) % MAX_COLOR;
    led[c].setPixelHue(i, hue);
    led[c].setPixelHue(NUM_LEDS - i, hue);
  }  
}

//// End specialized shows

//
// update_leds - push the interp frame on to the leds_buffer
//
void update_leds() {
  for (uint8_t channel = 0; channel < DUAL; channel++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds_buffer[channel][i] = led[channel].getInterpFrameColor(i);
    }
  }
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  CHSV color;  // (ArduinoBlue)
  
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t actual_led = pgm_read_byte_near(rewire + i);
    color = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][i], 
                                        leds_buffer[CHANNEL_A][i], 
                                        fract);  // interpolate a + b channels
    leds[actual_led] = lightning(narrow_palette(color));  // (ArduinoBlue)
  }
  
  turn_off_spacer_leds();
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

//// End DUAL SHOW LOGIC

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


void turn_off_spacer_leds() {
  for (uint8_t i = 0; i < 8; i++) {
    leds[spacer_leds[i]] = CRGB(0, 0, 0);
  }
}

//
// RGB to HSV - save having to translate RGB colors by hand
//
CHSV rgb_to_hsv( CRGB color) {
  return led[0].rgb_to_hsv(color);  // static method
}

//
// log status
//
void log_status(uint8_t i) {
  Serial.print("Channel: ");
  Serial.print(i);
  Serial.print(", Show: ");
  Serial.print(current_show[i]);
  Serial.print(", Wait: ");
  Serial.print(shows[i].getNumFrames());
  Serial.println(".");
}
