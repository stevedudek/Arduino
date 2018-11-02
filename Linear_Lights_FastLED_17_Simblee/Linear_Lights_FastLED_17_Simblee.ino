#include <Noise.h>
#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <SimbleeForMobile.h>  // SIMBLEE_STUB
//
//  Linear Lights with FastLED
//
//  Libraricized
//
//  Simblee stub (search for SIMBLEE_STUB)
//
//  7/14/2018
//
#define NUM_LEDS 9

uint8_t brightness = 128; // (0-255)  SIMBLEE_STUB

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 12
#define CLOCK_PIN 11

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library

// Palettes - all SIMBLEE_STUB
bool CAN_CHANGE_PALETTES = false;  // SIMBLEE_STUB
uint8_t palette_center = 0; // 0-255 (hue)
uint8_t palette_width = 255;  // 255 = full RGB
#define PALETTE_DURATION 300  // seconds between palettes

// Shows
#define NUM_SHOWS 9
uint8_t current_show = 0;

// Clocks and time
#define SHOW_DURATION 20  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 2   // seconds to fade out
#define FADE_IN_TIME 2    // seconds to fade out

// noise
bool HAVE_NOISE = false;  // SIMBLEE_STUB
Noise noise = Noise(NUM_LEDS);

// simblee values - SIMBLEE_STUB
uint8_t simblee_foreColor;
uint8_t simblee_backColor;
uint8_t simblee_current_show;
uint8_t simblee_wait;
bool is_on;
// end

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
//  Serial.begin(9600);
//  Serial.println("Start");
  
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( brightness );
  
  if (HAVE_NOISE) {
    noise.turnNoiseOn();
  } else {
    noise.turnNoiseOff();
  }

  if (CAN_CHANGE_PALETTES) {
    led.setPalette();  // turns on palettes with default valets
  }

  // Set up the various mappings here (1D lists in PROGMEM)
  //  led.setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  shows = Shows(&led);  // Show library - reinitialized for led mappings
  
  led.fillBlack();
  led.push_frame();

  //// SIMBLEE_STUB
  SimbleeForMobile.deviceName = "Lights";  // Big name
  SimbleeForMobile.advertisementData = "Fish";  // Small name
  SimbleeForMobile.begin();
  //// end
}

void loop() {
  
  switch (current_show) {
  
    case 0:
      shows.allOn();
      break;
    case 1:
      shows.morphChain();
      break;
    case 2:
      shows.randomFill();
      break;
    case 3:
      shows.randomColors();
      break;
    case 4:
      shows.twoColor();
      break;
    case 5:
      shows.lightWave();
      break;
    case 6:
      shows.sawTooth();
      break;
    case 7:
      noisyshow();
      break;
    default:
      shows.bands();
      break;
  }

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay

  noise.fillNoise();
  
  shows.advanceClock();
  
  if (shows.getSmallCycle() >= MAX_SMALL_CYCLE) { 
    random_show(); 
  }
  
  EVERY_N_SECONDS(PALETTE_DURATION) { 
    led.randomizePalette(); 
  }

  //// SIMBLEE_STUB
  update();  // Possibly update on every cycle? (or morph == 0)?
  SimbleeForMobile.process();
  //// end
}

//
// next_show
//
void next_show(uint8_t show_number) {
  current_show = show_number;
  led.fillBlack();
  led.push_frame();
  shows.resetAllClocks();
  noise.setRandomNoiseParams();
  shows.pickRandomColorSpeeds();
  shows.pickRandomWait();
}

void random_show() {
  next_show(random8(NUM_SHOWS));
}

//
// noisyshow
//
void noisyshow() {
  // The Shows.h library does not handle Noise, so it is handled here
  if (shows.isShowStart()) { 
    noise.makeVeryNoisy(); 
  }
  shows.allOn();
}
    
//
// morph_frame
//
void morph_frame() {
  shows.morphFrame();  // 1. calculate interp_frame 2. adjust palette
  add_noise();   // Use the noise library
  update_leds();  // push the interp_frame on to the leds
  check_fades();  // Fade start and end of shows
  FastLED.show();  // Update the display 
}

