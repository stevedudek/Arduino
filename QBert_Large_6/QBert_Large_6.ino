#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  QBert Large: 111 + 6 = 117 LEDs
//
//  8/29/21
//

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 20;  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
uint32_t timing[] = { millis(), millis() };  // last_time, last_connection
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 11
#define CLOCK_PIN 13

#define NUM_LEDS 111
#define NUM_SPACER_LEDS 6
#define TOTAL_LEDS  (NUM_LEDS + NUM_SPACER_LEDS)

#define SIZE 7
#define DIAMOND_NUMBER 3

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define ARE_CONNECTED true  // Are the pentagons talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

boolean is_lost = false;
#define MAX_SILENT_TIME  (3 * 1000)  // Time (in sec) without communication before marked as is_lost

#define XX  255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[TOTAL_LEDS];  // The Leds themselves
CHSV led_buffer[NUM_LEDS];  // For smoothing

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 19
#define START_SHOW_CHANNEL_A  1  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  0
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type

// wait times
#define SHOW_DURATION 25  // seconds
uint8_t FADE_TIME = 20;  // seconds to fade in + out (Arduino Blue)
#define MAX_SMALL_CYCLE  (SHOW_DURATION * 2 * (1000 / DELAY_TIME))  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out
uint8_t freq_storage[] = { 60, 80 };  // 1-byte storage for shows

#define NUM_PATTERNS 10   // Total number of patterns, each 4 bytes wide
#define NUM_COMPLEX_PATTERNS 6  // Total number of complex patterns

uint8_t SPACING[] = { 0, 4, 9, 15, 22, 28, 33 };

uint8_t neighbors[] PROGMEM = {
2, 1, XX, 13,
XX, XX, 0, 2,
3, 1, 0, 16,
5, 4, 2, 16,
XX, XX, 3, 5,
6, 4, 3, 19,
8, 7, 5, 19,
XX, XX, 6, 8,
9, 7, 6, 22,
11, 10, 8, 22,
XX, XX, 9, 11,
XX, 10, 9, 25,
14, 13, XX, 28,
0, XX, 12, 14,
15, 13, 12, 31,
17, 16, 14, 31,
3, 2, 15, 17,
18, 16, 15, 34,
20, 19, 17, 34,
6, 5, 18, 20,
21, 19, 18, 37,
23, 22, 20, 37,
9, 8, 21, 23,
24, 22, 21, 40,
26, 25, 23, 40,
XX, 11, 24, 26,
XX, 25, 24, 43,
29, 28, XX, 46,
12, XX, 27, 29,
30, 28, 27, 49,
32, 31, 29, 49,
15, 14, 30, 32,
33, 31, 30, 52,
35, 34, 32, 52,
18, 17, 33, 35,
36, 34, 33, 55,
38, 37, 35, 55,
21, 20, 36, 38,
39, 37, 36, 58,
41, 40, 38, 58,
24, 23, 39, 41,
42, 40, 39, 61,
44, 43, 41, 61,
XX, 26, 42, 44,
XX, 43, 42, 64,
47, 46, XX, XX,
27, XX, 45, 47,
48, 46, 45, 67,
50, 49, 47, 67,
30, 29, 48, 50,
51, 49, 48, 70,
53, 52, 50, 70,
33, 32, 51, 53,
54, 52, 51, 73,
56, 55, 53, 73,
36, 35, 54, 56,
57, 55, 54, 76,
59, 58, 56, 76,
39, 38, 57, 59,
60, 58, 57, 79,
62, 61, 59, 79,
42, 41, 60, 62,
63, 61, 60, 82,
65, 64, 62, 82,
XX, 44, 63, 65,
XX, 64, 63, XX,
68, 67, XX, XX,
48, 47, 66, 68,
69, 67, 66, 85,
71, 70, 68, 85,
51, 50, 69, 71,
72, 70, 69, 88,
74, 73, 71, 88,
54, 53, 72, 74,
75, 73, 72, 91,
77, 76, 74, 91,
57, 56, 75, 77,
78, 76, 75, 94,
80, 79, 77, 94,
60, 59, 78, 80,
81, 79, 78, 97,
83, 82, 80, 97,
63, 62, 81, 83,
XX, 82, 81, XX,
86, 85, XX, XX,
69, 68, 84, 86,
87, 85, 84, 100,
89, 88, 86, 100,
72, 71, 87, 89,
90, 88, 87, 103,
92, 91, 89, 103,
75, 74, 90, 92,
93, 91, 90, 106,
95, 94, 92, 106,
78, 77, 93, 95,
96, 94, 93, 109,
98, 97, 95, 109,
81, 80, 96, 98,
XX, 97, 96, XX,
101, 100, XX, XX,
87, 86, 99, 101,
102, 100, 99, XX,
104, 103, 101, XX,
90, 89, 102, 104,
105, 103, 102, XX,
107, 106, 104, XX,
93, 92, 105, 107,
108, 106, 105, XX,
110, 109, 107, XX,
96, 95, 108, 110,
XX, 109, 108, XX
};

