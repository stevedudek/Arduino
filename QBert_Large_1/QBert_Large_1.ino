#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>  // (ArduinoBlue)
//
//  QBert: 10 Hexes x 3 = 30 LEDs in on a triangle
//
//  6/21/21
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 20;  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
uint32_t timing[] = { millis(), millis() };  // last_time, last_connection
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 7
#define CLOCK_PIN 8

#define NUM_LEDS 30

#define SIZE 4
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
CRGB leds[NUM_LEDS];  // The Leds themselves
CHSV led_buffer[NUM_LEDS];  // For smoothing

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 19
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t show_variables[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type

// wait times
#define SHOW_DURATION 25  // seconds
uint8_t FADE_TIME = 20;  // seconds to fade in + out (Arduino Blue)
#define MAX_SMALL_CYCLE  (SHOW_DURATION * 2 * (1000 / DELAY_TIME))  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out
uint8_t freq_storage[] = { 60, 80 };  // 1-byte storage for shows

// ArduinoBlue
ArduinoBlue phone(Serial2); // Blue Tx = pin 9; Blue Rx = pin 10
#define HUE_SLIDER        0
#define HUE_WIDTH_SLIDER  1
#define SPEED_SLIDER      2
#define BRIGHTNESS_SLIDER 3
#define FADE_TIME_SLIDER  4
#define BAM_BUTTON        0
#define BOLT_TIME        20

// xBee language
#define COMMAND_START      '+'
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_BRIGHTNESS 'I'
#define COMMAND_PALETTE    'H'
#define COMMAND_PAL_WIDTH  'J'
#define COMMAND_SPEED      'Y'
#define COMMAND_MASK       'M'
#define COMMAND_P_TYPE     'p'
#define COMMAND_W_TYPE     'w'
#define COMMAND_FADE       'R'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_SM_CYCLE   'C'
#define COMMAND_PATTERN    'P'
#define COMMAND_BOLT       'X'
#define COMMAND_CHANNEL_A  'x'
#define COMMAND_CHANNEL_B  'y'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define EMPTY_CHAR         ' '
#define MAX_MESSAGE       120
#define MAX_NUM             6  // To handle 65,535 of small_cycle
char message[MAX_MESSAGE];     // Incoming message buffer
char number_buffer[MAX_NUM];   // Incoming number buffer

// ToDo:
// 1. Check shows
// 2. pattern shows with numbers
// 3. Cube faces

// Lookup table to convert pixels to LEDs
const uint8_t LED_LOOKUP[] PROGMEM = {
  27, 28, 29, 23, 22, 21, 18, 19, 20, 2, 1, 0,
  26, 25, 24, 15, 16, 17, 5, 4, 3,
  12, 13, 14, 8, 7, 6,
  11, 10, 9
};

const uint8_t neighbors[] PROGMEM = {
  1, 2, XX, XX,
  3, 2, 0, 14,  // 3, XX, XX, 14,
  XX, XX, 0, 1,
  4, 5, 1, 14,
  6, 5, 3, 17,
  XX, XX, 3, 4,
  7, 8, 4, 17,
  9, 8, 6, 20,
  XX, XX, 6, 7,
  10, 11, 7, 20,  // XX, XX, 7, 20,
  XX, 11, 9, XX,
  XX, XX, 9, 10,
  13, 14, XX, XX,
  15, 14, 12, 23,
  3, 1, 12, 13,
  16, 17, 13, 23,
  18, 17, 15, 26,
  6, 4, 15, 16,
  19, 20, 16, 26,
  XX, 20, 18, XX,
  9, 7, 18, 19,
  22, 23, XX, XX,
  24, 23, 21, 29,
  15, 13, 21, 22,
  25, 26, 22, 29,
  XX, 26, 24, XX,
  18, 16, 24, 25,
  28, 29, XX, XX,
  XX, 29, 27, XX,
  24, 22, 27, 28  // 24, 22, XX, XX
};

#define NUM_PATTERNS 10   // Total number of patterns, each 4 bytes wide
#define NUM_COMPLEX_PATTERNS 6  // Total number of complex patterns

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial1.begin(9600);  // Serial1: xBee port
  Serial2.begin(9600);  // Serial2: Bluetooth serial (ArduinoBlue)
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 4 neighbors for every pixel
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    
    shows[i].setColorSpeedMinMax(6, 30);  // Make colors change faster (lower = faster)
    shows[i].setBandsBpm(10, 30);

    shows[i].setAsSquare();
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
        patterns(i);
        break;
      case 1:
        shows[i].morphChain();
        break;
      case 2:
        shows[i].bounce();  // ToDo: FIX
        break;
      case 3:
        shows[i].bounceGlowing();  // ToDo: FIX
        break;
      case 4:
        shows[i].plinko(12);  // Works
        break;
      case 5:
        windmill(i);  // Works
        break;
      case 6:
        windmill_smoothed(i);  // Works
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

  check_phone();  // Check the phone settings (ArduinoBlue)
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  fixed_delay();
  advance_clocks();  // advance the cycle clocks and check for next show
  speak_and_hear();  // speak out or hear in signals
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
  
  if (curr_lightning > 0 ) {  // (ArduinoBlue)
    curr_lightning--; // Reduce the current bolt
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
  speak_and_hear();
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
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE - x; y++) {
      for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
        uint8_t i = get_pixel_from_coord(x,y,z);
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
            uint8_t cartesian_x = get_cartesian_x(x, y, z);
            uint8_t cartesian_y = get_cartesian_y(x, y, z);
            uint8_t intense = get_wipe_intensity(cartesian_x, cartesian_y, show_variables[c + 10], c, (pattern_type < 4));
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

  // ToDo: Check out windmill
  
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
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE - x; y++) {
      for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
        uint8_t value = 255 - (abs(beat - get_coord_distance_to_origin(x,y,z)) * 4);
        led[i].setPixelColor(get_pixel_from_coord(x,y,z), led[i].gradient_wheel(shows[i].getForeColor(), value));
      }
    }
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
      for (uint8_t y = 0; y < SIZE - x; y++) {
        led[i].setPixelColor(get_pixel_from_coord(x,y,z), color);
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
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE - x; y++) {
      for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
        uint8_t cart_x = get_cartesian_x(x, y, z);
        uint8_t cart_y = get_cartesian_y(x, y, z);
        uint8_t value = beatsin8(freq_storage[i] - (cart_y / 10), 0, 255, 0, cart_x);
        value = (value > 200) ? value : 0 ;
        led[i].setPixelColor(get_pixel_from_coord(x,y,z), CHSV(shows[i].getForeColor(), 255, value) );
      }
    }
  }
}

