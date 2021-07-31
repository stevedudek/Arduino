#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>

//
//  Sphere
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  10/27/2019
//

#define NUM_LEDS 144
#define NUM_FACES 12
#define NUM_SPOKES 10

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 40;  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
uint32_t timing = millis();  // last_time
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 8  // Check this - might be 7
#define CLOCK_PIN 7  // Check this - might be 8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // The Leds themselves
CHSV led_buffer[NUM_LEDS];  // For smoothing

// Shows
#define START_SHOW_CHANNEL_A  10  // Channels A starting show
#define START_SHOW_CHANNEL_B  0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 33

// ArduinoBlue
#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;
#define MAX_COLOR 256   // Colors are 0-255
#define WHITE  CHSV(0, 255, 255)
#define BLACK  CHSV(0, 255, 0)

// Clocks and time
#define SHOW_DURATION 30  // seconds
uint8_t FADE_TIME = 5;  // seconds to fade in + out (Arduino Blue)
#define MAX_SMALL_CYCLE  (SHOW_DURATION * 2 * (1000 / DELAY_TIME))  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// Spherical Effect
uint8_t show_freq;
uint8_t center_pixel;
float rotation_speed;

// ArduinoBlue
ArduinoBlue phone(Serial2); // Blue Tx = pin 9; Blue Rx = pin 10
#define HUE_SLIDER        0
#define HUE_WIDTH_SLIDER  1
#define SPEED_SLIDER      2
#define BRIGHTNESS_SLIDER 3
#define FADE_TIME_SLIDER  4
#define BAM_BUTTON        0
#define BOLT_TIME        20

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");
  Serial2.begin(9600);  // Serial2: Bluetooth serial (ArduinoBlue)

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);  // Only 1 leds object
  FastLED.setBrightness( BRIGHTNESS );

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show
//  shows[CHANNEL_A].setWaitRange(15, 50, 35);  // Slow down Channel A
  shows[CHANNEL_A].setWaitRange(50, 250, 50);  // Slow down Channel A
  shows[CHANNEL_B].setWaitRange(12, 150, 35);  // Slow down Channel B
  
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();
  }
  
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);
  timing = millis();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 255, 0);
  }

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_start = 192;
    hue_width = 124;
  }
}