uint8_t XCOORDS[] PROGMEM = {
  10, 0, 20, 29, 20, 39, 49, 39, 59, 69, 59, 79, 29, 20, 39, 49, 39, 59, 
  69, 59, 79, 88, 79, 98, 108, 98, 118, 49, 39, 59, 69, 59, 79, 88, 79, 
  98, 108, 98, 118, 128, 118, 138, 147, 138, 157, 69, 59, 79, 88, 79, 98, 
  108, 98, 118, 128, 118, 138, 147, 138, 157, 167, 157, 177, 187, 177, 196, 
  108, 98, 118, 128, 118, 138, 147, 138, 157, 167, 157, 177, 187, 177, 196, 
  206, 196, 216, 147, 138, 157, 167, 157, 177, 187, 177, 196, 206, 196, 216, 
  226, 216, 236, 187, 177, 196, 206, 196, 216, 226, 216, 236, 246, 236, 255
};

uint8_t YCOORDS[] PROGMEM = {
  119, 136, 136, 158, 175, 175, 197, 214, 214, 236, 253, 253, 79, 96, 96, 
  119, 136, 136, 158, 175, 175, 197, 214, 214, 236, 253, 253, 40, 57, 57, 79, 
  96, 96, 119, 136, 136, 158, 175, 175, 197, 214, 214, 236, 253, 253, 1, 18, 
  18, 40, 57, 57, 79, 96, 96, 119, 136, 136, 158, 175, 175, 197, 214, 214, 
  236, 253, 253, 1, 18, 18, 40, 57, 57, 79, 96, 96, 119, 136, 136, 158, 175, 
  175, 197, 214, 214, 1, 18, 18, 40, 57, 57, 79, 96, 96, 119, 136, 136, 158, 
  175, 175, 1, 18, 18, 40, 57, 57, 79, 96, 96, 119, 136, 136
};

uint8_t ROTATE[] PROGMEM = {
  15, 9, 4, 0, 22, 16, 10, 5, 1,
  28, 23, 17, 11, 6, 2, 33, 29, 24, 18,
  12, 7, 3, 34, 30, 25, 19, 13, 8,
  35, 31, 26, 20, 14, 36, 32, 27, 21
};

