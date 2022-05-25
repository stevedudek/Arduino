#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
//
//  Dragon Test pattern
//
//  3/19/22
//
//  On the Adafruit Feather
//
//  Listens to the Network and an IR Controller
//

uint8_t bright = 255;  // (0-255)
uint8_t curr_bright = bright;

uint8_t show_speed = 50;  // (0 = fast, 255 = slow)

#define DELAY_TIME 255  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN  0

#define NUM_LEDS 100

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define XX  255

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // The Leds themselves
CHSV led_buffer[NUM_LEDS];  // For smoothing

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 3
#define START_SHOW_CHANNEL_A  1  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  0
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

// wait times
uint8_t show_duration = 100;  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t fade_amount = 0; // 148;  // 0 = no fading, to 255 = always be fading
uint32_t max_small_cycle = (show_duration * 2 * (1000 / DELAY_TIME));  // *2! 50% = all on, 50% = all off, and rise + decay on edges


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(DATA_PIN, OUTPUT);
  
  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");

  FastLED.addLeds<WS2811, DATA_PIN>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness( bright );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
  }
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(max_small_cycle / 2);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
  }
}

//
// loop
//
void loop() {

  for (uint8_t i = 0; i < DUAL; i++) {

    switch (current_show[i]) {
  
      case 0:
        test_pattern(i, 0);
        break;
      case 1:
        test_pattern(i, 1);
        break;
      default:
        test_pattern(i, 2);
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show

  delay(50);
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
//  shows[i].fillForeBlack();
  led[i].push_frame();  // This might not work well with speaking & hearing
  shows[i].resetAllClocks();
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  current_show[i] = 1;
//  log_status(i);  // For debugging
}


//
// Test Pattern
//
void test_pattern(uint8_t channel, uint8_t version) {
  shows[channel].fillBlack();
  shows[channel].setForeColor(0);
  shows[channel].setBackColor(180);
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t outcome = (i + version) % 3;
    if (outcome == 1) {
      shows[channel].setPixeltoForeColor(i);
    } else if (outcome == 2) {
      shows[channel].setPixeltoBackColor(i);
    } else {
      shows[channel].setPixeltoBlack(i);
    }
  }
}
//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {  
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getInterpFrameColor(i);
    CHSV color_a = led[CHANNEL_A].getInterpFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }

    leds[i] = color;
    led_buffer[i] = color;
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