void loop() {

  for (uint8_t i = 0; i < DUAL; i++) {

    switch (current_show[i]) {
    
      case 0:
        color_band(i);
        break;
      case 1:
        dark_color_band(i);
        break;
      case 2:
        two_color_band(i);
        break;
      case 3:
        hue_band(i);
        break;
      case 4:
        propellor(i);
        break;
      case 5:
        propellor_two_color(i);
        break;
      case 6:
        fan(i);
        break;
      case 7:
        fan_two_color(i);
        break;
      case 8:
        hub_two_color(i);
        break;
      case 9:
        hub_only(i);
        break;
      case 10:
        hub_pulse_two_color(i);
        break;
      case 11:
        spinners(i);
        break;
      case 12:
        spinners_whole_sphere(i);
        break;
      case 13:
        hue_spinners(i);
        break;
      case 14:
        hue_spinners_whole_sphere(i);
        break;
      case 15:
        shows[i].twoColor();
        break;
      case 16:
        shows[i].packets();
        break;
      case 17:
        shows[i].packets_two();
        break;
      case 18:
        shows[i].sinelon_fastled();
        break;
      case 19:
        shows[i].juggle_fastled();
        break;
      case 20:
        shows[i].bands();
        break;
      case 21:
        draw_objects_static(i, 0);  // 0 = TRIANGLES
        break;
      case 22:
        draw_objects_pulsing(i, 0);  // 0 = TRIANGLES
        break;
      case 23:
        draw_objects_static(i, 1);  // 1 = DIAMONDS
        break;
      case 24:
        draw_objects_pulsing(i, 1);  // 1 = DIAMONDS
        break;
      case 25:
        rotating_white_moon(i, false);
        break;
      case 26:
        rotating_dark_moon(i, false);
        break;
      case 27:
        rotating_two_color_moon(i, false);
        break;
      case 28:
        rotating_white_moon(i, true);
        break;
      case 29:
        rotating_dark_moon(i, true);
        break;
      case 30:
        rotating_two_color_moon(i, true);
        break;
      default:
        shows[i].allOn();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  check_phone();  // Check the phone settings (ArduinoBlue)
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  fixed_delay();
  advance_clocks();  // advance the cycle clocks and check for next show 
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { 
      next_show(i); 
    }
  }
  
  if (curr_lightning > 0 ) {  // (ArduinoBlue)
    curr_lightning--;  // Reduce the current bolt
  }
}

//
// fixed_delay - make every cycle the same time
//
void fixed_delay() {
  long new_time = millis();
  long time_delta = new_time - timing;  // how much time has elapsed? Usually 3-5 milliseconds
  timing = new_time;  // update the counter
  if (time_delta < DELAY_TIME) {  // if we have excess time,
    delay(DELAY_TIME - time_delta);  // delay for the excess
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  current_show[i] = random8(NUM_SHOWS);
  
  led[i].fillBlack();
  led[i].push_frame();
  
  shows[i].resetAllClocks();
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
  shows[i].turnOnMorphing();
}

uint8_t get_other_channel_show(uint8_t i) {
  uint8_t other_channel_show = (i == CHANNEL_A) ? current_show[CHANNEL_B] : current_show[CHANNEL_A] ;
  return other_channel_show;
}

///// SPECIALIZED SHOWS

//
// pattern shows
//
void draw_objects_static(uint8_t i, uint8_t obj) {
  uint8_t c1 = shows[i].getForeColor();
  uint8_t c2 = shows[i].getBackColor();
  uint8_t c3 = led[i].interpolate_wrap(c1, c2, 128);
  
  draw_objects(led[i].wheel(c1), led[i].wheel(c2), led[i].wheel(c3), i, obj);
}

void draw_objects_pulsing(uint8_t i, uint8_t obj) {
  if (shows[i].isShowStart()) {
    shows[i].turnOffMorphing();
    show_freq = random8(3,30);
  }
  uint8_t c1 = shows[i].getForeColor();
  uint8_t c2 = shows[i].getBackColor();
  uint8_t c3 = led[i].interpolate_wrap(c1, c2, 128);
  
  CHSV color1 = led[i].gradient_wheel(c1, beatsin8(show_freq, 0, 255, 0, 0));
  CHSV color2 = led[i].gradient_wheel(c2, beatsin8(show_freq, 0, 255, 0, 128));
  CHSV color3 = led[i].gradient_wheel(c3, beatsin8(show_freq * 1.5, 0, 255, 0, 0));
  
  draw_objects(color1, color2, color3, i, obj);
}

void draw_objects(CHSV color1, CHSV color2, CHSV color3, uint8_t channel, uint8_t obj) {

  uint8_t TRIANGLES[] = {
    1, 0, 0, 2, 2, 1, 1, 0, 0, 1,
    2, 0, 0, 1, 1, 2, 2, 1, 1, 2,
    1, 0, 0, 2, 2, 1, 1, 2, 2, 1,
    2, 0, 0, 1, 1, 2, 2, 1, 1, 2,
    1, 2, 2, 1, 1, 0, 0, 2, 2, 1,
    0, 1, 1, 2, 2, 1, 1, 2, 2, 0,
  };
  
  uint8_t DIAMONDS[] = {
    1, 1, 2, 2, 1, 1, 2, 2, 0, 0,
    2, 2, 1, 1, 2, 2, 0, 0, 1, 1,
    1, 1, 2, 2, 0, 0, 1, 1, 2, 2,
    2, 2, 1, 1, 0, 0, 2, 2, 0, 0,
    1, 1, 2, 2, 0, 0, 1, 1, 0, 0,
    2, 2, 0, 0, 1, 1, 2, 2, 1, 1,
  };
  
  uint8_t hue;
  CHSV color;
  
  turn_off_hubs(channel);  
  for (uint8_t i = 0; i < NUM_FACES / 2; i++) {
    for (uint8_t j = 0; j < NUM_SPOKES; j++) {
      if (obj == 0) {
        hue = TRIANGLES[(i * NUM_SPOKES) + j];
      } else {
        hue = DIAMONDS[(i * NUM_SPOKES) + j];
      }
      
      switch (hue) {
        case 0:
          color = color3;
          break;
        case 1:
          color = color1;
          break;
        case 2:
          color = color2;
          break;
      }
      shows[channel].setPixeltoColor(      (i * (NUM_SPOKES + 2)) + j, color);  // top hemisphere
      shows[channel].setPixeltoColor(((6 + i) * (NUM_SPOKES + 2)) + j, color);  // bottom hemisphere
    }
  }
}

//
// propellor shows
//
void propellor(uint8_t i) {
  set_propellor_two_color(led[i].wheel(shows[i].getForeColor()), 
                          led[i].wheel(shows[i].getBackColor()), 
                          CHSV(0,0,0), i);
}

void propellor_two_color(uint8_t i) {
  set_propellor_two_color(led[i].wheel(shows[i].getForeColor()), 
                          led[i].wheel(shows[i].getForeColor()), 
                          led[i].wheel(shows[i].getBackColor()), i);
}

void set_propellor_two_color(CHSV hub_color, CHSV color1, CHSV color2, uint8_t channel) {
  if (shows[channel].isShowStart()) { shows[channel].makeWaitFaster(6); }
  
  CHSV color;
  set_hubs(hub_color, channel);
  set_spokes(color2, channel);  // Set the background

  uint8_t rot = shows[channel].getCycle() % NUM_SPOKES;
  set_blade(color1, (rot + 0) % NUM_SPOKES, channel);
  set_blade(color1, (rot + 1) % NUM_SPOKES, channel);
  set_blade(color1, (rot + 6) % NUM_SPOKES, channel);
  set_blade(color1, (rot + 7) % NUM_SPOKES, channel);
  
}

void fan(uint8_t i) {
  set_fan_two_color(led[i].wheel(shows[i].getForeColor()), 
                    led[i].wheel(shows[i].getBackColor()), 
                    CHSV(0,0,0), i);
}

void fan_two_color(uint8_t i) {
  set_fan_two_color(led[i].wheel(shows[i].getForeColor()), 
                    led[i].wheel(shows[i].getForeColor()), 
                    led[i].wheel(shows[i].getBackColor()), i);
}

void set_fan_two_color(CHSV hub_color, CHSV color1, CHSV color2, uint8_t channel) {
  if (shows[channel].isShowStart()) { shows[channel].makeWaitFaster(4); }
  
  CHSV color;
  set_hubs(hub_color, channel);
  set_spokes(color2, channel);  // Set the background

  uint8_t rot = shows[channel].getCycle() % NUM_SPOKES;
  for (uint8_t j = 0; j < 4; j++) {
    set_blade(color1, (rot + j) % NUM_SPOKES, channel);
  }
}

void set_blade(CHSV color, uint8_t blade, uint8_t channel) {
  uint8_t blade_num;
  for (uint8_t i = 0; i < NUM_FACES; i++) {
    blade_num = (blade + NUM_SPOKES - (i / 2)) % NUM_SPOKES;
    blade_num = (i % 2 == 0) ? blade_num : NUM_SPOKES - blade_num - 1;  // adjacent faces spin in opposite directions
    shows[channel].setPixeltoColor((i * (NUM_SPOKES + 2)) + blade_num, color);
  }
}

//
// hub_two_color - several kinds of hub & spoke shows
//
void hub_two_color(uint8_t i) {
  set_hubs(led[i].wheel(shows[i].getForeColor()), i);
  set_spokes(led[i].wheel(shows[i].getBackColor()), i);
}

void hub_only(uint8_t i) {
  set_hubs(led[i].wheel(shows[i].getForeColor()), i);
  set_spokes(shows[i].getForeBlack(), i);
}

void hub_pulse_two_color(uint8_t i) {
  if (shows[i].isShowStart()) {
    shows[i].turnOffMorphing();
    show_freq = random8(3,30);
  }
  set_hubs(led[i].gradient_wheel(shows[i].getForeColor(), beatsin8(show_freq, 0, 255, 0, 0)), i);
  set_spokes(led[i].gradient_wheel(shows[i].getBackColor(), beatsin8(show_freq, 0, 255, 0, 128)), i);
}

void spinners(uint8_t channel) {
  if (shows[channel].isShowStart()) {
    shows[channel].turnOffMorphing();
    show_freq = random8(20, 120);
  }
  set_hubs(led[channel].wheel(shows[channel].getForeColor()), channel);

  for (uint8_t j = 0; j < NUM_SPOKES; j++) {
    CHSV color = led[channel].gradient_wheel(shows[channel].getBackColor(), beatsin8(show_freq, 0, 255, 0, j * 255 / NUM_SPOKES));
    for (uint8_t i = 0; i < NUM_FACES; i++) {
      shows[channel].setPixeltoColor((i * (NUM_SPOKES + 2)) + j, color);
    }
  }
}

void spinners_whole_sphere(uint8_t channel) {
  if (shows[channel].isShowStart()) {
    shows[channel].turnOffMorphing();
    show_freq = random8(3,30);
  }
  set_hubs(led[channel].wheel(shows[channel].getForeColor()), channel);

  uint8_t pixel;
  CHSV color;
  for (uint8_t i = 0; i < NUM_FACES; i++) {
    for (uint8_t j = 0; j < NUM_SPOKES; j++) {
      pixel = (i * (NUM_SPOKES + 2)) + j;
      color = led[channel].gradient_wheel(shows[channel].getBackColor(), beatsin8(show_freq, 0, 255, 0, pixel * 255 / NUM_LEDS));
      shows[channel].setPixeltoColor(pixel, color);
    }
  }
}

void hue_spinners(uint8_t channel) {
  if (shows[channel].isShowStart()) {
    show_freq = random8(3,30);
    shows[channel].turnOffMorphing();
  }
  set_hubs(led[channel].wheel(shows[channel].getForeColor()), channel);

  for (uint8_t j = 0; j < NUM_SPOKES; j++) {
    uint8_t hue = beatsin8(show_freq, 0, 255, 0, j * 255 / (NUM_SPOKES * 5) );  // * X etc will make hues less rainbow
    for (uint8_t i = 0; i < NUM_FACES; i++) {
      shows[channel].setPixeltoHue((i * (NUM_SPOKES + 2)) + j, hue);
    }
  }
}

void hue_spinners_whole_sphere(uint8_t channel) {
  if (shows[channel].isShowStart()) {
    shows[channel].turnOffMorphing();
    show_freq = random8(3,30);
  }
  set_hubs(led[channel].wheel(shows[channel].getForeColor()), channel);

  uint8_t pixel;
  for (uint8_t i = 0; i < NUM_FACES; i++) {
    for (uint8_t j = 0; j < NUM_SPOKES; j++) {
      pixel = (i * (NUM_SPOKES + 2)) + j;
      shows[channel].setPixeltoHue(pixel, beatsin8(show_freq, 0, 255, 0, pixel * 255 / (NUM_LEDS * 2)) );
    }
  }
}

void turn_off_hubs(uint8_t channel) {
  set_hubs(BLACK, channel);
}

void set_hubs(CHSV color, uint8_t channel) {
  uint8_t hub_pixel;
  for (uint8_t i = 0; i < NUM_FACES; i++) {
    hub_pixel = (i * (NUM_SPOKES + 2)) + NUM_SPOKES + 1;
    shows[channel].setPixeltoColor(hub_pixel, color);
    shows[channel].setPixeltoColor(hub_pixel + 1, color);
  }
}

void set_spokes(CHSV color, uint8_t channel) {
  for (uint8_t i = 0; i < NUM_FACES; i++) {
    for (uint8_t j = 0; j < NUM_SPOKES; j++) {
      shows[channel].setPixeltoColor((i * (NUM_SPOKES + 2)) + j, color);
    }
  }
}

//
// bands - several kinds of traveling bands
//
void color_band(uint8_t i) {
  band_effect(i, led[i].wheel(shows[i].getForeColor()), shows[i].getForeBlack() );
}

void dark_color_band(uint8_t i) {
  band_effect(i, shows[i].getBackBlack(), led[i].wheel(shows[i].getBackColor()) );
}

void two_color_band(uint8_t i) {
  band_effect(i, led[i].wheel(shows[i].getForeColor()), led[i].wheel(shows[i].getBackColor()) );
}

void hue_band(uint8_t channel) {
  if (shows[channel].isShowStart()) { initialize_effect_band(); }
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t hue = beatsin8(show_freq, 0, 255, 0, get_distance(i, center_pixel) );
    shows[channel].setPixeltoHue(i, hue);
  }
}

void band_effect(uint8_t channel, CHSV foreColor, CHSV backColor) {
  if (shows[channel].isShowStart()) { initialize_effect_band(); }
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t fract = beatsin8(show_freq, 0, 255, 0, get_distance(i, center_pixel) );
    CHSV color = led[CHANNEL_A].getInterpHSV(backColor, foreColor, fract);
    shows[channel].setPixeltoColor(i, color);
  }
}