uint8_t SYMMETRY[] PROGMEM = {
  6, 6, 6, 3, 2, 6, 6, 3, 3, 2,
  2, 6, 3, 3, 3, 2, 2, 2, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, TOTAL_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 4 neighbors for every pixel
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    
    shows[i].setColorSpeedMinMax(6, 30);  // Make colors change faster (lower = faster)
    shows[i].setBandsBpm(10, 30);

    shows[i].setAsSquare();  // Each pixel has 4 neighbors
  }
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);
  timing[0] = millis();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }
  
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
//        shows[i].allOn();
//        test_neighbors(i);
        patterns(i);
        break;
      case 1:
        shows[i].morphChain();
        break;
      case 2:
        shows[i].bounce();
        break;
      case 3:
        shows[i].bounceGlowing();
        break;
      case 4:
        shows[i].plinko(66);
        break;
      case 5:
        windmill(i);
        break;
      case 6:
        windmill_smoothed(i);
        break;
      case 7:
        cone_push(i);
        break;
      case 8:
        shows[i].randomFill();
        break;
      case 9:
        shows[i].juggle_fastled();
        break;
      case 10:
        shows[i].randomTwoColorBlack();
        break;
      case 11:
        shows[i].randomOneColorBlack();
        break;
      case 12:
        shows[i].sawTooth();
        break;
      case 13:
        center_ring(i);
        break;
      case 14:
        corner_ring(i);
        break;
      case 15:
        qbert(i);
        break;
      case 16:
        complex_patterns(i, true);
        break;
      case 17:
        complex_patterns(i, false);
        break;
      default:
        shows[i].bands();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

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
      pick_next_show(i);
    }
  }
}

//
// fixed_delay - make every cycle the same time
//
void fixed_delay() {
  long new_time = millis();
  long time_delta = new_time - timing[0];  // how much time has elapsed? Usually 3-5 milliseconds
  timing[0] = new_time;  // update the counter
  if (time_delta < DELAY_TIME) {  // if we have excess time,
    delay(DELAY_TIME - time_delta);  // delay for the excess
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
//  shows[i].fillForeBlack();
  led[i].push_frame();  // This might not work well with speaking & hearing
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(5, 60);
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
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
  show_variables[i + 2] = random(3);  // rotation
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].pickRandomColorSpeeds();
//  shows[i].tweakColorSpeeds();
  shows[i].pickRandomWait();
  log_status(i);  // For debugging
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
  
  if (random_symmetry <= 3) {
    return random_symmetry;  // 2-fold (1), 3-fold (2), 6-fold (3), None
  }
  return 0;  // No symmetrizing
}

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
        uint8_t intense = get_wipe_intensity(XCOORDS[i], YCOORDS[i], show_variables[c + 10], c, (pattern_type < 4));
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
// complex patterns shows
//
void complex_patterns(uint8_t c, boolean smooth) {
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
      if (intense > 200 || smooth) {
        if (pattern_fill > 0) {
          shows[c].setPixeltoHue(i, shows[c].IncColor(foreColor, map8(intense, 0, 64)));
        } else {
          shows[c].setPixeltoColor(i, led[c].gradient_wheel(foreColor, map8(intense, 96, 255)));
        }
      }
    }
  }
}

//
// Test Neighbors
//
void test_neighbors(uint8_t channel) {
  shows[channel].fillBlack();
  shows[channel].setForeColor(0);
  shows[channel].setBackColor(120);
  uint8_t i = shows[channel].getCycle() % NUM_LEDS;
  led[channel].setPixelHue(i, shows[channel].getForeColor());
  
  for (uint8_t j = 0; j < 4; j++) {
    uint8_t n = pgm_read_byte_near(neighbors + (i * 4) + j);
    if (n != XX) {
      led[channel].setPixelHue(n, shows[channel].getBackColor());
    }
  }
}

boolean inbounds(uint8_t x, uint8_t y) {
  return (y >= 0) && (y < SIZE) && (x >= min(0, 3 - y)) && (x <= max(6, 9 - y));
}

