//
//  Led.cpp - Handles morphing, RGB/HSV interpolation, and color palettes
//
#include <FastLED.h>

#include "Led.h"  // include this library's description file

#define BLACK  CHSV(0, 0, 0)
#define XX  255

//
// Constructor
//
Led::Led(uint8_t i)
{
  numLeds = i;

  current_frame = (CHSV *)calloc(i, sizeof(CHSV));
  next_frame = (CHSV *)calloc(i, sizeof(CHSV));
  interp_frame = (CHSV *)calloc(i, sizeof(CHSV));

  is_only_red = false;
  turnPalettesOff();  // Palettes default to off

  is_mapped = false;
  is_2d_mapped = false;
  is_neighbor_mapped = false;

  turnOffBlur();

  num_neighbors = 6;
}

//
// Public Methods //////////////////////////////////////////////////////////////
//
uint8_t Led::getNumLeds(void)
{
  return numLeds;
}

void Led::fill(CHSV color)
{
  for (uint8_t i = 0; i < numLeds; i++) {
    setPixelColor(i, color);
  }
}

void Led::fillHue(uint8_t hue)
{
  for (uint8_t i = 0; i < numLeds; i++) {
    setPixelHue(i, hue);
  }
}

void Led::fillBlack(void)
{
  for (uint8_t i = 0; i < numLeds; i++) {
    setPixelColor(i, BLACK);
  }
}

void Led::setPixelColor(uint8_t i, CHSV color)
{
  if (i != XX) {
    if (is_only_red) {
      color.h = map8(color.h, 224, 32);
    }
    if (hasBlur()) {
      if (is_black(color)) return;
      color.v = scale8(color.v, 255 - blur_amount);
      addPixelColor(i, color);
    } else {
      next_frame[lookupLed(i)] = color;
    }
  }
}

void Led::setPixelHue(uint8_t i, uint8_t hue)
{
  setPixelColor(i, wheel(hue));
}

void Led::setPixelBlack(uint8_t i)
{
  setPixelColor(i, BLACK);
}

void Led::setPixelColorNoMap(uint8_t i, CHSV color)
{
  if (i != XX) {
    if (is_only_red) {
      color.h = map8(color.h, 224, 32);
    }
    if (hasBlur()) {
      if (is_black(color)) return;
      color.v = scale8(color.v, 255 - blur_amount);
      addPixelColorNoMap(i, color);
    } else {
      next_frame[i] = color;
    }
  }
}

void Led::setPixelHueNoMap(uint8_t i, uint8_t hue)
{
  setPixelColorNoMap(i, wheel(hue));
}

void Led::setPixelBlackNoMap(uint8_t i)
{
  setPixelColorNoMap(i, BLACK);
}

void Led::dimPixel(uint8_t i, uint8_t amount)
{
  if (i != XX) {
    next_frame[lookupLed(i)].v = scale8(next_frame[lookupLed(i)].v, 255 - amount);
  }
}

void Led::dimAllPixels(uint8_t amount)
{
  for (uint8_t i = 0; i < numLeds; i++) {
    dimPixel(i, amount);
  }
}

// This calculates the interpolated frame, but does not update the leds
void Led::morph_frame(uint8_t morph, uint8_t total_frames)
{
  uint8_t fract = map(morph, 0, total_frames, 0, 255);  // 0 - 255

  for (uint8_t i = 0; i < numLeds; i++) {
    interp_frame[i] = getInterpHSV(current_frame[i], next_frame[i], fract);
  }
}

void Led::push_frame(void)
{
  for (uint8_t i = 0; i < numLeds; i++) {
    current_frame[i] = next_frame[i];
  }
}

void Led::turnOffBlur(void)
{
  blur_amount = 0;
}

void Led::setBlur(uint8_t b)
{
  blur_amount = b;
}

bool Led::hasBlur(void)
{
  return (blur_amount != 0);
}

void Led::addPixelColor(uint8_t i, CHSV c2)
{
  // Pick the dominant color (c1 vs. c2) by value, instead of morphing them
  if (i != XX) {
    CHSV c1 = next_frame[lookupLed(i)];

    if (c1.v > c2.v) {
      next_frame[lookupLed(i)] = c1;
    } else {
      next_frame[lookupLed(i)] = c2;
    }
  }
}

void Led::addPixelColorNoMap(uint8_t i, CHSV c2)
{
  // Pick the dominant color (c1 vs. c2) by value, instead of morphing them
  if (i != XX) {
    CHSV c1 = next_frame[i];

    if (c1.v > c2.v) {
      next_frame[i] = c1;
    } else {
      next_frame[i] = c2;
    }
  }
}

