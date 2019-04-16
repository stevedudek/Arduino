//
//  Shows.cpp
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
#include <FastLED.h>
#include <Led.h>
#include "Shows.h"

#define HAS_2D_SHOWS true  // Save memory if you don't need 2D shows
#define USE_SHUFFLE true  // Save memory if you don't need shuffle functions

#define MAX_COLOR  256

#define MAX_PLINKO  10
#define MAX_BALLS    4
#define MAX_PACKETS  3
#define TRAIL_SIZE   5

#define XX 255  // Out of bounds (-1)

//
// Constructor
//
Shows::Shows(Led* led_pointer)
{
  led = new Led(*led_pointer);
  numLeds = led->getNumLeds();

  if (USE_SHUFFLE) {
    shuffle = (uint8_t *)calloc(numLeds, sizeof(uint8_t));  // Uses perhaps too much memory
  }

  // Assign memory for fancy shows - it's not free!
  if (HAS_2D_SHOWS) {
      plink = (uint8_t *)calloc(MAX_PLINKO, sizeof(uint8_t));

      bounce_dir = (uint8_t *)calloc(MAX_BALLS, sizeof(uint8_t));
      bounce_pos = (uint8_t *)calloc(MAX_BALLS, sizeof(uint8_t));
      trail = (uint8_t *)calloc(MAX_BALLS * TRAIL_SIZE, sizeof(uint8_t));
  }

  packet_intense = (uint8_t *)calloc(MAX_PACKETS, sizeof(uint8_t));
  packet_freq = (uint8_t *)calloc(MAX_PACKETS, sizeof(uint8_t));
  packet_color = (uint8_t *)calloc(MAX_PACKETS, sizeof(uint8_t));

  resetAllClocks();

  // color defaults. If desired, override with setter functions below.
  foreColor = 0;  // setForeColor(uint8_t c)
  backColor = MAX_COLOR / 2;  // setBackColor(uint8_t c)
  color_speed_min = 8;  // setColorSpeedMinMax(uint8_t speed_min, uint8_t speed_max)
  color_speed_max = 30;
  foreColorSpeed = color_speed_max / 2;
  backColorSpeed = color_speed_max / 2;

  // wait defaults. If desired, override with setter functions below.
  has_morphing = true;
  has_hex_shape = true;
  has_blur = false;

  wait_min = 4;  // setWaitRange(uint8_t min, uint8_t max, uint8_t values)
  wait_max = 50;
  num_wait_values = 10;
  wait = num_wait_values / 2;  // setWait(uint8_t w)

  // bands defaults. If desired, override with setter functions below.
  bands_min_bpm = 5;  // setBandsBpm(uint8_t bands_min, uint8_t bands_max)
  bands_max_bpm = 60;

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

void Shows::setPixeltoForeColor(uint8_t i)
{
  led->setPixelHue(i, foreColor);
}

void Shows::setPixeltoBackColor(uint8_t i)
{
  led->setPixelHue(i, backColor);
}

void Shows::setPixeltoHue(uint8_t i, uint8_t h)
{
  led->setPixelHue(i, h);
}

void Shows::setPixeltoBlack(uint8_t i)
{
  led->setPixelBlack(i);
}

void Shows::setPixeltoColor(uint8_t i, CHSV color)
{
  led->setPixelColor(i, color);
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

void Shows::resetNumLeds(uint8_t i)
{
  numLeds = i;
}

uint8_t Shows::getNumLeds(void)
{
  return numLeds;
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
  has_hex_shape = false;
  led->setAsSquare();
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

void Shows::shuffleLeds(void)
{
  if (!USE_SHUFFLE) { return; }

  uint8_t i, j, save;

  for (i=0; i < numLeds; i++) {
    shuffle[i] = i; // before shuffle
  }

  for (i=0; i < numLeds; i++) { // here's position
    j = random8(numLeds);         // there's position
    save = shuffle[i];
    shuffle[i] = shuffle[j];       // first swap
    shuffle[j] = save;             // second swap
  }
}

void Shows::shuffleColors(void)
{
  if (!USE_SHUFFLE) { return; }

  for (uint8_t i=0; i < numLeds; i++) {
    shuffle[i] = random(MAX_COLOR);
  }
}

uint8_t Shows::getShuffleValue(uint8_t i)
{
  if (!USE_SHUFFLE) { return 0; }

  return shuffle[i];
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

uint8_t Shows::getWait(void)
{
  return wait;
}

void Shows::setWait(uint8_t w)
{
  wait = w;
}

void Shows::setWaitRange(uint8_t min, uint8_t max, uint8_t values)
{
  wait_min = min;
  wait_max = max;
  num_wait_values = values;
}

void Shows::makeWaitFaster(uint8_t r)
{
  wait /= r;
}

void Shows::makeWaitSlower(uint8_t r)
{
  wait = min(num_wait_values-1, (wait * r));
}

void Shows::tweakWait(void)
{
  wait = up_or_down(wait, 0, num_wait_values);
}

void Shows::pickRandomWait(void)
{
  wait = random(num_wait_values);
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
  if (!USE_SHUFFLE) { return; }

  uint8_t pos = cycle % (numLeds * 2);  // Where we are in the show

  if (pos == 0 && morph == 0) {  // Start of show
    shuffleLeds();
    led->fillBlack();
  }

  if (pos > numLeds) {
    pos = (2 * numLeds) - pos;
  }

  for (uint8_t i=0; i < numLeds; i++) {
    if (i < pos) {
      led->setPixelHue(shuffle[i], foreColor + i);  // Turning on lights one at a time
    } else {
      led->setPixelBlack(shuffle[i]);  // Turning off lights one at a time
    }
  }
}

void Shows::randomColors(void)
{
  if (!USE_SHUFFLE) { return; }

  if (isShowStart()) {  // Start of show: assign lights to random colors
    shuffleColors();
  }
  for (uint8_t i=0; i < numLeds; i++) {
    led->setPixelHue(i, shuffle[i] + cycle);
  }
}

void Shows::twoColor(void)
{
  for (uint8_t i=0; i < numLeds; i++) {
    if (i % 2) {
      led->setPixelHue(i, foreColor);
    } else {
      led->setPixelHue(i, backColor);
    }
  }
}

void Shows::morphChain(void)
{
  uint8_t color;
  for (uint8_t i=0; i < numLeds; i++) {
    color = map8(sin8_C((i+cycle) * foreColorSpeed * 0.5), 0, sin8_C(backColor));
    led->setPixelHue(numLeds-i-1, foreColor + color);
  }
}

void Shows::sinelon_fastled(void)
{
  // a colored dot sweeping back and forth, with fading trails
  led->dimAllPixels(20);  // replaces: fadeToBlackBy( leds, NUM_LEDS, 20);
  uint8_t i = beatsin16(13, 0, numLeds);
  led->setPixelColor(i, CHSV(foreColor, 255, 192));  // 192 = suggested v
}

void Shows::bpm_fastled(void)
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for(uint8_t i=0; i < numLeds; i++) {
    led->setPixelColor(i, led->gradient_wheel(foreColor + (i*2), beat - foreColor + (i*10) ) );
  }
}

void Shows::juggle_fastled(void)
{
  // eight colored dots, weaving in and out of sync with each other
  uint8_t pixel;
  uint8_t dot_hue = 0;
  CHSV curr_color, new_color;

  led->dimAllPixels(20); // replaces: fadeToBlackBy( leds, NUM_LEDS, 20);

  for(uint8_t i=0; i < 8; i++) {
    pixel = beatsin16(i+7, 0, numLeds);
    curr_color = led->getCurrFrameColor(i);
    new_color = CHSV(curr_color.h | dot_hue, curr_color.s | 200, curr_color.v | 255);
    led->setPixelColor(pixel, new_color);
    dot_hue += 32;
  }
}

void Shows::sawTooth(void)
{
  uint8_t intense;
  for (uint8_t i=0; i < numLeds; i++) {
    intense = sin8_C((i + cycle) * foreColorSpeed * 0.5);
    // "i" will have pattern move up; "numLeds-i-1'' will have pattern move down
    led->setPixelColor(numLeds-i-1, led->gradient_wheel(foreColor+i, intense));
  }
}

void Shows::lightWave(void)
{
  if (!has_blur) {
    lightWave_blur(cycle, 10);
  } else {
    lightWave_blur(cycle + 0, 10);
    led->setBlur(128);
    lightWave_blur(cycle - 1, 10);
    lightWave_blur(cycle + 1, 10);
    led->turnOffBlur();
  }
}

void Shows::lightWave(uint8_t spacing)
{
  if (!has_blur) {
    lightWave_blur(cycle, spacing);
  } else {
    lightWave_blur(cycle + 0, spacing);
    led->setBlur(128);
    lightWave_blur(cycle - 1, spacing);
    lightWave_blur(cycle + 1, spacing);
    led->turnOffBlur();
  }
}

void Shows::lightWave_blur(uint16_t cyc, uint8_t spacing)
{
  for (uint8_t i=0; i < numLeds; i++) {
    if ((i + cyc) % spacing == 0) {
      led->setPixelHue(i, foreColor);
    } else {
      led->setPixelBlack(i);
    }
  }
}

void Shows::lightRunUp(void)
{
   if (!has_blur) {
     lightRunUp_blur(cycle);
   } else {
     lightRunUp_blur(cycle + 0);
     led->setBlur(128);
     lightRunUp_blur(cycle - 1);
     lightRunUp_blur(cycle + 1);
     led->turnOffBlur();
   }
}

void Shows::lightRunUp_blur(uint16_t cyc)
{
  uint8_t pos = cyc % (numLeds*2);  // Where we are in the show
  if (pos >= numLeds) {
    pos = (numLeds * 2) - pos;
  }

  for (uint8_t i=0; i < numLeds; i++) {
    if (i < pos) {
      led->setPixelHue(i, foreColor + (i * (foreColorSpeed / 5)));  // Turning on lights one at a time
    } else {
      led->setPixelBlack(i);
    }
  }
}

void Shows::bands(void)
{
  if (isShowStart()) {
    bands_bpm_1 = random(bands_min_bpm, bands_max_bpm);
    bands_bpm_2 = random(bands_min_bpm, bands_max_bpm);
    band_min_1 = random(64, 192);
    band_min_2 = random(64, 192);
    turnOffMorphing();
  }

  led->fillBlack();

  uint8_t wave;

  for (uint8_t i=0; i < numLeds; i++) {

    wave = beatsin8(bands_bpm_1, 0, 255, 0, map(i, 0, numLeds, 0, 255) );
    wave = (wave > band_min_1) ? map(wave, band_min_1, 255, 0, 255) : 0;
    if (wave > 0) {
      led->setPixelColor(i, CHSV(foreColor, 255, wave) );
    }

    wave = beatsin8(bands_bpm_2, 0, 255, 0, map(i, 0, numLeds, 0, 255) );
    wave = (wave > band_min_2) ? map(wave, band_min_2, 255, 0, 255) : 0;
    if (wave > 0) {
      led->addPixelColor(numLeds-i-1, CHSV(backColor, 255, wave) );
    }
  }
  led->push_frame();  // Prevents unwanted color interpolation
}

void Shows::packets(void)
{
  if (isShowStart()) {
    for (uint8_t i=0; i < MAX_PACKETS; i++) {
      packet_intense[i] = 0;  // Reset all packets
      packet_freq[i] = random(1,4);
    }
    turnOffMorphing();
  }

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
    for (uint8_t j=0; j < numLeds; j++) {
      wave = beatsin8(10, 0, packet_intense[i], 0, map(j, 0, numLeds, cycle, 128 * packet_freq[i]) );
      led->addPixelColor(j, CHSV(packet_color[i], 255, wave) );
    }
  }
  led->push_frame();  // Prevents unwanted color interpolation
}

void Shows::packets_two(void)
{
  if (isShowStart()) {
    for (uint8_t i=0; i < MAX_PACKETS; i++) {
      packet_freq[i] = random(15,40);
      packet_color[i] = random8(255);
      packet_intense[i] = random(150, 220);
    }
    turnOffMorphing();
  }

  led->fillBlack();

  uint8_t wave, pixel;

  for (uint8_t i=0; i < MAX_PACKETS; i++) {

    if (random(0, 256) == 1) {
      // Reset packet
      packet_color[i] += random8(20,50);
      packet_intense[i] = random(20,255);
    }

    // Apply each packet to the lights
    for (uint8_t j=0; j < numLeds; j++) {
      wave = beatsin8(packet_freq[i], 0, 255, 0, map(j, 0, numLeds, cycle, packet_freq[i]) );
      pixel =  (packet_freq[i] % 2) ? j : numLeds-j-1;
      led->addPixelColor(pixel, CHSV(packet_color[i], 255, wave) );
    }
  }
  led->push_frame();  // Prevents unwanted color interpolation
}

void Shows::plinko(uint8_t start_pos)
{
  if (!HAS_2D_SHOWS) { return; }  // Can't run show with HAS_2D_SHOWS = false

  uint8_t i, num_choices;
  uint8_t pos, new_pos;
  uint8_t dir_choices[4] = { 2, 3, 4, XX };

  // Refresh plinkos at the start of the show
  if (isShowStart()) {
    for (i = 0; i < MAX_PLINKO; i++) {
      plink[i] = XX;  // Move plinkos off the board
    }
    num_plinko = random(1, MAX_PLINKO);
    wait /= 2;  // make a lot faster
  }

  if (morph > 0) {
    return;
  }

  // Move plinko
  uint8_t choices[3] = { XX, XX, XX };  // For 2 and 3 directional plinko

  if (!has_hex_shape) {
    dir_choices[0] = 0;
    dir_choices[1] = 1;
    dir_choices[2] = 3;
  }

  for (i = 0; i < num_plinko; i++) {
    pos = plink[i];

    if (pos != XX) {  // is on the board?
      num_choices = 0;  // yes, on board. Itemize valid next positions.

      for (uint8_t dir_choice = 0; dir_choice < 4; dir_choice++) {
        uint8_t dir = dir_choices[dir_choice];
        if (dir == XX) {
          continue;
        }
        new_pos = led->getNeighbor(pos, dir);
        if (new_pos != XX) {
          choices[num_choices] = new_pos;
          num_choices++;
        }
      }

      if (num_choices <= 1) {
        plink[i] = XX;  // nowhere to go; put off board
      } else {
        plink[i] = choices[random(num_choices)];
      }
    } else {  // off board. Restart?
      if (random(0,3) == 0) {  // Restart 1 in 3 times
        plink[i] = start_pos;   // Start at start_pos
      }
    }
  }

  // Draw existing plinko
  led->fillBlack();  // Erase all

  for (i = 0; i < num_plinko; i++) {
    led->setPixelHueNoMap(plink[i], IncColor(foreColor, 5 * i)); // Draw the plinko
  }
}

void Shows::bounce(void)
{
  if (!HAS_2D_SHOWS) { return; }  // Can't run show with HAS_2D_SHOWS = false

  const uint8_t trail_colors[] = { 205, 145, 85, 45, 5 };
  uint8_t curr_pos;

  if (isShowStart()) {
    num_balls = random(1, MAX_BALLS);
    wait /= 2;  // make a lot faster
    clearTrails();
  }

  if (morph != 0) {
    return;
  }

  led->fill(CHSV(170, 255, 40));  // changed color.v from 2 to 40

  for (uint8_t n = 0; n < num_balls; n++) {
    for (uint8_t i = 0; i < TRAIL_SIZE; i++) {
      curr_pos = getTrail(n,i);

      if (curr_pos == XX) {
        continue;
      }
      led->addPixelColorNoMap(curr_pos, led->rgb_to_hsv(CRGB(0, 0, trail_colors[i])));
    }
    if (bounce_pos[n] != XX) {
      led->addPixelColorNoMap(bounce_pos[n], led->rgb_to_hsv(CRGB(200, 0, 128)));
    }

    uint8_t old_dir = bounce_dir[n];

    if (has_hex_shape) {
      while (led->getNeighbor(bounce_pos[n], bounce_dir[n]) == XX || (bounce_dir[n] + 3) % 6 == old_dir) {
        bounce_dir[n] = random(0,6);  // HexGrid
      }
    } else {
      while (led->getNeighbor(bounce_pos[n], bounce_dir[n]) == XX || (bounce_dir[n] + 2) % 4 == old_dir || random(0,5) == 0) {
        bounce_dir[n] = random(0,6);  // SquareGrid
      }
    }
    for (int i = TRAIL_SIZE - 1; i >= 0; i--) {
      setTrail(n, i, getTrail(n, i - 1) );
    }
    setTrail(n, 0, bounce_pos[n]);
    bounce_pos[n] = led->getNeighbor(bounce_pos[n], bounce_dir[n]);
  }
}

void Shows::bounceGlowing(void)
{
  if (!HAS_2D_SHOWS) { return; }  // Can't run show with HAS_2D_SHOWS = false

  const uint8_t glow_colors[] = { 255, 105, 55 };

  if (isShowStart()) {
    num_balls = random(1, MAX_BALLS);
    wait /= 2;  // make a lot faster
    clearTrails();
  }

  if (morph != 0) {
    return;
  }

  led->fill(led->rgb_to_hsv(CRGB(0, 40, 40)));  // changed g,b=2 to g,b=40

  for (int n = 0; n < num_balls; n++) {
    for (int i = 0; i < 4; i++) {
      int x = led->getNeighbor(bounce_pos[n], i);
      if (x == XX) continue;
      for (int j = 0; j < 4; j++) {
        int xx = led->getNeighbor(x, j);
        if (xx == XX) continue;
        led->addPixelColorNoMap(xx, led->rgb_to_hsv(CRGB(0, glow_colors[2], glow_colors[2])));
      }
    }
    for (int i = 0; i < 4; i++) {
      int x = led->getNeighbor(bounce_pos[n], i);
      if (x == XX) continue;
      led->addPixelColorNoMap(x, led->rgb_to_hsv(CRGB(0, glow_colors[1], glow_colors[1])));
    }
    led->addPixelColorNoMap(bounce_pos[n], led->rgb_to_hsv(CRGB(0, glow_colors[0], glow_colors[0])));

    int old_dir = bounce_dir[n];

    if (has_hex_shape) {
      while (led->getNeighbor(bounce_pos[n], bounce_dir[n]) == XX || (bounce_dir[n] + 3) % 6 == old_dir) {
        bounce_dir[n] = random(0,6);  // HexGrid
      }
    } else {
      while (led->getNeighbor(bounce_pos[n], bounce_dir[n]) == XX || (bounce_dir[n] + 2) % 4 == old_dir || random(0,6) == 0) {
        bounce_dir[n] = random(0,6);  // SquareGrid
      }
    }
    bounce_pos[n] = led->getNeighbor(bounce_pos[n], bounce_dir[n]);
  }
}


//
// Private Methods //////////////////////////////////////////////////////////////
//
void Shows::clearTrails(void)
{
  // clear out all the ball trails (set to XX)
  for (uint8_t x = 0; x < MAX_BALLS; x++) {
    for (uint8_t y = 0; y < TRAIL_SIZE; y++) {
      setTrail(x, y, XX);
    }
  }
}

uint8_t Shows::getTrail(uint8_t trail_num, uint8_t dist)
{
  return trail[(trail_num * TRAIL_SIZE) + dist];
}

void Shows::setTrail(uint8_t trail_num, uint8_t dist, uint8_t value)
{
  trail[(trail_num * TRAIL_SIZE) + dist] = value;
}