//
// Get Wipe Intensity
//
uint8_t get_wipe_intensity(uint8_t x, uint8_t y, uint8_t wipe_number, uint8_t c, boolean color_wipe) {
  uint8_t intensity;

  switch (wipe_number) {
    case 0:
      intensity = x * 4;  // coefficient = thickness
      break;
    case 1: 
      intensity = y * 4;
      break;
    case 2:
      intensity = (x * 2) + (y * 2);
      break;
    case 3:
      intensity = (x * 2) + ((SIZE - y - 1) * 2);
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
// Cone Push
//
void cone_push(uint8_t i) {
  if (shows[i].isShowStart()) {
    shows[i].turnOffMorphing();
  }
  uint8_t beat = beatsin8(4, 0, 128, 0, 0);

  for (uint8_t n = 0; n < NUM_LEDS; n++) {
    uint8_t value = 255 - (abs(beat - get_pixel_distance_to_origin(n)) * 4);
    led[i].setPixelColor(n, led[i].gradient_wheel(shows[i].getForeColor(), value));
  }
}

//
// Q*bert
//
void qbert(uint8_t i) {
  CHSV color;
  
  for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
    switch (z) {
      case 0: 
        color = led[i].gradient_wheel(shows[i].getForeColor(), 255);
        break;
      case 1:
        color = led[i].gradient_wheel(shows[i].getBackColor(), 255);
        break;
      case 2:
        color = led[i].gradient_wheel(shows[i].getForeColor(), 128);
        break;
    }
    for (uint8_t x = 0; x < SIZE; x++) {
      for (uint8_t y = 0; y < SIZE; y++) {
        if (inbounds(x, y)) {
          led[i].setPixelColor(get_pixel_from_coord(x,y,z), color);
        }
      }
    }
  }
}

//
// Windmill
//
void windmill(uint8_t i) {
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(20, 40);
    shows[i].turnOffMorphing();
  }

  for (uint8_t n = 0; n < NUM_LEDS; n++) {
    uint8_t value = beatsin8(freq_storage[i] - (YCOORDS[n] / 10), 0, 255, 0, XCOORDS[n]);
    value = (value > 200) ? value : 0 ;
    led[i].setPixelColor(n, CHSV(shows[i].getForeColor(), 255, value) );
  }
}

void windmill_smoothed(uint8_t i) {
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(20, 40);
    shows[i].turnOffMorphing();
  }

  for (uint8_t n = 0; n < NUM_LEDS; n++) {
    uint8_t value = beatsin8(freq_storage[i] - (YCOORDS[n] / 10), 0, 255, 0, XCOORDS[n]);
    led[i].setPixelColor(n, CHSV(shows[i].getForeColor(), 255, value) );
    led[i].setPixelColor(n, CHSV(shows[i].getForeColor(), 255, value) );
  }
}

//
// rings
//
void center_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 128, 128, 1, 64, 64);
}

void corner_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 0, 0, 3, 32, 128);
}

void ring(uint8_t c, uint8_t color, CHSV background, uint8_t center_x, uint8_t center_y, uint8_t ring_speed, uint8_t cutoff, uint8_t ring_freq) {
  // center_x, center_y = ring center coordinates; (5,5) is hex centers
  // ring_speed = 1+. Higher = faster.
  // cutoff = ring thickness with higher values = thicker
  // ring_freq = (255: 1 ring at a time; 128: 2 rings at a time)
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();
  }
  CHSV foreColor = led[c].wheel(color);
  led[c].fill(background);
  uint8_t value = shows[c].getCycle() / ring_speed;


  for (uint8_t n = 0; n < NUM_LEDS; n++) {
    uint8_t delta = abs(get_pixel_distance_to_point(n, center_x, center_y) - value) % ring_freq;
    if (delta < cutoff) {
      uint8_t intensity = map(delta, 0, cutoff, 255, 0);
      shows[c].setPixeltoColor(n, led[c].getInterpHSV(background, foreColor, intensity));
    }
  }
}

///// Distance functions

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t get_pixel_distance_to_origin(uint8_t i) {
  return get_pixel_distance_to_point(i, 128, 128);
}

