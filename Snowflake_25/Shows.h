//
//  Shows.h
//
#ifndef Shows_h
#define Shows_h

class Shows
{
  public:

    Shows(Led const* led_pointer);  // Constructor

    void
      allOn(void), randomFill(void), randomColors(void), twoColor(void),
      morphChain(void), sawTooth(void), bands(void), lightWave(void),
      lightRunUp(void), bounce(void), bounce_glowing(void),
      plinko(uint8_t *start_pos);
      // don't include noisyshow()
    void
      shuffleLeds(void);
    void
      setForeColor(uint8_t c), setBackColor(uint8_t c),
      IncForeColor(uint8_t amount), IncBackColor(uint8_t amount),
      morphFrame(void),
      advanceClock(void),
      resetAllClocks(void),
      resetMorph(void), resetCycle(void), resetSmallCycle(void),
      setBandsBpm(uint8_t bands_min, uint8_t bands_max),
      tweakColorSpeeds(void),
      setColorSpeedMinMax(uint8_t speed_min, uint8_t speed_max),
      tweakWait(void),
      setWait(uint8_t w),
      setWaitRange(uint8_t wait_min, uint8_t wait_max, uint8_t num_wait_values);
    uint8_t
      IncColor(uint8_t hue, uint8_t amount),
      up_or_down(uint8_t value, uint8_t min_value, uint8_t max_value);
    uint8_t
      getMorph(void),
      getNumFrames(void);
    uint16_t
      getCycle(void);
    uint32_t
      getSmallCycle(void);
    bool
      isShowStart(void);

  private:

    // private variables
    Led* led;  // Important hook back to the LEDs

    uint8_t numLeds;

    uint8_t foreColor, backColor;
    uint8_t foreColorSpeed, backColorSpeed;
    uint8_t morph;
    uint16_t cycle;
    uint32_t small_cycle;
    uint8_t wait;

    uint8_t color_speed_min, color_speed_max;
    uint8_t bands_min_bpm, bands_max_bpm;
    uint8_t bands_bpm_1, bands_bpm_2;
    uint8_t band_min_1, band_min_2;
    uint8_t wait_min, wait_max, num_wait_values;

    uint8_t *shuffle;

    // private functions

};

#endif
