//
//  Noise.h - 1D & 2D Perlin Noise
//
//
//  Example of using this Noise library
//
//  Include libraries Noise.h, then FastLED.h:
//    #include <Noise.h>
//    #include <FastLED.h>
//
//  1D: Instantiate a global instance of 1D Noise at the top:
//    Noise noise = Noise(NUM_LEDS);
//
//  2D: Instantiate a global instance of 2D Noise at the top:
//    Noise noise = Noise(x,y);
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
//
//  1D:
//    color.h = noise.addNoiseAtoValue(i, color.h);
//    color.s = noise.addNoiseBtoValueNoWrap(i, color.s);
//
//  2D:
//    color.h = noise.addNoiseAtoValue(x, y, color.h);
//    color.s = noise.addNoiseBtoValueNoWrap(x, y, color.s);
//
//  Noise can be turned on of off:
//    noise.turnNoiseOn(), noise.turnNoiseOff()
//
#ifndef Noise_h
#define Noise_h

class Noise
{
  public:

    Noise(uint8_t x_length);  // 1D Constructor
    Noise(uint8_t x_length, uint8_t y_length);  // 2D Constructor
    Noise();  // 0D (turn off) Constructor

    void
      turnNoiseOn(void), turnNoiseOff(void);
    bool isNoiseOn(void);
    uint8_t
      getRawNoiseA(uint8_t x), getRawNoiseB(uint8_t x),
      getRawNoiseA(uint8_t x, uint8_t y), getRawNoiseB(uint8_t x, uint8_t y),
      getScaledNoiseA(uint8_t x), getScaledNoiseB(uint8_t x),
      getScaledNoiseA(uint8_t x, uint8_t y), getScaledNoiseB(uint8_t x, uint8_t y),
      addNoiseAtoValue(uint8_t x, uint8_t value), addNoiseBtoValue(uint8_t x, uint8_t value),
      addNoiseAtoValue(uint8_t x, uint8_t y, uint8_t value), addNoiseBtoValue(uint8_t x, uint8_t y, uint8_t value),
      addNoiseAtoValueNoWrap(uint8_t x, uint8_t value), addNoiseBtoValueNoWrap(uint8_t x, uint8_t value),
      addNoiseAtoValueNoWrap(uint8_t x, uint8_t y, uint8_t value),
      addNoiseBtoValueNoWrap(uint8_t x, uint8_t y, uint8_t value);
    void
      setRandomNoiseParams(void), makeVeryNoisy(void);
    void fillNoise(void);
    void
      setMaxNoise(uint8_t max_a, uint8_t max_b);

  private:

    void init_noise(uint8_t x, uint8_t y);  // common constructor
    uint8_t
      getNoiseValue(uint8_t x, uint8_t y),
      addNoiseToValue(uint8_t value, uint8_t noise_amount, uint8_t noise_max),
      addNoiseToValueNoWrap(uint8_t value, uint8_t noise_amount, uint8_t noise_max);
    void setNoiseValue(uint8_t x, uint8_t y, uint8_t value);

    boolean _noise_on;
    uint8_t _max_x, _max_y;
    uint8_t _noise_set;
    uint8_t _max_noise_a, _max_noise_b;
    uint8_t _noise_a_intense, _noise_b_intense;
    uint16_t _noise_x, _noise_y, _noise_z;
    uint16_t _noise_speed, _noise_scale;
    uint8_t *noise_values;

    const uint8_t noise_param_set[24] PROGMEM =
        { 0,0, 20,30, 10,50, 8,120, 4,30, 8,50, 20,90, 20,30, 20,20, 50,50, 90,90, 30,20 };

};

#endif