uint8_t get_pixel_distance_to_point(uint8_t i, uint8_t point_x, uint8_t point_y) {
  return get_distance_to_coord(XCOORDS[i], YCOORDS[i], point_x, point_x);
}

uint8_t get_distance_to_origin(uint8_t x, uint8_t y) {
  return get_distance_to_coord(x, y, 128, 128);
}

uint8_t get_distance_to_coord(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  // distance = sqrt(x^2 + y^2)
  // input: 0-255 coordinates
  // output: 0-255 distance
  // dx and dy should be no larger than 180 each
  uint8_t dx = map8(abs(x2 - x1), 0, 100);
  uint8_t dy = map8(abs(y2 - y1), 0, 100);
  return sqrt16((dx * dx) + (dy * dy));
}


//// End specialized shows



//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {

  mirror_pixels(CHANNEL_A);
  mirror_pixels(CHANNEL_B);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = mask(led[CHANNEL_B].getInterpFrameColor(rotate_pixel(i, show_variables[2 + CHANNEL_B])), i, CHANNEL_B);
    CHSV color_a = mask(led[CHANNEL_A].getInterpFrameColor(rotate_pixel(i, show_variables[2 + CHANNEL_A])), i, CHANNEL_A);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    
    color = narrow_palette(color);  // (ArduinoBlue)
    
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }
    
    leds[lookup_led(i)] = color;
    led_buffer[i] = color;
  }
}

//
// lookup led: convert pixel to LED
//
uint8_t lookup_led(uint8_t i) {
  if (i <= 11) {  // Row 0
    return i;
  } else if (i <= 26) {  // Row 1
    return (26 - i + 14);
  } else if (i <= 44) {  // Row 2
    return i + 4;
  } else if (i <= 65) {  // Row 3
    return (65 - i + 51);
  } else if (i <= 83) {  // Row 4
    return i + 6;
  } else if (i <= 98) {  // Row 5
    return (98 - i + 90);
  } else {  // Row 6
    return i + 6;
  }
}

void turn_off_spacer_leds() {
  uint8_t SPACER_LEDS[] = { 12, 13, 29, 30, 49, 50 };
  for (uint8_t i = 0; i < NUM_SPACER_LEDS; i++) {
    leds[SPACER_LEDS[i]] = CHSV(0, 0, 0);
  }
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
// mirror_pixels
//
void mirror_pixels(uint8_t channel) {  
  uint8_t symmetry = show_variables[4 + channel];
  if (symmetry == 0) {
    return;  // no mirroring
  }
  // 2-fold (1), 3-fold (2), 6-fold (3), None
  uint8_t rotate_lookup[] = { 2, 3, 6 };
  uint8_t rotate_number = rotate_lookup[symmetry - 1];
  
  for (uint8_t hex = 0; hex < 37; hex++) {
    if (SYMMETRY[hex] >= rotate_number) {
      for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
        uint8_t pixel = (hex * 3) + z;
        CHSV color = led[channel].getInterpFrameColor(pixel);
        for (uint8_t r = 1; r < rotate_number; r++) {
          led[channel].setInterpFrame(rotate_pixel(pixel, r * rotate_number / 6), color);
        }
      }
    }
  }
}

//
// rotate_pixel - rotate triangle grid 60-degrees for each "r"
//
uint8_t rotate_pixel(uint8_t i, uint8_t rotation) {
  if (rotation == 0) {
    return i;
  }
  uint8_t z = get_z_from_pixel(i);
  uint8_t hex = i / 3;
  
  for (uint8_t r = rotation; r > 0; r--) {
    hex = ROTATE[hex];
    z = (z + 1) % DIAMOND_NUMBER;
  }
  return (hex * 3) + z;
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();

  // Similar logic to check_fades (deprecated)
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

//// End DUAL SHOW LOGIC

//
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  color.h = map8(color.h, hue_start, (hue_start + hue_width) % MAX_COLOR );
  return color;
}



