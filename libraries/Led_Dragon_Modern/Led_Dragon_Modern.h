//
//  Led_Dragon_Modern.h
//
#ifndef Led_Dragon_Modern_h
#define Led_Dragon_Modern_h

class Led
{
  public:

    Led(uint16_t i);

    uint16_t
      getNumLeds(void);
    void
      fill(CHSV color),
      fillHue(uint8_t hue),
      fillBlack(void);
    void
      setPixelColor(uint16_t i, CHSV color), setPixelHue(uint16_t i, uint8_t hue),
      increasePixelHue(uint16_t i, uint8_t increase), flipPixel(uint16_t i),
      setPixelBlack(uint16_t i);
    void
      dimPixel(uint16_t i, uint8_t amount),
      dimAllPixels(uint8_t amount);
    void
      smooth_frame(uint8_t max_change), push_frame(void);
    void
      addPixelColor(uint16_t i, CHSV c2), addPixelColorNoMap(uint16_t i, CHSV c2);
    CHSV
      getCurrFrameColor(uint16_t i), getNextFrameColor(uint16_t i);
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
    // CHSV getInterpRGBthruHSV(CRGB c1, CRGB c2, uint8_t fract);
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
    uint16_t numLeds;
    uint8_t width_2d;

    CHSV *current_frame;
    CHSV *next_frame;

    uint8_t num_neighbors = 6;

    // private functions

};

#endif

