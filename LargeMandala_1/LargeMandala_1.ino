#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>

//
//  Mandala with FastLED
//
//  67 pixels with 91 LEDs
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  1/14/21
//
//
#define MAX_SPOKE 12
#define MAX_DISTANCE 7

#define NUM_LEDS  (MAX_SPOKE * MAX_DISTANCE)
#define ACTUAL_LEDS 91

#define SIZE 7
#define HALF_SIZE 3

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 30;  // in milliseconds (ArduinoBlue)
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
#define BLACK  CHSV(0, 0, 0)

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[ACTUAL_LEDS];  // Hook for FastLED library

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

// Shows
#define START_SHOW_CHANNEL_A   1  // Channels A starting show
#define START_SHOW_CHANNEL_B   0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type
#define MAX_PLINKO  16
uint8_t *plink = (uint8_t *)calloc(MAX_PLINKO * 4, sizeof(uint8_t));;
#define NUM_SHOWS 15

// Clocks and time
#define SHOW_DURATION 10  // seconds
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

#define NUM_PATTERNS 12  // Total number of patterns
#define NUM_COMPLEX_PATTERNS 6  // Total number of complex patterns

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

  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i].setColorSpeedMinMax(6, 30);  // Make colors change faster (lower = faster)
    shows[i].setBandsBpm(10, 30);
  }

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = BLACK;
  }

  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);
  last_time = millis();

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
        shows[i].sawTooth();
        break;
      case 5:
        shows[i].sinelon_fastled();
        break;
      case 6:
        shows[i].juggle_fastled();
        break;
      case 7:
        cone_push(i);
        break;
      case 8:
        shows[i].randomFill();
        break;
      case 9:
        center_ring(i);
        break;
      case 10:
        corner_ring(i);
        break;
      case 11:
        well(i);
        break;
      case 12:
        complex_patterns(i);
        break;
      case 13:
        complex_patterns(i);
        break;
      case 14:
        clock_face(i);
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
  show_variables[i + 2] = random(MAX_SPOKE);  // rotate
  
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

uint8_t pick_random_symmetry() {
  uint8_t random_symmetry = random(8);
   
  if (random_symmetry < 4) {
    uint8_t symmetry_lookup[] = { 2, 3, 4, 6 };
    return symmetry_lookup[random_symmetry];
  }
  return 0;  // No symmetrizing
}


////// Specialized shows

//
// Cone Push
//
void cone_push(uint8_t channel) {
  if (shows[channel].isShowStart()) {
    shows[channel].turnOffMorphing();
  }
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t dist = get_distance_to_origin(get_led_x(i), get_led_y(i));
    uint8_t value = beatsin8(10, 64, 255, 0, dist);
    led[channel].setPixelColor(i, led[channel].gradient_wheel(shows[channel].getForeColor(), value));
  }
}

//
// rings
//
void center_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 128, 128, 1, 64, 128);
}

void well(uint8_t c) {
  ring(c, shows[c].getForeColor(), led[c].wheel(shows[c].getBackColor()), 128, 128, 2, 192, 192);
}

void corner_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 0, 0, 3, 96, 255);
}

void ring(uint8_t c, uint8_t color, CHSV background, uint8_t center_x, uint8_t center_y, uint8_t ring_speed, uint8_t cutoff, uint8_t ring_freq) {
  // center_x, center_y = ring center coordinates; (128,128) is centers
  // ring_speed = 1+. Higher = faster.
  // cutoff = ring thickness with higher values = thicker
  // ring_freq = (255: 1 ring at a time; 128: 2 rings at a time)
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();
  }
  CHSV foreColor = led[c].wheel(color);
  led[c].fill(background);
  uint8_t value = shows[c].getCycle() / ring_speed;


  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t delta = abs(get_distance_to_coord(center_x, center_y, get_led_x(i), get_led_y(i)) - value) % ring_freq;
    if (delta < cutoff) {
      uint8_t intensity = map(delta, 0, cutoff, 255, 0);
      shows[c].setPixeltoColor(i, led[c].getInterpHSV(background, foreColor, intensity));
    }
  }
}

