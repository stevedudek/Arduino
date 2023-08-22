//
//  Shows.h
//
#ifndef Shows_h
#define Shows_h

class Shows
{
  public:

    Shows(Led* led_pointer, uint8_t num_sections, uint8_t c);  // Constructor

    void
      setPointers(uint8_t num_sections, const uint8_t *section_list_p, const int8_t *section_table_p,
                        const int8_t *head_column_table_p, const int8_t *a_column_table_p,
                        const int8_t *b_column_table_p, const int8_t *tail_column_table_p);

    void
      allOn(void), randomFill(void), twoColor(void),
      randomColors(void), randomOneColorBlack(void), randomTwoColorBlack(void),
      morphChain(void), sawTooth(void), bands(void), confetti(void),
      lightWave(void), lightRunUp(void), patterns(void), tubes(void),
      pendulum_wave(boolean smoothed), test_neighs(void), twoWaves(void),
      bounce(void), bounceGlowing(void), plinko(void), fire(void),
      packets(void), packets_two(void), moving_bars(bool rain),
      expanding_drops(bool filled),
      sinelon_fastled(void), bpm_fastled(void), juggle_fastled(void),
      horiz_back_forth_dots(void), diag_back_forth_dots(void);
    void
      setForeColor(uint8_t c), setBackColor(uint8_t c),
      setForeColorSpeed(uint8_t amount), setBackColorSpeed(uint8_t amount),
      IncForeColor(uint8_t amount), IncBackColor(uint8_t amount),
      setPixeltoForeColor(uint16_t i), setPixeltoBackColor(uint16_t i),
      setPixeltoHue(uint16_t i, uint8_t h),
      setPixeltoBlack(uint16_t i), setPixeltoForeBlack(uint16_t i), setPixeltoBackBlack(uint16_t i),
      setPixeltoColor(uint16_t i, CHSV color), addPixelColor(uint16_t i, CHSV color),
      setCoordtoHue(uint16_t coord, uint8_t hue),
      setCoordtoColor(uint16_t coord, CHSV color),
      addColortoCoord(uint16_t coord, CHSV color),
      flipPixel(uint16_t i),
      fill(CHSV color), fillForeColor(void), fillBackColor(void),
      fillBlack(void), fillForeBlack(void), fillBackBlack(void);
    void
      setShowDuration(uint8_t duration), setShowSpeed(uint8_t duration), setElapsedShowTime(uint32_t elapsed_time),
      setFadeAmount(uint8_t fade), setDelayTime(uint8_t delay),
      setSmallCycleHalfway(void),
      morphFrame(void),
      checkCycleClock(void),
      resetAllClocks(void),
      set_symmetry(void), symmetrize(void),
      setBandsBpm(uint8_t bands_min, uint8_t bands_max),
      pickRandomColorSpeeds(void),
      tweakColorSpeeds(void),
      setColorSpeedMinMax(uint8_t show_speed),
      setColorSpeedMinMax(uint8_t speed_min, uint8_t speed_max),
      setCycleDuration(uint16_t cd), pickCycleDuration(void),
      setPattern(uint8_t value), setPatternFill(uint8_t value), setPatternWipe(uint8_t value),
      setTubeType(uint8_t value),
      setCycle(uint16_t c), setStartShowTime(uint32_t c),
      makeWaitSlower(uint8_t r), makeWaitFaster(uint8_t r);
    void
      turnOffMorphing(void), turnOnMorphing(void);
    void
      drawTwoLineSets(uint8_t patternSet),
      drawHexRing(uint16_t coord, uint8_t size, CHSV color),
      drawLineFromEdge(uint8_t dir, int8_t i, uint8_t hue),
      drawLine(uint16_t coord, uint8_t dir, uint8_t distance, uint8_t color);
    uint8_t
      getForeColor(void), getBackColor(void),
      getForeColorSpeed(void), getBackColorSpeed(void);
    CHSV
      getForeBlack(void), getBackBlack(void);
    uint8_t
      IncColor(uint8_t hue, uint8_t amount),
      up_or_down(uint8_t value, uint8_t min_value, uint8_t max_value);
    uint8_t
      getMorphFract(void),
      get_intensity(void),
      getSections(void),
      getPattern(void),
      getPatternFill(void),
      getPatternWipe(void),
      getTubeType(void);
    uint16_t
      getNumLeds(void),
      getCycle(void), getCycleDuration(void),
      getElapsedCycleTime(void);
    uint32_t
      getSmallCycle(void), getElapsedShowTime(void), getFinishShowTime(void);
    bool
      isShowStart(void), isCycleStart(), getCycleStart(),
      hasShowFinished(void), isMorphing(void);


  private:

    // private variables
    Led* led;  // Important hook back to the LEDs

    uint16_t numLeds;  // scales
    uint8_t numSections;

    uint8_t channel;  // 0 or 1

    const uint8_t *section_list;
    const int8_t
      *section_table,
      *head_column_table, *a_column_table, *b_column_table, *tail_column_table;
    int8_t
      min_x, max_x, min_y, max_y, aspect;  // frame coordinates
    uint8_t
      width_x, height_y;

    uint8_t foreColor, backColor;
    uint8_t foreColorSpeed, backColorSpeed;
    boolean is_show_start, is_cycle_start;
    uint16_t cycle;
    uint8_t delay_time = 25;  // default milliseconds loop time
    uint8_t show_duration = 40;  // seconds of show time
    uint8_t fade_amount = 128;  // 0 = no fading, to 255 = always be fading
    uint32_t start_show_time;
    uint32_t start_cycle_time;
    uint16_t cycle_duration = 500;  // milliseconds for an animation cycle
    uint8_t
      pattern, patternFill, patternWipe, tubeType;

    bool
      has_morphing, is_symmetric;

    uint8_t color_speed_min, color_speed_max;
    uint8_t bands_min_bpm, bands_max_bpm;
    uint8_t bands_bpm_1, bands_bpm_2;
    uint8_t band_min_1, band_min_2;
    uint8_t show_speed;

    uint16_t *memory;

    uint8_t
      get_wipe_intensity(int8_t x, int8_t y, uint8_t wipe, uint8_t wipe_type);

    int8_t
      get_x_from_coord(uint16_t coord), get_y_from_coord(uint16_t coord);

    uint8_t
      get_distance_to_origin(uint8_t x, uint8_t y),
      get_distance_to_coord(uint8_t x, uint8_t y, uint8_t x_coord, uint8_t y_coord);

    uint16_t
      get_coord_from_xy(int8_t x, int8_t y),
      get_scale_for_coord(uint16_t coord),
      get_scale_from_xy(int8_t x, int8_t y),
      get_coord_in_dir(uint8_t dir, uint16_t coord),
      getRandomCoord(),
      getRandomInBoundsCoord(),
      get_coord_col_min_y(int8_t x), get_coord_col_max_y(int8_t x);

    bool
      is_coord_inbounds(uint16_t coord);

    void
      calc_coordinate_frame(void), fillWithWipe(void);
    int8_t
        get_column_table_value(uint8_t column, uint8_t field_index, uint8_t section),
        get_section_table_value(uint8_t section, uint8_t field_index);

    void setMemory(uint16_t value, uint16_t position);
    uint16_t getMemory(uint16_t position);

    // trail functions
    uint8_t getBallDir(uint8_t ball);
    uint16_t getTrailCoord(uint8_t ball, uint8_t trail);
    void setBallDir(uint8_t ball, uint8_t dir);
    void setTrailCoord(uint8_t ball, uint8_t trail, uint16_t value);
    void clearTrails();
    void dropBall(uint8_t ball);
    void setUpBalls(void);
};

#endif

