#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>

//
//  Mandala with FastLED
//
//  37 pixels with 64 LEDs
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  11/22/20
//
//
#define NUM_LEDS 37  // Chance of memory shortage for large NUM_LEDS
#define ACTUAL_LEDS 64

uint8_t BRIGHTNESS = 64;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 20;  // in milliseconds (ArduinoBlue)
long last_time;
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 7
#define CLOCK_PIN 8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define XX  255  // Empty LED

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
#define FISH_HUE (FISH_NUM * (FISH_NUM / MAX_COLOR)) // Main fish color
#define BLACK  CHSV(0, 0, 0)

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[ACTUAL_LEDS];  // Hook for FastLED library

#define ONLY_RED true  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

// Shows
#define START_SHOW_CHANNEL_A  11  // Channels A starting show
#define START_SHOW_CHANNEL_B  0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type
#define NUM_SHOWS 13

// Clocks and time
#define SHOW_DURATION 30  // seconds
uint8_t FADE_TIME = 20;  // seconds to fade in + out (Arduino Blue)
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

#define NUM_PATTERNS 19   // Total number of patterns

// Lookup tables

// Convert pixel to type, order
const uint8_t pixel_coord[] PROGMEM = { 
  0, 0,  // 0
  1, 1,  // 1
  2, 0,  // 2
  1, 2,  // 3
  0, 2,  // 4
  1, 0,  // 5
  2, 0,  // 6
  3, 1,  // 7
  3, 2,  // 8
  2, 2,  // 9
  1, 3,  // 10
  3, 0,  // 11
  4, 0,  // 12
  4, 1,  // 13
  4, 2,  // 14
  3, 3,  // 15
  2, 6,  // 16
  4, 7,  // 17
  5, 0,  // 18
  4, 3,  // 19
  2, 2,  // 20
  3, 7,  // 21
  4, 6,  // 22
  4, 5,  // 23
  4, 4,  // 24
  3, 4,  // 25
  1, 7,  // 26
  2, 6,  // 27
  3, 6,  // 28
  3, 5,  // 29
  2, 4,  // 30
  1, 4,  // 31
  0, 6,  // 32
  1, 6,  // 33
  2, 4,  // 34
  1, 5,  // 35
  0, 4   // 36
};

const uint8_t pixel_matrix[] PROGMEM = { 
   0,  0,  4,  4, 36, 36, 32, 32,  // 0
   5,  1,  3, 10, 31, 35, 33, 26,  // 1
   2,  2, 20, 20, 34, 34, 16, 16,  // 2
  11,  7,  8, 15, 25, 29, 28, 21,  // 3
  12, 13, 14, 19, 24, 23, 22, 17,  // 4
  18, 18, 18, 18, 18, 18, 18, 18   // 5
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  // Set up the various mappings here (1D lists in PROGMEM)
  //  for (uint8_t i = 0; i < DUAL; i++) {
  //    led[i].setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  //    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
  //  }

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show
  last_time = millis();
  
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();
  }

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = BLACK;
  }

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_start = 192;
    hue_width = 124;
  }
}

