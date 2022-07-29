//
//  Shows_Dragon.cpp
//
//  This library is for LED show creation
//
//  Client responsibilities:
//     The show runner (usually a switch statement against current_show)
//     morph_frame()
//     fading the lights at the beginning and end of shows (usually a check_fades() function)
//     Implementation of novel shows addition to this library
//     small_cycle
//
//  Show library responsibilities:
//     morph, cycle, wait: don't store copies of these variables on the client side
//     foreColor, backColor: don't store copies of variables on the client side
//     colorSpeeds and tweaking colors
//
//  Shows
//     within this library, use the led->func() notation, such as led->setPixelHue(i, foreColor)
//
//
//   USE this REPO
//
#include <FastLED.h>
#include <Led_Dragon.h>
#include "Shows_Dragon.h"

#define MAX_COLOR  256

#define MAX_MEMORY  500

#define SECTION_HEAD  0
#define SECTION_A     1
#define SECTION_B     2
#define SECTION_TAIL  3

#define SECT_NUM_SCALES_FIELD      0
#define SECT_NUM_SPACERS_FIELD     1
#define SECT_NUM_LEDS_FIELD        2
#define SECT_NUM_COLUMNS_FIELD     3
#define SECT_Y_OFFSET_FIELD        4
#define SECT_dX_UP_OFFSET_FIELD    5
#define SECT_dX_DOWN_OFFSET_FIELD  6
#define SECT_TABLE_NUM_FIELDS      7

#define SPACER_FIELD 0
#define Y_START_FIELD 1
#define COLUMN_HEIGHT_FIELD 2
#define COLUMN_PIXEL_START_FIELD 3

#define MAX_BALLS    4  // 40 bytes per section
#define TRAIL_SIZE   5  // a ball takes up 10 bytes
#define TRAIL_MEM_SIZE  (1 + (TRAIL_SIZE * 2))
#define SYMMETRY_CHANCE  20

#define NUM_PATTERNS        9
#define NUM_PATTERN_FILLS   1
#define NUM_PATTERN_WIPES  13
#define NUM_TUBE_TYPES     12

#define OFF  255
#define XX   9999  // Out of bounds

//
// Constructor
//
Shows::Shows(Led* led_pointer, uint8_t num_sections)
{
  led = new Led(*led_pointer);
  numLeds = led->getNumLeds();
  numSections = num_sections;

  memory = (uint16_t *)calloc(MAX_MEMORY, sizeof(uint16_t));

  resetAllClocks();

  // color defaults. If desired, override with setter functions below.
  foreColor = 0;  // setForeColor(uint8_t c)
  backColor = MAX_COLOR / 2;  // setBackColor(uint8_t c)
  color_speed_min = 20;  // setColorSpeedMinMax(uint8_t speed_min, uint8_t speed_max)
  color_speed_max = 50;
  foreColorSpeed = color_speed_max / 2;
  backColorSpeed = color_speed_max / 2;

  // wait defaults. If desired, override with setter functions below.
  has_morphing = true;

  wait_min = 5;  // setWaitRange(uint8_t min, uint8_t max, uint8_t values)
  wait_max = 50;
  num_wait_values = 45;
  wait = num_wait_values / 2;  // setWait(uint8_t w)

  // bands defaults. If desired, override with setter functions below.
  bands_min_bpm = 5;  // setBandsBpm(uint8_t bands_min, uint8_t bands_max)
  bands_max_bpm = 30;

}

//
// Public Methods //////////////////////////////////////////////////////////////
//
void Shows::setPointers(uint8_t num_sections, const uint8_t *section_list_p, const int8_t *section_table_p,
                        const int8_t *head_column_table_p, const int8_t *a_column_table_p,
                        const int8_t *b_column_table_p, const int8_t *tail_column_table_p)
{
  numSections = num_sections;
  section_list = section_list_p;
  section_table = section_table_p;
  head_column_table = head_column_table_p;
  a_column_table = a_column_table_p;
  b_column_table = b_column_table_p;
  tail_column_table = tail_column_table_p;

  calc_coordinate_frame();
}

int8_t Shows::get_column_table_value(uint8_t column, uint8_t field_index, uint8_t section) {
  uint8_t max_columns = get_section_table_value(section, SECT_NUM_COLUMNS_FIELD);

  switch (section_list[section]) {
    case SECTION_HEAD:
      return pgm_read_byte_near(head_column_table + ((column % max_columns) * 4) + field_index);
    case SECTION_A:
      return pgm_read_byte_near(a_column_table + ((column % max_columns) * 4) + field_index);
    case SECTION_B:
      return pgm_read_byte_near(b_column_table + ((column % max_columns) * 4) + field_index);
    case SECTION_TAIL:
      return pgm_read_byte_near(tail_column_table + ((column % max_columns) * 4) + field_index);
    default:
      Serial.println("Out of bounds for section type!");
      return 0;
  }
}

int8_t Shows::get_section_table_value(uint8_t section, uint8_t field_index) {
  return pgm_read_byte_near(section_table + (section_list[section] * SECT_TABLE_NUM_FIELDS) + field_index);
}


void Shows::morphFrame(void)
{
  // wrapper for led.morph_frame(). Handles the 2 parameters
  if (has_morphing) {
    led->morph_frame(morph, getNumFrames());
  } else {
    led->morph_frame(getNumFrames(), getNumFrames());
  }
}

uint8_t Shows::getMorphFract(void)
{
  return map(morph, 0, getNumFrames(), 0, 255);  // 0 - 255
}

void Shows::setForeColor(uint8_t c)
{
  foreColor = c;
}

void Shows::setBackColor(uint8_t c)
{
  backColor = c;
}

uint8_t Shows::getForeColor(void)
{
  return foreColor;
}

uint8_t Shows::getBackColor(void)
{
  return backColor;
}

CHSV Shows::getForeBlack(void)
{
  return CHSV(foreColor, 255, 0);
}

CHSV Shows::getBackBlack(void)
{
  return CHSV(backColor, 255, 0);
}

void Shows::setForeColorSpeed(uint8_t amount)
{
  foreColorSpeed = amount;
}

void Shows::setBackColorSpeed(uint8_t amount)
{
  backColorSpeed = amount;
}

uint8_t Shows::getForeColorSpeed(void)
{
  return foreColorSpeed;
}

uint8_t Shows::getBackColorSpeed(void)
{
  return backColorSpeed;
}

void Shows::setPixeltoForeColor(uint16_t i)
{
  led->setPixelHue(i, foreColor);
}

void Shows::setPixeltoBackColor(uint16_t i)
{
  led->setPixelHue(i, backColor);
}

void Shows::setPixeltoHue(uint16_t i, uint8_t h)
{
  led->setPixelHue(i, h);
}

void Shows::setPixeltoBlack(uint16_t i)
{
  led->setPixelBlack(i);
}