void windmill_smoothed(uint8_t i) {
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(20, 40);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE - x; y++) {
      for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
        uint8_t cart_x = get_cartesian_x(x, y, z);
        uint8_t cart_y = get_cartesian_y(x, y, z);
        uint8_t value = beatsin8(freq_storage[i] - (cart_y / 10), 0, 255, 0, cart_x);
        led[i].setPixelColor(get_pixel_from_coord(x,y,z), CHSV(shows[i].getForeColor(), 255, value) );
      }
    }
  }
}

//
// rings
//
void center_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), shows[c].getForeBlack(), 80, 128, 1, 64, 64);
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

  for (uint8_t x = 0; x < SIZE; x++) {
    for (uint8_t y = 0; y < SIZE - x; y++) {
      for (uint8_t z = 0; z < DIAMOND_NUMBER; z++) {
        uint8_t delta = abs(get_coord_distance_to_point(x, y, z, center_x, center_y) - value) % ring_freq;
        if (delta < cutoff) {
          uint8_t intensity = map(delta, 0, cutoff, 255, 0);
          shows[c].setPixeltoColor(get_pixel_from_coord(x, y, z), led[c].getInterpHSV(background, foreColor, intensity));
        }
      }
    }
  }
}

///// Distance functions

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t get_coord_distance_to_origin(uint8_t x, uint8_t y, uint8_t z) {
  return get_coord_distance_to_point(x, y, z, 80, 128);
}

