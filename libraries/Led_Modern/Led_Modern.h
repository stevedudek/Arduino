//
//  Led_Modern.h - handles new clock cycling, 16-bit LED number
//
#ifndef Led_Modern_h
#define Led_Modern_h

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
      setCurrentFrame(uint8_t i, CHSV color),
      setPixelHue(uint8_t i, uint8_t hue), setPixelHueNoMap(uint8_t i, uint8_t hue),
      increasePixelHue(uint8_t i, uint8_t increase),
      setPixelBlack(uint8_t i), setPixelBlackNoMap(uint8_t i);
    void
      dimPixel(uint8_t i, uint8_t amount),
      dimAllPixels(uint8_t amount);
    void
      setLedMap(uint8_t *led_map_pointer), turnOnLedMap(), turnOffLedMap(),
      setCoordMap(uint8_t width, const uint8_t *coord_pointer),
      setNeighborMap(const uint8_t *neighbor_map);
    uint8_t
      lookupLed(uint8_t i),
      getLedFromCoord(uint8_t x, uint8_t y),
      getNeighbor(uint8_t pos, uint8_t dir);
    uint8_t
      getSymmetry(void);
    void
      setAsSquare(void), setAsPentagon(void), setAsHexagon(void), setOnlyRed(void);
    void
      smooth_frame(uint8_t max_change), push_frame(void);
    void
      addPixelColor(uint8_t i, CHSV c2), addPixelColorNoMap(uint8_t i, CHSV c2);
    CHSV
      getCurrFrameColor(uint8_t i), getNextFrameColor(uint8_t i);
    CHSV
      wheel(uint8_t hue),
      gradient_wheel(uint8_t hue, uint8_t intensity);
    CHSV
      rgb_to_hsv( CRGB color),
      getInterpHSV(CHSV c1, CHSV c2, uint8_t fract),
      smooth_color(CHSV old_color, CHSV new_color, uint8_t max_hue_change, uint8_t max_value_change),
      narrow_palette(CHSV color, uint8_t hue_center, uint8_t hue_width, uint8_t saturation);
    CRGB
        getInterpRGB(CRGB c1, CRGB c2, uint8_t fract),
        smooth_rgb_color(CRGB old_color, CRGB new_color, uint8_t max_change);
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

  private:

    // variables
    uint8_t numLeds;
    uint8_t width_2d;

    CHSV *current_frame;
    CHSV *next_frame;

    bool
      is_only_red;

    const uint8_t
      *led_map, *coords, *neighbors;

    bool
      is_mapped = false,
      is_2d_mapped = false,
      is_neighbor_mapped = false;  // Are the LEDs explicitly mapped?

    uint8_t num_neighbors = 6;

};

#endif