void Shows::setPixeltoForeBlack(uint16_t i)
{
  led->setPixelColor(i, getForeBlack());
}

void Shows::setPixeltoBackBlack(uint16_t i)
{
  led->setPixelColor(i, getBackBlack());
}

void Shows::setPixeltoColor(uint16_t i, CHSV color)
{
  led->setPixelColor(i, color);
}

void Shows::setCoordtoHue(uint16_t coord, uint8_t hue)
{
  setPixeltoHue(get_scale_for_coord(coord), hue);
}

void Shows::setCoordtoColor(uint16_t coord, CHSV color)
{
  setPixeltoColor(get_scale_for_coord(coord), color);
}

void Shows::addColortoCoord(uint16_t coord, CHSV color)
{
  led->addPixelColor(get_scale_for_coord(coord), color);
}

void Shows::addPixelColor(uint16_t i, CHSV color)
{
  led->addPixelColor(i, color);
}

void Shows::flipPixel(uint16_t i)
{

 if (led->getNextFrameColor(i).v == 0) {
    setPixeltoForeColor(i);
 } else {
    setPixeltoBlack(i);
 }
}

void Shows::fill(CHSV color)
{
  led->fill(color);
}

void Shows::fillForeColor()
{
  led->fillHue(foreColor);
}

void Shows::fillBackColor()
{
  led->fillHue(backColor);
}

void Shows::fillBlack()
{
  led->fillBlack();
}

void Shows::fillForeBlack()
{
  led->fill(getForeBlack());
}

void Shows::fillBackBlack()
{
  led->fill(getBackBlack());
}

void Shows::resetMorph(void)
{
  morph = 0;
}

void Shows::resetCycle(void)
{
  cycle = 0;
}

void Shows::resetSmallCycle(void)
{
  smallCycle = 0;
}

uint16_t Shows::getNumLeds(void)
{
  return numLeds;
}

uint8_t Shows::getSections(void)
{
  return numSections;
}

uint8_t Shows::getMorph(void)
{
  return morph;
}

uint8_t Shows::getPattern(void)
{
  return pattern;
}

uint8_t Shows::getPatternFill(void)
{
  return patternFill;
}

uint8_t Shows::getPatternWipe(void)
{
  return patternWipe;
}

uint8_t Shows::getTubeType(void)
{
  return tubeType;
}

void Shows::setPattern(uint8_t value)
{
  pattern = value;
}

void Shows::setPatternFill(uint8_t value)
{
  patternFill = value;
}

void Shows::setPatternWipe(uint8_t value)
{
  patternWipe = value;
}

void Shows::setTubeType(uint8_t value)
{
  tubeType = value;
}

uint16_t Shows::getCycle(void)
{
  return cycle;
}

void Shows::setCycle(uint16_t c)
{
  cycle = c;
}

uint32_t Shows::getSmallCycle(void)
{
  return smallCycle;
}

void Shows::setSmallCycle(uint32_t c)
{
  smallCycle = c;
}

void Shows::IncForeColor(uint8_t amount)
{
  foreColor = IncColor(foreColor, amount);
}

void Shows::IncBackColor(uint8_t amount)
{
  backColor = IncColor(backColor, amount);
}

uint8_t Shows::IncColor(uint8_t hue, uint8_t amount)
{
  return (hue + amount) % MAX_COLOR;
}

void Shows::advanceClock(void)
{
  // Handles advancement of both the "minute" morph hand and "hour" cycle hand
  morph++;
  if (has_morphing == false || morph >= getNumFrames()) { // Finished morphing
    morph = 0;
    cycle++;
    led->push_frame();
  }
  smallCycle++;
  if (smallCycle % foreColorSpeed == 0) {
    IncForeColor(1);
  }
  if (smallCycle % backColorSpeed == 0) {
    IncBackColor(2);
  }
}

void Shows::resetAllClocks(void)
{
   morph = 0;
   cycle = 0;
   smallCycle = 0;
}

bool Shows::isMorphing(void)
{
  return has_morphing;
}

void Shows::turnOffMorphing(void)
{
  has_morphing = false;
}

void Shows::turnOnMorphing(void)
{
  has_morphing = true;
}

void Shows::set_symmetry(void)
{
  if (random8(SYMMETRY_CHANCE) == 1) {
    is_symmetric = true;
  } else {
    is_symmetric = false;
  }
}

void Shows::symmetrize(void)
{
  if (is_symmetric) {
    uint16_t dscale = 0;  // sectional offset
    // Iterate over all sections
    for (uint8_t section = 0; section < numSections; section++) {
      for (uint8_t column = 0; column < get_section_table_value(section, SECT_NUM_COLUMNS_FIELD); column++) {
        uint8_t col_height = get_column_table_value(column, COLUMN_HEIGHT_FIELD, section);
        uint16_t col_pixel_start = dscale + get_column_table_value(column, COLUMN_PIXEL_START_FIELD, section);

        for (uint8_t y = 0; y < col_height / 2; y++) {
          led->setPixelColor(col_pixel_start + col_height - y - 1, led->getNextFrameColor(col_pixel_start + y));
        }
      }
      dscale += get_section_table_value(section, SECT_NUM_SCALES_FIELD);
    }
  }
}

void Shows::pickRandomColorSpeeds(void)
{
  foreColorSpeed = random(color_speed_min, color_speed_max);
  backColorSpeed = random(color_speed_min, color_speed_max);
}

void Shows::tweakColorSpeeds(void)
{
  foreColorSpeed = up_or_down(foreColorSpeed, color_speed_min, color_speed_max);
  backColorSpeed = up_or_down(backColorSpeed, color_speed_min, color_speed_max);
}

uint8_t Shows::up_or_down(uint8_t value, uint8_t min_value, uint8_t max_value)
{
  value += (random8(2) * 2 - 1);  // 0-1 -> 0,2 -> -1 or +1
  if (value < min_value) {
    return min_value + 1;
  } else if (value > max_value) {
    return max_value - 1;
  } else {
    return value;
  }
}

void Shows::setBandsBpm(uint8_t bands_min, uint8_t bands_max)
{
  bands_min_bpm = bands_min;
  bands_max_bpm = bands_max;
}

void Shows::setColorSpeedMinMax(uint8_t speed_min, uint8_t speed_max)
{
  color_speed_min = speed_min;
  color_speed_max = speed_max;
}

void Shows::setColorSpeedMinMax(uint8_t show_speed)
{
  color_speed_min = map8(show_speed, 1, 10);
  color_speed_max = map8(show_speed, 10, 100);
}

uint8_t Shows::getWait(void)
{
  return wait;
}

void Shows::setWaitAbsolute(uint8_t w)
{
  wait = w;
}

void Shows::setWait(uint8_t w)
{
  wait = max(min(w, wait_max), wait_min);
}