//
// Get pixel from coordinate
//
uint8_t get_pixel_from_coord(uint8_t x, uint8_t y, uint8_t z) {
  return (((x + SPACING[y % SIZE]) * DIAMOND_NUMBER) + z) % NUM_LEDS;
}

uint8_t get_x_from_pixel(uint8_t i) {
  i /= DIAMOND_NUMBER;
  for (uint8_t y = SIZE - 1; y >= 0; y--) {
    if (i >=  SPACING[y]) {
      return i - SPACING[y];
    }
  }
}

uint8_t get_y_from_pixel(uint8_t i) {
  i /= DIAMOND_NUMBER;
  for (uint8_t y = SIZE - 1; y >= 0; y--) {
    if (i >=  SPACING[y]) {
      return y;
    }
  }
}

uint8_t get_z_from_pixel(uint8_t i) {
  return i % DIAMOND_NUMBER;
}

//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t PatternMatrix[] = {
    0x24, 0x92, 0x49, 0x24,
    0xa4, 0xb9, 0xd4, 0x58,
    0x51, 0x43, 0xca, 0x84,
    0xaa, 0x35, 0xe1, 0x38,
    0xe, 0x81, 0xc3, 0xbc,
    0xd, 0x1, 0x2b, 0x40,
    0xf1, 0x73, 0x8b, 0xa0,
    0x9c, 0xc9, 0x14, 0xf0,
    0xb0, 0x53, 0x60, 0x34,
    0x51, 0x4c, 0x32, 0x98
  }; 
  pattern_number = pattern_number % NUM_PATTERNS;
  uint8_t pattern_byte = PatternMatrix[(pattern_number * 4) + (n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

uint8_t lookup_complex_pattern_table(uint8_t n, uint8_t complex_number) {
  // See iPython Notebook: Pattern Maker QBert
  
  uint8_t ComplexMatrix[] = {
    9,0,8,3,4,7,0,0,6,0,4,5,10,1,2,6,11,5,0,3,0,11,8,7,9,2,10,12,1,0,
    1,0,1,0,0,1,0,0,1,0,1,1,1,0,0,1,1,1,0,1,0,1,0,0,0,1,0,1,1,0,
    0,1,0,1,0,0,0,1,0,1,0,0,0,0,1,1,1,1,0,0,1,0,1,0,1,0,0,0,0,1,
    1,0,1,0,1,0,1,0,0,0,1,1,0,1,0,1,1,1,1,0,0,0,0,1,0,0,1,1,1,0,
    0,0,0,0,1,1,1,0,1,0,0,0,0,0,0,1,1,1,0,0,0,0,1,1,1,0,1,1,1,1,
    0,0,0,0,1,1,0,1,0,0,0,0,0,0,0,1,0,0,1,0,1,0,1,1,0,1,0,0,0,0,
    1,1,1,1,0,0,0,1,0,1,1,1,0,0,1,1,1,0,0,0,1,0,1,1,1,0,1,0,0,0,
    1,0,0,1,1,1,0,0,1,1,0,0,1,0,0,1,0,0,0,1,0,1,0,0,1,1,1,1,0,0,
    1,0,1,1,0,0,0,0,0,1,0,1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,1,1,0,1,
    0,1,0,1,0,0,0,1,0,1,0,0,1,1,0,0,0,0,1,1,0,0,1,0,1,0,0,1,1,0
  };
  return ComplexMatrix[((complex_number % NUM_COMPLEX_PATTERNS) * NUM_LEDS) + n];
}

uint8_t get_complex_max(uint8_t complex_number) {
  uint8_t complex_max[] = { 12, 8, 12, 3, 15, 9 };
  return complex_max[complex_number % NUM_COMPLEX_PATTERNS];
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
  Serial.print(", Mask: ");
  Serial.print(show_variables[i + 6]);
  Serial.print(", Symmetry: ");
  Serial.print(show_variables[i + 4]);
  Serial.println(".");
}
