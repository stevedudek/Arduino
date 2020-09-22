#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>  // (ArduinoBlue)
//
//  Small Pentagon: 6 x 6 = 36 lights in a square/pentagon grid
//
//  8/11/20
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Dual shows - Blend together 2 shows running at once
//
//  Removed: Noise
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds

//// Check Patterns
//// Game of Life (4-bit?)

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 40;  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
long last_time;
#define MIN_DELAY_CYCLES  10
#define MAX_DELAY_CYCLES 100

#define DATA_PIN 7
#define CLOCK_PIN 8

#define NUM_LEDS 36
#define NUMBER_SPACER_LEDS 6
#define TOTAL_LEDS  (NUM_LEDS + NUMBER_SPACER_LEDS)

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define MAX_XCOORD  6
#define MAX_YCOORD  6

#define XX  255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[NUM_LEDS];  // The Leds themselves

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 15
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

uint8_t current_pattern[] = { 0, 0 };

// wait times
#define SHOW_DURATION 40  // seconds
uint8_t FADE_TIME = 20;  // seconds to fade in + out (Arduino Blue)
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out
uint8_t freq_storage[] = { 60, 80 };  // 1-byte storage for shows

// Game of Life
#define LIFE_DIMENSION  10
#define TOTAL_LIFE  (LIFE_DIMENSION * LIFE_DIMENSION)
#define LIFE_OFFSET  2  //((LIFE_DIMENSION - MAX_XCOORD) / 2)
boolean LIFE_BOARD[TOTAL_LIFE];  // Make this a 2-bit compression

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

#define NUMBER_SPACER_LEDS 6
uint8_t spacer_leds[] = { 12, 13, 14, 27, 28, 29 };

const uint8_t LED_LOOKUP[] PROGMEM = {
    0,  2,  5,  6,  9, 10,
    1,  3,  4,  7,  8, 11,
   26, 23, 22, 19, 18, 16,
   25, 24, 21, 20, 17, 15,
   30, 32, 35, 36, 39, 40,
   31, 33, 34, 37, 38, 41
};

uint8_t neighbors[] PROGMEM = {
  11, 1, XX, XX, 
  10, 2, XX, 0, 
  9, 3, XX, 1, 
  8, 4, XX, 2, 
  7, 5, XX, 3, 
  6, XX, XX, 4, 
  17, XX, 5, 7, 
  16, 6, 4, 8, 
  15, 7, 3, 9, 
  14, 8, 2, 10, 
  13, 9, 1, 11, 
  12, 10, 0, XX, 
  23, 13, 11, XX, 
  22, 14, 10, 12, 
  21, 15, 9, 13, 
  20, 16, 8, 14, 
  19, 17, 7, 15, 
  18, XX, 6, 16, 
  29, XX, 17, 19, 
  28, 18, 16, 20, 
  27, 19, 15, 21, 
  26, 20, 14, 22, 
  25, 21, 13, 23, 
  24, 22, 12, XX, 
  35, 25, 23, XX, 
  34, 26, 22, 24, 
  33, 27, 21, 25, 
  32, 28, 20, 26, 
  31, 29, 19, 27, 
  30, XX, 18, 28, 
  XX, XX, 29, 31, 
  XX, 30, 28, 32, 
  XX, 31, 27, 33, 
  XX, 32, 26, 34, 
  XX, 33, 25, 35, 
  XX, 34, 24, XX,
};

#define NUM_PATTERNS 8   // Total number of patterns, each 5 bytes wide

const uint8_t PatternMatrix[] PROGMEM = {
  0xcf, 0x33, 0xc, 0xcf, 0x30,
  0xa9, 0x5a, 0x95, 0xa9, 0x50,
  0x1, 0xe4, 0x92, 0x78, 0x0,
  0x91, 0x22, 0x64, 0x48, 0x90,
  0x30, 0xcf, 0xff, 0x30, 0xc0,
  0x0, 0x3, 0xc, 0x0, 0x0,
  0xc, 0x73, 0x9c, 0xe3, 0x0,
  0x30, 0xcf, 0x3c, 0x0, 0x0,
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, TOTAL_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 4 neighbors for every pixel
    led[i].fillBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    
    shows[i].setColorSpeedMinMax(1,10);  // Make colors change faster (lower = faster)
    shows[i].setBandsBpm(10, 30);

    shows[i].setAsSquare();  // Set shows & leds objects to use 4-neighbor square geometry
  }
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);
  last_time = millis();

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
        shows[i].bounce();
        break;
      case 3:
        shows[i].bounceGlowing();
        break;
      case 4:
        shows[i].plinko(2);
        break;
      case 5:
        shows[i].packets();
        break;
      case 6:
        shows[i].packets_two();
        break;
      case 7:
        windmill(i);
        break;
      case 8:
        windmill_smoothed(i);
        break;
      case 9:
        cone_push(i);
        break;
      case 10:
        pendulum_wave(i, true);
        break;
      case 11:
        pendulum_wave(i, false);
        break;
      case 12:
        game_of_life(i, true);
        break;
      case 13:
        game_of_life(i, false);
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
  
  if (curr_lightning > 0 ) {  // (ArduinoBlue)
    curr_lightning--; // Reduce the current bolt
  }
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
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  led[i].fillBlack();
  led[i].push_frame();
  
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(2, 22, 20);
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
// get_pixel_from_coord - get LED i from x,y grid coordinate
//
uint8_t get_pixel_from_coord(uint8_t x, uint8_t y) {
  return (y * MAX_XCOORD) + x;
}