void loop() {

  for (uint8_t i = 0; i < DUAL; i++) {
      
    switch (current_show[i]) {
    
      case 0:
        patterns(i);
        break;
      case 1:
        shows[i].allOn();
        break;
      case 2:
        shows[i].morphChain();
        break;
      case 3:
        shows[i].twoColor();
        break;
      case 4:
        shows[i].lightWave();
        break;
      case 5:
        shows[i].sawTooth();
        break;
      case 6:
        shows[i].lightRunUp();
        break;
      case 7:
        shows[i].packets();
        break;
      case 8:
        shows[i].packets_two();
        break;
      case 9:
        shows[i].sinelon_fastled();
        break;
      case 10:
        shows[i].bpm_fastled();
        break;
      case 11:
        shows[i].juggle_fastled();
        break;
      default:
        shows[i].bands();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }
  check_phone();  // Check the phone settings (ArduinoBlue)
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
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { 
      next_show(i); 
    }
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  uint8_t mask = 0;
  uint8_t symmetry = 0;
  
  current_show[i] = is_other_channel_show_zero(i) ? random8(1, NUM_SHOWS) : 0 ;
  show_variables[i] = random8(NUM_PATTERNS);
  mask = pick_random_mask(i);
  
  if (mask == 0) {
    symmetry = pick_random_symmetry();
  }
  show_variables[i + 6] = mask;
  show_variables[i + 4] = symmetry;
  show_variables[i + 2] = random(4);
  
  led[i].push_frame();
  
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();

  // Set new show's colors to those of the other channel
  uint8_t other_channel = (i == 0) ? 1 : 0 ;
  shows[i].setForeColor(shows[other_channel].getForeColor());
  shows[i].setBackColor(shows[other_channel].getBackColor());
}

boolean is_other_channel_show_zero(uint8_t channel) {
  if (channel == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

uint8_t pick_random_mask(uint8_t i) {
  // 2 bit identities x 2 color/black = 4 possibilities
  if (current_show[i] != 0 && random8(1, 4) == 1) {
    return random8(1, 5);
  } else {
    return 0;
  }
}

uint8_t pick_random_pattern_type() {
  return random(12);
}

uint8_t pick_random_symmetry() {
  uint8_t random_symmetry = random(12);
  
  if (random_symmetry <= 6) {
    return random_symmetry;
  }
  return 0;  // No symmetrizing
}


////// Specialized shows

//
// patterns shows
//
void patterns(uint8_t c) {
  // Reset a lot of variables at the start of the show
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();  // Because we are using a beatsin8
    show_variables[c] = random(NUM_PATTERNS);  // Pick a pattern
    show_variables[c + 8] = pick_random_pattern_type();   // Pick a fill algorithm
    show_variables[c + 10] = random(6);  // Pick a different wipes
  }
  uint8_t pattern = show_variables[c];
  uint8_t change_value = show_variables[c + 8] % 2;
  uint8_t pattern_type = show_variables[c + 8] / 2;
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
      boolean value = get_bit_from_pattern_number(i, pattern);
      if (change_value == 1) {
        value = !value;
      }

      if (value) {
        if (pattern_type % 2 == 1) {
          shows[c].setPixeltoForeColor(i);
        } else {
          shows[c].setPixeltoForeBlack(i);
        }
        
      } else {
        if (pattern_type < 2) {
          shows[c].setPixeltoBackColor(i);
        } else {
          uint8_t intense = get_wipe_intensity(i, show_variables[c + 10], (pattern_type < 4));
          uint8_t back_hue = shows[c].getBackColor();
          if (pattern_type < 4) {
            shows[c].setPixeltoHue(i, shows[c].IncColor(back_hue, intense / 2));
          } else {
            shows[c].setPixeltoColor(i, led[c].gradient_wheel(back_hue, map8(intense, 64, 255)));
          }
        }
      }
  }
}

//
// Get Wipe Intensity
//
uint8_t get_wipe_intensity(uint8_t i, uint8_t wipe_number, boolean color_wipe) {
  uint8_t intensity;
  uint8_t x = get_led_x(i);  // 0-255
  uint8_t y = get_led_y(i);  // 0-255
  
  switch (wipe_number) {
    case 0:
      intensity = x;  // coefficient = thickness
      break;
    case 1: 
      intensity = y;
      break;
    case 2:
      intensity = (x + y) % 255;
      break;
    case 3:
      intensity = (x + (255 - y)) % 255;
      break;
    case 4:
      intensity = 255 - get_distance_to_origin(x, y);  // center wipe
      break;
    default:
      intensity = (255 - get_distance_to_coord(x, y, 0, 0)) / 2;  // corner wipe
      break;
  }
  uint8_t freq = (color_wipe) ? 1 : 10;
  return beatsin8(freq, 0, 255, 0, intensity);
}

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t get_distance_to_origin(uint8_t x, uint8_t y) {
  return get_distance_to_coord(x, y, 128, 128);
}

uint8_t get_distance_to_coord(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  // distance = sqrt(x^2 + y^2)
  // input: 0-255 coordinates
  // output: 0-255 distance
  // dx and dy should be no larger than 180 each
  uint8_t dx = abs(x2 - x1);  // 0-255
  uint8_t dy = abs(y2 - y1);  // 0-255
  return sqrt16(((dx * dx) + (dy * dy)) / 2);
}

//
// get led x, y - lookup table for pixel x and y coordinate (0-255)
//
uint8_t get_led_x(uint8_t i) {
  uint8_t led_lookup_x_table[NUM_LEDS] = {
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0
  };
  return led_lookup_x_table[i];
}

uint8_t get_led_y(uint8_t i) {
  uint8_t led_lookup_y_table[NUM_LEDS] = {
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0
  };
  return led_lookup_y_table[i];
}

//
// mirror_pixels
//
void mirror_pixels(uint8_t channel) {  
  uint8_t symmetry = show_variables[4 + channel];

  for (uint8_t i = 0 ; i < NUM_LEDS; i++) {
    uint8_t type = get_pixel_type(i);
    uint8_t order = get_pixel_order(i);
    CHSV color = led[channel].getInterpFrameColor(i);

    if (symmetry == 1 || symmetry == 3) {  // Horizontal mirroring
      uint8_t horiz_mirror[] = {5, 4, 3, 2, 1, 0, 7, 6};
      led[channel].setInterpFrame(get_pixel_by_type_order(type, horiz_mirror[order]), color);
    }
    
    if (symmetry == 2 || symmetry == 3) {  // Vertical mirroring
      uint8_t vert_mirror[] = {1, 0, 7, 6, 5, 4, 3, 2};
      led[channel].setInterpFrame(get_pixel_by_type_order(type, vert_mirror[order]), color);
    }

    if (symmetry == 4 || symmetry == 6) {  // Diagonal 1 mirroring
      uint8_t diag_1_mirror[] = {7, 6, 5, 4, 3, 2, 1, 0};
      led[channel].setInterpFrame(get_pixel_by_type_order(type, diag_1_mirror[order]), color);
    }
    
    if (symmetry == 5 || symmetry == 6) {  // Diagonal 2 mirroring
      uint8_t diag_2_mirror[] = {3, 2, 1, 0, 7, 6, 5, 4};
      led[channel].setInterpFrame(get_pixel_by_type_order(type, diag_2_mirror[order]), color);
    }
  }
}

//
// rotate_pixel - rotate square grid 90-degrees for each "r"
//
uint8_t rotate_pixel(uint8_t i, uint8_t channel) {
  uint8_t order = get_pixel_order(i);
  order = (order + (show_variables[2 + channel] * 2)) % 8;  // rotate
  return get_pixel_by_type_order(get_pixel_type(i), order);
}

uint8_t get_pixel_by_type_order(uint8_t type, uint8_t order) {
  return pgm_read_byte_near(pixel_matrix + (type * 8) + order);
}

uint8_t get_pixel_type(uint8_t i) {
  return pgm_read_byte_near(pixel_matrix + (i * 2));
}

uint8_t get_pixel_order(uint8_t i) {
  return pgm_read_byte_near(pixel_matrix + (i * 2) + 1);
}

//
// Test Strand
//
void test_strand(uint8_t channel) {
  shows[channel].fillBlack();
  uint8_t i = shows[channel].getCycle() % NUM_LEDS;
  led[channel].setPixelHue(i, shows[channel].getForeColor());
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  mirror_pixels(CHANNEL_A);
  mirror_pixels(CHANNEL_B);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = mask(led[CHANNEL_B].getInterpFrameColor(rotate_pixel(i, CHANNEL_B)), i, CHANNEL_B);
    CHSV color_a = mask(led[CHANNEL_A].getInterpFrameColor(rotate_pixel(i, CHANNEL_A)), i, CHANNEL_A);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    
    color = lightning(narrow_palette(color));  // (ArduinoBlue)
    
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }
    remap_leds(i, color);
    led_buffer[i] = color;
  }
  turn_off_spacers();
}

