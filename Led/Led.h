//
//  Led.h
//
#ifndef Led_h
#define Led_h

class Led
{
  public:

    Led(uint8_t i);

    uint8_t
      getNumLeds(void);
    void
      fill(CHSV color),
      fillHue(uint8_t hue),
      fillBlack(void);
    void
      setPixelColor(uint8_t i, CHSV color), setPixelColorNoMap(uint8_t i, CHSV color),
      setPixelHue(uint8_t i, uint8_t hue), setPixelHueNoMap(uint8_t i, uint8_t hue),
      setPixelBlack(uint8_t i), setPixelBlackNoMap(uint8_t i);
    void
      dimPixel(uint8_t i, uint8_t amount),
      dimAllPixels(uint8_t amount);
    void
      setLedMap(uint8_t *led_map_pointer), turnOnLedMap(), turnOffLedMap(),
      setCoordMap(uint8_t width, uint8_t *coord_pointer),
      setNeighborMap(uint8_t *neighbor_map);
    uint8_t
      lookupLed(uint8_t i),
      getLedFromCoord(uint8_t x, uint8_t y),
      getNeighbor(uint8_t pos, uint8_t dir),
      getSymmetry(void);
    void morph_frame(uint8_t morph, uint8_t total_frames);
    void push_frame(void);
    void
      setBlur(uint8_t b), turnOffBlur(void);
    bool
      hasBlur(void);
    void
      addPixelColor(uint8_t i, CHSV c2), addPixelColorNoMap(uint8_t i, CHSV c2);
    CHSV
      getCurrFrameColor(uint8_t i),
      getNextFrameColor(uint8_t i),
      getInterpFrameColor(uint8_t i);
    uint8_t
      getInterpFrameHue(uint8_t i),
      getInterpFrameSat(uint8_t i),
      getInterpFrameVal(uint8_t i);
    void
      setInterpFrame(uint8_t i, CHSV color),
      setInterpFrameHue(uint8_t i, uint8_t hue),
      setInterpFrameSat(uint8_t i, uint8_t sat),
      setInterpFrameVal(uint8_t i, uint8_t val);
    CHSV
      wheel(uint8_t hue),
      gradient_wheel(uint8_t hue, uint8_t intensity);
    CHSV rgb_to_hsv( CRGB color);
    CHSV
      getInterpHSV(CHSV c1, CHSV c2, uint8_t fract),
      getInterpHSV(CHSV c1, CHSV c2, CHSV old_color, uint8_t fract),
      getInterpHSVthruRGB(CHSV c1, CHSV c2, uint8_t fract),
      smooth_color(CHSV old_color, CHSV new_color, uint8_t max_change);
    CRGB
        getInterpRGB(CRGB c1, CRGB c2, uint8_t fract);
    CHSV getInterpRGBthruHSV(CRGB c1, CRGB c2, uint8_t fract);
    void RGBtoHSV(uint8_t red, uint8_t green, uint8_t blue, float *h, float *s, float *v );
    void rgb2hsv_new(uint8_t src_r, uint8_t src_g, uint8_t src_b, uint8_t *dst_h, uint8_t *dst_s, uint8_t *dst_v);
    bool
      is_black(CHSV color);
    uint8_t
      interpolate(uint8_t a, uint8_t b, uint8_t fract),
      interpolate_wrap(uint8_t a, uint8_t b, uint8_t fract),
      interpolate_wrap(uint8_t a, uint8_t b, uint8_t old_value, uint8_t fract),
      smooth(uint8_t old_value, uint8_t new_value, uint8_t max_change),
      smooth_wrap(uint8_t old_value, uint8_t new_value, uint8_t max_change);
    void
      setPalette(uint8_t palette_start, uint8_t palette_width),  // turns palettes on
      setPalette(void),  // turn palettes on with default values
      setOnlyRed(void);  // Use only purple-red-yellow colors
    void
      turnPalettesOn(void), turnPalettesOff(void);
    void
      setAsSquare(void), setAsPentagon(void);
    void randomizePalette(void);
    CHSV constrain_palette(CHSV color);

  private:

    // variables
    uint16_t numLeds;
    uint8_t width_2d;

    CHSV *current_frame;
    CHSV *next_frame;
    CHSV *interp_frame;

    bool
      canChangePalette, is_only_red;

    uint8_t
      _palette_start, _palette_width;

    uint8_t
      *led_map, *coords, *neighbors;

    uint8_t blur_amount;

    bool
      is_mapped, is_2d_mapped, is_neighbor_mapped;  // Are the LEDs explicitly mapped?

    uint8_t num_neighbors;
    // private functions

};

#endif