uint8_t get_coord_distance_to_point(uint8_t x, uint8_t y, uint8_t z, uint8_t point_x, uint8_t point_y) {
  return get_distance_to_coord(get_cartesian_x(x, y, z), get_cartesian_y(x, y, z), point_x, point_x);
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

//
// Get Cartesian X
//
uint8_t get_cartesian_x(uint8_t x, uint8_t y, uint8_t z) {
  float z_lookup_delta_x[] = { -17, 17, 0 };
  float cartesian_x = ((70 * x) + (35 * y) + 17);
  cartesian_x += z_lookup_delta_x[z];
  return uint8_t(cartesian_x);
}

//
// Get Cartesian Y
//
uint8_t get_cartesian_y(uint8_t x, uint8_t y, uint8_t z) {
  float z_lookup_delta_y[] = { 10, 10, -20.2 };
  float cartesian_y = ((60.6 * float(y)) + 35);
  cartesian_y += z_lookup_delta_y[z];
  return uint8_t(cartesian_y);
}

//// End specialized shows



////// Speaking and Hearing

void speak_and_hear() {
  if (ARE_CONNECTED && IS_SPEAKING) { speak_commands(); }
  if (ARE_CONNECTED && !IS_SPEAKING) { hear_signal(); }  
}

void speak_commands() {
  for (uint8_t i = 0; i < DUAL; i++) {  // Send one channel, then the next
    speak_channel_commands(i);
//    if (shows[i].getMorph() == 0) {
//      speak_channel_commands(i);
//    }
  }
}

void speak_channel_commands(uint8_t i) {
  // Speak all the commands for a particular channel
  uint8_t m = 0;  // where we are in the send-out message string

  m = speak_start(m);
  m = speak_channel(i, m);  // Send the channel A or B prompt
  
  m = speak_command(COMMAND_SM_CYCLE, shows[CHANNEL_A].getSmallCycle(), m);
  m = speak_command(COMMAND_FORE, shows[i].getForeColor(), m);
  m = speak_command(COMMAND_BACK, shows[i].getBackColor(), m);
  m = speak_command(COMMAND_SHOW, current_show[i], m);
  m = speak_command(COMMAND_WAIT, shows[i].getWait(), m);
  m = speak_command(COMMAND_MASK, show_variables[i + 6], m);
  m = speak_command(COMMAND_P_TYPE, show_variables[i + 8], m);
  m = speak_command(COMMAND_W_TYPE, show_variables[i + 10], m);
  
  message[m++] = COMMAND_PERIOD;  // Terminate message with period character
  message[m++] = '\0';  // Terminate message with null (new-line) character
  Serial1.print(message);  // Only done once
//  Serial.println(message);  // For debugging
}

void speak_arduinoblue_commands() {
  uint8_t m = 0;  // where we are in the send-out message string

  m = speak_start(m);
  m = speak_command(COMMAND_BRIGHTNESS, BRIGHTNESS, m);
  m = speak_command(COMMAND_PALETTE, hue_start, m);
  m = speak_command(COMMAND_PAL_WIDTH, hue_width, m);
  m = speak_command(COMMAND_SPEED, DELAY_TIME, m);
  m = speak_command(COMMAND_FADE, FADE_TIME, m);
  m = speak_command(COMMAND_BOLT, curr_lightning, m);
  
  message[m++] = COMMAND_PERIOD;  // Terminate message with null character
  message[m++] = '\0';  // Terminate message with null character
  Serial1.print(message);  // Only done once
//  Serial.println(message);  // For debugging
}

uint8_t speak_start(uint8_t m) {
  message[m++] = COMMAND_START;
  return m;
}

uint8_t speak_command(char cmd, int value, uint8_t m) {
  message[m++] = cmd;

  String value_str = String(value);
  for (uint8_t i = 0; i < value_str.length(); i++) {
    message[m++] = value_str[i];
  }

  message[m++] = COMMAND_COMMA;
  return m;
}

uint8_t speak_channel(int i, uint8_t m) {
  char command_channel = (i == 0) ? COMMAND_CHANNEL_A : COMMAND_CHANNEL_B ;
  message[m++] = command_channel;
  return m;
}

////// Hearing

void hear_signal() {
  if (Serial1.available()) {  // Heard a signal!
    ResetLostCounter();    
    boolean msg_received = GetMessage(message);  // Load global message buffer by pointer
    if (msg_received) {
      Serial.println(message);  // For debugging
      ProcessMessage(message);
    }
  } else {
    if (!is_lost && (millis() - timing[1] > MAX_SILENT_TIME) ) {
      is_lost = true;
    }
  }
}

//
// ResetLostCounter - board is not lost
//
void ResetLostCounter() {
  is_lost = false;
  timing[1] = millis();
}

//
// GetMessage - pulls the whole serial buffer until a period
//
boolean GetMessage(char* message) {
  char tmp;
  boolean have_start_signal = false;
  uint8_t tries = 0;
  uint16_t MAX_TRIES = 2000;
  uint8_t i = 0;

  while (tries++ < MAX_TRIES) {
    if (Serial1.available()) {
      tries = 0;
      tmp = Serial1.read();
//      Serial.print(tmp);  // For debugging

      if (!have_start_signal) {
        if (tmp == COMMAND_START) { have_start_signal = true; }  // Start of message
        else { } // discard prefix garbage
      } else {
        if (tmp == COMMAND_PERIOD) {
          message[i++] = tmp;  // End of message
          message[i++] = '\0';  // End of string
          return true;
        }  
        if (isAscii(tmp)) {
          message[i++] = tmp;
          if (i >= MAX_MESSAGE) {
            Serial.println("Message too long!");  // Ran out of message space
            return false;
          }
        }
      }
    }
  }
  Serial.println("Ran out of tries");
  return false;  // discard message
}

//
// ProcessMessage
//
void ProcessMessage(char* message) {
  uint8_t i = 0;
  uint8_t channel = 0;  // Default to channel 0
  char cmd;
  
  while (i < MAX_MESSAGE) {
    
    cmd = message[i];  // Get one-letter command

    if (cmd == COMMAND_PERIOD) {
      return;
    } else if (!isAscii(cmd)) {
      return;
    } else if (cmd == COMMAND_CHANNEL_A) {
      i++;
      channel = 0;
    } else if (cmd == COMMAND_CHANNEL_B) {
      i++;
      channel = 1;
    } else if (isAlpha(cmd)) {
      for (uint8_t j = 0; j < MAX_NUM; j++) { 
        number_buffer[j] = EMPTY_CHAR;  // Clear number buffer
      }
      
      int numsiz = ReadNum(message, i+1, number_buffer);  // Read in the number
      i = i + 2 + numsiz; // 2 = leap beginning command and trailing comma
      
      ExecuteOrder(cmd, atoi(number_buffer), channel);
    } else {
      return;  // Garbage
    }
  }
}

//
// ReadNum - reads numbers in a string
//
uint8_t ReadNum(char* msg, uint8_t place, char* number) {
  uint8_t i = 0; // Start of number
  char tmp;
  
  while (i < MAX_NUM) {
    tmp = msg[place];
    if (tmp == COMMAND_COMMA) {
      return (i);
    } else {
      number[i] = msg[place];
      i++;
      place++;
      if (place >= MAX_MESSAGE) { 
        break;  
      }
    }
  }
  Serial.println("Number too long");
  return (0); // Number too long
}

//
// Execute Order - execute a letter command followed by a number
//
void ExecuteOrder(char command, uint16_t value, uint8_t i) {
      
  switch (command) {
    
    case COMMAND_FORE:
      shows[i].setForeColor(value);
      break;
    
    case COMMAND_BACK:
      shows[i].setBackColor(value);
      break;

    case COMMAND_BRIGHTNESS:
      if (BRIGHTNESS != value) {
        BRIGHTNESS = value;
        FastLED.setBrightness( BRIGHTNESS );
      }
      break;
    
    case COMMAND_WAIT:
      if (shows[i].getWait() != value) { 
        shows[i].setWait(value);
      }
      break;
    
    case COMMAND_SHOW:  // Show
      if (current_show[i] != value) {
        current_show[i] = value % NUM_SHOWS;
        next_show(i);
      }
      break;

    case COMMAND_MASK:
      show_variables[i + 6] = value;
      break;

    case COMMAND_P_TYPE:
      show_variables[i + 8] = value;
      break;

    case COMMAND_W_TYPE:
      show_variables[i + 10] = value;
      break;

    case COMMAND_SM_CYCLE: // small_cycle
      shows[i].setSmallCycle(value);
      break;
  }
}

////// End Speaking & Hearing



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
    
    color = lightning(narrow_palette(color));  // (ArduinoBlue)
    
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }
    
    leds[LED_LOOKUP[i % NUM_LEDS]] = color;
    led_buffer[i] = color;
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
  
  // 2-fold (1), 3-fold (2), 6-fold (3), None
  if (symmetry == 1 || symmetry == 3) {  // Vertical mirroring
    for (uint8_t x = 0; x < SIZE; x++) {
      for (uint8_t y = 0; y < SIZE - x; y++) {
        for (uint8_t z = 0 ; z < DIAMOND_NUMBER; z++) {
          if (y >= x) {
            uint8_t new_z = z;
            if (z != 1) {
              new_z = (z + 2) % 4;
            }
            led[channel].setInterpFrame(get_pixel_from_coord(y, x, new_z), led[channel].getInterpFrameColor(get_pixel_from_coord(x, y, z)));
          }
        }
      }
    }
  }
  
  if (symmetry == 2 || symmetry == 3) {  // Rotational mirroring
    for (uint8_t x = 0; x < SIZE; x++) {
      for (uint8_t y = 0; y < SIZE - x; y++) {
        for (uint8_t z = 0 ; z < DIAMOND_NUMBER; z++) {
          if ((x + y <= 1) || (x == 1 && y == 1 && z == 0)) {
            uint8_t i = get_pixel_from_coord(x, y, z);
            CHSV color = led[channel].getInterpFrameColor(i);
//            Serial.println(i);
            led[channel].setInterpFrame(rotate_pixel(i, 2), color);  // Does not work
            led[channel].setInterpFrame(rotate_pixel(i, 1), color);  // Does not work
//            Serial.print(i);
//            Serial.print(", ");
//            Serial.print(rotate_pixel(i, 1));
//            Serial.print(", ");
//            Serial.println(rotate_pixel(i, 2));
          }
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
  uint8_t new_x, new_y;
  uint8_t x = get_x_from_pixel(i);
  uint8_t y = get_y_from_pixel(i);
  uint8_t z = get_z_from_pixel(i);
  
  for (uint8_t r = rotation; r > 0; r--) {
    new_y = x;
    new_x = SIZE - 1 - (x + y);
    x = new_x;
    y = new_y;
    z = (z - 1 + DIAMOND_NUMBER) % DIAMOND_NUMBER;
  }
  return get_pixel_from_coord(x, y, z);
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
// Get pixel from coordinate
//
uint8_t get_pixel_from_coord(uint8_t x, uint8_t y, uint8_t z) {
  uint8_t SPACING[] = { 0, 4, 7, 9 };
  return (((x + SPACING[y % 4]) * DIAMOND_NUMBER) + z) % NUM_LEDS;
}

uint8_t get_x_from_pixel(uint8_t i) {
  uint8_t SPACING[] = { 0, 4, 7, 9 };
  i /= DIAMOND_NUMBER;
  for (uint8_t y = SIZE - 1; y >= 0; y--) {
    if (i >=  SPACING[y]) {
      return i - SPACING[y];
    }
  }
}

uint8_t get_y_from_pixel(uint8_t i) {
  uint8_t SPACING[] = { 0, 4, 7, 9 };
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