//
// get_dist - normalized to 0 - 255 (far corner)
//
uint8_t get_dist(uint8_t x1, uint8_t y1, float x2, float y2) {
  uint16_t dx = sq(uint8_t(x2*30) - (x1*30));
  uint16_t dy = sq(uint8_t(y2*30) - (y1*30));
  return sqrt( dx + dy );
}

//
// patterns shows
//
void patterns(uint8_t c) {
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      boolean value = get_bit_from_pattern_number((y * MAX_XCOORD) + x, current_pattern[c]);
      uint8_t i = get_pixel_from_coord(x,y);

      if (value) {
        led[c].setPixelHue(i, shows[c].getBackColor());
      } else {
        led[c].setPixelHue(i, shows[c].getForeColor());
      }
    }
  }
}

//
// Cone Push
//
void cone_push(uint8_t i) {
  float center_x;
  float center_y;
  
  if (shows[i].isShowStart()) {
    shows[i].turnOffMorphing();
    center_x = 2.5;
    center_y = 2.5;
  }
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      uint8_t dist = get_dist(x, y, center_x, center_y);
      uint8_t value = beatsin8(20, 0 + dist, 255 - dist, 0, 0);
      led[i].setPixelColor(get_pixel_from_coord(x,y), CHSV(shows[i].getForeColor(), 255, value) );
    }
  }
}

//
// Pendulum Wave
//
void pendulum_wave(uint8_t i, boolean smoothed) {
  uint8_t value, pos, dist;
  
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(40, 100);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    value = beatsin8(freq_storage[i] - (4 * x));
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      pos = (y * 256 / MAX_YCOORD) + (256 / (MAX_YCOORD * 2));
      dist = (pos >= value) ? 255 - (pos - value) : 255 - (value - pos) ;
      if (smoothed == false) {
        dist = (dist > 230) ? dist : 0 ;
      }
      led[i].setPixelColor(get_pixel_from_coord(x,y), CHSV(shows[i].getForeColor(), 255, dist) );
    }
  }
}

//
// Windmill
//
void windmill(uint8_t i) {
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(40, 100);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      uint8_t value = beatsin8(freq_storage[i] - (4 * y), 0, 255, 0, x * 128 / MAX_XCOORD );
      value = (value > 235) ? value : 0 ;
      led[i].setPixelColor(get_pixel_from_coord(x,y), CHSV(shows[i].getForeColor(), 255, value) );
    }
  }
}

void windmill_smoothed(uint8_t i) {
  if (shows[i].isShowStart()) {
    freq_storage[i] = random(40, 100);
    shows[i].turnOffMorphing();
  }
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      uint8_t value = beatsin8(freq_storage[i] - (4 * y), 0, 255, 0, x * 255 / MAX_XCOORD );
      led[i].setPixelColor(get_pixel_from_coord(x,y), CHSV(shows[i].getForeColor(), 255, value) );
    }
  }
}

//
// Game of Life
//
void game_of_life(uint8_t i, boolean faster) {
  if (shows[i].isShowStart() || count_life() < 10) {
    randomly_populate_life(random(20, 50));
    if (faster) { shows[i].makeWaitFaster(2); }
    else { shows[i].makeWaitSlower(2); }
  }
  if (shows[i].getMorph() == 0) { 
    grow_life(i);
  }
}

