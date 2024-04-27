//
//  Noise.h - 1D Perlin Noise
//
#ifndef Noise_h
#define Noise_h


// library interface description
class Noise
{
  // user-accessible "public" interface
  public:
    Noise(uint8_t x_length, uint8_t max_noise_a, uint8_t max_noise_b);  // Constructor

    void
      turnNoiseOn(void), turnNoiseOff(void);
    bool isNoiseOn(void);
    uint8_t
      getRawNoiseA(uint8_t), getRawNoiseB(uint8_t),
      getScaledNoiseA(uint8_t), getScaledNoiseB(uint8_t),
      addNoiseAtoValue(uint8_t, uint8_t), addNoiseBtoValue(uint8_t, uint8_t),
      addNoiseAtoValueNoWrap(uint8_t, uint8_t), addNoiseBtoValueNoWrap(uint8_t, uint8_t);
    void setRandomNoiseParams(void);
    void fillNoise(void);
    void
      setMaxA(uint8_t), setMaxB(uint8_t);

  // library-accessible "private" interface
  private:
    uint8_t
      getNoiseValue(uint8_t, uint8_t),
      addNoiseToValue(uint8_t, uint8_t, uint8_t), addNoiseToValueNoWrap(uint8_t, uint8_t, uint8_t);
    void setNoiseValue(uint8_t, uint8_t, uint8_t);

    boolean _noise_on;
    uint8_t _max_x;
    uint8_t _noise_set;
    uint8_t _max_noise_a, _max_noise_b;
    uint8_t _noise_a_intense, _noise_b_intense;
    static uint16_t _noise_x, _noise_y, _noise_z;
    uint16_t _noise_speed, _noise_scale;
    uint8_t *noise_values;  // Needs to be length x_length

    const uint8_t noise_param_set[24] PROGMEM =
        { 0,0, 20,30, 10,50, 8,120, 4,30, 8,50, 20,90, 20,30, 20,20, 50,50, 90,90, 30,20 };

};

#endif