void Shows::setWaitRange(uint8_t show_speed)
{
  wait_min = map8(show_speed, 1, 40);
  wait_max = map8(show_speed, 20, 200);
  num_wait_values = wait_max - wait_min;
  setWait(wait);  // make sure wait is within the new range
}

void Shows::setWaitRange(uint8_t min, uint8_t max)
{
  wait_min = min;
  wait_max = max;
  num_wait_values = max - min;
  setWait(wait);  // make sure wait is within the new range
}

void Shows::setWaitRange(uint8_t min, uint8_t max, uint8_t values)
{
  wait_min = min;
  wait_max = max;
  num_wait_values = max - min;
  setWait(wait);  // make sure wait is within the new range
}

void Shows::makeWaitFaster(uint8_t r)
{
  setWait(wait / r);
}

void Shows::makeWaitSlower(uint8_t r)
{
  setWait(min(num_wait_values-1, (wait * r)));
}

void Shows::tweakWait(void)
{
  wait = up_or_down(wait, 0, num_wait_values);
}

void Shows::pickRandomWait(void)
{
  wait = wait_min + random(num_wait_values);
}

uint8_t Shows::getNumFrames(void)
{
  return map(wait, 0, num_wait_values-1, wait_min, wait_max);
}

bool Shows::isShowStart(void)
{
  return (cycle == 0 && morph == 0);
}

//
// Show functions - all should return void; use led->func notation
//
void Shows::allOn(void)
{
  led->fillHue(foreColor);
}

void Shows::randomFill(void)
{
  if (isShowStart()) {
    led->fill(getForeBlack());
  }
  flipPixel(random16(numLeds));
}

void Shows::randomColors(void)
{
  if (isShowStart()) {  // Start of show: assign lights to random colors
    for (uint16_t i=0; i < numLeds; i++) {
      led->setPixelHue(i, random8());
    }
  }
  for (uint16_t i=0; i < numLeds; i++) {
    led->increasePixelHue(i, 1);
  }
}

void Shows::randomOneColorBlack(void)
{
  if (isShowStart()) {  // Start of show: assign lights to random colors
    for (uint16_t i=0; i < numLeds; i++) {
      if(random8(2) == 0) {
        setPixeltoForeColor(i);
      } else {
        setPixeltoForeBlack(i);
      }
    }
  }
  for (uint8_t i=0; i < numLeds; i++) {
    led->increasePixelHue(i, 1);
  }
}

void Shows::randomTwoColorBlack(void)
{
  if (isShowStart()) {  // Start of show: assign lights to random colors
    for (uint16_t i=0; i < numLeds; i++) {
      switch (random8(3)) {
        case 0:
          setPixeltoForeColor(i);
          break;
        case 1:
          setPixeltoBackColor(i);
          break;
        default:
          setPixeltoForeBlack(i);
          break;
      }
    }
  }
  for (uint16_t i=0; i < numLeds; i++) {
    led->increasePixelHue(i, 1);
  }
}

void Shows::twoColor(void)
{
  if (isShowStart()) {  // Start of show: assign lights to random colors
    for (uint16_t i=0; i < numLeds; i++) {
      if(random8(2) == 0) {
        setPixeltoForeColor(i);
      } else {
        setPixeltoBackColor(i);
      }
    }
  }
  for (uint16_t i=0; i < numLeds; i++) {
    led->increasePixelHue(i, 1);
  }
}

void Shows::morphChain(void)
{
  uint8_t color;
  for (uint16_t i=0; i < numLeds; i++) {
    color = map8(sin8_C((i+cycle) * foreColorSpeed * 0.5), 0, sin8_C(backColor));
    led->setPixelHue(numLeds-i-1, foreColor + color);
  }
}

void Shows::confetti(void)
{
  led->dimAllPixels(10);  // Try different amounts

  for (uint8_t i=0; i < numSections; i++) {
    if (random8(4) == 0) {  // Fewer confetti for higher numbers
      setPixeltoForeColor(random16(numLeds - 1));
    }
  }
}

void Shows::expanding_drops(bool filled)
{

  // A drop is 3 bytes: coord, size, color

  // Empty out drops at the start of the show
  if (isShowStart()) {
    fillForeBlack();
    makeWaitFaster(3);
    uint8_t num_drops = random8(3,8) * numSections;
    setMemory(num_drops, 0);  // 2-8 is number of drops per section
    setMemory(random8(4,10), 1);  // max drop size
    for (uint8_t i = 0; i < num_drops; i++) {
      setMemory(XX, 2 + (i * 3));  // default coord to XX = off
    }
  }

  if (filled) {
    led->dimAllPixels(2);  // Try different amounts
  }

  if (morph > 0) {
    return;
  }

  if (!filled) {
    fillBlack();
  }

  for (uint8_t i = 0; i < getMemory(0); i++) {
    uint16_t coord = getMemory(2 + (i * 3));

    if (coord == XX) {  // drop is off
      if (random8(10) == 1) {  // 1 in 10 chance a dead drop will wake up
        setMemory(getRandomCoord(), 2 + (i * 3) + 0);  // coord
        setMemory(0, 2 + (i * 3) + 1);  // size = 0 at start
        setMemory(random(255), 2 + (i * 3) + 2);  // hue = random
      }
    } else {  // bar is alive
      uint8_t size = lowByte(getMemory(2 + (i * 3) + 1));
      if (size >= getMemory(1)) {
        // drop is too big
        setMemory(XX, 2 + (i * 3) + 0);  // turn off
      } else {
        if (filled) {
          drawHexRing(coord, size, led->gradient_wheel(getMemory(2 + (i * 3) + 2), 255 * (getMemory(1) - size) / getMemory(1)));
        } else {
          drawHexRing(coord, size, led->wheel(getMemory(2 + (i * 3) + 2)));
        }
        setMemory(size + 1, 2 + (i * 3) + 1);
      }
    }
  }
}