void grow_life(uint8_t i) {
  shows[i].fillBlack();
  
  for (uint8_t x = 0; x < LIFE_DIMENSION; x++) {
    for (uint8_t y = 0; y < LIFE_DIMENSION; y++) {
      uint8_t neighbors = count_life_neighbors(x,y);
      
      if (has_life(x,y) == 0) {
        if (neighbors == 3) {
          set_life_coord(x,y, true, 180, i);  // spontaneously grow a pixel
        } else {
          set_life_coord(x,y, false, 0, i);  // Turn pixel off
        }
      } else {
        
        switch(neighbors) {
          case 2:
            set_life_coord(x,y, true, 160, i);
            break;
          case 3:
            set_life_coord(x,y, true, 160, i);
            break;
          default:
            set_life_coord(x,y, false, 200, i);
            break;
        }
      }
    }
  }
}

uint8_t count_life() {
  uint8_t life = 0;
  for (uint8_t x = 0; x < LIFE_DIMENSION; x++) {
    for (uint8_t y = 0; y < LIFE_DIMENSION; y++) {
      life += has_life(x,y);
    }
  }
  return life;
}

uint8_t count_life_neighbors(uint8_t x, uint8_t y) {
  uint8_t neighbors = 0;
  
  for (uint8_t dx = 0; dx < 3; dx++) {  // Look at grid of 9 pixels
    for (uint8_t dy = 0; dy < 3; dy++) {
      if (dx == 1 && dy == 1) { continue; }  // Center (identity) square
      neighbors += has_life(x + dx - 1, y + dy - 1);
    }
  }
  return neighbors;
}

uint8_t has_life(uint8_t x, uint8_t y) {
  if (x < 0 || x >= LIFE_DIMENSION || y < 0 || y >= LIFE_DIMENSION || get_life_from_coord(x,y) == false) {
    return 0;
  }
  return 1;
}

void randomly_populate_life(uint8_t seeds) {
  for (uint8_t i = 0; i < TOTAL_LIFE; i++) { set_life(i, false); }
  for (uint8_t i = 0; i < seeds; i++) { set_life(random8(TOTAL_LIFE), true); }
}

void print_life_board() {
  for (uint8_t x = 0; x < LIFE_DIMENSION; x++) {
    for (uint8_t y = 0; y < LIFE_DIMENSION; y++) {
      Serial.print(has_life(x,y));
    }
    Serial.println();
  }
}

uint8_t get_life_pix_from_coord(uint8_t x, uint8_t y) {
  return (y * LIFE_DIMENSION) + x;
}

boolean get_life_from_coord(uint8_t x, uint8_t y) {
  boolean life = LIFE_BOARD[get_life_pix_from_coord(x,y)];
  return life;
}

boolean get_life(uint8_t i) {
  return LIFE_BOARD[i];
}

void set_life(uint8_t i, boolean value) {
  LIFE_BOARD[i] = value;
}

void set_life_coord(uint8_t x, uint8_t y, boolean value, uint8_t hue, uint8_t i) {
  set_life(get_life_pix_from_coord(x,y), value);
  
  if ((x >= LIFE_OFFSET) && (x < (LIFE_DIMENSION - LIFE_OFFSET)) && 
      (y >= LIFE_OFFSET) && (y < (LIFE_DIMENSION - LIFE_OFFSET))) {
        if (value == false && hue == 0) {
          shows[i].setPixeltoBlack(get_pixel_from_coord(x - LIFE_OFFSET, y - LIFE_OFFSET));
        } else {
          shows[i].setPixeltoHue(get_pixel_from_coord(x - LIFE_OFFSET, y - LIFE_OFFSET), hue);
        }
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
  uint8_t led_number;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    led_number = convert_pixel_to_led(i);
    if (led_number != XX) {
      CHSV color = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][i], 
                                             leds_buffer[CHANNEL_A][i], 
                                             fract);  // interpolate a + b channels
      leds[led_number] = lightning(narrow_palette(color));  // (ArduinoBlue)
    }
  }
//  turn_off_spacer_leds();
}

//
// convert pixel to led: account for the spacer pixels
//
uint8_t convert_pixel_to_led(uint8_t i) {
  if (i == XX) {
    return XX;
  }
  return pgm_read_byte_near(LED_LOOKUP + (i % NUM_LEDS));
}

//
// turn off spacer leds - blacken the 16 space pixels
//
void turn_off_spacer_leds() {
  for (uint8_t i = 0; i < NUMBER_SPACER_LEDS; i++) {
    leds[spacer_leds[i]] = CRGB(0, 0, 0);
  }
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
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern_byte = pgm_read_byte_near(PatternMatrix + (pattern_number * 5) + (n / 8) );
  return (pattern_byte & (1 << (7 - (n % 8)) ));
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
