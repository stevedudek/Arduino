//
//  Shows_Modern.cpp
//
//  This library is for LED show creation
//
//  Modernized clock cycling
//     no interp frame
//  Do not use for Dragons or Owls
//
//  Client responsibilities:
//     The show runner (usually a switch statement against current_show)
//     morph_frame()
//     fading the lights at the beginning and end of shows (usually a check_fades() function)
//     Implementation of novel shows addition to this library
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
#include <Led_Modern.h>
#include "Shows_Modern.h"

#define HAS_2D_SHOWS true  // Save memory if you don't need 2D shows

#define MAX_COLOR  256

#define MEMORY     100   // allocate MEMORY bytes

#define MAX_PLINKO  15
#define MAX_BALLS    4
#define MAX_PACKETS  3

#define XX     255  // Out of bounds — be careful about 8-bit vs. 16-bit XX
#define XXXX   900  // Out of bounds — be careful about 8-bit vs. 16-bit XX

//
// Constructor
//
Shows::Shows(Led* led_pointer, uint8_t c)
{
  channel = c;

  led = new Led(*led_pointer);
  numLeds = led->getNumLeds();

  memory = (uint8_t *)calloc(MEMORY, sizeof(uint8_t));

  resetAllClocks();
  setColorSpeedMinMax();
}

//
// Public Methods //////////////////////////////////////////////////////////////
//
void Shows::morphFrame(void)
{
  if (has_morphing) {
    // With cycle animation, slow morphing to occur across 1 cycle = X cycles_per_frame
    led->smooth_frame(255 / cycles_per_frame);
  } else {
    led->push_frame();  // Otherwise, push next -> current
  }
}

void Shows::updateFrameClock(void)
{
  start_frame_time = millis();
}

uint16_t Shows::delta(void)
{
  return millis() - start_frame_time;  // get milliseconds since last render
}

uint16_t Shows::getElapsedCycleTime(void)
{
  return millis() - start_cycle_time;
}

uint32_t Shows::getElapsedShowTime(void)
{
  return millis() - start_show_time;
}

void Shows::setElapsedShowTime(uint32_t elapsed_time)
{
  start_show_time = millis() + elapsed_time;
}

void Shows::dimAllPixels(uint8_t dim_amount) {
  led->dimAllPixels(dim_amount);
}

