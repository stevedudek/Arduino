//
//  Shows.h
//
#ifndef Shows_h
#define Shows_h

class Shows
{
  public:

    Shows(Led* led_pointer, uint8_t num_sections);  // Constructor

    void
      allOn(void), randomFill(void), twoColor(void),
      randomColors(void), randomOneColorBlack(void), randomTwoColorBlack(void),
      morphChain(void), sawTooth(void), bands(void), confetti(void),
      lightWave(void), lightRunUp(void), TestNeighbors(void),
      bounce(void), bounceGlowing(void), plinko(void),
      // packets(void), packets_two(void),
      sinelon_fastled(void), bpm_fastled(void), juggle_fastled(void);
    void
      setForeColor(uint8_t c), setBackColor(uint8_t c),
      setForeColorSpeed(uint8_t amount), setBackColorSpeed(uint8_t amount),
      IncForeColor(uint8_t amount), IncBackColor(uint8_t amount),
      setPixeltoForeColor(uint16_t i), setPixeltoBackColor(uint16_t i),
      setPixeltoHue(uint16_t i, uint8_t h),
      setPixeltoBlack(uint16_t i), setPixeltoForeBlack(uint16_t i), setPixeltoBackBlack(uint16_t i),
      setPixeltoColor(uint16_t i, CHSV color),
      flipPixel(uint16_t i),
      fill(CHSV color), fillForeColor(void), fillBackColor(void),
      fillBlack(void), fillForeBlack(void), fillBackBlack(void),
      morphFrame(void),
      advanceClock(void),
      resetAllClocks(void),
      resetMorph(void), resetCycle(void), resetSmallCycle(void),
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
      makeWaitSlower(uint8_t r), makeWaitFaster(uint8_t r);
    void
      turnOffMorphing(void), turnOnMorphing(void);
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
      getSections(void);
    uint16_t
      getNumLeds(void),
      getCycle(void);
    uint32_t
      getSmallCycle(void);
    bool
      isShowStart(void);
    bool
      isMorphing(void);
    bool
      isInBounds(int8_t x, int8_t y);
    int8_t
      move_x(int8_t x, uint8_t dir),
      move_y(int8_t x, int8_t y, uint8_t dir);
    void
      setColumnTableUp(const uint8_t *column_table_up_map);

  private:

    // private variables
    Led* led;  // Important hook back to the LEDs

    const uint8_t *column_table_up, *column_table_down, *column_table_head, *column_table_tail;
    uint16_t get_scale_for_xy(int8_t x, int8_t y);

    uint16_t numLeds;  // scales
    uint8_t numSections;

    uint8_t foreColor, backColor;
    uint8_t foreColorSpeed, backColorSpeed;
    uint8_t morph;
    uint16_t cycle;
    uint32_t smallCycle;
    uint8_t wait;

    bool has_morphing;

    uint8_t color_speed_min, color_speed_max;
    uint8_t bands_min_bpm, bands_max_bpm;
    uint8_t bands_bpm_1, bands_bpm_2;
    uint8_t band_min_1, band_min_2;
    uint8_t wait_min, wait_max, num_wait_values;

    uint8_t *memory;

    uint8_t get_map_value(int8_t column, uint8_t field_index);
    void setMemory(uint8_t value, uint16_t position);
    uint8_t getMemory(uint16_t position);

    // mapping functions
    int8_t get_x(uint16_t scale);
    int8_t get_y(uint16_t scale);
    uint16_t get_neighbor(uint16_t scale, uint8_t dir);

    // trail functions
    uint8_t getBallDir(uint8_t ball);
    uint8_t getTrailX(uint8_t ball, uint8_t trail);
    uint8_t getTrailY(uint8_t ball, uint8_t trail);
    void setBallDir(uint8_t ball, uint8_t dir);
    void setTrailX(uint8_t ball, uint8_t trail, uint8_t value);
    void setTrailY(uint8_t ball, uint8_t trail, uint8_t value);
    void clearTrails();
    void dropBall(uint8_t ball);
    void setUpBalls(void);
};

#endif