//
// complex patterns shows
//
void complex_patterns(uint8_t c) {
  // Reset a lot of variables at the start of the show
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();  // Because we are using a beatsin8
    show_variables[c] = random(NUM_COMPLEX_PATTERNS);  // Pick a pattern
    show_variables[c + 8] = random(3);   // 0 = gradient/black, 1 = color/black, 2 = color/color
  }
  uint8_t pattern = show_variables[c];
  uint8_t pattern_max = get_complex_max(pattern);
  uint8_t pattern_fill = show_variables[c + 8];
  uint8_t foreColor = shows[c].getForeColor();
  uint8_t cycle = shows[c].getCycle() % 255;

  if (pattern_fill == 2) {
    shows[c].fillBackColor();
  } else {
    shows[c].fillForeBlack();
  }
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t value = lookup_complex_pattern_table(i, pattern);
    
    if (value > 0) {
      uint8_t intense = sin8_C((cycle + map(value, 1, pattern_max, 0, 255)) % 255);
      if (pattern_fill > 0) {
        shows[c].setPixeltoHue(i, shows[c].IncColor(foreColor, map8(intense, 0, 64)));
      } else {
        shows[c].setPixeltoColor(i, led[c].gradient_wheel(foreColor, map8(intense, 96, 255)));
      }
    }
  }
}

//
// patterns shows
//
void patterns(uint8_t c) {
  // Reset a lot of variables at the start of the show
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();  // Because we are using a beatsin8
    show_variables[c] = random(NUM_PATTERNS);  // Pick a pattern
    show_variables[c + 8] = random(14);   // Pick a fill algorithm
    show_variables[c + 10] = random(6);  // Pick a different wipes
  }
  uint8_t pattern = show_variables[c];
//  uint8_t change_value = show_variables[c + 8] % 2;
  uint8_t pattern_type = show_variables[c + 8] / 2;

  // rotate!
  if (pattern_type > 12) {
    show_variables[2 + c] = shows[c].getCycle() % MAX_SPOKE;  // rotate amount
  }
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    boolean value = get_bit_from_pattern_number(i, pattern);

    // Don't invert patterns