//
// initialize_effect_band - reset effect parameters
//
void initialize_effect_band() {
  center_pixel = random8(NUM_LEDS);
  show_freq = random8(3, 30);
  rotation_speed = random8(1, 9) / 4.0;
}

//
// Rotating moon
//
void rotating_dark_moon(uint8_t i, boolean fade) {
  rotating_moon(led[i].wheel(shows[i].getForeColor()), shows[i].getForeBlack(), i, fade);
}

void rotating_white_moon(uint8_t i, boolean fade) {
  rotating_moon(WHITE, BLACK, i, fade);
}

void rotating_two_color_moon(uint8_t i, boolean fade) {
  rotating_moon(led[i].wheel(shows[i].getForeColor()), led[i].wheel(shows[i].getBackColor()), i, fade);
}

void rotating_moon(CHSV lightColor, CHSV darkColor, uint8_t channel, boolean fade) {
  if (shows[channel].isShowStart()) {
    initialize_effect_band();
    shows[channel].setWait(rotation_speed * 4);
  }

  // Only rotates around the z-axis
  // Equator is a unit circle on the xy plane: x(2) + y(2) = 1
  // Points are determined by: x = sin(i), y = cos(i)
  uint8_t x = sin8_C(shows[channel].getSmallCycle() * rotation_speed);
  uint8_t y = cos8(shows[channel].getSmallCycle() * rotation_speed);
  uint8_t z = 128;
  
  light_half_moon_from_point(x,y,z, lightColor, darkColor, channel, fade);
}

