#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>

//
//  Roses with FastLED
//
//  Ported from Python / Processing / Pixel Pusher
//
//  24 spokes x 6 distance = 144 pixels
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  1/30/21
//
//
#define MAX_SPOKE 24
#define MAX_DISTANCE 6

#define NUM_LEDS  (MAX_SPOKE * MAX_DISTANCE)

#define SIZE 7
#define HALF_SIZE 3

uint8_t BRIGHTNESS = 48;  // (0-255) (ArduinoBlue)

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
CRGB leds[NUM_LEDS];  // Hook for FastLED library

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

// Shows
#define START_SHOW_CHANNEL_A   0  // Channels A starting show
#define START_SHOW_CHANNEL_B   1  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUMBER_VARIABLES 8
uint8_t show_variables[NUMBER_VARIABLES * DUAL];  // 2 each: pattern, rotate, symmetry, mask, pattern_type, wipe_type, spare
#define MEMORY_SIZE  (NUM_LEDS * DUAL)
uint8_t memory[MEMORY_SIZE];

#define MAX_PLINKO  16
#define NUM_SHOWS 26

// Clocks and time
#define SHOW_DURATION 100  // seconds
uint8_t FADE_TIME = 0;  // seconds to fade in + out (Arduino Blue)
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
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
    set_random_pattern(i);
  }

  for (uint8_t i = 0; i < NUMBER_VARIABLES * 2; i++) {
    show_variables[i] = 0;  // Reset show variables
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
        patterns(i);
        break;
      case 2:
        patterns(i);
        break;
      case 3:
        wave(i);
        break;
      case 4:
        fan_spoke(i);
        break;
      case 5:
        fan_distance(i);
        break;
      case 6:
        in_out(i);
        break;
      case 7:
        up_down(i);
        break;
      case 8:
        shows[i].morphChain();
        break;
      case 9:
        windmill(i);
        break;
      case 10:
        windmill_smoothed(i);
        break;
      case 11:
        shows[i].twoColor();
        break;
      case 12:
        shows[i].sawTooth();
        break;
      case 13:
        shows[i].sinelon_fastled();
        break;
      case 14:
        shows[i].juggle_fastled();
        break;
      case 15:
        cone_push(i);
        break;
      case 16:
        shows[i].randomFill();
        break;
      case 17:
        center_ring(i);
        break;
      case 18:
        corner_ring(i);
        break;
      case 19:
        well(i);
        break;
      case 20:
        clock_face(i);
        break;
      case 21:
        pendulum_wave(i, true);
        break;
      case 22:
        pendulum_wave(i, false);
        break;
      case 23:
        plinko(i, true);
        break;
      case 24:
        plinko(i, false);
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
  
  current_show[i] = random8(NUM_SHOWS);
  set_random_pattern(i);
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

void no_symmetry(uint8_t i) {
  // Set channel i to have no symmetry
  show_variables[i + 4] = 0;
}

void no_mask(uint8_t i) {
  // Set channel i to have no mask
  show_variables[i + 6] = 0;
}

uint8_t pick_random_symmetry() {
  uint8_t symmetry = (random(12) < 6) ? get_random_symmetry() : 0;
  return symmetry;
}

uint8_t get_random_symmetry() {
  uint8_t choice = random(5);
  uint8_t symmetry_lookup[] = { 2, 3, 4, 6, 8 };
  return symmetry_lookup[choice];
}

void set_spare_show_variable(uint8_t value, uint8_t index, uint8_t channel) {
  show_variables [channel + 12 + (index * DUAL)] = value;
}

uint8_t get_spare_show_variable(uint8_t index, uint8_t channel) {
  return show_variables [channel + 12 + (index * DUAL)];
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
    uint8_t dist = get_pixel_distance_to_origin(i);
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
    uint8_t delta = abs(get_pixel_distance_to_coord(center_x, center_y, i) - value) % ring_freq;
    if (delta < cutoff) {
      uint8_t intensity = map(delta, 0, cutoff, 255, 0);
      shows[c].setPixeltoColor(i, led[c].getInterpHSV(background, foreColor, intensity));
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
    no_symmetry(c);  // May want to experiment
    show_variables[c + 8] = random(14);   // Pick a fill algorithm
    show_variables[c + 10] = random(6);  // Pick a different wipes
  }
  uint8_t change_value = show_variables[c + 8] % 2;
  uint8_t pattern_type = show_variables[c + 8] / 2;

  // rotate!
  if (pattern_type > 12) {
    show_variables[2 + c] = shows[c].getCycle() % MAX_SPOKE;  // rotate amount
  }
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t value = get_memory_index(i, c);

    if (change_value == 1) {
      value = !value;
    }
    if (value > 0) {
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
  uint8_t x = 0;  // 0-255
  uint8_t y = 0;  // 0-255
  get_led_xy(i, &x, &y);

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
  for (uint8_t x = 0; x <= 4; x++) {
    for (uint8_t y = 0; y <= x; y++) {
      shows[c].setPixeltoForeColor(get_pixel(4 - y + minute_position , x - y));
    }
  }

  // Draw hour hand
  for (uint8_t x = 0; x <= 2; x++) {
    for (uint8_t y = 0; y <= x; y++) {
      shows[c].setPixeltoBackColor(get_pixel(2 - y + hour_position , x - y));
    }
  }

  // Draw Outer ring
  uint8_t hue = shows[c].IncColor(shows[c].getForeColor(), shows[c].getBackColor());
  for (uint8_t s = 0; s <= MAX_SPOKE; s++) {
    shows[c].setPixeltoHue(get_pixel(s, MAX_DISTANCE - 1), hue);
  }
}

//
// Windmill
//
void windmill(uint8_t i) {
  if (shows[i].isShowStart()) {
    set_spare_show_variable(random(40, 100), 0, i);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < MAX_SPOKE; x++) {
    for (uint8_t y = 0; y < MAX_DISTANCE; y++) {
      uint8_t value = beatsin8(get_spare_show_variable(0, i) - (4 * y), 0, 255, 0, x * 128 / MAX_SPOKE );
      value = (value > 235) ? value : 0 ;
      shows[i].setPixeltoColor(get_pixel(x,y), CHSV(shows[i].getForeColor(), 255, value) );
    }
  }
}

void windmill_smoothed(uint8_t i) {
  if (shows[i].isShowStart()) {
    set_spare_show_variable(random(40, 100), 0, i);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < MAX_SPOKE; x++) {
    for (uint8_t y = 0; y < MAX_DISTANCE; y++) {
      uint8_t value = beatsin8(get_spare_show_variable(0, i) - (4 * y), 0, 255, 0, x * 255 / MAX_SPOKE );
      shows[i].setPixeltoColor(get_pixel(x,y), CHSV(shows[i].getForeColor(), 255, value) );
    }
  }
}

//
// set random pattern
//
void set_random_pattern(uint8_t c) {
  fill_memory(c, 0);  // Clear memory for the channel
  uint8_t symmetry = get_random_symmetry();
  uint8_t petal_size = random(2, MAX_DISTANCE - 1);
  uint8_t fan_or_petal = random(0, 2);
  
  for (uint8_t s = 0; s < MAX_SPOKE; s = s+symmetry) {
    if (fan_or_petal == 0) {
      draw_fan(petal_size, s, c, 1);
    } else {
      draw_petal(petal_size, s, c, 1);
    }
  }
}

//
// draw petal - draw a petal into memory with value
//
void draw_petal(uint8_t petal_size, uint8_t spoke, uint8_t channel, uint8_t value) {
  for (uint8_t x = 0; x <= petal_size; x++) {
    for (uint8_t y = 0; y <= x; y++) {
      set_memory(petal_size - y + spoke , x - y, channel, value);
    }
  }
}

//
// draw fan - draw a fan into memory with value
//
void draw_fan(uint8_t fan_size, uint8_t spoke, uint8_t channel, uint8_t value) {
  for (uint8_t x = 0; x < fan_size; x++) {
    for (uint8_t y = 0; y < fan_size; y++) {
      set_memory(x + spoke, y, channel, value);
    }
  }
}

//
// fill memory - set all memory for that channel to value
//
void fill_memory(uint8_t channel, uint8_t value) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    memory[i + (channel * NUM_LEDS)] = value;
  }
}