//    if (change_value == 1) {
//      value = !value;
//    }

    if (value) {
      if (pattern_type % 2 == 1) {
        shows[c].setPixeltoForeColor(i);
      } else {
        shows[c].setPixeltoForeBlack(i);
      }
      
    } else {
      if (pattern_type < 2 || pattern_type >= 12) {
        shows[c].setPixeltoBackColor(i);
      } else if (pattern_type < 12) {
        uint8_t intense = get_wipe_intensity(i, show_variables[c + 10], (pattern_type < 4));
        uint8_t back_hue = shows[c].getBackColor();
        if (pattern_type < 4) {
          shows[c].setPixeltoHue(i, shows[c].IncColor(back_hue, intense / 8));
        } else {
          shows[c].setPixeltoColor(i, led[c].gradient_wheel(back_hue, map8(intense, 128, 255)));
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

  wipe_number = 0;
  
  switch (wipe_number) {
    case 0:
      intensity = x;  // coefficient = thickness
      break;
    case 1: 
      intensity = y;
      break;
    case 2:
      intensity = (x + y) / 2;
      break;
    case 3:
      intensity = (x + (255 - y)) / 2;
      break;
    case 4:
      intensity = 255 - (get_distance_to_origin(x, y) * 2);  // center wipe
      break;
    default:
      intensity = 255 - get_distance_to_coord(x, y, 0, 0);  // corner wipe
      break;
  }
  uint8_t beat_freq = (color_wipe) ? 2 : 10;
  return beatsin8(beat_freq, 0, 255, 0, intensity);
}

//
// clock
//
void clock_face(uint8_t c) {
  shows[c].fillBlack();

  uint8_t minute_position = shows[c].getCycle() % MAX_SPOKE;
  uint8_t hour_position = (shows[c].getCycle() / MAX_SPOKE)  % MAX_SPOKE;
  
  // Draw minute hand
  shows[c].setPixeltoForeColor(get_pixel(minute_position, 1));
  shows[c].setPixeltoForeColor(get_pixel((minute_position + 1) % MAX_SPOKE, 1));
  shows[c].setPixeltoForeColor(get_pixel(minute_position, 2));
  shows[c].setPixeltoForeColor(get_pixel(minute_position, 4));

  // Draw hour hand
  shows[c].setPixeltoBackColor(get_pixel(11, 0));
  shows[c].setPixeltoBackColor(get_pixel(hour_position, 1));
  shows[c].setPixeltoBackColor(get_pixel((hour_position + 1) % MAX_SPOKE, 1));
  shows[c].setPixeltoBackColor(get_pixel(hour_position, 2));
}

//
// plinko - not really plinko but lines of light
//
void plinko(uint8_t c) {
  if (shows[c].isShowStart()) {
    for (uint8_t i = 0; i < MAX_PLINKO; i++) {
      uint8_t plink_index = (i * 2) + (c * MAX_PLINKO * 2);
      plink[plink_index] = XX;  // Move plinkos off the board
    }
  }

  if (shows[c].getMorph() > 0) {
    return;
  }

  shows[c].fillForeBlack();

  for (uint8_t i = 0; i < MAX_PLINKO; i++) {
    uint8_t plink_index = (i * 2) + (c * MAX_PLINKO * 2);
    
    if (plink[plink_index] == XX) {
      // Chance to start a new plinko
      if (random(24) == 1) {
        plink[plink_index] = random(MAX_SPOKE);
        plink[plink_index + 1] = 0;
      }
    } else {
      plink[plink_index + 1] += 1;
      if (plink[plink_index + 1] > 8) {
        plink[plink_index] = XX;  // Turn off plinko
      }
    }
    
    if (plink[plink_index] != XX) {
      uint8_t spoke = plink[plink_index];
      uint8_t plink_distance = plink[plink_index + 1];
      uint8_t hue_adjusted = shows[c].getForeColor() + (5 * i) % MAX_COLOR;
      uint8_t DIST_LOOKUP[] = { 0, 1, 2, 4, 6 };
      
      if (plink_distance > 4) {
        shows[c].setPixeltoHue(get_pixel((spoke + 6) % MAX_SPOKE, DIST_LOOKUP[plink_distance - 4]), hue_adjusted);
      } else {
//        uint8_t spoke = (plink_distance == 4) ? 11 : spoke;  // Center needs have spoke of 11
        shows[c].setPixeltoHue(get_pixel(spoke, DIST_LOOKUP[4 - plink_distance]), hue_adjusted);
      }
    }
  }
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
  uint8_t dx = abs(x2 - x1);  // 0-255
  uint8_t dy = abs(y2 - y1);  // 0-255
  return sqrt16(((dx * dx) + (dy * dy)) / 2);
}

//
// get led x, y - lookup table for pixel x and y coordinate (0-255)
//
uint8_t get_led_x(uint8_t i) {
  uint8_t led_lookup_x_table[NUM_LEDS] = {
     128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 
     128, 128, 158, 150, 136, 120, 106,  98,  98, 106, 
     120, 136, 150, 158, 189, 181, 159, 128,  97,  75, 
      67,  75,  97, 128, 159, 181, 210, 188, 150, 106, 
      68,  46,  46,  68, 106, 150, 188, 210, 222, 210, 
     175, 128,  81,  46,  34,  46,  81, 128, 175, 210, 
     233, 205, 156, 100,  51,  23,  23,  51, 100, 156, 
     205, 233, 255, 238, 192, 128,  64,  18,   0,  18, 
      64, 128, 192, 238
  };
  return led_lookup_x_table[i];
}

uint8_t get_led_y(uint8_t i) {
  uint8_t led_lookup_y_table[NUM_LEDS] = {
     128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 
     128, 128, 136, 150, 158, 158, 150, 136, 120, 106, 
      98,  98, 106, 120, 128, 159, 181, 189, 181, 159, 
     128,  97,  75,  67,  75,  97, 150, 188, 210, 210, 
     188, 150, 106,  68,  46,  46,  68, 106, 128, 175, 
     210, 222, 210, 175, 128,  81,  46,  34,  46,  81, 
     156, 205, 233, 233, 205, 156, 100,  51,  23,  23, 
      51, 100, 128, 192, 238, 255, 238, 192, 128,  64, 
      18,   0,  18,  64
  };
  return led_lookup_y_table[i];
}

//
// mirror_pixels
//
void mirror_pixels(uint8_t channel) {  
  // 2 = halves, 3 = thirds, 4 = quarters, 6 = sixths
  uint8_t symmetry = show_variables[4 + channel];  // symmetry

  if (symmetry > 0) {
    for (uint8_t s = 0 ; s < MAX_SPOKE / symmetry; s++) {
      for (uint8_t d = 0 ; d < MAX_DISTANCE; d++) {
        CHSV color = led[channel].getInterpFrameColor(get_pixel(s, d));
        for (uint8_t n = 1 ; n < symmetry; n++) {
          led[channel].setInterpFrame(get_pixel(s + (n * (MAX_SPOKE / symmetry)), d), color);
        }
      }
    }
  }
}

//
// rotate_pixel - rotate spoke by r (0 to MAX_SPOKE)
//
uint8_t rotate_pixel(uint8_t i, uint8_t channel) {
  uint8_t s = get_spoke_from_pixel(i);
  uint8_t d = get_distance_from_pixel(i);
  uint8_t r = show_variables[2 + channel];
  return get_pixel((s + r) % MAX_SPOKE, d);
}


//
// Geometry
//
uint8_t get_spoke_from_pixel(uint8_t pixel) {
  return pixel % MAX_SPOKE;
}

uint8_t get_distance_from_pixel(uint8_t pixel) {
  return pixel / MAX_SPOKE;
}

uint8_t get_pixel(uint8_t s, uint8_t d) {
  return ((d * MAX_SPOKE) + s) % NUM_LEDS;
}

void set_leds_by_coord(uint8_t s, uint8_t d, CHSV color) {
  if (d == 0) {
    for (uint8_t i = 86; i < 91; i++) {
      leds[i] = color;
    }
  } else if (d == 1) {
    leds[74 + s] = color;
  } else if (d == 2) {
    leds[50 + (s * 2)] = color;
    leds[51 + (s * 2)] = color;
  } else if (d == 3) {
    uint8_t MIDDLE_LOOKUP[] = { 46, 0, 6, 8, 14, 16, 22, 24, 30, 32, 38, 40 };
    leds[MIDDLE_LOOKUP[s]] = color;
  } else if (d == 4) {
    uint8_t FAR_LOOKUP[] = { 47, 3, 7, 11, 15, 19, 23, 27, 31, 35, 39, 43 };
    leds[FAR_LOOKUP[s]] = color;
  } else if (d == 5) {
    leds[1 + (rotate_right(s, 1) * 4)] = color;
  } else if (d == 6) {
    if (s % 2 == 1) {
      s = (s - 1) / 2;
      leds[(s * 8) + 2] = color;
      leds[(s * 8) + 4] = color;
    }
  }
}

uint8_t rotate_left(uint8_t value, uint8_t amount) {
  return (value + amount) % MAX_SPOKE;
}

uint8_t rotate_right(uint8_t value, uint8_t amount) {
  return (value + MAX_SPOKE - amount) % MAX_SPOKE;
}

void set_leds_by_pixel(uint8_t pixel, CHSV color) {
  set_leds_by_coord(get_spoke_from_pixel(pixel), get_distance_from_pixel(pixel), color);
}

//
// Test Strand
//
void test_strand(uint8_t channel) {
  shows[channel].fillBlack();
  uint8_t i = shows[channel].getCycle() % NUM_LEDS;
  led[channel].setPixelHue(i, shows[channel].getForeColor());
}

void test_ring(uint8_t c) {
  shows[c].fillBlack();
  uint8_t d = shows[c].getCycle() % MAX_DISTANCE;
  
  for (uint8_t s = 0; s < MAX_SPOKE; s++) {
    shows[c].setPixeltoForeColor(get_pixel(s, d));
  }
}

void test_spoke(uint8_t c) {
  shows[c].fillBlack();
  uint8_t s = shows[c].getCycle() % MAX_SPOKE;
  
  for (uint8_t d = 0; d < MAX_DISTANCE; d++) {
    shows[c].setPixeltoForeColor(get_pixel(s, d));
  }
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
    set_leds_by_pixel(i, color);
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
// turn_off_spacers - turn off the 2 spacer pixels
//
void turn_off_spacers() {
  leds[48] = BLACK;
  leds[49] = BLACK;
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
    0xff, 0xfb, 0x6d, 0x92, 0x40, 0x4, 0x92, 0x40, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x92, 0x4b, 0x6d, 0x92, 0x4b, 0x2d, 0x30, 0xc0,
    0x0, 0x0, 0x0, 0x55, 0x5f, 0xff, 0x55, 0x5f, 0xff, 0xff, 0xf0,
    0xff, 0xff, 0xff, 0xff, 0xf6, 0xdb, 0x24, 0x90, 0x0, 0x0, 0x0,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x55, 0x50, 0x0, 0x0, 0x0,
    0x0, 0xf, 0xff, 0x0, 0xf, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x3, 0xc, 0x38, 0xe7, 0x9e, 0x38, 0xe7, 0x9e, 0x0, 0x0,
    0xff, 0xf7, 0x9e, 0x28, 0xa0, 0x0, 0x28, 0xa7, 0x9e, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x55, 0x5f, 0xff, 0xff, 0xf0,
    0xff, 0xfc, 0xf3, 0xc7, 0x18, 0x61, 0x84, 0x0, 0x0, 0x0, 0x0,
    0x0, 0xa, 0xaa, 0x0, 0x5, 0x55, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x6d, 0xb0, 0x0, 0x6d, 0xb4, 0x92, 0x0, 0x0
  };
  n = NUM_LEDS - n - 1;  // Need to invert
  pattern_number = pattern_number % NUM_PATTERNS;
  uint8_t pattern_byte = PatternMatrix[(pattern_number * 11) + (n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

uint8_t lookup_complex_pattern_table(uint8_t n, uint8_t complex_number) {
  uint8_t ComplexMatrix[] = {

        1,1,1, 1,1,1, 1,1,1, 1,1,1,
        2,0,2, 2,0,2, 2,0,2, 2,0,2,
        3,0,0, 3,0,0, 3,0,0, 3,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        4,0,0, 4,0,0, 4,0,0, 4,0,0,
        5,0,6, 6,0,5, 5,0,6, 6,0,5,
        6,6,5, 5,0,0, 0,0,5, 5,6,6,

        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        7,6,5, 4,3,2, 1,12,11, 10,9,8,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        1,2,3, 4,5,6, 7,8,9, 10,11,12,
        12,1,2, 3,4,5, 6,7,8, 9,10,11,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,

        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,1,0, 1,0,1, 0,1,0, 1,0,1,
        6,2,6, 2,6,2, 6,2,6, 2,6,2,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        5,3,5, 3,5,3, 5,3,5, 3,5,3,
        4,4,4, 4,4,4, 4,4,4, 4,4,4,

        5,5,5, 5,5,5, 5,5,5, 5,5,5,
        4,0,0, 0,0,6, 6,0,0, 0,0,4,
        3,0,0, 0,0,0, 7,0,0, 0,0,0,
        2,0,0, 0,0,8, 8,0,0, 0,0,2,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        1,0,0, 0,0,9, 9,0,0, 0,0,1,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,

        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        7,0,3, 0,7,0, 3,0,7, 0,3,0,
        8,2,4, 6,8,2, 4,6,8, 2,4,6,
        1,1,5, 5,1,1, 5,5,1, 1,5,5,
        
        0,0,0, 0,0,0, 0,0,0, 0,0,0,
        1,2,3, 4,5,6, 7,8,9, 10,11,12,
        4,3,2, 1,12,11, 10,9,8, 7,6,5,
        7,8,9, 10,11,12, 1,2,3, 4,5,6,
        10,9,8, 7,6,5, 4,3,2, 1,12,11,
        1,2,3, 4,5,6, 7,8,9, 10,11,12,
        4,3,2, 1,12,11, 10,9,8, 7,6,5
  };
  n = NUM_LEDS - n - 1;  // Need to invert
  return ComplexMatrix[((complex_number % NUM_COMPLEX_PATTERNS) * NUM_LEDS) + n];
}

uint8_t get_complex_max(uint8_t complex_number) {
  uint8_t complex_max[] = { 6, 12, 6, 9, 8, 12 };
  return complex_max[complex_number % NUM_COMPLEX_PATTERNS];
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
