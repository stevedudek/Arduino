//
//  Shows.h
//
#ifndef Shows_h
#define Shows_h

class Shows
{
  public:

    Shows(Led* led_pointer);  // Constructor

    void
      allOn(void), randomFill(void), twoColor(void),
      randomColors(void), randomOneColorBlack(void), randomTwoColorBlack(void),
      morphChain(void), sawTooth(void), bands(void), confetti(void),
      lightWave(void),
      lightRunUp(void), bounce(void), bounceGlowing(void), packets(void),
      packets_two(void), plinko(uint8_t start_pos),
      sinelon_fastled(void), bpm_fastled(void), juggle_fastled(void);
    void
      shuffleLeds(void), shuffleColors(void);
    void
      setForeColor(uint8_t c), setBackColor(uint8_t c),
      setForeColorSpeed(uint8_t amount), setBackColorSpeed(uint8_t amount),
      IncForeColor(uint8_t amount), IncBackColor(uint8_t amount),
      setPixeltoForeColor(uint8_t i), setPixeltoBackColor(uint8_t i),
      setPixeltoHue(uint8_t i, uint8_t h),
      setPixeltoBlack(uint8_t i), setPixeltoForeBlack(uint8_t i), setPixeltoBackBlack(uint8_t i),
      setPixeltoColor(uint8_t i, CHSV color),
      fill(CHSV color), fillForeColor(void), fillBackColor(void),
      fillBlack(void), fillForeBlack(void), fillBackBlack(void),
      morphFrame(void),
      advanceClock(void),
      resetAllClocks(void),
      resetMorph(void), resetCycle(void), resetSmallCycle(void),
      resetNumLeds(uint8_t i),
      setBandsBpm(uint8_t bands_min, uint8_t bands_max),
      pickRandomColorSpeeds(void),
      tweakColorSpeeds(void),
      setColorSpeedMinMax(uint8_t show_speed),
      setColorSpeedMinMax(uint8_t speed_min, uint8_t speed_max),
      pickRandomWait(void),
      tweakWait(void),
      setCycle(uint16_t c), setSmallCycle(uint32_t c),
      setWait(uint8_t w), setWaitAbsolute(uint8_t w),
      setWaitRange(uint8_t show_speed),
      setWaitRange(uint8_t wait_min, uint8_t wait_max),
      setWaitRange(uint8_t wait_min, uint8_t wait_max, uint8_t num_wait_values),
      makeWaitSlower(uint8_t r), makeWaitFaster(uint8_t r),
      setAsSquare(void), setAsPentagon(void);
    void
      turnOffMorphing(void), turnOnMorphing(void);
    void
      turnOnBlur(void), turnOffBlur(void);
    uint8_t
      getForeColor(void), getBackColor(void),
      getForeColorSpeed(void), getBackColorSpeed(void);
    CHSV
      getForeBlack(void), getBackBlack(void);
    uint8_t
      IncColor(uint8_t hue, uint8_t amount),
      up_or_down(uint8_t value, uint8_t min_value, uint8_t max_value);
    uint8_t
      getWait(void),
      getMorph(void),
      getMorphFract(void),
      getNumFrames(void),
      getNumLeds(void);
    uint8_t
      getShuffleValue(uint8_t i);
    uint16_t
      getCycle(void);
    uint32_t
      getSmallCycle(void);
    bool
      isShowStart(void);
    bool
      isMorphing(void);
    uint8_t
      num_balls;
    uint8_t
      *bounce_dir, *bounce_pos;
    void
      clearTrails(void);

  private:

    // private variables
    Led* led;  // Important hook back to the LEDs

    uint8_t numLeds;

    uint8_t foreColor, backColor;
    uint8_t foreColorSpeed, backColorSpeed;
    uint8_t morph;
    uint16_t cycle;
    uint32_t smallCycle;
    uint8_t wait;

    bool has_morphing, has_blur;

    uint8_t color_speed_min, color_speed_max;
    uint8_t bands_min_bpm, bands_max_bpm;
    uint8_t bands_bpm_1, bands_bpm_2;
    uint8_t band_min_1, band_min_2;
    uint8_t wait_min, wait_max, num_wait_values;
    uint8_t num_neighbors;

    uint8_t *shuffle;

    uint8_t *plink, *trail;
    uint8_t num_plinko;
    uint8_t *packet_intense, *packet_freq, *packet_color;

    // private functions
    uint8_t getTrail(uint8_t trail_num, uint8_t dist);
    void setTrail(uint8_t trail_num, uint8_t dist, uint8_t value);
};

#endif