//
// mask
//
CHSV mask(CHSV color, uint8_t i, uint8_t channel) {
  if (show_variables[channel + 6] == 0 || current_show[i] == 0) {
    return color;  // no masking
  }
  uint8_t mask_value = show_variables[channel + 6] - 1;
  boolean value = get_bit_from_pattern_number(i, show_variables[channel]);
  if (mask_value % 2 == 0) {
    value = !value;
  }
  if (value) {
    return (mask_value > 2) ? shows[channel].getBackBlack() : led[channel].wheel(shows[channel].getBackColor());
  } else {
    return color;
  }
}


//
// remap_leds - send color to the correct leds
//
void remap_leds(uint8_t i, CHSV color) {
  uint8_t led_lookup_table[NUM_LEDS * 3] = {
     0,  2,  3,  // 0
     6, XX, XX,
     8, XX, XX,
    10, XX, XX,
    13, 14, 16,
     1, XX, XX,  // 5
     4, XX, XX,
     5,  7, XX,
     9, 11, XX,
    12, XX, XX,
    15, XX, XX,  // 10
    29, 31, XX,
    27, XX, XX,
    25, XX, XX,
    23, XX, XX,
    19, 22, XX,  // 15
    32, XX, XX,
    30, XX, XX,
    24, 26, 28,
    21, XX, XX,
    20, XX, XX,  // 20
    33, 35, XX,
    36, XX, XX,
    38, XX, XX,
    40, XX, XX,
    42, 44, XX,  // 25
    62, XX, XX,
    59, XX, XX,
    56, 58, XX,
    52, 54, XX,
    51, XX, XX,  // 30
    48, XX, XX,
    60, 61, 63,
    57, XX, XX,
    55, XX, XX,
    53, XX, XX,  // 35
    47, 49, 50
  };

  if (i == 18) {  // Center star
    light_center(color);
  } else {
    for (uint8_t j = 0; j < 3; j++) {
      uint8_t led_position = led_lookup_table[((i % NUM_LEDS) * 3) + j];
      if (led_position != XX) {
        leds[led_position % ACTUAL_LEDS] = color;
      }
    }
  }
}