void set_memory(uint8_t s, uint8_t d, uint8_t c, uint8_t value) {
  s = (s + MAX_SPOKE) % MAX_SPOKE;
  d = (d + MAX_DISTANCE) % MAX_DISTANCE;
  set_memory_index(get_pixel(s, d), c, value);
}

void set_memory_index(uint8_t i, uint8_t c, uint8_t value) {
  memory[(i % NUM_LEDS) + (c * NUM_LEDS)] = value;
}

uint8_t get_memory(uint8_t s, uint8_t d, uint8_t c) {
  s = (s + MAX_SPOKE) % MAX_SPOKE;
  d = (d + MAX_DISTANCE) % MAX_DISTANCE;
  return get_memory_index(get_pixel(s, d), c);
}

uint8_t get_memory_index(uint8_t i, uint8_t c) {
  return memory[(i % NUM_LEDS) + (c * NUM_LEDS)];
}

//
// Pendulum Wave
//
void pendulum_wave(uint8_t i, boolean smoothed) {
  uint8_t value, pos, dist;
  
  if (shows[i].isShowStart()) {
    set_memory_index(random(40, 100), 0, i);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < MAX_SPOKE; x++) {
    value = beatsin8(get_memory_index(0, i) - (4 * x));
    for (uint8_t y = 0; y < MAX_DISTANCE; y++) {
      pos = (y * 256 / MAX_DISTANCE) + (256 / (MAX_DISTANCE * 2));
      dist = (pos >= value) ? 255 - (pos - value) : 255 - (value - pos) ;
      if (smoothed == false) {
        dist = (dist > 230) ? dist : 0 ;
      }
      led[i].setPixelColor(get_pixel(x,y), CHSV(shows[i].getForeColor(), 255, dist) );
    }
  }
}

