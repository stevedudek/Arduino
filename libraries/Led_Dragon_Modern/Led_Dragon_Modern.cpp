//
//  Led_Dragon_Modern.cpp - Handles morphing, RGB/HSV interpolation
//
//  Updated clock cycling
//
//  9/16/23 USE THIS REPO
//
#include <FastLED.h>

#include "Led_Dragon_Modern.h"

#define BLACK  CHSV(0, 0, 0)
#define XX   9999  // Out of bounds

//
// Constructor
//
Led::Led(uint16_t i)
{
  numLeds = i;

  current_frame = (CHSV *)calloc(numLeds, sizeof(CHSV));
  next_frame = (CHSV *)calloc(numLeds, sizeof(CHSV));

}

//
// Public Methods //////////////////////////////////////////////////////////////
//
uint16_t Led::getNumLeds(void)
{
  return numLeds;
}

void Led::fill(CHSV color)
{
  for (uint16_t i = 0; i < numLeds; i++) {
    setPixelColor(i, color);
  }
}

void Led::fillHue(uint8_t hue)
{
  for (uint16_t i = 0; i < numLeds; i++) {
    setPixelHue(i, hue);
  }
}

void Led::fillBlack(void)
{
  for (uint16_t i = 0; i < numLeds; i++) {
    setPixelColor(i, BLACK);
  }
}

void Led::setPixelColor(uint16_t i, CHSV color)
{
  if (i != XX) {
    next_frame[i] = color;
  }
}

void Led::setPixelHue(uint16_t i, uint8_t hue)
{
  setPixelColor(i, wheel(hue));
}

void Led::setPixelBlack(uint16_t i)
{
  setPixelColor(i, BLACK);
}

void Led::increasePixelHue(uint16_t i, uint8_t increase)
{
  if (i != XX) {
    next_frame[i].h += increase;
  }
}

void Led::dimPixel(uint16_t i, uint8_t amount)
{
  if (i != XX) {
    next_frame[i].v = scale8(next_frame[i].v, 255 - amount);
  }
}

void Led::dimAllPixels(uint8_t amount)
{
  for (uint16_t i = 0; i < numLeds; i++) {
    dimPixel(i, amount);
  }
}

void Led::addPixelColor(uint16_t i, CHSV c2)
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

// Don't interpolate. Try to force the current frame into next frame with smoothing.
void Led::smooth_frame(uint8_t max_change)
{
  for (uint16_t i = 0; i < numLeds; i++) {
    current_frame[i] = smooth_color(current_frame[i], next_frame[i], max_change, max_change);
  }
}

void Led::push_frame(void)
{
  for (uint16_t i = 0; i < numLeds; i++) {
    current_frame[i] = next_frame[i];
  }
}

CHSV Led::getCurrFrameColor(uint16_t i)
{
  return current_frame[i];
}

CHSV Led::getNextFrameColor(uint16_t i)
{
  return next_frame[i];
}

bool Led::is_black(CHSV color)
{
  return color.v == 0;
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

CHSV Led::narrow_palette(CHSV color, uint8_t hue_center, uint8_t hue_width, uint8_t saturation)
{
  uint8_t h1 = (hue_center - (hue_width / 2)) % 255;
  uint8_t h2 = (hue_center + (hue_width / 2)) % 255;
  color.h = map8(color.h, h1, h2 );
  if (color.s != 0) {
    color.s = saturation;  // Reset saturation
  }
  return color;
}

CHSV Led::getInterpHSV(CHSV c1, CHSV c2, uint8_t fract)
{
  if (c1 == c2) {
    return c1;
  } else if (fract == 0) {
    return c1;
  } else if (fract == 255) {
    return c2;
  } else if (is_black(c1)) {
    return CHSV(c2.h, c2.s, interpolate(c1.v, c2.v, fract));  // safe approach
  } else if (is_black(c2)) {
    return CHSV(c1.h, c1.s, interpolate(c1.v, c2.v, fract));  // safe approach
  } else {
    return CHSV(interpolate_wrap(c1.h, c2.h, fract),
                interpolate(c1.s, c2.s, fract),  // Always saturated?
                interpolate(c1.v, c2.v, fract)
               );
  }
}

/*  Deprecated? Sep 2023
CHSV Led::getInterpHSV(CHSV c1, CHSV c2, CHSV old_color, uint8_t fract)
{
  if (c1 == c2) {
    return c1;
  } else if (fract == 0) {
    return c1;
  } else if (fract == 255) {
    return c2;
  } else {

    return CHSV(interpolate_wrap(c1.h, c2.h, old_color.h, fract),
                255, // Always saturated
                interpolate(c1.v, c2.v, fract)
               );
  }
}
*/

CRGB Led::getInterpRGB(CRGB c1, CRGB c2, uint8_t fract)
{
  // Simple CRGB interpolation
  return CRGB(interpolate(c1.r, c2.r, fract),
              interpolate(c1.g, c2.g, fract),
              interpolate(c1.b, c2.b, fract)
             );
}

/* Deprecated? Sept 2023
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
*/

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
  if (new_value == old_value) {
    return new_value;
  }
  if (new_value > old_value) {
    return old_value + min(int(max_change), int(new_value - old_value));
  } else {
    return old_value - min(int(max_change), int(old_value - new_value));
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

CHSV Led::smooth_color(CHSV old_color, CHSV new_color, uint8_t max_hue_change, uint8_t max_value_change)
{
  return CHSV(smooth_wrap(old_color.h, new_color.h, max_hue_change),
              255,  // try always saturated
              smooth(old_color.v, new_color.v, max_value_change));
}

CRGB Led::smooth_rgb_color(CRGB old_color, CRGB new_color, uint8_t max_change)
{
  return CRGB(smooth(old_color.r, new_color.r, max_change),
              smooth(old_color.g, new_color.g, max_change),
              smooth(old_color.b, new_color.b, max_change));
}
