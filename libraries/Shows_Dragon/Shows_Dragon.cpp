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

#define MAX_MEMORY  300  // 50 bytes per section - 6 sections!

#define UP_SECTION    0
#define DOWN_SECTION  1
#define HEAD_SECTION  2
#define TAIL_SECTION  3

#define MAX_COLUMNS 14  // width of one section
#define MAX_ROWS 12  // eventually 16 with the head and tail

#define SPACER_FIELD 0
#define Y_START_FIELD 1
#define COLUMN_HEIGHT_FIELD 2
#define COLUMN_PIXEL_START_FIELD 3

#define MAX_BALLS    4  // 40 bytes per section
#define TRAIL_SIZE   5  // a ball takes up 10 bytes
#define TRAIL_MEM_SIZE  (1 + (TRAIL_SIZE * 2))

#define XX     255  // Out of bounds (-1)
#define XXXX  9999

//
// Constructor
//
Shows::Shows(Led* led_pointer, uint8_t num_sections)
{
  led = new Led(*led_pointer);
  numLeds = led->getNumLeds();
  numSections = num_sections;

  memory = (uint8_t *)calloc(MAX_MEMORY, sizeof(uint8_t));

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
void Shows::morphFrame(void)
{
  // wrapper for led.morph_frame(). Handles the 2 parameters
  if (has_morphing) {
    led->morph_frame(morph, getNumFrames());
  } else {
    led->morph_frame(getNumFrames(), getNumFrames());
  }
}

void Shows::setColumnTableUp(const uint8_t *column_table_up_map)
{
  column_table_up = column_table_up_map;
}

bool Shows::isInBounds(int8_t x, int8_t y)
{
  if (x < 0 || x > 13) {
    return false;
  }
  int8_t y_start = get_map_value(x, Y_START_FIELD);
  return (y >= y_start && y < (y_start + get_map_value(x, COLUMN_HEIGHT_FIELD)));
}

uint8_t Shows::get_map_value(int8_t column, uint8_t field_index)
{
  return pgm_read_byte_near(column_table_up + ((column % MAX_COLUMNS) * 4) + field_index);
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

uint16_t Shows::get_scale_for_xy(int8_t x, int8_t y) {
  if (x < 0 || y < 0 || x >= MAX_COLUMNS || y >= MAX_ROWS) {
    return XXXX;  // out of bounds
  }
  uint8_t col_pixel_start = get_map_value(x, Y_START_FIELD);
  if (y < col_pixel_start || y >= col_pixel_start + get_map_value(x, COLUMN_HEIGHT_FIELD)) {
    return XXXX;  // out of bounds
  }
  return get_map_value(x, COLUMN_PIXEL_START_FIELD) + (y - col_pixel_start);
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
    setPixeltoForeColor(random16(numLeds - 1));
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
  led->dimAllPixels(10);

  uint16_t pos = cycle % (numLeds*2);  // Where we are in the show
  if (pos >= numLeds) {
    pos = (numLeds * 2) - pos;
  }

  for (uint16_t i=0; i < pos; i++) {
    led->setPixelHue(i, foreColor + (i * (foreColorSpeed / 5)));  // Turning on lights one at a time
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
//    if (wave > 0) {
//      led->setPixelColor(i, CHSV(foreColor, 255, wave) );
//    }

    wave = beatsin8(bands_bpm_2, 0, 255, 0, map(i, 0, numLeds, 0, 255) );
    wave = (wave > band_min_2) ? map(wave, band_min_2, 255, 0, 255) : 0;
    wave2 = CHSV(backColor, 255, wave);

    led->setPixelColor(i, led->getInterpHSV(wave1, wave2, 128));
//    if (wave > 0) {
//      led->addPixelColor(numLeds-i-1, CHSV(backColor, 255, wave) );
//    }
  }
  led->push_frame();  // Prevents unwanted color interpolation
}

/*
void Shows::packets(void)
{
  if (isShowStart()) {
    for (uint8_t i=0; i < MAX_PACKETS; i++) {
      packet_intense[i] = 0;  // Reset all packets
      packet_freq[i] = random(1,2);
    }
  }
  turnOffMorphing();
  led->fillBlack();

  uint8_t wave;

  for (uint8_t i=0; i < MAX_PACKETS; i++) {

    // Adjust each packet either increasing or decreasing
    if (packet_intense[i] == 0) {
      // New packet
      packet_intense[i] = 1;
      packet_freq[i] = random(1,4);
      packet_color[i] += random8(10,40);

    } else if (packet_intense[i] % 2 == 1) {
      // Increasing
      if (packet_intense[i] + (packet_freq[i] * 2) > 253) {
        packet_intense[i] = 252;  // Switch to decreasing
      } else {
        packet_intense[i] += (packet_freq[i] * 2);  // Going up
      }

    } else {
      // Decreasing
      if (packet_intense[i] < packet_freq[i] * 2) {
        packet_intense[i] = (random(0,4) == 1) ? 0 : 1;
      } else {
        packet_intense[i] -= (packet_freq[i] * 2);
      }
    }

    // Apply each packet to the lights
    for (uint16_t j=0; j < numLeds; j++) {
      wave = beatsin8(5, 0, packet_intense[i], 0, map(j, 0, numLeds, cycle, 128 * packet_freq[i]) );
      led->addPixelColor(j, CHSV(packet_color[i], 255, wave) );
    }
  }
  led->push_frame();  // Prevents unwanted color interpolation
}

void Shows::packets_two(void)
{
  if (isShowStart()) {
    for (uint8_t i=0; i < MAX_PACKETS; i++) {
      packet_freq[i] = random(15, 80);
      packet_color[i] = random8(255);
    }
  }
  turnOffMorphing();
  led->fillBlack();

  uint8_t wave;
  uint16_t pixel;

  for (uint8_t i=0; i < MAX_PACKETS; i++) {

    if (random(0, 255) == 1) {
      packet_color[i] += random8(20,50);  // Reset packet
    }

    // Apply each packet to the lights
    for (uint8_t j=0; j < numLeds; j++) {
      wave = beatsin8(packet_freq[i], 0, 255, 0, j);
//      wave = beatsin8(packet_freq[i]);
      pixel =  (packet_freq[i] % 2) ? j : numLeds-j-1;
      led->addPixelColor(pixel, CHSV(packet_color[i], 255, wave) );
    }
  }
  led->push_frame();  // Prevents unwanted color interpolation
}
*/

void Shows::plinko(void)
{
  uint8_t dir, num_choices;
  uint8_t curr_x, curr_y, new_x, new_y;

  // A plinko is 3 bytes: (1 up or 0 down), (x), (y)

  // Refresh plinko at the start of the show
  if (isShowStart()) {
    clearTrails();  // Set memory to XX
    setMemory(random8(5, 11), 0);  // 5-11 is number of plinkos per section
  }

  if (morph > 0) {
    return;
  }

  led->fillBlack();  // Erase all

  for (uint8_t i = 0; i < getMemory(0) * numSections; i++) {  // 30 bytes per section
    dir = getMemory(1 + (i * 3));
    if (dir == XX) {
      if (random8(10) == 1) {  // 1 in 10 chance a dead plinko will wake up
        dir = random8(2);  // 0 or 1
        curr_x = random8(MAX_COLUMNS);  // FIX for multiple sections
        curr_y = get_map_value(curr_x, Y_START_FIELD);
        if (dir == 0) {
          curr_y += get_map_value(curr_x, COLUMN_HEIGHT_FIELD);
        }
        setMemory(dir, 1 + (i * 3) + 0);
        setMemory(curr_x, 1 + (i * 3) + 1);
        setMemory(curr_y, 1 + (i * 3) + 2);
        led->setPixelHue(get_scale_for_xy(curr_x, curr_y), IncColor(foreColor, 5 * i)); // Draw the plinko
      }
    } else {  // plinko is alive
      curr_x = getMemory(1 + (i * 3) + 1);
      curr_y = getMemory(1 + (i * 3) + 2);

      uint8_t directions[] = { 2, 3, 4 };
      if (dir == 1) {
        directions[0] = 5;
        directions[1] = 0;
        directions[2] = 1;
      }
      num_choices = 0;
      int8_t choices[3] = { -1, -1, -1 };

      for (uint8_t dir_choice = 0; dir_choice < 3; dir_choice++) {
        new_x = move_x(curr_x, directions[dir_choice]);
        new_y = move_y(curr_x, curr_y, directions[dir_choice]);
        if (isInBounds(new_x, new_y)) {
          choices[num_choices] = directions[dir_choice];
          num_choices++;
        }
      }
      if (num_choices == 0) {
        setMemory(XX, 1 + (i * 3));;  // nowhere to go; set direction to XX
      } else {
        dir = choices[random(num_choices)];
        new_x = move_x(curr_x, dir);
        new_y = move_y(curr_x, curr_y, dir);
        setMemory(new_x, 1 + (i * 3) + 1);
        setMemory(new_y, 1 + (i * 3) + 2);
        led->setPixelHue(get_scale_for_xy(new_x, new_y), IncColor(foreColor, 5 * i)); // Draw the plinko
      }
    }
  }
}

void Shows::bounce(void)
{
  const uint8_t trail_colors[] = { 195, 145, 85, 45, 5 };
  uint8_t new_x = 0;
  uint8_t new_y = 0;
  uint8_t curr_x, curr_y;

  if (isShowStart()) {
    setUpBalls();
  }

  if (morph != 0) {
    return;
  }

  led->fill(CHSV(170, 255, 40));  // changed color.v from 2 to 40

  // Draw the trail
  for (uint8_t n = 0; n < MAX_BALLS; n++) {
    if (getBallDir(n) == XX) {
      continue;  // Not an active ball
    }
    for (uint8_t i = 0; i < TRAIL_SIZE; i++) {
      curr_x = getTrailX(n, i);
      curr_y = getTrailY(n, i);

      if (curr_x == XX || curr_y == XX) {
        continue;
      }
      led->addPixelColor(get_scale_for_xy(curr_x, curr_y), led->rgb_to_hsv(CRGB(0, 0, trail_colors[i])));
    }

    // Move the ball
    curr_x = getTrailX(n, 0);
    curr_y = getTrailY(n, 0);

    uint8_t max_attempts = 10;

    while (max_attempts-- > 0) {
      new_x = move_x(curr_x, getBallDir(n));
      new_y = move_y(curr_x, curr_y, getBallDir(n));
      if (isInBounds(new_x, new_y)) {
        break;
      } else {
        setBallDir(n, random8(6));
      }
    }

    for (uint8_t i = TRAIL_SIZE - 1; i > 0; i--) {
      setTrailX(n, i, getTrailX(n, i - 1) );
      setTrailY(n, i, getTrailY(n, i - 1) );
    }
    setTrailX(n, 0, new_x);
    setTrailY(n, 0, new_y);
  }
}

void Shows::bounceGlowing(void)
{
  uint8_t new_x, new_2x, new_y, new_2y;
  uint8_t curr_x, curr_y;

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
    if (getBallDir(n) == XX) {
      continue;  // Not an active ball
    }
    curr_x = getTrailX(n, 0);
    curr_y = getTrailY(n, 0);

    // Draw the 2nd ring
    for (uint8_t dir = 0; dir < 6; dir++) {
      new_x = move_x(curr_x, dir);
      new_y = move_y(curr_x, curr_y, dir);

      for (uint8_t dir2 = 0; dir2 < 6; dir2++) {
        new_2x = move_x(new_x, dir2);
        new_2y = move_y(new_x, new_y, dir2);

        if (isInBounds(new_2x, new_2y)) {
          led->addPixelColor(get_scale_for_xy(new_2x, new_2y), color_2);
        }
      }
    }

    // Draw the first ring
    for (uint8_t dir = 0; dir < 6; dir++) {
      new_x = move_x(curr_x, dir);
      new_y = move_y(curr_x, curr_y, dir);
      if (isInBounds(new_x, new_y)) {
        led->addPixelColor(get_scale_for_xy(new_x, new_y), color_1);
      }
    }

    // Draw the center
    if (isInBounds(curr_x, curr_y)) {
      led->addPixelColor(get_scale_for_xy(curr_x, curr_y), color_0);
    }

    // Move the ball
    uint8_t max_attempts = 10;

    while (max_attempts-- > 0) {
      new_x = move_x(curr_x, getBallDir(n));
      new_y = move_y(curr_x, curr_y, getBallDir(n));
      if (isInBounds(new_x, new_y)) {
        setTrailX(n, 0, new_x);
        setTrailY(n, 0, new_y);
        break;
      } else {
        setBallDir(n, random8(6));
      }
    }
  }
}

void Shows::TestNeighbors(void)
{
  led->fillBlack();

  uint16_t scale = getCycle() % numLeds;
  for (uint8_t dir = 0; dir < 6; dir++) {
    setPixeltoHue(get_neighbor(scale, dir), 128);
  }
  setPixeltoHue(scale, 0);
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
// Private Methods //////////////////////////////////////////////////////////////
//

//
// Mapping Functions
//
int8_t Shows::get_x(uint16_t scale)
{
  // Slow sort function, but that's okay
  for (int8_t x = 1; x < MAX_COLUMNS; x++) {
    if (scale < get_map_value(x, COLUMN_PIXEL_START_FIELD)) {
      return x - 1 ;
    }
  }
  return MAX_COLUMNS - 1;
}

int8_t Shows::get_y(uint16_t scale)
{
  int8_t x = get_x(scale);
  return scale - get_map_value(x, COLUMN_PIXEL_START_FIELD) + get_map_value(x, Y_START_FIELD);

}

uint16_t Shows::get_neighbor(uint16_t scale, uint8_t dir) {
  int8_t x = get_x(scale);
  int8_t y = get_y(scale);
  return get_scale_for_xy(move_x(x, dir), move_y(x, y, dir));
}

// Directions: 0 is up
int8_t Shows::move_x(int8_t x, uint8_t dir)
{
  const int8_t x_change[] = { 0, 1, 1, 0, -1, -1 };
  return x + x_change[dir % 6];
}

int8_t Shows::move_y(int8_t x, int8_t y, uint8_t dir)
{
  if (x % 2) {
    const int8_t y_change[] = { 1, 0, -1, -1, -1, 0 };
    return y + y_change[dir % 6];
  } else {
    const int8_t y_change[] = { 1, 1, 0, -1, 0, 1 };
    return y + y_change[dir % 6];
  }
}

//
//   Trails:  (dir) (x1, y1) (x2, y2) (x3, y3)...
//
void Shows::clearTrails(void)
{
  // clear out all the ball trails (set to XX)
  for (uint8_t ball = 0; ball < MAX_BALLS * numSections; ball++) {
    setBallDir(ball, XX);
    for (uint8_t trail = 0; trail < TRAIL_SIZE; trail++) {
      setTrailX(ball, trail, XX);
      setTrailY(ball, trail, XX);
    }
  }
}

void Shows::dropBall(uint8_t ball)
{
  uint8_t x = 0;
  uint8_t y = 0;
  while (true) {
    x = random8(MAX_COLUMNS);
    y = random8(MAX_ROWS);
    if (isInBounds(x, y)) {
      break;
    }
  }
  setTrailX(ball, 0, x);
  setTrailY(ball, 0, y);
  setBallDir(ball, random8(6));
}

uint8_t Shows::getBallDir(uint8_t ball)
{
  return getMemory(ball * TRAIL_MEM_SIZE);
}

void Shows::setBallDir(uint8_t ball, uint8_t dir)
{
  return setMemory(dir, ball * TRAIL_MEM_SIZE);
}

uint8_t Shows::getTrailX(uint8_t ball, uint8_t dist)
{
  return getMemory((ball * TRAIL_MEM_SIZE) + 1 + (dist * 2) + 0);
}

uint8_t Shows::getTrailY(uint8_t ball, uint8_t dist)
{
  return getMemory((ball * TRAIL_MEM_SIZE) + 1 + (dist * 2) + 1);
}

void Shows::setTrailX(uint8_t ball, uint8_t dist, uint8_t value)
{
  setMemory(value, (ball * TRAIL_MEM_SIZE) + 1 + (dist * 2) + 0);
}

void Shows::setTrailY(uint8_t ball, uint8_t dist, uint8_t value)
{
  setMemory(value, (ball * TRAIL_MEM_SIZE) + 1 + (dist * 2) + 1);
}

void Shows::setMemory(uint8_t value, uint16_t position)
{
  if (position < MAX_MEMORY) {
    memory[position] = value;
  }
}

uint8_t Shows::getMemory(uint16_t position)
{
  if (position < MAX_MEMORY) {
    return memory[position];
  } else {
    return XX;
  }
}