void light_half_moon_from_point(uint8_t x, uint8_t y, uint8_t z, CHSV lightColor, CHSV darkColor, uint8_t channel, boolean fade) {
  CHSV color;
  uint8_t dist;
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {  // 180 = 255 / sqrt(2)
    dist = get_distance_pixel_point(i, x,y,z);
    if (fade) {
      color = led[CHANNEL_A].getInterpHSV(lightColor, darkColor, dist);
    } else {
      color = dist ? lightColor : darkColor ;
    }
    shows[channel].setPixeltoColor(i, color);
  }
}

///// END SPECIALIZED SHOWS


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV color = led[CHANNEL_A].getInterpHSV(led[CHANNEL_B].getInterpFrameColor(i),
                                             led[CHANNEL_A].getInterpFrameColor(i),
                                             fract);  // interpolate a + b channels
    color = lightning(narrow_palette(color));  // (ArduinoBlue)

    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }
    
    leds[i] = color;
    led_buffer[i] = color;
  }
}

//
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  color.h = map8(color.h, hue_start, (hue_start + hue_width) % MAX_COLOR );
  return color;
}

//
// lightning - ramp all pixels quickly up to white (down sat & up value) and back down
//
CHSV lightning(CHSV color) {  // (ArduinoBlue)
  if (curr_lightning > 0) {
    uint8_t increase = 255 - cos8( map(curr_lightning, 0, BOLT_TIME, 0, 255));
    color.s -= increase;
    color.v += increase;
  }
  return color;
}

