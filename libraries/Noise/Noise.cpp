//
//  Noise.cpp - 1D Perlin Noise
//
#include <FastLED.h>  // Need this library for math functions

#include "Noise.h"  // include this library's description file

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

//
// Constructor: Noise (1D length, max noise A (0-255), max noise b (0-255) )
//
Noise::Noise(uint8_t x_length, uint8_t max_noise_a, uint8_t max_noise_b)
{
  _x_length = x_length;
  _max_noise_a = max_noise_a;
  _max_noise_b = max_noise_b;
  _noise_x = random16();
  _noise_y = random16();
  _noise_z = random16();
  noise_values = (uint8_t *)calloc(x_length, 2);  // Assign 2 bytes per x-length

  turnNoiseOn();
  setRandomNoiseParams();
  fillNoise();
}

//
// Public Methods //////////////////////////////////////////////////////////////
//
void Noise::turnNoiseOn(void)
{
  _noise_on = True;
}

void Noise::turnNoiseOff(void)
{
  _noise_on = False;
}

bool Noise::isNoiseOn(void)
{
  return _noise_on;
}

void Noise::setMaxNoise(uint8_t max_noise_a, max_noise_b)
{
  _max_noise_a = max_noise_a;
  _max_noise_b = max_noise_b;
}

void Noise::setMaxNoise()
{
  setMaxNoise(64, 64);  // max_noise_a (hue), max_noise_b (saturation)
}

// Limit Noise channel A to Value (0-255)
void Noise::setMaxA(uint8_t max_noise_a)
{
  _max_noise_a = max_noise_a;
}

// Limit Noise channel B to Value (0-255)
void Noise::setMaxB(uint8_t max_noise_b)
{
  _max_noise_b = max_noise_b;
}

// Get Raw Noise from channel A (0-255)
uint8_t Noise::getRawNoiseA(uint8_t i)
{
  return getNoiseValue(0, i);
}

// Get Raw Noise from channel B (0-255)
uint8_t Noise::getRawNoiseB(uint8_t i)
{
  return getNoiseValue(1, _x_length - i - 1);
}

// Get Scaled Noise from channel A (0 - max_noise_a)
uint8_t Noise::getScaledNoiseA(uint8_t i)
{
  return map8(getRawNoiseA(i), 0, _noise_a_intense);
}

// Get Scaled Noise from channel B (0 - max_noise_b)
uint8_t Noise::getScaledNoiseB(uint8_t i)
{
  return map8(getRawNoiseB(i), 0, _noise_b_intense);
}

// Add Scaled Channel A noise at pixel i to value (will wrap)
uint8_t Noise::addNoiseAtoValue(uint8_t i, uint8_t value)
{
  if (_noise_on == False) {
    return value;
  }
  return addNoiseToValue(value, getScaledNoiseA(i), _noise_a_intense);
}

// Add Scaled Channel B noise at pixel i to value (will wrap)
uint8_t Noise::addNoiseBtoValue(uint8_t i, uint8_t value)
{
  if (_noise_on == False) {
    return value;
  }
  return addNoiseToValue(value, getScaledNoiseB(i), _noise_b_intense);
}

// Add Scaled Channel A noise at pixel i to value (no wrapping)
uint8_t Noise::addNoiseAtoValueNoWrap(uint8_t i, uint8_t value)
{
  if (_noise_on == False) {
    return value;
  }
  return addNoiseToValueNoWrap(value, getScaledNoiseA(i), _noise_a_intense);
}

// Add Scaled Channel B noise at pixel i to value (no wrapping)
uint8_t Noise::addNoiseBtoValueNoWrap(uint8_t i, uint8_t value)
{
  if (_noise_on == False) {
    return value;
  }
  return addNoiseToValueNoWrap(value, getScaledNoiseB(i), _noise_b_intense);
}

//
void Noise::setRandomNoiseParams(void)
{
  if (_noise_on == False) {
    return;
  }
  _noise_a_intense = random8(_max_noise_a);
  _noise_b_intense = random8(_max_noise_b);
  _noise_set = random8(ARRAY_SIZE(noise_param_set) / 2);

  _noise_speed = pgm_read_byte_near(noise_param_set + (_noise_set * 2));
  _noise_scale = pgm_read_byte_near(noise_param_set + (_noise_set * 2) + 1);
}

void Noise::fillNoise(void)
{
  if (_noise_on == False) {
    return;
  }
  uint8_t dataSmoothing = 0;
  if( _noise_speed < 50) {
    dataSmoothing = 200 - (_noise_speed * 4);
  }

  for(int i = 0; i < 2; i++) {  // 0 = noise_a, 1 = noise_b
    int ioffset = _noise_scale * i;
    for(int j = 0; j < _x_length; j++) {
      int joffset = _noise_scale * j;

      uint8_t data = inoise8(_noise_x + ioffset, _noise_y + joffset, _noise_z);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data,16);
      data = qadd8(data,scale8(data,39));

      if( dataSmoothing ) {
        uint8_t olddata = noise_values[i][j];
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }

      setNoiseValue(i, j, data);
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
uint8_t Noise::getNoiseValue(uint8_t i, uint8_t j)
{
  return noise_values[(j*2) + i];
}

void Noise::setNoiseValue(uint8_t i, uint8_t j, uint8_t value)
{
  noise_values[(j*2) + i] = value;
}

uint8_t Noise::addNoiseToValue(uint8_t value, uint8_t noise_amount, uint8_t noise_max)
{
  int new_value = value + (map8(noise_amount, 0, noise_max) / 2) - (noise_max / 2);
  return new_value % 256;
}

uint8_t Noise::addNoiseToValueNoWrap(uint8_t value, uint8_t noise_amount, uint8_t noise_max)
{
  int new_value = value + (map8(noise_amount, 0, noise_max) / 2) - (noise_max / 2);
  if (new_value >= 255) {
    return 255;
  } else if (new_value < 0) {
    return 0;
  } else {
    return new_value;
  }
}

