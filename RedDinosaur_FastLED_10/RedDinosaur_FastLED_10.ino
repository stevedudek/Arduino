#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Red Dinosaur
//
//  29 LEDs = 7 left side + 15 center + 7 right side
//
//  12/18/22
//
//  Feather Controller
//
//  No WiFi
//
//  Do not use PROGMEM - crashes
//
uint8_t bright = 128;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 128;  // (0 = fast, 255 = slow)

uint8_t DELAY_TIME = 20;  // in milliseconds (ArduinoBlue)
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 0  // 7
#define CLOCK_PIN 2  //  8

#define NUM_LEDS 29
#define SYM_LEDS 22
#define SPINE_LEDS 14

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define XX  255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[NUM_LEDS];  // Hook for FastLED library

// Shows
#define NUM_SHOWS 18
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 1 };

#define ONLY_RED true  // This is the RED Dinosaur
uint8_t hue_center = 0;
uint8_t hue_width = 255;
uint8_t saturation = 255;

#define MAX_COLOR 256   // Colors are 0-255
#define WHITE  CHSV(0, 0, 255)
#define BLACK  CHSV(0, 0, 0)

// wait times
uint8_t show_duration = 40;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 196; // 148;  // 0 = no fading, to 255 = always be fading
uint32_t max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // *2! 50% = all on, 50% = all off, and rise + decay on edges
uint32_t last_time = 0;

// center column | left side | right side
const uint8_t ConeLookUp[] = { 0,1,3,5,6,7,8,19,18,17,16,15,26,27,28,
                                2,9,10,11,12,13,14,
                                4,20,21,22,23,24,25 };

const uint8_t ConeSize[] = { 2,1,2,3,2,1,1,1,1,1,1,2,3,4,5,
                      1,0,0,0,0,0,0,
                      1,0,0,0,0,0,0 };

const uint8_t ArrowPattern[] = 
{ 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                  2, 7, 6, 5, 4, 3, 2,
                  2, 7, 6, 5, 4, 3, 2 };

#define NUM_PATTERNS 6   // Total number of patterns

const uint8_t PatternMatrix[] = {
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

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  
  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( bright );

  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
//    led[i].setLedMap(ConeLookUp);  // Handled
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
  }
  
  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);  // Start Channel B offset at halfway through show
  last_time = millis();
  
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();
  }

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_center = 0;
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
      case 14:
        shows[i].sinelon_fastled();
        break;
      case 15:
        shows[i].bpm_fastled();
        break;
      case 16:
        shows[i].juggle_fastled();
        break;
      default:
        shows[i].bands();
        break;
    }
  
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }
  
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
    if (shows[i].getSmallCycle() >= max_small_cycle) { 
      next_show(i); 
    }
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  current_show[i] = is_other_channel_show_zero(i) ? random8(1, NUM_SHOWS) : 0 ;
  current_pattern[i] = random8(NUM_PATTERNS);
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging

  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(show_speed);
  shows[i].setColorSpeedMinMax(show_speed);
  
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
}

boolean is_other_channel_show_zero(uint8_t c) {
  if (c == 0) {
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
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getInterpFrameColor(i);
    CHSV color_a = led[CHANNEL_A].getInterpFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    
    color = narrow_palette(color);

    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }

    led_buffer[i] = color;

    if (i < SYM_LEDS) {
      leds[ConeLookUp[i]] = color;
      
      if (i >= SPINE_LEDS) {
        leds[ConeLookUp[i + 7]] = color;
      }
    }
  }
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();
  uint16_t fade_cycles =  (map8(fade_amount, 0, show_duration) * 1000 / DELAY_TIME);  // cycles to fade in + out

  if (small_cycle < fade_cycles) {
    intensity = map(small_cycle, 0, fade_cycles, 0, 255);  // rise
  } else if (small_cycle <= (max_small_cycle / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((max_small_cycle / 2) + fade_cycles)) {
    intensity = map(small_cycle - (max_small_cycle / 2), 0, fade_cycles, 255, 0);  // decay
  } else {
    intensity = 0;
  }

  return ease8InOutApprox(intensity);
}

//// End DUAL SHOW LOGIC

//
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  uint8_t h1 = (hue_center - (hue_width / 2)) % MAX_COLOR;
  uint8_t h2 = (hue_center + (hue_width / 2)) % MAX_COLOR;
  color.h = map8(color.h, h1, h2 );
  if (color.s != 0) {
    color.s = saturation;  // Reset saturation
  }
  return color;
}

//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}