//
// check_phone - poll the phone for updated values  (ArduinoBlue)
//
void check_phone() {
  int8_t sliderId = phone.getSliderId();  // ID of the slider moved
  int8_t buttonId = phone.getButton();  // ID of the button

  if (sliderId != -1) {
    int16_t sliderVal = phone.getSliderVal();  // Slider value goes from 0 to 200
    sliderVal = map(sliderVal, 0, 200, 0, 255);  // Recast to 0-255

    switch (sliderId) {
      case BRIGHTNESS_SLIDER:
        BRIGHTNESS = sliderVal;
        FastLED.setBrightness( BRIGHTNESS );
        break;
      case HUE_SLIDER:
        hue_start = sliderVal;
        break;
      case HUE_WIDTH_SLIDER:
        hue_width = sliderVal;
        break;
      case SPEED_SLIDER:
        DELAY_TIME = map8(sliderVal, 10, 100);
        break;
      case FADE_TIME_SLIDER:
        FADE_TIME = map8(sliderVal, 0, SHOW_DURATION);
        break;
    }
  }
  
  if (buttonId == BAM_BUTTON) { curr_lightning = BOLT_TIME; }
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint16_t small_cycle = shows[i].getSmallCycle();
  uint8_t intensity;  // 0 = Off, 255 = full On

  if (small_cycle < FADE_CYCLES) {
    intensity = map(small_cycle, 0, FADE_CYCLES, 0, 255);  // rise
  } else if (small_cycle <= (MAX_SMALL_CYCLE / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((MAX_SMALL_CYCLE / 2) + FADE_CYCLES)) {
    intensity = map(small_cycle - (MAX_SMALL_CYCLE / 2), 0, FADE_CYCLES, 255, 0);  // decay
  } else {
    intensity = 0;
  }
  return ease8InOutQuad(intensity);
}

//
// get_distance
//
uint8_t get_distance_pixel_point(uint8_t i, uint8_t x, uint8_t y, uint8_t z) {
  // 3D distance = sqrt(x2 + y2 + z2) scaled to 0-255
  uint16_t d = sqrt(get_diff_sq_pixel_value_by_index(i, 0, x) +
                    get_diff_sq_pixel_value_by_index(i, 1, y) +
                    get_diff_sq_pixel_value_by_index(i, 2, z));
  return map(d, 0, 262, 0, 255);
}

uint8_t get_distance(uint8_t c1, uint8_t c2) {
  // 3D distance = sqrt(x2 + y2 + z2) scaled to 0-255
  uint16_t d = sqrt(get_diff_sq_by_index(c1, c2, 0) +
                    get_diff_sq_by_index(c1, c2, 1) +
                    get_diff_sq_by_index(c1, c2, 2));
  return map(d, 0, 262, 0, 255);
}

uint16_t get_diff_sq_pixel_value_by_index(uint8_t i, uint8_t index, uint8_t value) {
  return sq( get_coord_value(i, index) - value );
}

uint16_t get_diff_sq_by_index(uint8_t c1, uint8_t c2, uint8_t index) {
  return sq( get_coord_value(c2, index) - get_coord_value(c1, index) );
}

//
// get_coord_value
//
uint8_t get_coord_value(uint8_t coord, uint8_t index) {
  uint8_t COORDS[] = {
    215, 86, 215, 231, 113, 205, 231, 141, 205, 215, 168, 215, 
    192, 176, 229, 165, 166, 246, 151, 143, 255, 151, 111, 255, 
    165, 88, 246, 192, 78, 229, 195, 128, 236, 195, 128, 236,   // End of face 0
    176, 229, 192, 166, 246, 165, 143, 255, 151, 111, 255, 151, 
    88, 246, 165, 78, 229, 192, 86, 215, 215, 113, 205, 231, 
    141, 205, 231, 168, 215, 215, 128, 236, 195, 128, 236, 195,   // End of face 1
    49, 231, 113, 39, 215, 86, 25, 192, 78, 8, 165, 88, 
    0, 151, 111, 0, 151, 143, 8, 165, 166, 25, 192, 176, 
    39, 215, 168, 49, 231, 141, 19, 195, 127, 19, 195, 127,   // End of face 2
    8, 89, 88, 25, 62, 78, 39, 39, 86, 49, 23, 113, 
    49, 23, 141, 39, 39, 168, 25, 62, 176, 8, 89, 166, 
    0, 103, 143, 0, 103, 111, 19, 60, 127, 19, 60, 127,   // End of face 3
    111, 0, 151, 143, 0, 151, 166, 8, 165, 176, 25, 192, 
    168, 39, 215, 141, 49, 231, 113, 49, 231, 86, 39, 215, 
    78, 25, 192, 88, 8, 165, 128, 19, 195, 128, 19, 195,   // End of face 4
    103, 111, 255, 103, 143, 255, 89, 166, 246, 62, 176, 229, 
    39, 168, 215, 23, 141, 205, 23, 113, 205, 39, 86, 215, 
    62, 78, 229, 89, 88, 246, 60, 128, 236, 60, 128, 236,   // End of face 5
    205, 231, 141, 215, 215, 168, 229, 192, 176, 246, 165, 166, 
    255, 151, 143, 255, 151, 111, 246, 165, 88, 229, 192, 78, 
    215, 215, 86, 205, 231, 113, 236, 195, 127, 236, 195, 127,   // End of face 6
    246, 89, 166, 229, 62, 176, 215, 39, 168, 205, 23, 141, 
    205, 23, 113, 215, 39, 86, 229, 62, 78, 246, 89, 88, 
    255, 103, 111, 255, 103, 143, 236, 60, 127, 236, 60, 127,   // End of face 7
    143, 0, 103, 111, 0, 103, 88, 8, 89, 78, 25, 62, 
    86, 39, 39, 113, 49, 23, 141, 49, 23, 168, 39, 39, 
    176, 25, 62, 166, 8, 89, 128, 19, 60, 128, 19, 60,   // End of face 8
    39, 86, 39, 23, 113, 49, 23, 141, 49, 39, 168, 39, 
    62, 176, 25, 89, 166, 8, 103, 143, 0, 103, 111, 0, 
    89, 88, 8, 62, 78, 25, 60, 128, 19, 60, 128, 19,   // End of face 9
    78, 229, 62, 88, 246, 89, 111, 255, 103, 143, 255, 103, 
    166, 246, 89, 176, 229, 62, 168, 215, 39, 141, 205, 23, 
    113, 205, 23, 86, 215, 39, 128, 236, 60, 128, 236, 60,   // End of face 10
    215, 168, 39, 231, 141, 49, 231, 113, 49, 215, 86, 39, 
    192, 78, 25, 165, 88, 8, 151, 111, 0, 151, 143, 0, 
    165, 166, 8, 192, 176, 25, 195, 128, 19, 195, 128, 19,   // End of face 11
  };

  return COORDS[(coord * 3) + index];
}