void plinko(boolean in_direction, uint8_t c) {

  // Refresh plinkos at the start of the show
  if (shows[c].isShowStart()) {
    no_mask(c);
    set_spare_show_variable(random(0, 10), 0, c);  // frequency a new plinko starts
    for (uint8_t i = 0; i < MAX_PLINKO; i++) {
      set_plinko_position(XX, XX, i, c);  // s, d, index, channel - clear the positions
    }
  }

  if (shows[c].getMorph() > 0) {
    return;
  }

  // Start new plinko
  uint8_t freq = get_spare_show_variable(0, c);
  uint8_t number_new_plinko = 0;
  if (freq > 4) {
    if (random(0, freq - 3) == 0) {
      number_new_plinko = 1;
    }
  } else {
    number_new_plinko = 5 - freq;
  }
  for (uint8_t i = 0; i < number_new_plinko; i++) {
    uint8_t new_index = get_open_plinko(c);
    if (new_index != XX) {
      uint8_t start_d = (in_direction == true) ? MAX_DISTANCE - 1 : 0;
      uint8_t new_hue = shows[c].IncColor(shows[c].getForeColor(), random(0, 20));
      set_plinko_position(random(0, MAX_SPOKE), start_d, new_index, c);  // s, d, index, c
      set_plinko_color(new_hue, new_index, c);  // hue, index, c
    }
  }

  // Draw plinko
  shows[c].fillForeBlack();
  for (uint8_t i = 0; i < MAX_PLINKO; i++) {
    if (get_plinko_s(i, c) != XX) {
      shows[c].setPixeltoHue(get_pixel(get_plinko_s(i, c), get_plinko_d(i, c)), get_plinko_color(i, c));
    }
  }

  // Move plinko
  for (uint8_t i = 0; i < MAX_PLINKO; i++) {
    if (get_plinko_s(i, c) != XX) {
      uint8_t current_d = get_plinko_d(i, c);
      if (in_direction) {
        if (current_d == 0) {
          set_plinko_position(XX, XX, i, c);  // turn off plinko
        } else {
          set_plinko_position(get_plinko_s(i, c), current_d - 1, i, c);
        }
      } else {
        if (current_d >= MAX_DISTANCE - 1) {
          set_plinko_position(XX, XX, i, c);  // turn off plinko
        } else {
          set_plinko_position(get_plinko_s(i, c), current_d + 1, i, c);
        }
      }
    }
  }
}