//
// light_center - light the center 6 leds with the same color
//
void light_center(CHSV color) {
  uint8_t center_pixels[] = { 24, 26, 28, 37, 39, 41 };
  for (uint8_t i = 0; i < 6; i++) {
    leds[center_pixels[i]] = color;
  }
}

//
// lightning - ramp all pixels quickly up to white (down sat & up value) and back down
//
CHSV lightning(CHSV color) {  // (ArduinoBlue)
//  if (curr_lightning > 0) {
//    uint8_t increase = 255 - cos8( map(curr_lightning, 0, BOLT_TIME, 0, 255));
//    color.s -= increase;
//    color.v += increase;
//  }
  return color;
}

//
// turn_off_spacers - turn off the 4 spacer pixels
//
void turn_off_spacers() {
  uint8_t spacer_pixels[] = { 17, 18, 45, 46 };
  for (uint8_t i = 0; i < 4; i++) {
    leds[spacer_pixels[i]] = CHSV(0, 0, 0);
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

//// End DUAL SHOW LOGIC

//
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  color.h = map8(color.h, hue_start, (hue_start + hue_width) % 256 );
  return color;
}

//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t PatternMatrix[] = {
    0xfe, 0x6a, 0xaa, 0xb3, 0xf8,
    0x46, 0x4, 0x71, 0x3, 0x10,
    0x21, 0x8a, 0xaa, 0x8c, 0x20,
    0x20, 0xe, 0x53, 0x80, 0x88,
    0x1a, 0x28, 0x20, 0xa2, 0xc0,
    0x21, 0xcb, 0x8e, 0x9c, 0x20,
    0x24, 0xd3, 0x4c, 0xe6, 0x20,
    0x0, 0x0, 0x0, 0x0, 0x0,
    0x18, 0x28, 0x32, 0x1, 0x18,
    0x0, 0x0, 0x0, 0x0, 0x0,
    0x88, 0x0, 0x20, 0x0, 0x88,
    0x1, 0x91, 0x4, 0x4c, 0x0,
    0x56, 0x6e, 0x53, 0xb3, 0x50,
    0x70, 0xe, 0xdb, 0x80, 0x70,
    0x92, 0x8c, 0x21, 0x8a, 0x48,
    0xf8, 0x1b, 0xa6, 0xc0, 0xf8,
    0x0, 0x0, 0x0, 0x0, 0x0,
    0x18, 0xa5, 0x75, 0x28, 0xc0
  };
  pattern_number = pattern_number % NUM_PATTERNS;
  uint8_t pattern_byte = PatternMatrix[(pattern_number * 5) + (n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
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