void Shows::moving_bars(bool rain)
{

  // A bar is 4 bytes: 0 (direction-XX if off), 1 coord, 2 speed, 3 color

  // Empty out bars at the start of the show
  if (isShowStart()) {
    uint8_t num_bars = random8(6,16) * numSections;
    setMemory(num_bars, 0);  // number of bars per section
    for (uint8_t i = 0; i < num_bars; i++) {
      setMemory(XX, 2 + (i * 4));
    }
    uint8_t bar_dir = (rain) ? 3 : random8(2,4) ;
    setMemory(bar_dir, 1);  // general direction of the bars
  }

  led->dimAllPixels(10);  // Try different amounts

  for (uint8_t i = 0; i < getMemory(0); i++) {
    uint16_t dir = getMemory(2 + (i * 4) + 0);

    if (dir == XX) {  // bar is off
      if (random8(10) == 1) {  // 1 in 10 chance a dead bar will wake up
        int8_t x_start = (min_x - height_y) + random8(width_x + height_y + height_y);  // Bad for a wide Dragon
        setMemory(random(1, 10), 2 + (i * 4) + 2);  // speed
        setMemory((getForeColor() + random8(60)) % MAX_COLOR, 2 + (i * 4) + 3);  // hue
        if (!rain && random(2) == 0) {  // head up
          setMemory((getMemory(1) + 3) % 6, 2 + (i * 4) + 0);  // up dir
          setMemory(get_coord_from_xy(x_start, min_y), 2 + (i * 4) + 1);  // get_coord_col_min_y may not work
        } else {
          setMemory(getMemory(1), 2 + (i * 4) + 0);  // down dir
          setMemory(get_coord_from_xy(x_start, max_y), 2 + (i * 4) + 1);
        }
      }
    } else {  // bar is alive
      if (getSmallCycle() % getMemory(2 + (i * 4) + 2) == 0) {
        uint16_t coord = getMemory(2 + (i * 4) + 1);
        coord = get_coord_in_dir(dir, coord);
        setMemory(coord, 2 + (i * 4) + 1);

        int8_t y = get_y_from_coord(coord);
        if (y < min_y || y >= max_y) {
          setMemory(XX, 2 + (i * 4) + 0);  // off grid; set direction to XX
        } else {
          setCoordtoHue(coord, getMemory(2 + (i * 4) + 3)); // Draw a colored dot
        }
      }
    }
  }
}

//
// horiz back forth dots - horizontal dots moving back and forth
//
void Shows::horiz_back_forth_dots(void) {
  if (isShowStart()) {
    makeWaitFaster(3);
    setMemory(random8(1,10), 0);
  }

  led->dimAllPixels(getMemory(0));

  if (morph > 0) {
    return;
  }

  for (uint8_t x = 0; x < width_x; x++) {
    for (uint8_t y = 0; y < height_y; y++) {
      uint8_t temp_y = (x % 2) ? y : height_y - y - 1;
      if ((temp_y + getCycle()) % height_y == 0) {
        uint16_t coord = get_coord_from_xy(x + min_x, y + min_y);
        setCoordtoHue(coord, getForeColor());
      }
    }
  }
}

//
// diag back forth dots - diagonal dots moving back and forth
//
void Shows::diag_back_forth_dots(void) {
  if (isShowStart()) {
    makeWaitFaster(3);
    setMemory(random8(1,10), 0);
  }

  led->dimAllPixels(getMemory(0));

  if (morph > 0) {
    return;
  }

  for (uint8_t x = 0; x < width_x; x++) {
    for (uint8_t y = 0; y < height_y; y++) {
      uint8_t temp_y = (x % 2) ? y : height_y - y - 1;
      uint8_t temp_x = (y % 2) ? x : width_x - x - 1;
      if ((temp_x + temp_y + getCycle()) % height_y == 0) {
        uint16_t coord = get_coord_from_xy(x + min_x, y + min_y);
        setCoordtoHue(coord, getForeColor());
      }
    }
  }
}

void Shows::sinelon_fastled(void)
{
  // a colored dot sweeping back and forth, with fading trails
  led->dimAllPixels(20);  // replaces: fadeToBlackBy( leds, NUM_LEDS, 20);
  uint8_t i = beatsin16(8, 0, numLeds);  // lower first number = slower
  led->setPixelColor(i, CHSV(foreColor, 255, 192));  // 192 = suggested v
}

void Shows::bpm_fastled(void)
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for(uint16_t i=0; i < numLeds; i++) {
    led->setPixelColor(i, led->gradient_wheel(foreColor + (i*2), beat - foreColor + (i*10) ) );
  }
}

void Shows::juggle_fastled(void)
{
  turnOffMorphing();
  // eight colored dots, weaving in and out of sync with each other
  uint8_t pixel;
  uint8_t dot_hue = 0;
  CHSV curr_color, new_color;

  led->dimAllPixels(20); // replaces: fadeToBlackBy( leds, NUM_LEDS, 20);

  for(uint8_t i=0; i < 8; i++) {
    pixel = beatsin16(i+3, 0, numLeds);
    curr_color = led->getCurrFrameColor(i);
    new_color = CHSV(curr_color.h | dot_hue, curr_color.s | 200, curr_color.v | 255);
    led->setPixelColor(pixel, new_color);
    dot_hue += 32;
  }
}

void Shows::sawTooth(void)
{
  uint8_t intense;
  for (uint16_t i=0; i < numLeds; i++) {
    intense = sin8_C((i + cycle) * foreColorSpeed * 0.5);
    // "i" will have pattern move up; "numLeds-i-1'' will have pattern move down
    led->setPixelColor(numLeds-i-1, led->gradient_wheel(foreColor+i, intense));
  }
}

void Shows::lightWave(void)
{
  led->dimAllPixels(3);
  setPixeltoForeColor(cycle % numLeds);
}

void Shows::lightRunUp(void)
{
  if (isShowStart()) {  // Start of show: assign lights to random colors
    setWait(1);
  }

  led->dimAllPixels(10);

  uint16_t pos = cycle % (numLeds*2);  // Where we are in the show
  if (pos >= numLeds) {
    pos = (numLeds * 2) - pos;
  }

  for (uint16_t i=0; i < pos; i++) {
    led->setPixelHue(i, foreColor + (i * (foreColorSpeed / (10 * numSections))));  // Turning on lights one at a time
  }
}

void Shows::bands(void)
{
  if (isShowStart()) {
    bands_bpm_1 = random(bands_min_bpm, bands_max_bpm);
    bands_bpm_2 = random(bands_min_bpm, bands_max_bpm);
    band_min_1 = random(64, 192);
    band_min_2 = random(64, 192);
  }
  turnOffMorphing();
  led->fill(getForeBlack());

  uint8_t wave;
  CHSV wave1, wave2;

  for (uint16_t i=0; i < numLeds; i++) {

    wave = beatsin8(bands_bpm_1, 0, 255, 0, map(i, 0, numLeds, 0, 255) );
    wave = (wave > band_min_1) ? map(wave, band_min_1, 255, 0, 255) : 0;
    wave1 = CHSV(foreColor, 255, wave);
    if (wave > 0) {
      led->setPixelColor(i, CHSV(foreColor, 255, wave) );
    }

    wave = beatsin8(bands_bpm_2, 0, 255, 0, map(i, 0, numLeds, 0, 255) );
    wave = (wave > band_min_2) ? map(wave, band_min_2, 255, 0, 255) : 0;
    wave2 = CHSV(backColor, 255, wave);

    led->setPixelColor(numLeds-i-1, led->getInterpHSV(wave1, wave2, 128));
    if (wave > 0) {
      led->addPixelColor(numLeds-i-1, CHSV(backColor, 255, wave) );
    }
  }
  led->push_frame();  // Prevents unwanted color interpolation
}

