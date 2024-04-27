#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>

//
//  Red Dinosaur
//
//  29 LEDs = 7 left side + 15 center + 7 right side
//
//  4/13/19
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  FastLED
//
//  Dual shows - Blend together 2 shows running at once
//
uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 40;  // in milliseconds (ArduinoBlue)

#define DATA_PIN 7
#define CLOCK_PIN 8

#define NUM_LEDS 29
#define SYM_LEDS 22
#define SPINE_LEDS 14

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define XX  255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[NUM_LEDS];  // The Leds themselves

// Shows
#define NUM_SHOWS 15
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 1 };

// ArduinoBlue
#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

#define MAX_COLOR 256   // Colors are 0-255
#define WHITE  CHSV(0, 0, 255)
#define BLACK  CHSV(0, 0, 0)

// wait times
#define SHOW_DURATION 30  // seconds
uint8_t FADE_TIME = 10;  // seconds to fade in + out (Arduino Blue)
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

// center column | left side | right side
const uint8_t ConeLookUp[NUM_LEDS] PROGMEM = { 0,1,3,5,6,7,8,19,18,17,16,15,26,27,28,
                                2,9,10,11,12,13,14,
                                4,20,21,22,23,24,25 };

const uint8_t ConeSize[NUM_LEDS] PROGMEM = { 2,1,2,3,2,1,1,1,1,1,1,2,3,4,5,
                      1,0,0,0,0,0,0,
                      1,0,0,0,0,0,0 };

const uint8_t ArrowPattern[NUM_LEDS] PROGMEM = 
{ 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                  2, 7, 6, 5, 4, 3, 2,
                  2, 7, 6, 5, 4, 3, 2 };

#define NUM_PATTERNS 6   // Total number of patterns

const uint8_t PatternMatrix[] PROGMEM = {
  B00000001, B01010100, B10101001, B01010000, 
  B10101010, B10101010, B10101010, B10100000, 
  B10101010, B10101011, B01010101, B01010000, 
  B11111101, B01010101, B01010010, B10100000, 
  B11111111, B01101110, B11011001, B10110000, 
  B11111111, B11111110, B00000000, B00000000,                
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
//    led[i].setLedMap(ConeLookUp);  // Handled
    led[i].fillBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
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
        shows[i].morphChain();
        break;
      case 2:
        shows[i].allOn();
        break;
      case 3:
        shows[i].randomFill();
        break;
      case 4:
        shows[i].randomColors();
        break;
      case 5:
        shows[i].twoColor();
        break;
      case 6:
        shows[i].lightWave();
        break;
      case 7:
        shows[i].sawTooth();
        break;
      case 8:
        shows[i].packets();
        break;
      case 9:
        shows[i].packets_two();
        break;
      case 10:
        colorsize(i);
        break;
      case 11:
        brightsize(i);
        break;
      case 12:
        colorarrow(i);
        break;
      case 13:
        brightarrow(i);
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
      next_show(i);
      pick_next_show(i);
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
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(4, 40, 36);
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

//// Start specialized shows

//
// patterns shows
//
void patterns(uint8_t c) {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    boolean value = get_bit_from_pattern_number(i, current_pattern[c]);
    
    if (value) {
      led[c].setPixelHue(i, shows[c].getBackColor());
    } else {
      led[c].setPixelHue(i, shows[c].getForeColor());
    }
  }
}

boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern_byte = PatternMatrix[(pattern_number * 4) + (n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

//
// colorsize - light each cone according to its cone size
//
void colorsize(uint8_t c) {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), pgm_read_byte_near(ArrowPattern + i) * sin8(shows[c].getBackColor())));
  }
}

//
// brightsize -Light just one cone size at a time
//
void brightsize(uint8_t c) {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), calcIntensity(pgm_read_byte_near(ArrowPattern + i), 9, c)));
  }
}

//
// colorarrow - light each cone according to its arrow position
//
void colorarrow(uint8_t c) {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    led[c].setPixelHue(i, IncColor(shows[c].getForeColor(), pgm_read_byte_near(ConeSize + i) * sin8(shows[c].getBackColor())));
  }
}

//
// brightarrow -Light just one arrow piece at a time
//
void brightarrow(uint8_t c) {
  for (int i=0; i < NUM_LEDS; i++) {
    led[c].setPixelColor(i, led[c].gradient_wheel(shows[c].getBackColor(), calcIntensity(pgm_read_byte_near(ConeSize + i), 5, c)));
  }
}

//
// calcIntensity - use sine wave + cycle + variable to calculate intensity
//
uint8_t calcIntensity(uint8_t x, uint8_t max_x, uint8_t i) {
  return sin8( 255 * ((shows[i].getCycle() + x) % max_x) / (max_x - 1));
}

//// End specialized shows

//
// update_leds - push the interp frame on to the leds_buffer
//
void update_leds() {
  for (uint8_t channel = 0; channel < DUAL; channel++) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      leds_buffer[channel][i] = led[channel].getInterpFrameColor(i);
    }
  }
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][i], 
                                             leds_buffer[CHANNEL_A][i], 
                                             fract);  // interpolate a + b channels
    color = lightning(narrow_palette(color));               
//    leds[i] = color;  // (ArduinoBlue)
    
    if (i < SYM_LEDS) {
      leds[ConeLookUp[i]] = color;
  
      if (i >= SPINE_LEDS) {
        leds[ConeLookUp[i + 7]] = color;
      }
    }
    
  }
}

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
  return ease8InOutQuad(intensity);
}

//// End DUAL SHOW LOGIC


//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}
