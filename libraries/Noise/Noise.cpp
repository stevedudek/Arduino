//
//  Noise.cpp - 1D & 2D Perlin Noise
//
#include <FastLED.h>  // Need this library for math functions

#include "Noise.h"  // include this library's description file

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

//
// Constructors
//

// 1D
Noise::Noise(uint8_t x)
{
  uint8_t y = 2;  // 1D noise has 2 noise rows for channels A + B
  init_noise(x, y);
}

// 2D
Noise::Noise(uint8_t x, uint8_t y)
{
  init_noise(x, y);
}

// 0D - dummy/empty Noise
Noise::Noise()
{
}


//
// Public Methods //////////////////////////////////////////////////////////////
//
void Noise::turnNoiseOn(void)
{
  _noise_on = true;
}

void Noise::turnNoiseOff(void)
{
  _noise_on = false;
}

bool Noise::isNoiseOn(void)
{
  return _noise_on;
}

// Limit Noise channels A+B to Values (0-255, 0-255)
void Noise::setMaxNoise(uint8_t max_noise_a, uint8_t max_noise_b)
{
  _max_noise_a = max_noise_a;
  _max_noise_b = max_noise_b;
}

// Get 1D Raw Noise from channel A (0-255)
uint8_t Noise::getRawNoiseA(uint8_t x)
{
  uint8_t y = 0;
  return getRawNoiseA(x,y);
}

// Get 1D Raw Noise from channel B (0-255)
uint8_t Noise::getRawNoiseB(uint8_t x)
{
  uint8_t y = 0;
  return getRawNoiseB(x,y);
}

// Get 2D Raw Noise from channel A (0-255)
uint8_t Noise::getRawNoiseA(uint8_t x, uint8_t y)
{
  return getNoiseValue(x,y);
}

// Get 2D Raw Noise from channel B (0-255)
uint8_t Noise::getRawNoiseB(uint8_t x, uint8_t y)
{
  return getNoiseValue(_max_x - x - 1, _max_y - y - 1);
}

// Get Scaled Noise from channel A (0 - max_noise_a)
uint8_t Noise::getScaledNoiseA(uint8_t x)
{
  return map8(getRawNoiseA(x), 0, _noise_a_intense);
}

// Get Scaled Noise from channel B (0 - max_noise_b)
uint8_t Noise::getScaledNoiseB(uint8_t x)
{
  return map8(getRawNoiseB(x), 0, _noise_b_intense);
}

// Get 2D Scaled Noise from channel A (0 - max_noise_a)
uint8_t Noise::getScaledNoiseA(uint8_t x, uint8_t y)
{
  return map8(getRawNoiseA(x,y), 0, _noise_a_intense);
}

// Get 2D Scaled Noise from channel B (0 - max_noise_b)
uint8_t Noise::getScaledNoiseB(uint8_t x, uint8_t y)
{
  return map8(getRawNoiseB(x,y), 0, _noise_b_intense);
}

// Add Scaled Channel A noise at pixel x to value (will wrap)
uint8_t Noise::addNoiseAtoValue(uint8_t x, uint8_t value)
{
  return addNoiseToValue(value, getScaledNoiseA(x), _noise_a_intense);
}

// Add Scaled Channel B noise at pixel x to value (will wrap)
uint8_t Noise::addNoiseBtoValue(uint8_t x, uint8_t value)
{
  return addNoiseToValue(value, getScaledNoiseB(x), _noise_b_intense);
}

// Add 2D Scaled Channel A noise at pixel (x,y) to value (will wrap)
uint8_t Noise::addNoiseAtoValue(uint8_t x, uint8_t y, uint8_t value)
{
  return addNoiseToValue(value, getScaledNoiseA(x,y), _noise_a_intense);
}

// Add 2D Scaled Channel B noise at pixel (x,y) to value (will wrap)
uint8_t Noise::addNoiseBtoValue(uint8_t x, uint8_t y, uint8_t value)
{
  return addNoiseToValue(value, getScaledNoiseB(x,y), _noise_b_intense);
}

// Add Scaled Channel A noise at pixel x to value (no wrapping)
uint8_t Noise::addNoiseAtoValueNoWrap(uint8_t x, uint8_t value)
{
  return addNoiseToValueNoWrap(value, getScaledNoiseA(x), _noise_a_intense);
}