void Shows::packets(void)
{
  uint8_t max_packets = numSections * 2;

  if (isShowStart()) {
    for (uint8_t i=0; i < max_packets; i++) {
      setMemory(0, i * 3);  // (intense, freq, color) turn off all packets
    }
  }
  turnOffMorphing();
  led->fillBlack();

  uint8_t wave;

  for (uint8_t i=0; i < max_packets; i++) {

    // Adjust each packet either increasing or decreasing
    uint16_t packet_intense = getMemory(i * 3);
    uint16_t packet_freq = getMemory((i * 3) + 1);

    if (packet_intense == 0) {
      // New packet
      setMemory(1, i * 3);  // intense
      setMemory(random(1,4), (i * 2) + 1);  // frequency
      setMemory(random8(255), (i * 2) + 2);  // color

    } else if (packet_intense % 2 == 1) {
      // Increasing
      if (packet_intense + packet_freq > 253) {
        setMemory(252, i * 3);  // Switch to decreasing
      } else {
        setMemory(packet_intense + packet_freq, i * 3);  // Switch to decreasing
      }

    } else {
      // Decreasing
      if (packet_intense < packet_freq * 2) {
        setMemory(0, i * 3);
      } else {
        setMemory(packet_intense - packet_freq, i * 3);  // Switch to decreasing
      }
    }

    // Apply each packet to the lights
    if (packet_intense > 0 ) {
      uint8_t packet_color = getMemory((i * 3) + 2);
      for (uint16_t j=0; j < numLeds; j++) {
        wave = beatsin8(5, 0, packet_intense, 0, map(j, 0, numLeds, cycle, 128 * packet_freq) );
        led->addPixelColor(j, CHSV(packet_color, 255, wave) );
      }
    }
  }
  led->push_frame();  // Prevents unwanted color interpolation
}

void Shows::packets_two(void)
{

  uint8_t max_packets = numSections * 2;

  if (isShowStart()) {
    for (uint8_t i=0; i < max_packets; i++) {  // (intense, freq, color)
      setMemory(random(15, 80), (i * 3) + 1);  //  freq
      setMemory(random8(255), (i * 3) + 2);    //  color
    }
  }
  turnOffMorphing();
  led->fillBlack();

  uint8_t wave;
  uint16_t pixel;

  for (uint8_t i=0; i < max_packets; i++) {

    if (random(0, 255) == 1) {
      setMemory((getMemory((i * 3) + 2) + random8(20,50)) % 255, (i * 3) + 2);    //  color
    }

    uint8_t packet_freq = getMemory((i * 3) + 1);
    uint8_t packet_color = getMemory((i * 3) + 2);

    // Apply each packet to the lights
    for (uint8_t j=0; j < numLeds; j++) {
      wave = beatsin8(packet_freq, 0, 255, 0, j);
//      wave = beatsin8(packet_freq[i]);
      pixel =  (packet_freq % 2) ? j : numLeds-j-1;
      led->addPixelColor(pixel, CHSV(packet_color, 255, wave) );
    }
  }
  led->push_frame();  // Prevents unwanted color interpolation
}

void Shows::plinko(void)
{
  uint16_t dir, num_choices;
  uint16_t coord;
  int8_t x_start;

  // A plinko is 2 bytes: (1 up or 0 down), coord

  // Refresh plinko at the start of the show
  if (isShowStart()) {
    makeWaitFaster(4);
    uint8_t num_plinko = random8(5, 10) * numSections;
    for (uint8_t i = 0; i < num_plinko; i++) {
      setMemory(XX, 1 + (i * 2));
    }
    setMemory(num_plinko, 0);  // 5-11 is number of plinkos per section
  }

  if (morph > 0) {
    return;
  }

  led->fillBlack();  // Erase all

  for (uint8_t i = 0; i < getMemory(0); i++) {  // 30 bytes per section
    dir = getMemory(1 + (i * 2));

    if (dir == XX) {  // Plinko is off
      if (random8(4) == 1) {  // 1 in 10 chance a dead plinko will wake up
        dir = random8(2);  // 0 or 1
        x_start = (dir == 0) ? 0 : max_x - 1 ;
        coord = get_coord_from_xy(x_start, random8(min_y + random8(max_y - min_y)));
        setMemory(dir, 1 + (i * 2) + 0);
        setMemory(coord, 1 + (i * 2) + 1);
      }
    } else {  // plinko is alive
      coord = getMemory(1 + (i * 2) + 1);
      led->setPixelHue(get_scale_for_coord(coord), IncColor(foreColor, 5 * i)); // Draw the plinko

      uint8_t directions[] = { 1, 2 };
      if (dir == 1) {
        directions[0] = 4;
        directions[1] = 5;
      }
      num_choices = 0;
      int8_t choices[2] = { -1, -1 };

      for (uint8_t dir_choice = 0; dir_choice < 2; dir_choice++) {
        if (is_coord_inbounds(get_coord_in_dir(directions[dir_choice], coord))) {
          choices[num_choices] = directions[dir_choice];
          num_choices++;
        }
      }
      if (num_choices == 0) {
        setMemory(XX, 1 + (i * 2));;  // nowhere to go; set direction to XX
      } else {
        dir = choices[random(num_choices)];
        setMemory(get_coord_in_dir(dir, coord), 1 + (i * 2) + 1);
      }
    }
  }
}

void Shows::bounce(void)
{
  const uint8_t trail_colors[] = { 195, 145, 85, 45, 5 };
  uint16_t coord, new_coord;

  if (isShowStart()) {
    setUpBalls();
  }

  if (morph != 0) {
    return;
  }

  led->fill(CHSV(170, 255, 40));  // changed color.v from 2 to 40

  for (uint8_t n = 0; n < MAX_BALLS; n++) {
    if (getBallDir(n) == OFF) {
      continue;  // Not an active ball
    }

    // Draw the trail
    for (uint8_t i = 0; i < TRAIL_SIZE; i++) {
      led->addPixelColor(get_scale_for_coord(getTrailCoord(n, i)), led->rgb_to_hsv(CRGB(0, 0, trail_colors[i])));
    }

    // Move the ball
    coord = getTrailCoord(n, 0);

    uint8_t max_attempts = 10;

    while (max_attempts-- > 0) {
      new_coord = get_coord_in_dir(getBallDir(n), coord);
      if (is_coord_inbounds(new_coord)) {
        break;
      } else {
        setBallDir(n, random8(6));
      }
    }

    // Update the trail
    for (uint8_t i = TRAIL_SIZE - 1; i > 0; i--) {
      setTrailCoord(n, i, getTrailCoord(n, i - 1) );
    }
    setTrailCoord(n, 0, new_coord);
  }
}