//
// add_noise - from library - uses led.library getters and setters
//
void add_noise() {
  if (HAVE_NOISE) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      led.setInterpFrameHue(i, noise.addNoiseAtoValue(i, led.getInterpFrameHue(i))); 
      led.setInterpFrameSat(i, noise.addNoiseBtoValueNoWrap(i, led.getInterpFrameSat(i)));
    }
  }
}

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = led.getInterpFrameColor(i);
  }
}

//
// check_fades - check the fade-to-blacks at beginning and end of show
//
void check_fades() {
  uint8_t fade_amount = 0;
  uint32_t small_cycle = shows.getSmallCycle();
  
  if (small_cycle <= (FADE_IN_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map(small_cycle, 0, (FADE_IN_TIME * 1000 / DELAY_TIME), 255, 0);
  } else if ((MAX_SMALL_CYCLE - small_cycle) <= (FADE_OUT_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map((MAX_SMALL_CYCLE - small_cycle), 0, (FADE_OUT_TIME * 1000 / DELAY_TIME), 255, 0);
  }
  if (fade_amount > 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(fade_amount);
    }
  }
}

//////////////
//
//  SIMBLEE_STUB - from here to end
//
#define BKGND_COLOR  85,85,85  // r,g,b
#define TEXT_COLOR      WHITE
#define NORMAL_TEXT_SIZE   18
#define LRG_TEXT_SIZE      24

uint8_t eventId;

// Widget identifiers (uint8_t)
uint8_t brightness_slider, brightness_textfield;
uint8_t wait_slider, wait_textfield;
uint8_t foreColor_slider, foreColor_swatch;
uint8_t backColor_slider, backColor_swatch;
uint8_t show_stepper, show_textfield;
uint8_t on_switch, noise_switch, palette_switch;
uint8_t palette_center_text, palette_center_slider, palette_center_swatch;
uint8_t palette_width_text, palette_width_slider, palette_width_textfield;

#define NUM_BARS  20  // How many color bars for the static color bars
#define palette_swatch_number  20  // How many bars for the dynamic palette swatch
uint8_t palette_swatch[palette_swatch_number];
uint8_t palette_center_rects[NUM_BARS];

//
//  ui - define buttons and sliders
//
void ui() {

  // iPhone 6 is (320 x 568)
  
  color_t bkgnd_color = rgb(BKGND_COLOR);
  
  // Background
  SimbleeForMobile.beginScreen(bkgnd_color);

  // Top text
  SimbleeForMobile.drawText(90, 25, "LED Controller", TEXT_COLOR, LRG_TEXT_SIZE);

  // Brightness slider - 40 vertical pixels between sliders
  SimbleeForMobile.drawText(10, 70, "Brightness", TEXT_COLOR);  // left_start, vert_position, text, color
  brightness_slider = SimbleeForMobile.drawSlider(95, 65, 155, 0, 255); // left_start, vert_pos, width, min, max
  brightness_textfield = SimbleeForMobile.drawTextField(255, 65, 40, brightness, "", TEXT_COLOR, bkgnd_color);  // left_start, vert_pos, width, start_value
  updateBrightness();
    
  // Speed slider
  SimbleeForMobile.drawText(30, 110, "Speed", TEXT_COLOR);
  wait_slider = SimbleeForMobile.drawSlider(95, 105, 155, 2, 50);
  wait_textfield = SimbleeForMobile.drawTextField(255, 105, 40, shows.getWait(), "", TEXT_COLOR, bkgnd_color);
  updateSpeed();
  
  // Forecolor slider
  SimbleeForMobile.drawText(15, 150, "Forecolor", TEXT_COLOR);
  drawColorBar(95, 145, 155, 30);
  foreColor_slider = SimbleeForMobile.drawSlider(95, 145, 155, 0, 255);
  foreColor_swatch = SimbleeForMobile.drawRect(255, 145, 40, 30, getColorFromHue(shows.getForeColor()));
  updateForeColor();
  
  // Backcolor slider
  SimbleeForMobile.drawText(15, 190, "Backcolor", TEXT_COLOR);
  drawColorBar(95, 185, 155, 30);
  backColor_slider = SimbleeForMobile.drawSlider(95, 185, 155, 0, 255);
  backColor_swatch = SimbleeForMobile.drawRect(255, 185, 40, 30, getColorFromHue(shows.getBackColor()));
  updateBackColor();
  
  // Show
  SimbleeForMobile.drawText(30, 235, "Show", TEXT_COLOR);
  show_stepper = SimbleeForMobile.drawStepper(95, 232, 100, -1, NUM_SHOWS);
  show_textfield = SimbleeForMobile.drawTextField(255, 235, 40, current_show, "", TEXT_COLOR, bkgnd_color);
  updateShows();

  // 3 Toggle switches for on, noise, palette
  int switch_height = 400;
  int switch_text_height = switch_height + 5;
  
  SimbleeForMobile.drawText(10, switch_text_height, "On", TEXT_COLOR);
  on_switch = SimbleeForMobile.drawSwitch(35, switch_height);
  SimbleeForMobile.updateValue(on_switch, 1);

  SimbleeForMobile.drawText(95, switch_text_height, "Noise", TEXT_COLOR);
  noise_switch = SimbleeForMobile.drawSwitch(137, switch_height, RED);
  SimbleeForMobile.updateValue(noise_switch, HAVE_NOISE);

  SimbleeForMobile.drawText(201, switch_text_height, "Palette", TEXT_COLOR);
  palette_switch = SimbleeForMobile.drawSwitch(255, switch_height, BLUE);
  SimbleeForMobile.updateValue(palette_switch, CAN_CHANGE_PALETTES);

  // Palette center slider
  palette_center_text = SimbleeForMobile.drawText(10, 450, "Center", TEXT_COLOR);
  drawPaletteCenterRects(70, 447, 175, 30);
  palette_center_slider = SimbleeForMobile.drawSlider(70, 445, 180, 0, 255);
  palette_center_swatch = SimbleeForMobile.drawRect(255, 445, 40, 30, getColorFromHue(shows.getForeColor()));
  updatePaletteCenter();
  
  // Palette width slider
  palette_width_text = SimbleeForMobile.drawText(10, 490, "Width", TEXT_COLOR);
  palette_width_slider = SimbleeForMobile.drawSlider(70, 485, 180, 0, 255, palette_width);
  palette_width_textfield = SimbleeForMobile.drawTextField(255, 482, 40, palette_width, "", TEXT_COLOR, bkgnd_color);
  updatePaletteWidth();
  
  // Palette swatch
  uint8_t width = 10;//FULL_WIDTH / (palette_swatch_number - 1);
  uint8_t swatch_x = 50;
  for (uint8_t i=0; i < palette_swatch_number; i++) {
    palette_swatch[i] = SimbleeForMobile.drawRect(swatch_x, 525, width, 30, getColorForPaletteSwatch(i));
    swatch_x += width;
  }

  turnOnPaletteDetails();
  
  SimbleeForMobile.endScreen();
}

//
//  Check local variables against Simblee variables
//
void update() {

  // Brightness slider
  // Noise switch
  // On switch
  //
  // No check here as randomized runner cannot change these
  
  // Speed Slider
  if (simblee_wait != shows.getWait() && SimbleeForMobile.updatable) {
    simblee_wait = shows.getWait();
    updateSpeed();
  }
  // Forecolor slider
  if (simblee_foreColor != shows.getForeColor() && SimbleeForMobile.updatable) {
    simblee_foreColor = shows.getForeColor();
    updateForeColor();
  }
  // Backcolor slider
  if (simblee_backColor != shows.getBackColor() && SimbleeForMobile.updatable) {
    simblee_backColor = shows.getBackColor();
    updateBackColor();
  }
  // Shows
  if (simblee_current_show != current_show && SimbleeForMobile.updatable) {
    simblee_current_show = current_show;
    updateShows();
  }
}

//
//  update functions: use these functions to adjust sliders
//
void updateBrightness() {
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(brightness_slider, brightness);
    SimbleeForMobile.updateValue(brightness_textfield, brightness);
  }
}