CHSV Led::getCurrFrameColor(uint8_t i)
{
  return current_frame[i];
}

CHSV Led::getNextFrameColor(uint8_t i)
{
  return next_frame[i];
}

CHSV Led::getInterpFrameColor(uint8_t i)
{
  return interp_frame[i];
}

uint8_t Led::getInterpFrameHue(uint8_t i)
{
  return interp_frame[i].h;
}

uint8_t Led::getInterpFrameSat(uint8_t i)
{
  return interp_frame[i].s;
}

uint8_t Led::getInterpFrameVal(uint8_t i)
{
  return interp_frame[i].v;
}

void Led::setInterpFrame(uint8_t i, CHSV color)
{
  interp_frame[i] = color;
}

void Led::setInterpFrameHue(uint8_t i, uint8_t hue)
{
  interp_frame[i].h = hue;
}

void Led::setInterpFrameSat(uint8_t i, uint8_t sat)
{
  interp_frame[i].s = sat;
}

void Led::setInterpFrameVal(uint8_t i, uint8_t val)
{
  interp_frame[i].v = val;
}

bool Led::is_black(CHSV color)
{
  return color.v == 0;
//  return color == BLACK;
}

CHSV Led::wheel(uint8_t hue)
{
  return gradient_wheel(hue, 255);  // 255 = full brightness
}

CHSV Led::gradient_wheel(uint8_t hue, uint8_t intensity)
{
  return CHSV(hue, 255, dim8_raw(intensity));  // middle = saturation
}


CHSV Led::rgb_to_hsv( CRGB rgb)
{
    // Modified 10/28/18 to avoid floats: works!
    // uint8_t's conserve more memory, but will be less accurate
    // https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
    CHSV hsv;
    uint8_t rgbMin, rgbMax;

    rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
    rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

    hsv.v = rgbMax;
    if (hsv.v == 0)
    {
        hsv.h = 0;
        hsv.s = 0;
        return hsv;
    }

    hsv.s = 255 * long(rgbMax - rgbMin) / hsv.v;
    if (hsv.s == 0)
    {
        hsv.h = 0;
        return hsv;
    }

    if (rgbMax == rgb.r)
        hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
    else if (rgbMax == rgb.g)
        hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
    else
        hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

    return hsv;
}


/*
CHSV Led::rgb_to_hsv( CRGB color)
{
  // Can this be rewritten to avoid floats? SD 10/24/2018
  // DEPRECATED CODE (10/16/2018) saved for archival purposes
  float h,s,v;
  float* p_h;
  float* p_s;
  float* p_v;
  p_h = &h;
  p_s = &s;
  p_v = &v;

  RGBtoHSV(color.r, color.g, color.b, p_h, p_s, p_v);

  return CHSV(byte(h * 255 / 360), byte(s * 255), byte(v * 255));
}


void Led::RGBtoHSV(uint8_t red, uint8_t green, uint8_t blue, float *h, float *s, float *v )
{
  // Can this be rewritten to avoid floats? SD 10/24/2018
  //
  // r,g,b values are from 0 to 255
  // h = [0,360], s = [0,1], v = [0,1]
  // if s == 0, then h = -1 (undefined)
  //
  // code from http://www.cs.rit.edu/~ncs/color/t_convert.html
  //
  float r = red/float(255);
  float g = green/float(255);
  float b = blue/float(255);

  float MIN = min(r, min(g,b));  // min(r,g,b)
  float MAX = max(r, max(g,b));  // max(r,g,b)

  *v = MAX;            // v

  float delta = MAX - MIN;

  if (MAX != 0 ) *s = delta / MAX;  // s
  else { // r = g = b = 0   // s = 0, v is undefined
    *s = 0;
    *h = -1;
    return;
  }
  if( r == MAX ) *h = 60.0 * ( g - b ) / delta; // between yellow & magenta
  else {
    if( g == MAX ) {
      *h = 120.0 + 60.0 * ( b - r ) / delta; // between cyan & yellow
    } else {
      *h = 240.0 + 60.0 * ( r - g ) / delta;  // between magenta & cyan
    }
  }
  if( *h < 0 ) *h += 360;
}
*/


void Led::setAsSquare(void)
{
  num_neighbors = 4;
}

void Led::setAsPentagon(void)
{
  num_neighbors = 5;
}

void Led::setOnlyRed(void)
{
  is_only_red = true;
  turnPalettesOff();
}

void Led::setPalette(uint8_t palette_start, uint8_t palette_width)
{
  _palette_start = palette_start;
  _palette_width = palette_width;
  turnPalettesOn();
}