// Add Scaled Channel B noise at pixel x to value (no wrapping)
uint8_t Noise::addNoiseBtoValueNoWrap(uint8_t x, uint8_t value)
{
  return addNoiseToValueNoWrap(value, getScaledNoiseB(x), _noise_b_intense);
}

// Add 2D Scaled Channel A noise at pixel (x,y) to value (no wrapping)
uint8_t Noise::addNoiseAtoValueNoWrap(uint8_t x, uint8_t y, uint8_t value)
{
  return addNoiseToValueNoWrap(value, getScaledNoiseA(x,y), _noise_a_intense);
}

// Add 2D Scaled Channel B noise at pixel (x,y) to value (no wrapping)
uint8_t Noise::addNoiseBtoValueNoWrap(uint8_t x, uint8_t y, uint8_t value)
{
  return addNoiseToValueNoWrap(value, getScaledNoiseB(x,y), _noise_b_intense);
}

// Randomize noise parameters at the start of a show
void Noise::setRandomNoiseParams(void)
{
  if (!isNoiseOn()) {
    return;
  }
  _noise_a_intense = random8(_max_noise_a);
  _noise_b_intense = random8(_max_noise_b);
  _noise_set = random8(ARRAY_SIZE(noise_param_set) / 2);

  _noise_speed = pgm_read_byte_near(noise_param_set + (_noise_set * 2));
  _noise_scale = pgm_read_byte_near(noise_param_set + (_noise_set * 2) + 1);
}

// Make very noisy. Effects will be erased by the next setRandomNoiseParams().
void Noise::makeVeryNoisy(void)
{
  if (!isNoiseOn()) {
    return;
  }
  _noise_a_intense = random8(255);
  _noise_b_intense = random8(255);
  _noise_set = random8(ARRAY_SIZE(noise_param_set) / 2);

  _noise_speed = pgm_read_byte_near(noise_param_set + (_noise_set * 2));
  _noise_scale = pgm_read_byte_near(noise_param_set + (_noise_set * 2) + 1);
}

void Noise::fillNoise(void)
{
  if (!isNoiseOn()) {
    return;
  }
  uint8_t dataSmoothing = 0;
  if( _noise_speed < 50) {
    dataSmoothing = 200 - (_noise_speed * 4);
  }

  for(int y = 0; y < _max_y; y++) {
    int yoffset = _noise_scale * y;

    for(int x = 0; x < _max_x; x++) {
      int xoffset = _noise_scale * x;

      uint8_t data = inoise8(_noise_x + xoffset, _noise_y + yoffset, _noise_z);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data,16);
      data = qadd8(data,scale8(data,39));

      if( dataSmoothing ) {
        uint8_t olddata = getNoiseValue(x, y);
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }

      setNoiseValue(x, y, data);
    }
  }

  _noise_z += _noise_speed;

  // apply slow drift to X and Y, just for visual variation.
  _noise_x += _noise_speed / 8;
  _noise_y -= _noise_speed / 16;
}


//
//  Private Methods //////////////////////////////////////////////////////////////
//
void Noise::init_noise(uint8_t x, uint8_t y)
{
  _max_x = x;
  _max_y = y;
  _max_noise_a = 128;
  _max_noise_b = 128;
  _noise_x = random16();
  _noise_y = random16();
  _noise_z = random16();

  noise_values = (uint8_t *)calloc(x, y);  // "2D" array that holds noise values

  turnNoiseOn();
  setRandomNoiseParams();
  fillNoise();
}

uint8_t Noise::getNoiseValue(uint8_t x, uint8_t y)
{
  return noise_values[(y * _max_x) + x];
}

void Noise::setNoiseValue(uint8_t x, uint8_t y, uint8_t value)
{
  noise_values[(y * _max_x) + x] = value;
}

uint8_t Noise::addNoiseToValue(uint8_t value, uint8_t noise_amount, uint8_t noise_max)
{
  if (!isNoiseOn()) {
    return value;
  }
  int new_value = value + (map8(noise_amount, 0, noise_max) / 2) - (noise_max / 2);
  return new_value % 256;
}

uint8_t Noise::addNoiseToValueNoWrap(uint8_t value, uint8_t noise_amount, uint8_t noise_max)
{
  if (!isNoiseOn()) {
    return value;
  }
  int new_value = value + (map8(noise_amount, 0, noise_max) / 2) - (noise_max / 2);
  if (new_value >= 255) {
    return 255;
  } else if (new_value < 0) {
    return 0;
  } else {
    return new_value;
  }
}