void Shows::dimAllPixelsFrames(uint8_t frames) {
  // Dim all pixels by an amount that trails x frames
  uint16_t dim_frames = cycles_per_frame * frames;
  if (dim_frames > 255) {
    if (getSmallCycle() % (1 + (dim_frames / 255)) == 0) {
      led->dimAllPixels(1);
    }
  } else {
    led->dimAllPixels(1 + (255 / dim_frames));
  }
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t Shows::get_intensity() {

  uint8_t intensity;  // 0 = Off, 255 = full On
  uint32_t elapsed_show_time = getElapsedShowTime();
  uint32_t fade_time = fade_amount * show_duration * 500 / 255;

  if (elapsed_show_time < fade_time) {
    intensity = 255 * elapsed_show_time / fade_time;  // rise
  } else if (elapsed_show_time <= (show_duration * 500)) {
    intensity = 255;  // Show is 100%
  } else if (elapsed_show_time <= ((show_duration * 500) + fade_time)) {
    intensity = 255 - (255 * (elapsed_show_time - (show_duration * 500)) / fade_time);
  } else {
    intensity = 0;
  }
  return ease8InOutApprox(intensity);
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

void Shows::addPixelColor(uint16_t i, CHSV color)
{
  led->addPixelColor(i, color);
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

void Shows::setSmallCycleHalfway(void)
{
  start_show_time = millis() - (show_duration * 1000 / 2);
}

void Shows::resetNumLeds(uint16_t i)
{
  numLeds = i;
}

uint16_t Shows::getNumLeds(void)
{
  return numLeds;
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
  return (millis() - start_show_time) / delay_time;
}

void Shows::setStartShowTime(uint32_t c)
{
  start_show_time = c;
}

void Shows::setShowSpeed(uint8_t spd)
{
  show_speed = spd;  // 0-255
}

void Shows::setShowDuration(uint8_t duration)
{
  show_duration = duration;  // seconds of show time
}

void Shows::setDelayTime(uint8_t delay)
{
  delay_time = delay;  // milliseconds loop time
}

void Shows::setFadeAmount(uint8_t fade)
{
  fade_amount = fade;  // 0 = no fading, to 255 = always be fading
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

void Shows::checkCycleClock(void)
{
  updateFrameClock();  // Mark the render time
  uint32_t smallCycle = getSmallCycle();

  // Automatically check whether fore and back colors should increment
  if (smallCycle % foreColorSpeed == 0) {
    IncForeColor(1);
  }
  if (smallCycle % backColorSpeed == 0) {
    IncBackColor(1);
  }

  is_cycle_start = false;
  while (getElapsedCycleTime() > cycle_duration) {
    start_cycle_time += cycle_duration;
    cycle++;
    is_cycle_start = true;
  }
}

bool Shows::hasShowFinished(void)
{
  return millis() > (start_show_time + (show_duration * 1000));
}

void Shows::resetAllClocks(void)
{
   is_show_start = true;
   is_cycle_start = true;
   cycle = 0;
   start_show_time = millis();
   start_cycle_time = millis();
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

void Shows::setAsSquare(void)
{
  num_neighbors = 4;
  led->setAsSquare();
}

void Shows::setAsPentagon(void)
{
  num_neighbors = 5;
  led->setAsPentagon();
}

void Shows::setAsHexagon(void)
{
  num_neighbors = 6;
  led->setAsHexagon();
}

void Shows::pickRandomColorSpeeds(void)
{
  foreColorSpeed = random(color_speed_min, color_speed_max);
  backColorSpeed = random(color_speed_min, color_speed_max);
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

void Shows::setColorSpeedMinMax()
{
  color_speed_min = map8(show_speed, 4, 20);
  color_speed_max = map8(show_speed, 20, 200);
}

void Shows::pickCycleDuration(void)
{
  // This is generally ignored. Each show has its own ranges.
  // show speed 0: 50–1070
  // show speed 255: 560–1580
  cycle_duration = 50 + (show_speed * 2) + (random8() * 4);
  setCyclePerFrame();
}

void Shows::pickRandomCycleDuration(uint16_t min_duration, uint16_t max_duration)
{
  // durations are milliseconds for one cycle
  int8_t random_adj = random8(20 * 2) - 20;  // ± this value
  uint8_t random_speed = show_speed;
  if (random_speed + random_adj > 255) {
    random_speed = 255;
  } else if (random_speed - random_adj < 0) {
    random_speed = 0;
  } else {
    random_speed += random_adj;
  }

  cycle_duration = min_duration + scale16by8(max_duration - min_duration, random_speed);
  setCyclePerFrame();
}

void Shows::setCyclePerFrame(void)
{
  cycles_per_frame = cycle_duration / delay_time;  // Number of animation cycles per frame
}

uint8_t Shows::getCyclesPerFrame(void)
{
  return cycles_per_frame;
}

uint16_t Shows::getCycleDuration(void)
{
  return cycle_duration;
}

void Shows::setCycleDuration(uint16_t cd)
{
  cycle_duration = max(uint16_t(delay_time), cd);  // Must be ≥ 1 frame long
  setCyclePerFrame();
}

bool Shows::isShowStart(void)
{
  // Forced to be true only once
  if (is_show_start) {
    is_show_start = false;
    return true;
  } else {
    return false;
  }
}

bool Shows::isCycleStart(void)
{
  // Forced to be true only once
  if (is_cycle_start) {
    is_cycle_start = false;
    return true;
  } else {
    return false;
  }
}

//
// Shows - all should return void; use led->func notation
//
void Shows::allOn(void)
{
  if (isShowStart()) {
    turnOnMorphing();
    pickRandomCycleDuration(50, 300);
  }
  if (isCycleStart()) {
    fillForeColor();
  }
}

void Shows::randomFlip(void)
{
  if (isShowStart()) {
    turnOnMorphing();
    uint8_t fast_change = map8(min(uint16_t(255), numLeds), 100, 20);
    pickRandomCycleDuration(fast_change, fast_change * 5);

    for (uint16_t i = 0; i < numLeds; i++) {
      if (random8(2) == 0) {
        setPixeltoForeBlack(i);
      } else {
        setPixeltoForeColor(i);
      }
    }
  }

  if (isCycleStart()) {
    uint16_t pixel = random(numLeds);
    if (led->getNextFrameColor(pixel).v == 0) {
      setPixeltoForeColor(pixel);
    } else {
      setPixeltoForeBlack(pixel);
    }
  }
}

void Shows::randomColors(void)
{
  if (isShowStart()) {  // Start of show: assign lights to random colors
    turnOffMorphing();
    for (uint16_t i = 0; i < numLeds; i++) {
      setPixeltoHue(i, random8());
    }
  }
  if (backColorSpeed != 0 && getSmallCycle() % backColorSpeed == 0) {
    for (uint16_t i = 0; i < numLeds; i++) {
      led->increasePixelHue(i, 1);
    }
  }
}

void Shows::randomOneColorBlack(void)
{
  if (isShowStart()) {  // Start of show: assign lights to random colors
    turnOffMorphing();
    for (uint16_t i = 0; i < numLeds; i++) {
      if(random8(2) == 0) {
        setPixeltoForeColor(i);
      } else {
        setPixeltoForeBlack(i);
      }
    }
  }
  if (backColorSpeed != 0 && getSmallCycle() % backColorSpeed == 0) {
    for (uint16_t i = 0; i < numLeds; i++) {
      led->increasePixelHue(i, 1);
    }
  }
}

void Shows::randomTwoColorBlack(void)
{
  if (isShowStart()) {  // Start of show: assign lights to random colors
    turnOffMorphing();
    for (uint16_t i = 0; i < numLeds; i++) {
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
  if (backColorSpeed != 0 && getSmallCycle() % backColorSpeed == 0) {
    for (uint16_t i = 0; i < numLeds; i++) {
      led->increasePixelHue(i, 1);
    }
  }
}

void Shows::twoColor(void)
{
  if (isShowStart()) {  // Start of show: assign lights to random colors
    turnOffMorphing();
    for (uint16_t i = 0; i < numLeds; i++) {
      if(random8(2) == 0) {
        setPixeltoForeColor(i);
      } else {
        setPixeltoBackColor(i);
      }
    }
  }
  if (backColorSpeed != 0 && getSmallCycle() % backColorSpeed == 0) {
    for (uint16_t i = 0; i < numLeds; i++) {
      led->increasePixelHue(i, 1);
    }
  }
}

void Shows::stripes(void)
{
  if (isShowStart()) {  // Start of show: assign lights to random colors
    turnOnMorphing();
    setMemory(random(2, 6) , 0);
  }
  if (isCycleStart()) {
    uint8_t stripe_width = getMemory(0);
    for (uint16_t i = 0; i < numLeds; i++) {
      if((i % stripe_width) % 2) {
        setPixeltoForeColor(i);
      } else {
        setPixeltoBackColor(i);
      }
    }
  }
}

void Shows::morphChain(void)
{
  if (isShowStart()) {
    turnOnMorphing();  // Need this because cycle is used
    pickRandomCycleDuration(40, 400);
  }

  if (isCycleStart()) {
    for (uint16_t i = 0; i < numLeds; i++) {
      uint8_t color_add = sin8_C((i / map8(min(numLeds, uint16_t(255)), 1, 10)) + cycle);
      setPixeltoHue(i, foreColor + color_add);
    }
  }
}

void Shows::confetti(void)
{
  if (isShowStart()) {
    turnOnMorphing();  // Cycle starts a confetti pixel
    uint8_t fast_change = map8(min(numLeds, uint16_t(255)), delay_time * 5, delay_time);
    pickRandomCycleDuration(fast_change, fast_change * 5);
  }
  dimAllPixels(1);  // Number = trailing frames

  if (isCycleStart()) {
    for (uint16_t i = 0; i < 1 + (numLeds / 20); i++) {
      setPixeltoForeColor(random8(numLeds));
    }
  }
}

void Shows::sinelon_fastled(void)
{
  if (isShowStart()) {
    turnOffMorphing();
  }
  // a colored dot sweeping back and forth, with fading trails
  dimAllPixels(2);  // replaces: fadeToBlackBy( leds, NUM_LEDS, 20);
  uint8_t i = beatsin16(8, 0, numLeds);  // lower first number = slower
  setPixeltoColor(i, CHSV(foreColor, 255, 192));  // 192 = suggested v
}

void Shows::bpm_fastled(void)
{
  if (isShowStart()) {
    turnOffMorphing();
  }
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM))
  uint8_t BeatsPerMinute = 1 + ((255 - show_speed) / 10);
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for(uint16_t i = 0; i < numLeds; i++) {
    setPixeltoColor(i, led->gradient_wheel(foreColor + beat + (i / 10), beat - backColor + (i*2) ) );
  }
}

void Shows::juggle_fastled(void)
{
  if (isShowStart()) {
    turnOffMorphing();
  }
  dimAllPixels(4); // replaces: fadeToBlackBy( leds, NUM_LEDS, 20);

  // eight colored dots, weaving in and out of sync with each other
  uint8_t dot_hue = 0;

  for(uint8_t i = 0; i < 8; i++) {
    uint16_t pixel = beatsin16(i, 0, numLeds);
    CHSV curr_color = led->getCurrFrameColor(i);
    CHSV new_color = CHSV(curr_color.h | dot_hue, curr_color.s | 200, curr_color.v | 255);
    setPixeltoColor(pixel, new_color);
    dot_hue += 32;
  }
}

float Shows::time_sawtooth(uint16_t time_window) {
  // .015 for approximately 1 second. 15 ms frames = 67 fps.
  time_window = max(uint16_t(1), time_window);  // Prevent mod % 0 errors
  float value = (getElapsedShowTime() % (time_window * 2)) / time_window;
//  float value = (delta() % (time_window * 2)) / time_window;
  if (value > 1.0) {
    value = 2.0 - value;
  }
  return value;
}

float Shows::clamp(float value, float min_value, float max_value) {
  return max(min(value, max_value), min_value);
}


void Shows::sawTooth(void)
{
  if (isShowStart()) {
    turnOnMorphing();
    pickRandomCycleDuration(30, 300);
  }
  if (isCycleStart()) {
    for (uint16_t i = 0; i < numLeds; i++) {
      uint8_t intense = sin8_C(((cycle + i) % numLeds) * 255 / min(numLeds, uint16_t(255)));
      setPixeltoColor(numLeds - i - 1, led->gradient_wheel(foreColor+i, intense));
    }
  }
}

void Shows::lightWave(void)
{
  if (isShowStart()) {
    turnOnMorphing();
    pickRandomCycleDuration(30, 90);
  }

  dimAllPixels(1 + ((101 - cycle_duration) / 10));  // Number = trailing frames

  if (isCycleStart()) {
    setPixeltoForeColor(cycle % numLeds);
  }
}

void Shows::lightRunUp(void)
{
  if (isShowStart()) {
    turnOnMorphing();
    pickRandomCycleDuration(30, 200);
  }

  dimAllPixels(1 + ((101 - cycle_duration) / 10));  // Number = trailing frames

  uint16_t pos = cycle % (numLeds * 2);  // Where we are in the show
  if (pos >= numLeds) {
    pos = (numLeds * 2) - pos;
  }

  for (uint16_t i = 0; i < pos; i++) {
    // interpolation kinda handled by main smoothing function
    setPixeltoHue(i, foreColor + i);  // Turning on lights one at a time
  }
}

void Shows::plinko(uint8_t start_pos)
{
  if (!HAS_2D_SHOWS) { return; }  // Can't run show with HAS_2D_SHOWS = false

  uint8_t i, num_choices;
  uint8_t pos, new_pos;
  uint8_t dir_choices[4] = { 2, 3, XX, XX };
//  uint8_t dir_choices[4] = { 2, 3, 4, XX };

  // Refresh plinkos at the start of the show
  if (isShowStart()) {
    turnOnMorphing();
    pickRandomCycleDuration(40, 290);  // ToDo: Find the min/max
    setNumBalls(random(3, MAX_PLINKO));
    for (i = 0; i < MAX_PLINKO; i++) {
      setBallPos(i, XX);  // Move plinkos off the board
    }
  }

  dimAllPixels((300 - cycle_duration) / 10);  // Number = trailing frames

  if (!isCycleStart()) { return; }  // Only move at the start of a cycle

  for (i = 0; i < getNumBalls(); i++) {
    led->setPixelHueNoMap(getBallPos(i), IncColor(foreColor, 5 * i)); // Draw the plinko
  }

  // Move plinko
  uint8_t choices[3] = { XX, XX, XX };  // For 2 and 3 directional plinko

  if (num_neighbors == 4) {
    dir_choices[0] = 0;
    dir_choices[1] = 1;
    dir_choices[2] = 3;
  }

  if (num_neighbors == 5) {
    dir_choices[0] = XX;
    dir_choices[1] = 3;
    dir_choices[2] = 4;
  }

  for (i = 0; i < getNumBalls(); i++) {
    pos = getBallPos(i);

    if (pos != XX) {  // is on the board?
      num_choices = 0;  // yes, on board. Itemize valid next positions.

      for (uint8_t dir_choice = 0; dir_choice < 4; dir_choice++) {
        uint8_t dir = dir_choices[dir_choice];
        if (dir == XX) { continue; }
        new_pos = led->getNeighbor(pos, dir);
        if (new_pos != XX) {
          choices[num_choices] = new_pos;
          num_choices++;
        }
      }

      if (num_choices <= 1) {
        setBallPos(i, XX);  // nowhere to go; put off board
      } else {
        setBallPos(i, choices[random(num_choices)]);
      }
    } else {  // off board. Restart?
      if (random(0,3) == 0) {  // Restart 1 in 3 times
        setBallPos(i, start_pos);   // Start at start_pos
      }
    }
  }
}

void Shows::bounce(void)
{
  if (!HAS_2D_SHOWS) { return; }  // Can't run show with HAS_2D_SHOWS = false

  if (isShowStart()) {
    turnOnMorphing();
    pickRandomCycleDuration(30, 100);
    setNumBalls(random(2, MAX_BALLS));
  }

  dimAllPixels((300 - cycle_duration) / 20);  // Number = trailing frames

  if (!isCycleStart()) { return; }  // Only move at the start of a cycle

//  led->fill(CHSV(170, 255, 40));  // changed color.v from 2 to 40

  // Print the balls
  for (uint8_t n = 0; n < getNumBalls(); n++) {
    if (getBallPos(n) != XX) {
      setPixeltoForeColor(getBallPos(n));
    }
  }

  for (uint8_t n = 0; n < getNumBalls(); n++) {
    uint8_t max_attempts = 10;

    while (led->getNeighbor(getBallPos(n), getBallDir(n)) == XX || random(0, num_neighbors * 3) == 0) {
  //          (bounce_dir[n] + (num_neighbors / 2)) % num_neighbors == old_dir ||
      setBallDir(n, random(0, num_neighbors));  // HexGrid
      if (max_attempts-- == 0) {
        break;
      }
    }
    setBallPos(n, led->getNeighbor(getBallPos(n), getBallDir(n)));
  }
}

void Shows::bounceGlowing(void)
{
  if (!HAS_2D_SHOWS) { return; }  // Can't run show with HAS_2D_SHOWS = false

  const uint8_t glow_colors[] = { 255, 128, 48 };

  if (isShowStart()) {
    turnOnMorphing();
    pickRandomCycleDuration(30, 100);
    setNumBalls(random(2, MAX_BALLS));
  }

//  dimAllPixels(20);  // a lot!

  if (!isCycleStart()) { return; }  // Only move at the start of a cycle

  led->fill(CHSV(170, 255, 2));  // changed color.v from 2 to 40

  uint8_t hue = getForeColor();

  // Print the balls
  for (uint8_t n = 0; n < getNumBalls(); n++) {

    // Dimmest outer ring
    for (uint8_t i = 0; i < num_neighbors; i++) {
      uint8_t x = led->getNeighbor(getBallPos(n), i);
      if (x != XX) {
        for (uint8_t j = 0; j < num_neighbors; j++) {
          uint8_t y = led->getNeighbor(x, j);
          if (y != XX) {
            led->addPixelColor(y, CHSV(hue + 20, 255, glow_colors[2]));
          }
        }
      }
    }

    // Middle ring
    for (uint8_t i = 0; i < num_neighbors; i++) {
      uint8_t x = led->getNeighbor(getBallPos(n), i);
      if (x != XX) {
        led->addPixelColor(x, CHSV(hue + 10, 255, glow_colors[1]));
      }
    }

    // Center spot
    led->addPixelColor(getBallPos(n), CHSV(hue, 255, glow_colors[0]));
  }

  for (uint8_t n = 0; n < getNumBalls(); n++) {
    uint8_t max_attempts = 20;

    while (led->getNeighbor(getBallPos(n), getBallDir(n)) == XX || random(0, num_neighbors * 3) == 0) {
      setBallDir(n, random(0, num_neighbors));
      if (max_attempts-- == 0) {
        break;
      }
    }
    setBallPos(n, led->getNeighbor(getBallPos(n), getBallDir(n)));
  }
}

uint8_t Shows::getBallDir(uint8_t ball)
{
  return getMemory(1 + (ball * 2));  // dir is first position
}

void Shows::setBallDir(uint8_t ball, uint8_t dir)
{
  return setMemory(dir, 1 + (ball * 2));  // dir is first position
}

uint8_t Shows::getBallPos(uint8_t ball)
{
  return getMemory(1 + (ball * 2) + 1);  // pos is second position
}

void Shows::setBallPos(uint8_t ball, uint8_t pos)
{
  setMemory(pos, 1 + (ball * 2) + 1);  // pos is second position
}

uint8_t Shows::getNumBalls(void)
{
  return getMemory(0);  // number of object is index 0
}

void Shows::setNumBalls(uint8_t i)
{
  setMemory(i, 0);  // number of object is index 0
}

void Shows::setMemory(uint8_t value, uint8_t position)
{
  if (position < numLeds * MEMORY) {
    memory[position] = value;
  }
}

uint8_t Shows::getMemory(uint8_t position)
{
  if (position < numLeds * MEMORY) {
    return memory[position];
  } else {
    return XX;
  }
}