void Led::setPalette()
{
  setPalette(0, 255);  // palette_start, palette_width
}

void Led::turnPalettesOn(void)
{
  canChangePalette = true;
}

void Led::turnPalettesOff(void)
{
  canChangePalette = false;
}

void Led::randomizePalette(void)
{
  _palette_start = random8(255);
  _palette_width = random8(10, 255);
}

CHSV Led::constrain_palette(CHSV color)
{
  if (!canChangePalette) {
    return color;
  }
  uint8_t hue = map8(sin8(color.h), _palette_start, (_palette_start + _palette_width) % 256);
  CHSV new_color = CHSV(hue, color.s, color.v);
  return new_color;
}

CHSV Led::getInterpHSV(CHSV c1, CHSV c2, uint8_t fract)
{
  // Includes palette adjustment
  if (c1 == c2) {
    return c1;
  } else if (fract == 0) {
    return c1;
  } else if (fract == 255) {
    return c2;
  } else {
//    return getInterpHSVthruRGB(c1, c2, fract);
//    color = CHSV(interpolate_wrap(c1.h, c2.h, fract),
//                 255,  // s
//                 interpolate(c1.v, c2.v, fract)
//                );
//  // Correct approach below does not work well for blacks
//  } else if (is_black(c1)) {
//    color = CHSV(c2.h, c2.s, interpolate(c1.v, c2.v, fract));  // safe approach
//  } else if (is_black(c2)) {
//    color = CHSV(c1.h, c1.s, interpolate(c1.v, c2.v, fract));  // safe approach
//  } else {
//    color = getInterpHSVthruRGB(c1, c2, fract);  // Much sadness: this does not work
    return CHSV(interpolate_wrap(c1.h, c2.h, fract),
                 interpolate(c1.s, c2.s, fract),
                 interpolate(c1.v, c2.v, fract)
                );
  }
//  return constrain_palette(color);  // palette adjustment
}

CHSV Led::getInterpHSV(CHSV c1, CHSV c2, CHSV old_color, uint8_t fract)
{
  // Includes palette adjustment
  if (c1 == c2) {
    return c1;
  } else if (fract == 0) {
    return c1;
  } else if (fract == 255) {
    return c2;
  } else {
//    return getInterpHSVthruRGB(c1, c2, fract);
//    color = CHSV(interpolate_wrap(c1.h, c2.h, fract),
//                 255,  // s
//                 interpolate(c1.v, c2.v, fract)
//                );
//  // Correct approach below does not work well for blacks
//  } else if (is_black(c1)) {
//    color = CHSV(c2.h, c2.s, interpolate(c1.v, c2.v, fract));  // safe approach
//  } else if (is_black(c2)) {
//    color = CHSV(c1.h, c1.s, interpolate(c1.v, c2.v, fract));  // safe approach
//  } else {
//    color = getInterpHSVthruRGB(c1, c2, fract);  // Much sadness: this does not work
    return CHSV(interpolate_wrap(c1.h, c2.h, old_color.h, fract),
                 interpolate(c1.s, c2.s, fract),
                 interpolate(c1.v, c2.v, fract)
                );
  }
//  return constrain_palette(color);  // palette adjustment
}

CRGB Led::getInterpRGB(CRGB c1, CRGB c2, uint8_t fract)
{
  fract = ease8InOutQuad(fract);
  // Simple CRGB interpolation
  return CRGB(interpolate(c1.r, c2.r, fract),
              interpolate(c1.g, c2.g, fract),
              interpolate(c1.b, c2.b, fract)
             );
}

CHSV Led::getInterpRGBthruHSV(CRGB c1, CRGB c2, uint8_t fract)
{
  // 1. Convert RGB colors to HSV
  // 2. Interpolate HSV colors
  // 3. Return an HSV interpolated color
  return getInterpHSV(rgb_to_hsv(c1), rgb_to_hsv(c2), rgb_to_hsv(c1), fract);
}

CHSV Led::getInterpHSVthruRGB(CHSV c1, CHSV c2, uint8_t fract)
{
  CRGB crgb1, crgb2;
  hsv2rgb_rainbow(c1, crgb1);
  hsv2rgb_rainbow(c2, crgb2);
  CRGB color = CRGB(interpolate(crgb1.r, crgb2.r, fract),
                    interpolate(crgb1.g, crgb2.g, fract),
                    interpolate(crgb1.b, crgb2.b, fract));
  CHSV new_color = rgb_to_hsv(color);
  return CHSV(new_color.h, 255, new_color.s);  // Force always to saturate
}

