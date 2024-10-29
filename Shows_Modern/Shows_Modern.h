//
//  Shows_Modern.h - improved clocking; not for Dragons or Owls
//
#ifndef Shows_Modern_h
#define Shows_Modern_h

class Shows
{
  public:

    Shows(Led* led_pointer, uint8_t c);  // Constructor

    void
      allOn(void), randomFlip(void), twoColor(void), stripes(void),
      randomColors(void), randomOneColorBlack(void), randomTwoColorBlack(void),
      morphChain(void), sawTooth(void), confetti(void),
      lightWave(void), lightRunUp(void),
      bounce(void), bounceGlowing(void), plinko(uint8_t start_pos),
      sinelon_fastled(void), bpm_fastled(void), juggle_fastled(void);
    void
      setForeColor(uint8_t c), setBackColor(uint8_t c),
      setForeColorSpeed(uint8_t amount), setBackColorSpeed(uint8_t amount),
      IncForeColor(uint8_t amount), IncBackColor(uint8_t amount);
    void
      setPixeltoForeColor(uint16_t i), setPixeltoBackColor(uint16_t i),
      setPixeltoHue(uint16_t i, uint8_t h),
      setPixeltoBlack(uint16_t i), setPixeltoForeBlack(uint16_t i), setPixeltoBackBlack(uint16_t i),
      setPixeltoColor(uint16_t i, CHSV color), addPixelColor(uint16_t i, CHSV color);
    void
      fill(CHSV color), fillForeColor(void), fillBackColor(void),
      fillBlack(void), fillForeBlack(void), fillBackBlack(void);
    void
      setShowDuration(uint8_t duration), setShowSpeed(uint8_t duration),
      setElapsedShowTime(uint32_t elapsed_time),
      setFadeAmount(uint8_t fade), setDelayTime(uint8_t delay),
      setSmallCycleHalfway(void),
      setCycle(uint16_t c), setStartShowTime(uint32_t c),
      morphFrame(void), updateFrameClock(void),
      dimAllPixels(uint8_t dim_amount), dimAllPixelsFrames(uint8_t frames),
      checkCycleClock(void), resetAllClocks(void);
    void
      resetNumLeds(uint16_t i);
    void
      setColorSpeedMinMax(void), pickRandomColorSpeeds(void),
      pickCycleDuration(void), pickRandomCycleDuration(uint16_t min_duration, uint16_t max_duration),
      setCycleDuration(uint16_t cd);
    void
      turnOffMorphing(void), turnOnMorphing(void);
    void
      setAsSquare(void), setAsPentagon(void), setAsHexagon(void);
    uint8_t
      getForeColor(void), getBackColor(void),
      getForeColorSpeed(void), getBackColorSpeed(void);
    CHSV
      getForeBlack(void), getBackBlack(void);
    uint8_t
      IncColor(uint8_t hue, uint8_t amount),
      up_or_down(uint8_t value, uint8_t min_value, uint8_t max_value);
    uint8_t
      getCyclesPerFrame(void), getMorphFract(void), get_intensity(void);
    uint16_t
      getNumLeds(void), getCycle(void), getCycleDuration(void), delta(void),
      getElapsedCycleTime(void);
    uint32_t
      getSmallCycle(void), getElapsedShowTime(void);
    bool
      isShowStart(void), isCycleStart(),
      hasShowFinished(void), isMorphing(void);
    float
      clamp(float value, float min_value, float max_value),
      time_sawtooth(uint16_t time_window);


  private:

    // private variables
    Led* led;  // Important hook back to the LEDs

    uint16_t numLeds;
    uint8_t
      num_neighbors, channel;  // 0 or 1

    uint8_t foreColor = 128;
    uint8_t backColor = 0;
    uint8_t foreColorSpeed = 20;
    uint8_t backColorSpeed = 40;

    bool
      is_show_start, is_cycle_start;
    uint16_t cycle;
    uint8_t cycles_per_frame = 10;
    uint8_t delay_time = 15;  // default milliseconds loop time
    uint8_t show_duration = 40;  // seconds of show time
    uint8_t fade_amount = 128;  // 0 = no fading, to 255 = always be fading
    uint32_t
      start_show_time, start_cycle_time, start_frame_time;
    uint16_t cycle_duration = 500;  // milliseconds for an animation cycle

    bool
      has_morphing = true;

    uint8_t
      color_speed_min, color_speed_max, show_speed;

    uint8_t *memory;

    void
      setMemory(uint8_t value, uint8_t position);
    uint8_t
      getMemory(uint8_t position);

    uint8_t
      getNumBalls(void), getBallPos(uint8_t ball), getBallDir(uint8_t ball);
    void
      setNumBalls(uint8_t i),
      setBallDir(uint8_t ball, uint8_t dir),
      setBallPos(uint8_t ball, uint8_t pos);

    void
      setCyclePerFrame(void);
};

#endif