void Shows::bounceGlowing(void)
{
  uint16_t coord, new_coord, new_coord2;

  const uint8_t glow_colors[] = { 255, 105, 26 };
  CHSV color_0 = led->rgb_to_hsv(CRGB(0, glow_colors[0], glow_colors[0]));
  CHSV color_1 = led->rgb_to_hsv(CRGB(0, glow_colors[1], glow_colors[1]));
  CHSV color_2 = led->rgb_to_hsv(CRGB(0, glow_colors[2], glow_colors[2]));

  if (isShowStart()) {
    setUpBalls();
  }

  if (morph != 0) {
    return;
  }

  led->fill(led->rgb_to_hsv(CRGB(0, 20, 20)));  // changed g,b=2 to g,b=40

  // Draw the glowing ball
  for (uint8_t n = 0; n < MAX_BALLS; n++) {
    if (getBallDir(n) == OFF) {
      continue;  // Not an active ball
    }
    coord = getTrailCoord(n, 0);

    // Draw the 2nd ring
    for (uint8_t dir = 0; dir < 6; dir++) {
      new_coord = get_coord_in_dir(dir, coord);

      for (uint8_t dir2 = 0; dir2 < 6; dir2++) {
        new_coord2 = get_coord_in_dir(dir2, new_coord);
        led->addPixelColor(get_scale_for_coord(new_coord2), color_2);
      }
    }

    // Draw the first ring
    for (uint8_t dir = 0; dir < 6; dir++) {
      new_coord = get_coord_in_dir(dir, coord);
      led->addPixelColor(get_scale_for_coord(new_coord), color_1);
    }

    // Draw the center
    led->addPixelColor(get_scale_for_coord(coord), color_0);

    // Move the ball
    uint8_t max_attempts = 10;

    while (max_attempts-- > 0) {
      new_coord = get_coord_in_dir(getBallDir(n), coord);
      if (is_coord_inbounds(new_coord)) {
        setTrailCoord(n, 0, new_coord);
        break;
      } else {
        setBallDir(n, random8(6));
      }
    }
  }
}

void Shows::setUpBalls(void)
{
  clearTrails();
  makeWaitFaster(4);

  uint8_t num_balls = random(2, MAX_BALLS);
  for (uint8_t n = 0; n < num_balls * numSections; n++) {
    dropBall(n);
  }
}


//
// tubes shows
//
void Shows::tubes(void)
{
  // Reset a lot of variables at the start of the show
  if (isShowStart()) {
    // turnOffMorphing();  // Because we are using a beatsin8 - not yet
    setTubeType(random8(NUM_TUBE_TYPES));
    setMemory(random(40, 100), 0);  // For TubeType 1
  }

  fillForeBlack();

  uint8_t intense, intense2;
  uint16_t x = 0;
  uint16_t dscale = 0;  // sectional offset

  // Iterate over all sections
  for (uint8_t section = 0; section < numSections; section++) {
    for (int8_t column = 0; column < get_section_table_value(section, SECT_NUM_COLUMNS_FIELD); column++) {
      int8_t col_height = get_column_table_value(column, COLUMN_HEIGHT_FIELD, section);
      int8_t col_pixel_start = get_column_table_value(column, COLUMN_PIXEL_START_FIELD, section);

      for (int8_t i = 0; i < col_height; i++) {
        uint8_t frac_height = 255 * i / col_height;
        intense2 = 0;

        switch (getTubeType() / 2) {

          case 0:
              // Center empty; edges bright
              intense = frac_height;
              if (intense > 128) {
                intense = 255 - (255 / col_height) - intense;  // 0 - 128
              }
              intense = (intense < 86) ? (85 - intense) * 3 : 0;
              break;
          case 1:
              // Center bright; edges empty
              intense = frac_height;
              if (intense > 128) {
                intense = 255 - (255 / col_height) - intense;  // 0 - 128
              }
              intense = (intense < 86) ? intense * 3 : 255;
              break;
          case 2:
              // Center one color; edges another color
              intense = frac_height;
              if (intense > 128) {
                intense = 255 - (255 / col_height) - intense;  // 0 - 128
              }
              if (intense < 86) {
                intense2 = intense * 3;
                intense = (85 - intense) * 3;
              } else {
                intense2 = 255;
                intense = 0;
              }
              break;
          case 3:
              // descending wave
              intense = sin8_C(frac_height + getSmallCycle());
              break;
          case 4:
              // vertical wave
              intense = sin8_C((x * 2) + getSmallCycle());
              break;
          default:
              // Two waves
              intense = sin8_C(frac_height + getSmallCycle());
              intense2 = cos8(frac_height - getSmallCycle());
              break;
        }
        if (intense > 0) {
          if (getTubeType() % 2) {
            addPixelColor(dscale + col_pixel_start + i, led->wheel(intense));
          } else {
            addPixelColor(dscale + col_pixel_start + i, led->gradient_wheel(getForeColor(), intense));
          }
        }
        if (intense2 > 0) {
          if (getTubeType() % 2) {
            addPixelColor(dscale + col_pixel_start + i, led->wheel(intense2));
          } else {
            addPixelColor(dscale + col_pixel_start + i, led->gradient_wheel(getForeColor(), intense2));
          }
        }

      }
      x++;
    }
    dscale += get_section_table_value(section, SECT_NUM_SCALES_FIELD);
  }
}

//
// fire - https://github.com/FastLED/FastLED/blob/master/examples/Fire2012/Fire2012.ino
//
// Fire2012 by Mark Kriegsman, July 2012
//
// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.

#define COOLING_MEMORY  (MAX_MEMORY - 1)
#define SPARKING_MEMORY  (MAX_MEMORY - 2)