void set_plinko_position(uint8_t s, uint8_t d, uint8_t index, uint8_t c) {
  set_memory_index(index * 3, c, s);  // index, channel, value
  set_memory_index((index * 3) + 1, c, d);  // index, channel, value
}

void set_plinko_color(uint8_t color, uint8_t index, uint8_t c) {
  set_memory_index((index * 3) + 2, c, color);  // index, channel, value
}

uint8_t get_plinko_s(uint8_t index, uint8_t c) {
  return get_memory_index(index * 3, c);  // index, channel
}

uint8_t get_plinko_d(uint8_t index, uint8_t c) {
  return get_memory_index((index * 3) + 1, c);  // index, channel
}

uint8_t get_plinko_color(uint8_t index, uint8_t c) {
  return get_memory_index((index * 3) + 2, c);  // index, channel
}

uint8_t get_open_plinko(uint8_t c) {
  for (uint8_t i = 0; i < MAX_PLINKO; i++) {
    if (get_plinko_s(i, c) == XX) {
      return i;
    }
  }
  return XX;
}

//
// in out
//
void in_out(uint8_t c) {
  set_start_wave_shows(c);
  for (uint8_t s = 0; s < MAX_SPOKE; s++) {
    for (uint8_t d = 0; d < MAX_DISTANCE; d++) {
      uint8_t phase_offset = sin8_C((d * 255 / MAX_DISTANCE) + (s % MAX_SPOKE) * 255 / MAX_SPOKE);
      uint8_t intensity = beatsin8(get_spare_show_variable(0, c), 0, 255, 0, phase_offset);
      CHSV color = led[c].gradient_wheel(shows[c].IncColor(shows[c].getForeColor(), phase_offset / 4), intensity);
      shows[c].setPixeltoColor(get_pixel(s, d), color);
    }
  }
}

//
// up down
//
void up_down(uint8_t c) {
  set_start_wave_shows(c);
  
  for (uint8_t s = 0; s < MAX_SPOKE; s++) {
    for (uint8_t d = 0; d < MAX_DISTANCE; d++) {
      uint8_t intensity_d = beatsin8(get_spare_show_variable(0, c), 0, 255, 0, sin8_C(d * 255 / MAX_DISTANCE));
      uint8_t intensity_s = beatsin8(get_spare_show_variable(1, c), 0, 255, 0, sin8_C(s * 255 / MAX_SPOKE));
      CHSV color = led[c].gradient_wheel(shows[c].getForeColor(), (intensity_d + intensity_s) / 2);
      shows[c].setPixeltoColor(get_pixel(s, d), color);
    }
  }
}

//
// wave-style shows
//
void wave(uint8_t c) {
  set_start_wave_shows(c);
  shows[c].fillForeBlack();

  for (uint8_t s = 0; s < MAX_SPOKE; s++) {  // "x"
    uint8_t d = beatsin8(get_spare_show_variable(0, c), 0, 255, 0, s * beatsin8(get_spare_show_variable(1, c)) / MAX_SPOKE) / (260 / MAX_DISTANCE);
    shows[c].setPixeltoForeColor(get_pixel(s, d));
  }
}