uint8_t Led::interpolate(uint8_t a, uint8_t b, uint8_t fract)
{
  return lerp8by8(a, b, fract);
}

uint8_t Led::interpolate_wrap(uint8_t a, uint8_t b, uint8_t old, uint8_t fract)
{
  uint8_t distCCW, distCW, answer1, answer2;

  if (a >= b) {
    distCW = 256 + b - a;
    distCCW = a - b;
  } else {
    distCW = b - a;
    distCCW = 256 + a - b;
  }
  answer1 = a + map8(fract, 0, distCW);
  answer2 = a - map8(fract, 0, distCCW);
  if (abs(answer1 - old) < abs(answer2 - old)) {
    return answer1;
  } else {
    return answer2;
  }
//  return answer % 256;
}

uint8_t Led::interpolate_wrap(uint8_t a, uint8_t b, uint8_t fract)
{
  uint8_t distCCW, distCW, answer;

  if (a >= b) {
    distCW = 256 + b - a;
    distCCW = a - b;
  } else {
    distCW = b - a;
    distCCW = 256 + a - b;
  }
  if (distCW <= distCCW) {
    answer = a + map8(fract, 0, distCW);
  } else {
    answer = a - map8(fract, 0, distCCW);
  }
  return answer;
}

uint8_t Led::smooth(uint8_t old_value, uint8_t new_value, uint8_t max_change)
{
  if (new_value >= old_value) {
    return old_value + min(max_change, new_value - old_value);
  } else {
    return old_value - min(max_change, old_value - new_value);
  }
}

uint8_t Led::smooth_wrap(uint8_t old_value, uint8_t new_value, uint8_t max_change)
{
  uint8_t distCCW, distCW, answer;

  if (old_value >= new_value) {
    distCW = 256 + new_value - old_value;
    distCCW = old_value - new_value;
  } else {
    distCW = new_value - old_value;
    distCCW = 256 + old_value - new_value;
  }
  if (distCW <= distCCW) {
    answer = old_value + min(max_change, distCW);
  } else {
    answer = old_value - min(max_change, distCCW);
  }
  return answer % 256;
}

CHSV Led::smooth_color(CHSV old_color, CHSV new_color, uint8_t max_change)
{
  // Should I worry about saturation and value?
  return CHSV(smooth_wrap(old_color.h, new_color.h, max_change),
              new_color.s,
              smooth(old_color.v, new_color.v, min(max_change * 4, 255)));
}

void Led::setLedMap(uint8_t *led_map_pointer)
{
  // led_map should be stored on the client as:
  //   const uint8_t LedMap[] PROGMEM = {
  //
  led_map = led_map_pointer;
  is_mapped = true;
}

void Led::setCoordMap(uint8_t width, uint8_t *coord_pointer)
{
  // coord_map should be stored on the client as:
  //   const uint8_t coords[] PROGMEM = {
  //     XX,XX, 1, 0,XX,XX,XX,  // etc.
  // 1D array of 2D data. -1 = no LED
  coords = coord_pointer;
  width_2d = width;
  is_2d_mapped = true;
}

void Led::setNeighborMap(uint8_t *neighbor_map)
{
  // neighbor_map should be stored on the client as:
  //   const uint8_t neighbors[] PROGMEM = {
  //      XX,6,7,XX,XX,XX, // 0
  //      XX,6,7,XX,XX,XX, // 1  etc.
  // one row of 6 values per hexagonal pixel:
  //      ul, ur, r, lr, ll, l (u = upper, l = lower, r = right, l = left)
  //      XX = no neighbor
  neighbors = neighbor_map;
  is_neighbor_mapped = true;
}

uint8_t Led::lookupLed(uint8_t i)
{
  if (is_mapped) {
    return pgm_read_byte_near(led_map + i);
  } else {
    return i;  // Otherwise default to just i
  }
}

void Led::turnOffLedMap()
{
  is_mapped = false;
}

void Led::turnOnLedMap()
{
  is_mapped = true;
}

uint8_t Led::getNeighbor(uint8_t pos, uint8_t dir)
{
  if (is_neighbor_mapped && pos != XX) {
    return pgm_read_byte_near(neighbors + (pos * num_neighbors) + (dir % num_neighbors));
  } else {
    return pos;  // Dummy default will show strange behaviour
  }
}

uint8_t Led::getSymmetry()
{
   return num_neighbors;
}

uint8_t Led::getLedFromCoord(uint8_t x, uint8_t y)
{
  if (is_2d_mapped) {
    return pgm_read_byte_near(coords + (x * width_2d) + (y % width_2d));
  } else {
    return x;  // Dummy default will show strange behaviour
  }
}