void Shows::fire(void)
{
  if (isShowStart()) {
    setMemory(random(50, 100), COOLING_MEMORY);  // COOLING
    setMemory(random(50, 200), SPARKING_MEMORY);  // SPARKING
  }

  // Array of temperature readings at each simulation cell
  uint8_t heat;
  uint8_t cooling = lowByte(getMemory(COOLING_MEMORY));
  uint8_t sparking = lowByte(getMemory(SPARKING_MEMORY));

  // Step 1.  Cool down every cell a little
  for (uint16_t i = 0; i < numLeds; i++) {
      heat = qsub8( getMemory(i),  random8(0, ((cooling * 10) / numLeds) + 2));
      setMemory(heat, i);
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (uint16_t k = numLeds - 1; k >= 2; k--) {
      heat = (getMemory(k - 1) + getMemory(k - 2) + getMemory(k - 2) ) / 3;
      setMemory(heat, k);
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < sparking) {
      uint8_t y = random8(7);
      heat = qadd8( getMemory(y), random8(160, 255) );
      setMemory(heat, y);
  }

    // Step 4.  Map from heat cells to LED colors
  for (uint16_t j = 0; j < numLeds; j++) {
      setPixeltoColor(j, led->rgb_to_hsv(HeatColor(getMemory(j))));  // this could be expensive
  }
}


//
// patterns shows
//
void Shows::patterns(void)
{
  // Reset a lot of variables at the start of the show
  if (isShowStart()) {
    turnOffMorphing();  // Because we are using a beatsin8
    setPattern(random8(NUM_PATTERNS));
    setPatternFill(random8(NUM_PATTERN_FILLS));
    setPatternWipe(random8(NUM_PATTERN_WIPES));
  }

  // Set background to wipe
  // Draw each pattern 0 and 1

  fillWithWipe();

  uint8_t patternType = getPattern();

  if (patternType < 9) {
    drawTwoLineSets(patternType);
  }
}

void Shows::drawHexRing(uint16_t coord, uint8_t size, CHSV color)
{
  if (size == 0) {
    addColortoCoord(coord, color);
    return;
  }

  // Move away from the center
  for (uint8_t i = 0; i < size; i++) {
    coord = get_coord_in_dir(4, coord);
  }

  for (uint8_t dir = 0; dir < 6; dir++) {
    for (uint8_t i = 0; i < size; i++) {
      addColortoCoord(coord, color);
      if (size == 2) {
        Serial.printf("(%d, %d), ", get_x_from_coord(coord), get_y_from_coord(coord));
      }
      coord = get_coord_in_dir(dir, coord);
    }
  }
}

void Shows::drawTwoLineSets(uint8_t patternSet)
{
  for (int8_t i = min_y; i < max_y + (max_x - min_x); i = i + 3) {
    drawLineFromEdge(patternSet % 3, i,     getForeColor());
    drawLineFromEdge(patternSet / 3, i + 1, getBackColor());
  }
}

void Shows::drawLineFromEdge(uint8_t dir, int8_t i, uint8_t hue)
{
  uint16_t coord = (dir == 0) ? get_coord_from_xy(i, min_y) : get_coord_from_xy(min_x, i) ;
  uint8_t distance = (dir == 0) ? max_y - min_y - 1 : max_x - min_x - 1 ;

  drawLine(coord, dir, distance, hue);
}

void Shows::drawLine(uint16_t coord, uint8_t dir, uint8_t distance, uint8_t hue)
{
  for (uint8_t d = 0; d < distance; d++) {
    if (is_coord_inbounds(coord)) {
      setCoordtoHue(coord, hue);
    }
    coord = get_coord_in_dir(dir, coord);
  }
}

void Shows::fillWithWipe(void)
{
  // 0 = black
  // evens = color gradient, odds = intensity gradient
  // 6 wipe types

  uint8_t wipe = getPatternWipe();

  if (wipe == 0) {
    fillForeBlack();
    return;
  }
  wipe -= 1;

  uint8_t back_hue = getBackColor();

  for (int8_t x = min_x; x < max_x; x++) {
    for (int8_t y = min_y; y < max_y; y++) {
      uint8_t wipe_intensity = get_wipe_intensity(x, y, wipe / 2, wipe % 2);

      if (wipe % 2) {
        setPixeltoHue(get_scale_from_xy(x, y), IncColor(back_hue, wipe_intensity / 2));
      } else {
        setPixeltoColor(get_scale_from_xy(x, y), led->gradient_wheel(back_hue, map8(wipe_intensity, 64, 255)));
      }
    }
  }
}

uint8_t Shows::get_wipe_intensity(int8_t x, int8_t y, uint8_t wipe, uint8_t wipe_type)
{
  uint8_t intensity;

  x = map(x, min_x, max_x, 0, 255);
  y = map(y, min_y, max_y, 0, aspect);

  switch (wipe) {
    case 0:
      intensity = x * 4;  // coefficient = thickness
      break;
    case 1:
      intensity = y * 4;
      break;
    case 2:
      intensity = (x * 2) + (y * 2);
      break;
    case 3:
      intensity = (x * 2) + ((255 - y - 1) * 2);
      break;
    case 4:
      intensity = 255 - get_distance_to_origin(x, y);  // center wipe
      break;
    default:
      intensity = (255 - get_distance_to_coord(x, y, 0, 0)) / 2;  // corner wipe
      break;
  }
  uint8_t freq = (wipe_type == 0) ? 1 : 10;
  return beatsin8(freq, 0, 255, 0, intensity);
}

//
// Pendulum Wave
//
void Shows::pendulum_wave(boolean smoothed)
{

  if (isShowStart()) {
    setMemory(random(70, 120), 0);
    turnOffMorphing();
  }

  uint8_t frequency = lowByte(getMemory(0));
  uint8_t freq_step = 5 - numSections;

  for (uint8_t x = 0; x < width_x; x++) {
    uint8_t value = beatsin8(frequency - (freq_step * x));

    for (uint8_t y = 0; y < height_y; y++) {
      uint8_t pos = (y * 256 / height_y) + (256 / (height_y * 2));
      uint8_t dist = (pos >= value) ? 255 - (pos - value) : 255 - (value - pos) ;
      if (smoothed == false) {
        dist = (dist > 230) ? dist : 0 ;
      }
      setCoordtoColor(get_coord_from_xy(x + min_x, y + min_y), CHSV(getForeColor(), 255, dist) );
    }
  }
}

//
// Two Waves - two sine waves
//
void Shows::twoWaves(void)
{

  fillForeBlack();

  for (int8_t x = 0; x < width_x; x++) {
    uint8_t value1 = sin8_C((255 * x / width_x) - (getSmallCycle() / 2));
    uint8_t value2 = sin8_C((255 * x / width_x) - (getSmallCycle() / 3));

    for (int8_t y = 0; y < height_y; y++) {
      uint8_t scale_y = 255 * y / height_y;

      if (abs(value1 - scale_y) < 40) {
        uint8_t v = map(abs(value1 - scale_y), 0, 40, 255, 0);
        addColortoCoord(get_coord_from_xy(x + min_x, y + min_y), CHSV(getForeColor(), 255, v));
      }
      if (abs(value2 - scale_y) < 40) {
        uint8_t v = map(abs(value1 - scale_y), 0, 40, 255, 0);
        addColortoCoord(get_coord_from_xy(x + min_x, y + min_y), CHSV(getBackColor(), 255, v));
      }
    }
  }
}

//
// test_neighs - sandbox to test mapping
//
void Shows::test_neighs(void)
{
    if (isShowStart()) {
      fillForeBlack();
      Serial.println("ha!");
    }
    int8_t x = 10;
    int8_t y = -3;
    drawLine(get_coord_from_xy(x,y), 1, 6, getForeColor());
}


//
// Private Methods //////////////////////////////////////////////////////////////
//

//
//   Trails:  (dir) (x1, y1) (x2, y2) (x3, y3)...
//
void Shows::clearTrails(void)
{
  // clear out all the ball trails (set to OFF)
  for (uint8_t ball = 0; ball < MAX_BALLS * numSections; ball++) {
    setBallDir(ball, OFF);
  }
}

void Shows::dropBall(uint8_t ball)
{
  setTrailCoord(ball, 0, getRandomInBoundsCoord());
  setBallDir(ball, random8(6));
}

uint16_t Shows::getRandomInBoundsCoord(void)
{
  uint16_t coord;
  while (true) {
    coord = getRandomCoord();
    if (get_scale_for_coord(coord) != XX) {
      return coord;
    }
  }
}

uint16_t Shows::getRandomCoord(void)
{
  return get_coord_from_xy(min_x + random8(max_x - min_x),
                           min_y + random8(max_y - min_y));
}

uint8_t Shows::getBallDir(uint8_t ball)
{
  return lowByte(getMemory(ball * TRAIL_MEM_SIZE));  // dir is first position
}

void Shows::setBallDir(uint8_t ball, uint8_t dir)
{
  return setMemory(dir, ball * TRAIL_MEM_SIZE);  // dir is first position
}

uint16_t Shows::getTrailCoord(uint8_t ball, uint8_t dist)
{
  return getMemory((ball * TRAIL_MEM_SIZE) + 1 + dist);
}

void Shows::setTrailCoord(uint8_t ball, uint8_t dist, uint16_t value)
{
  setMemory(value, (ball * TRAIL_MEM_SIZE) + 1 + dist);
}

//
// get scale for xy (column, row) - return the scale number (or XX if out of bounds)
//
uint16_t Shows::get_scale_from_xy(int8_t x, int8_t y) {
  return get_scale_for_coord(get_coord_from_xy(x, y));
}


uint16_t Shows::get_scale_for_coord(uint16_t coord) {
  if (is_coord_inbounds(coord)) {
    int8_t dx = 0;
    int8_t dy = 0;
    uint16_t dscale = 0;
    int8_t section_width;
    int8_t x = get_x_from_coord(coord);
    int8_t y = get_y_from_coord(coord);

    for (uint8_t section = 0; section < numSections; section++) {
      section_width = get_section_table_value(section, SECT_NUM_COLUMNS_FIELD);
      if (x < dx + section_width) {  // Found the correct section
        int8_t column = x - dx;
        int8_t row = y - dy;
//        Serial.printf("section, column, row (%d, %d, %d)\n", section, column, row);
        int8_t y_start = get_column_table_value(column, Y_START_FIELD, section);
        if (row >= y_start && row < y_start + get_column_table_value(column, COLUMN_HEIGHT_FIELD, section)) {
          uint16_t scale = dscale + (row - y_start) + get_column_table_value(column, COLUMN_PIXEL_START_FIELD, section);
//          Serial.printf("x,y (%d, %d) = (%d, %d) = %d\n", x, y, column, row, scale);
          return scale;
        } else {
          return XX;
        }
      }
      dx += section_width;
      dy += get_section_table_value(section, SECT_Y_OFFSET_FIELD);
      dscale += get_section_table_value(section, SECT_NUM_SCALES_FIELD);
    }
  }
  return XX;  // out of bounds
}

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t Shows::get_distance_to_origin(uint8_t x, uint8_t y) {
  return get_distance_to_coord(x, y, 128, 128);
}

uint8_t Shows::get_distance_to_coord(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  // distance = sqrt(x^2 + y^2)
  // input: 0-255 coordinates
  // output: 0-255 distance
  // dx and dy should be no larger than 180 each
  uint8_t dx = map8(abs(x2 - x1), 0, 100);
  uint8_t dy = map8(abs(y2 - y1), 0, 100);
  return sqrt16((dx * dx) + (dy * dy));
}

//
// Get Coord in Dir - directions go 0 to 5 - does not say whether in bounds!
//
uint16_t Shows::get_coord_in_dir(uint8_t dir, uint16_t coord) {
//  const int8_t dx[] = { 0, 1, 1, 0, -1, -1 };
//  const int8_t dy[] = { 1, 1, 0, -1, 1, 0 };
  const int8_t dx[] = { 0, 1, 1, 0, -1, -1 };
  const int8_t dy[] = { 1, 1, 0, -1, 0, 1 };

  int8_t x = get_x_from_coord(coord);
  int8_t y = get_y_from_coord(coord);

  if (x % 2 && dx[dir % 6]) {  // odd column offsets 1 hex down
    y -= 1;
  }

  x += dx[dir % 6];
  y += dy[dir % 6];

  return get_coord_from_xy(x, y);
}

bool Shows::is_coord_inbounds(uint16_t coord) {
  int8_t x = get_x_from_coord(coord);
  int8_t y = get_y_from_coord(coord);
  return (x >= min_x && x < max_x && y >= min_y && y < max_y);
}

uint16_t Shows::get_coord_col_min_y(int8_t x) {
  for (int8_t y = min_y; y < max_y; y++) {
    if (get_scale_from_xy(x, y) != XX) {
      return get_coord_from_xy(x, y);
    }
  }
  return XX;
}

uint16_t Shows::get_coord_col_max_y(int8_t x) {
  for (int8_t y = max_y; y >= min_y; y--) {
    if (get_scale_from_xy(x, y) != XX) {
      return get_coord_from_xy(x, y);
    }
  }
  return XX;
}

int8_t Shows::get_x_from_coord(uint16_t coord) {
  return lowByte(coord>>8);
}

int8_t Shows::get_y_from_coord(uint16_t coord) {
  return lowByte(coord);
}

uint16_t Shows::get_coord_from_xy(int8_t x, int8_t y) {
  return (lowByte(x)<<8) | lowByte(y);
}

void Shows::calc_coordinate_frame(void)
{
  int8_t dy = 0;
  int8_t temp;  // max is not playing nice with this temp indirection

  for (uint8_t section = 0; section < numSections; section++) {
    max_x += get_section_table_value(section, SECT_NUM_COLUMNS_FIELD);
    temp = dy + get_section_table_value(section, SECT_dX_UP_OFFSET_FIELD);
    max_y = max(max_y, temp);
    temp = dy + get_section_table_value(section, SECT_dX_DOWN_OFFSET_FIELD);
    min_y = min(min_y, temp);

    dy += get_section_table_value(section, SECT_Y_OFFSET_FIELD);
  }
  width_x = max_x - min_x;
  height_y = max_y - min_y;
  aspect = 255 * height_y / width_x;
  // Serial.printf("(%d, %d) to (%d, %d)\n", min_x, min_y, max_x, max_y);
  // Serial.printf("width x height = %d x %d\n", width_x, height_y);
}

void Shows::setMemory(uint16_t value, uint16_t position)
{
  if (position < MAX_MEMORY) {
    memory[position] = value;
  }
}

uint16_t Shows::getMemory(uint16_t position)
{
  if (position < MAX_MEMORY) {
    return memory[position];
  } else {
    return XX;
  }
}