void fan_spoke(uint8_t c) {
  set_start_wave_shows(c);
  shows[c].fillForeBlack();

  for (uint8_t s = 0; s < MAX_SPOKE; s++) {  // "x"
    uint8_t d_max = beatsin8(get_spare_show_variable(0, c), 0, 255, 0, s * beatsin8(get_spare_show_variable(1, c)) / MAX_SPOKE) / (260 / MAX_DISTANCE);
    for (uint8_t d = 0; d < MAX_DISTANCE; d++) {  // "y"
      shows[c].setPixeltoColor(get_pixel(s, d), 
                               led[c].gradient_wheel(shows[c].getForeColor(), 
                                                     abs(d - d_max) * 255 / MAX_DISTANCE));
    }
  }
}

void fan_distance(uint8_t c) {
  set_start_wave_shows(c);
  shows[c].fillForeBlack();
  
  for (uint8_t d = 0; d < MAX_DISTANCE; d++) {  // "x"
    uint8_t s_max = beatsin8(get_spare_show_variable(0, c), 0, 255, 0, d * beatsin8(get_spare_show_variable(1, c)) / MAX_DISTANCE) / (260 / MAX_SPOKE);
    for (uint8_t s = 0; s < MAX_SPOKE; s++) {  // "y"
      shows[c].setPixeltoColor(get_pixel(s, d), 
                               led[c].gradient_wheel(shows[c].getForeColor(), 
                                                     abs(s - s_max) * 255 / MAX_SPOKE));
    }
  }
}

void set_start_wave_shows(uint8_t c) {
  // Reset a lot of variables at the start of the show
  if (shows[c].isShowStart()) {
    shows[c].turnOffMorphing();  // Because we are using a beatsin8
    uint8_t bpm1 = random(1, 6);
    uint8_t bpm2 = bpm1 * random(1, 4);
    set_spare_show_variable(bpm1, 0, c);
    set_spare_show_variable(bpm2, 1, c);
  }
}

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t get_pixel_distance_to_origin(uint8_t i) {
  uint8_t x = 0;
  uint8_t y = 0;
  get_led_xy(i, &x, &y);
  return get_distance_to_origin(x, y);
}

uint8_t get_distance_to_origin(uint8_t x, uint8_t y) {
  return get_distance_to_coord(x, y, 128, 128);
}

uint8_t get_pixel_distance_to_coord(uint8_t x1, uint8_t y1, uint8_t i) {
  uint8_t x2 = 0;
  uint8_t y2 = 0;
  get_led_xy(i, &x2, &y2);
  return get_distance_to_coord(x1, y1, x2, y2);
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
void get_led_xy(uint8_t i, uint8_t *x, uint8_t *y) {
  uint8_t s = get_spoke_from_pixel(i);
  uint8_t d = get_distance_from_pixel(i);
  
  s = (((s % 5) * 5) + (s / 5)) % 24; // correct petals to line them up concurrently
  
  float theta = 1.833 * (s + ((d * 7) + 7) / 100);
  float r = 128 * sin(1.7143 * theta);
  *x = uint8_t((r * cos(theta)) + 128);
  *y = uint8_t((r * sin(theta)) + 128);
}

//
// mirror_pixels
//
void mirror_pixels(uint8_t channel) {  
  // 2 = halves, 3 = thirds, 4 = quarters, 6 = sixths, etc.
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
  uint8_t pixel = (((5 - d) / 2) * 48) + (s * 2) + ((d + 1) % 2);
  
  if (d == 2 || d == 3) {  // Middle two rings of LEDs 48-95
    pixel++;           // Shift the ring due to wiring
    if (pixel >= 96) {
      pixel -= 48;
    }
  }
  
  if (d <= 1) {   // Inner two rings of LEDs 96-143
    pixel += 4;  // Shift the ring due to wiring
    if (pixel >= 144) {
      pixel -= 48;
    }
  }
  
  leds[pixel % NUM_LEDS] = color;
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
}

//
// mask
//
CHSV mask(CHSV color, uint8_t i, uint8_t channel) {
  if (show_variables[channel + 6] == 0 || current_show[i] == 0) {
    return color;  // no masking
  }
  uint8_t mask_value = show_variables[channel + 6] - 1;
  uint8_t pattern_number = get_memory_index(i, channel);
  boolean value = (pattern_number == 0) ? true : false;
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