void updateSpeed() {
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(wait_slider, simblee_wait);
    SimbleeForMobile.updateValue(wait_textfield, simblee_wait);
  }
}

void updateForeColor() {
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(foreColor_slider, simblee_foreColor);
    SimbleeForMobile.updateColor(foreColor_swatch, getColorFromHue(simblee_foreColor));
  }
}

void updateBackColor() {
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(backColor_slider, simblee_backColor);
    SimbleeForMobile.updateColor(backColor_swatch, getColorFromHue(simblee_backColor));
  }
}

void updateShows() {
  if (current_show == NUM_SHOWS) {
    current_show = 0;
  } else if (current_show == 255) {
    current_show = NUM_SHOWS - 1;
  }
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(show_stepper, current_show);
    SimbleeForMobile.updateValue(show_textfield, current_show);
  }
}

void updatePaletteCenter() {
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(palette_center_slider, palette_center);
    SimbleeForMobile.updateColor(palette_center_swatch, getColorFromHue(palette_center));  
  }
}

void updatePaletteWidth() {
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(palette_width_slider, palette_width);
    SimbleeForMobile.updateValue(palette_width_textfield, palette_width);
  }
}

//
//  ui_event - define callbacks
//
void ui_event(event_t &event) {
  eventId = event.id;
  /*
  if (event.id == brightness_slider || event.id == brightness_textfield) {
    brightness = event.value;
    FastLED.setBrightness( brightness );
    updateBrightness();
    
  } else if (event.id == wait_slider || event.id == wait_textfield) {
    simblee_wait = event.value;
    shows.setWait(simblee_wait);
    updateSpeed();
    
  } else if (event.id == foreColor_slider) {
    simblee_foreColor = event.value;
    shows.setForeColor(simblee_foreColor);
    updateForeColor();
    
  } else if (event.id == backColor_slider) {
    simblee_backColor = event.value;
    shows.setBackColor(simblee_backColor);
    updateBackColor();
    
  } else if (event.id == show_stepper) {
    simblee_current_show = event.value;
    next_show(simblee_current_show);
    updateShows();
    
  } else if (event.id == on_switch) {
    is_on = event.value;
    
  } else if (event.id == noise_switch) {
    HAVE_NOISE = event.value;
    
  } else if (event.id == palette_switch) {
    CAN_CHANGE_PALETTES = event.value;
    
  } else if (event.id == palette_center_slider) {
    palette_center = event.value;
    updatePaletteCenter();
    
  } else if (event.id == palette_width_slider || event.id == palette_width_textfield) {
    palette_width = event.value;
    updatePaletteWidth();
  }
  */
}

