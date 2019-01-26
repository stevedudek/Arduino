//
//  Example of using 1D Noise library
//
//  Include libraries Noise.h, then FastLED.h:
//    #include <Noise.h>
//    #include <FastLED.h>
//
//  Instantiate a global instance of 1D Noise at the top:
//    Noise noise = Noise(NUM_LEDS);
//
//  Within setup(), set the maximum noise range for hue (A) and saturation (B).
//  If not defined, 128 is the default for both A and B,
//  meaning noise can be 0-128, zero-centered to -64 to 64 of the value.
//    noise.setMaxNoise(128, 128);
//
//  Within loop(), fill the noise array with current noise values:
//    noise.fillNoise();
//
//  Periodically change the noise parameters, such as between shows:
//    noise.setRandomNoiseParams();
//
//  To add noise to pixel colors, add the noise specifically to HSV or RGB.
//  Hue, on a wheel, can wrap 0-255
//  Saturation (as well as V or R,G,B) should not wrap
//    color.h = noise.addNoiseAtoValue(i, color.h);
//    color.s = noise.addNoiseBtoValueNoWrap(i, color.s);
//
//  Noise can be turned on of off:
//    noise.turnNoiseOn(), noise.turnNoiseOff()
//
#include <Noise.h>
#include <FastLED.h>

//
//  Linear Lights with FastLED
//
//  6/6/2018
//
#define NUM_LEDS 18

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

CRGB leds[NUM_LEDS];
CHSV current_frame[NUM_LEDS]; // framebuffers
CHSV next_frame[NUM_LEDS];  // framebuffers

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
uint8_t foreColor =  0;    // Starting foreground color
CHSV BLACK = CHSV( 0, 0, 0);

// Clocks and time
uint8_t morph = 0;
uint32_t small_cycle = 0;
uint16_t cycle = 0;
#define SHOW_DURATION 20  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);

// wait times
#define WAIT_DURATION 20 // second between increasing wait time
#define MIN_WAIT   2  // Minimum number of morph steps in a cycle
#define MAX_WAIT  50  // Maximum number of morph steps in a cycle
#define NUM_WAIT_VALUES 10 // Number of stored delay times
uint8_t wait = NUM_WAIT_VALUES / 2;
uint8_t total_frames;

// noise
#define MAX_HUE_NOISE 64   // 255 max
#define MAX_SAT_NOISE 64   // 255 max
Noise noise = Noise(NUM_LEDS);  // library class object of Noise.h

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));

  Serial.begin(4800);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  set_all_black();

  total_frames = getNumFrames(wait);

  noise.setMaxNoise(MAX_HUE_NOISE, MAX_SAT_NOISE);
}

void loop() {

  allOn();
  
  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay

  morph++;
  small_cycle++;
  noise.fillNoise();

  if (morph >= total_frames) {  // Finished morphing

    morph = 0;

    if (cycle++ > 10000) {
      cycle = 0;  // Advance the cycle clock
    }
    push_frame();
  }

  if (small_cycle >= MAX_SMALL_CYCLE) {
    next_show();
  }
}

//
// next_show
//
void next_show() {
  morph = 0;
  small_cycle = 0;
  cycle = 0;
  noise.setRandomNoiseParams();
}

void set_all_black() {
  for (int i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = BLACK;
    next_frame[i] = BLACK;
    leds[i] = BLACK;
  }
  FastLED.show();
}

void fill(CHSV color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, color);
  }
}

//
// All On - turns all the pixels on the foreColor
//
void allOn() {
   fill(Wheel(foreColor));
}

//
// morph_frame
//
void morph_frame() {
   uint8_t fract = map(morph, 0, total_frames-1, 0, 255);  // 0 - 255

   for (int i = 0; i < NUM_LEDS; i++) {
     setLEDinterpHSV(i, current_frame[i], next_frame[i], fract);
   }
   FastLED.show();  // Update the display
}

//
// push_frame
//
void push_frame() {
  for (int i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = next_frame[i];
  }
}

//
// constrain_palette
//
CHSV constrain_palette(uint8_t i, CHSV color) {
  if (noise.isNoiseOn()) {
    color.h = noise.addNoiseAtoValue(i, color.h);
    color.s = noise.addNoiseBtoValueNoWrap(i, color.s);
  }
  if (i==0) {
    Serial.println(color.h + " ");
  }
  return color;
}

void setPixelColor(int pos, CHSV color) {
  next_frame[pos] = color;
}

//
// addPixelColor - add color to the existing color in next_frame
//
void addPixelColor(int pos, CHSV c2) {
  CHSV c1 = next_frame[pos];

  if (c1.v > c2.v) {
      next_frame[pos] = c1;
    } else {
      next_frame[pos] = c2;
    }
}

//
// getNumFrames - convert a wait value into a number of morph frames
//
uint8_t getNumFrames(uint8_t wait_value) {
  return map(wait_value, 0, NUM_WAIT_VALUES-1, MIN_WAIT, MAX_WAIT);
}

//
//  Wheel - Input a hue (0-255) to get a color
//
CHSV Wheel(uint8_t hue)
{
  return Gradient_Wheel(hue, 255);  // 255 = full brightness
}

//
//  Gradient_Wheel - Input a hue and intensity (0-255) to get a CHSV from the palette
//
CHSV Gradient_Wheel(uint8_t hue, uint8_t intensity)
{
  return CHSV(hue, 255, intensity);
}

//
//  setLEDinterpHSV - Set LED i to the interpolate of two HSV colors
//
void setLEDinterpHSV(int i, CHSV c1, CHSV c2, uint8_t fract)
{
  if (c1 == c2) {
    leds[i] = constrain_palette(i, c1);
  } else if (fract == 0) {
    leds[i] = constrain_palette(i, c1);
    return;
  } else if (fract == 255) {
    leds[i] = constrain_palette(i, c2);
    return;
  } else if (is_black(c1)) {
    leds[i] = constrain_palette(i, c2);
    leds[i].fadeToBlackBy(255 - fract);
    return;
  } else if (is_black(c2)) {
    leds[i] = constrain_palette(i, c1);
    leds[i].fadeToBlackBy(fract);
    return;
  } else {
    leds[i] = constrain_palette(i, CHSV(interpolate_wrap(c1.h, c2.h, fract),
                                        interpolate(c1.s, c2.s, fract),
                                        interpolate(c1.v, c2.v, fract) ));
    return;
  }
}

// is_black
boolean is_black(CHSV color) {
  return color == BLACK;
}

//
// Interpolate - returns the fractional point from a to b
//
float interpolate(uint8_t a, uint8_t b, uint8_t fract)
{
  return lerp8by8(a, b, fract);
}

//
// Interpolate Wrap - returns the fractional point from a to b, checking both ways around a circle
//
uint8_t interpolate_wrap(uint8_t a, uint8_t b, uint8_t fract)
{
  uint8_t distCCW, distCW;

  if (a >= b) {
    distCW = 256 + b - a;
    distCCW = a - b;
  } else {
    distCW = b - a;
    distCCW = 256 + a - b;
  }
  if (distCW <= distCCW) {
    return a + map8(fract, 0, distCW);
  } else {
    return a - map8(fract, 0, distCCW);
  }
}