color_t getColorFromHue(uint8_t hue) {
  CRGB color = CHSV(hue, 255, 255);  // FastLED does the CHSV -> CRGB for us!
  return rgb(color.r, color.g, color.b);  // Use rgb for Simblee
}

color_t getColorForPaletteSwatch(uint8_t i) {
  uint8_t hue = ((palette_center - (palette_width / 2)) + (i * (palette_width / palette_swatch_number))) % 256;
  return getColorFromHue(hue);
}

void updatePaletteSwatch() {
  for (uint8_t i=0; i < palette_swatch_number; i++) {
    SimbleeForMobile.updateColor(palette_swatch[i], getColorForPaletteSwatch(i));
  }
}

void turnOnPaletteDetails() {
  turnPaletteDetails(false);
}

void turnOffPaletteDetails() {
  turnPaletteDetails(true);
}

void turnPaletteDetails(bool state) {
  SimbleeForMobile.setVisible(palette_center_text, state);
  SimbleeForMobile.setVisible(palette_center_slider, state);
  SimbleeForMobile.setVisible(palette_center_swatch, state);
  SimbleeForMobile.setVisible(palette_width_text, state);
  SimbleeForMobile.setVisible(palette_width_slider, state);
  SimbleeForMobile.setVisible(palette_width_textfield, state);

  for (uint8_t i=0; i < palette_swatch_number; i++) {
    SimbleeForMobile.setVisible(palette_swatch[i], state);
  }
  for (uint8_t i=0; i < NUM_BARS; i++) {
    SimbleeForMobile.setVisible(palette_center_rects[i], state);
  }
}

void drawColorBar(int x, int y, int width, int height) {
  uint8_t barwidth = width / NUM_BARS;
  for (uint8_t i=0; i < NUM_BARS; i++) {
    x += barwidth;
    SimbleeForMobile.drawRect(x, y, barwidth, height, getColorFromHue(i * 256 / NUM_BARS));
  }
}

void drawPaletteCenterRects(int x, int y, int width, int height) {
  uint8_t barwidth = width / NUM_BARS;
  for (uint8_t i=0; i < NUM_BARS; i++) {
    x += barwidth;
    palette_center_rects[i] = SimbleeForMobile.drawRect(x, y, barwidth, height, getColorFromHue(i * 256 / NUM_BARS));
  }
}
