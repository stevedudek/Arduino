#include <FastLED.h>
#include <SimbleeForMobile.h>

//
//  Fish
//
//  6/19/18
//
//  FastLED
//
//  Simblee!!!
//
//  Fixes: 
//  Dumb speaker
//  Cannot operate its own lights
//
//  xBee - Speaking Mode
//
uint8_t brightness = 255; // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 12
#define CLOCK_PIN 11

bool is_on = true;

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
#define FISH_HUE 0 // Main fish color: red (https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors)
uint8_t foreColor = FISH_HUE;        // Starting foreground color
uint8_t backColor = MAX_COLOR / 2;   // Starting background color
CHSV BLCK = CHSV(0, 0, 0);
#define MIN_COLOR_SPEED 6    // Higher = slower
#define MAX_COLOR_SPEED 40   // Higher = slower
uint8_t foreColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster
uint8_t backColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster

// Palettes
bool do_palettes = false;
uint8_t palette_center = 0; // 0-255 (hue)
uint8_t palette_width = 255;  // 255 = full RGB
#define PALETTE_DURATION 300  // seconds between palettes

// Shows
#define NUM_SHOWS 21
#define NUM_PATTERNS 19   // Total number of patterns
uint8_t current_show = 0;
uint8_t current_pattern = 0;
#define CHANGE_SHOW false  // use "false" for debugging purposes

#define SHOW_DURATION 20  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 1   // seconds to fade out
#define FADE_IN_TIME 1    // seconds to fade out

// Clocks and time
uint8_t morph = 0;
uint32_t small_cycle = 0;
uint16_t cycle = 0;
uint8_t bands_bpm_1, bands_bpm_2;
uint8_t band_min_1, band_min_2;
#define MIN_BPM 10
#define MAX_BPM 50

// wait times
#define MIN_WAIT   4  // Minimum number of morph steps in a cycle
#define MAX_WAIT  20  // Maximum number of morph steps in a cycle
#define NUM_WAIT_VALUES 16 // Number of stored delay times
uint8_t wait = 20; //NUM_WAIT_VALUES / 2;
uint8_t total_frames;

// noise
bool has_noise = false;
#define MAX_HUE_NOISE 128   // 255 max 
#define MAX_SAT_NOISE 128   // 255 max
uint8_t noise_set;
uint8_t noise_hue_intense = MAX_HUE_NOISE / 2;
uint8_t noise_sat_intense = MAX_SAT_NOISE / 2;
static uint16_t noise_x;
static uint16_t noise_y;
static uint16_t noise_z;
uint16_t noise_speed = 20; // speed is set dynamically once we've started up
uint16_t noise_scale = 30; // scale is set dynamically once we've started up

const uint8_t noise_param_set[] PROGMEM = 
    { 0,0, 20,30, 10,50, 8,120, 4,30, 8,50, 20,90, 20,30, 20,20, 50,50, 90,90, 30,20 };

// xBee language
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_BRIGHTNESS 'I'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_CYCLE      'C'
#define COMMAND_PATTERN    'P'
#define COMMAND_NOISE      'N'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define MAX_MESSAGE        40
#define MAX_NUM             5  // To handle 65,535 of small_cycle

//
// Simblee
//
volatile bool needsUpdate;  // For Simblee

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
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);

  total_frames = getNumFrames(wait);

  SimbleeForMobile.deviceName = "Lights";  // Big name
  SimbleeForMobile.advertisementData = "Fish";  // Small name
  SimbleeForMobile.begin(); 
}

//
// loop
//
void loop() { 

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay
  
  morph++;
  small_cycle++;

  if (small_cycle % foreColorSpeed == 0) { 
    foreColor = IncColor(foreColor, 1);  
  }
  if (small_cycle % backColorSpeed == 0) {
    backColor = IncColor(backColor, 1);
  }
  
  if (morph >= total_frames) {  // Finished morphing
    
    morph = 0;
    
    cycle++;  // Advance the cycle clock
    
    change_it_up();
  }

  if (small_cycle >= MAX_SMALL_CYCLE) { next_show(); }

  // Simblee
  if(needsUpdate) { 
    needsUpdate = false;
    update();
    speak_all_commands();
  }
  
  SimbleeForMobile.process();  // end loop with this Simble function
}

//
// change_it_up
//
void change_it_up() {
  EVERY_N_SECONDS(PALETTE_DURATION) { 
    change_palette();
  }
}

//
// next_show
//
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void next_show() {
  // Switch between a patterns show and all the other shows
  if (CHANGE_SHOW) {
    current_pattern = random8(NUM_PATTERNS);
    
    if (current_show < 2) {
      current_show++;
    } else if (current_show == 2) {
      current_show = random8(3, NUM_SHOWS);
    } else {
      current_show = 0;
    }
  }
  set_show(current_show);
}

void set_show(uint8_t show_num) {
  current_show = show_num;
  morph = 0;
  small_cycle = 0;
  cycle = 0;
  wait = random8(NUM_WAIT_VALUES);
  updateSpeed();
  total_frames = getNumFrames(wait);
  foreColorSpeed = up_or_down(foreColorSpeed, MIN_COLOR_SPEED, MAX_COLOR_SPEED);
  backColorSpeed = up_or_down(backColorSpeed, MIN_COLOR_SPEED, MAX_COLOR_SPEED);

  speak_all_commands();
}

//
// is_show_start
//
boolean is_show_start() {
  return (cycle == 0 && morph == 0);
}

//
// change_palette
//
void change_palette() {
  if (do_palettes) {
    palette_center = random8(255);
    palette_width = random8(10, 255);
    updatePaletteCenter();
    updatePaletteWidth();
  }
}
  

//
// speak_command - send out a letter and value command
//
void speak_command(char command, int value) {
  Serial.print(command);
  Serial.print(value);
  Serial.print(COMMAND_COMMA);
}

void speak_end_command() {
  Serial.println(COMMAND_PERIOD);  // Speak terminal period
}

void speak_all_commands() {
  // Send all commands at once
  speak_command(COMMAND_SHOW, current_show);
  speak_command(COMMAND_PATTERN, current_pattern);
  speak_command(COMMAND_NOISE, noise_set);
  speak_command(COMMAND_FORE, foreColor);
  speak_command(COMMAND_BACK, backColor);
  speak_command(COMMAND_BRIGHTNESS, brightness);
  speak_command(COMMAND_WAIT, wait);
  speak_end_command();
}

//
// morph_frame
//
void morph_frame() {
   check_fades();  // Dim at start + end of shows
}

//
// check_fades - check the fade-to-blacks at beginning and end of show
//
void check_fades() {
  uint8_t fade_amount = 0;
  
  if (small_cycle <= (FADE_IN_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map(small_cycle, 0, (FADE_IN_TIME * 1000 / DELAY_TIME), 256, 0);
  } else if ((MAX_SMALL_CYCLE - small_cycle) <= (FADE_OUT_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map((MAX_SMALL_CYCLE - small_cycle), 0, (FADE_OUT_TIME * 1000 / DELAY_TIME), 256, 0);
  }
    
  if (fade_amount > 0) {
    if (morph == 0) {
      speak_all_commands();
    }
  }
}

//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint16_t amount) {
  return (c + amount) % 256;
}

//
// up_or_down - increase on decrease a counter randomly within bounds
//
uint8_t up_or_down(uint8_t counter, uint8_t min_value, uint8_t max_value) {
  counter += (random8(2) * 2 - 1);  // 0-1 -> 0,2 -> -1 or +1
  if (counter < min_value) {
    return min_value + 1;
  } else if (counter > max_value) {
    return max_value - 1;
  } else {
    return counter;
  }
}

//
// getNumFrames - convert a wait value into a number of morph frames
//
uint8_t getNumFrames(uint8_t wait_value) {
  return map(wait_value, 0, NUM_WAIT_VALUES-1, MIN_WAIT, MAX_WAIT);
}


///////////////
//
//  Simblee portion
//

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
  wait_slider = SimbleeForMobile.drawSlider(95, 105, 155, 0, NUM_WAIT_VALUES);
  wait_textfield = SimbleeForMobile.drawTextField(255, 105, 40, wait, "", TEXT_COLOR, bkgnd_color);
  updateSpeed();
  
  // Forecolor slider
  SimbleeForMobile.drawText(15, 150, "Forecolor", TEXT_COLOR);
  drawColorBar(95, 145, 155, 30);
  foreColor_slider = SimbleeForMobile.drawSlider(95, 145, 155, 0, 255);
  foreColor_swatch = SimbleeForMobile.drawRect(255, 145, 40, 30, getColorFromHue(foreColor));
  updateForeColor();
  
  // Backcolor slider
  SimbleeForMobile.drawText(15, 190, "Backcolor", TEXT_COLOR);
  drawColorBar(95, 185, 155, 30);
  backColor_slider = SimbleeForMobile.drawSlider(95, 185, 155, 0, 255);
  backColor_swatch = SimbleeForMobile.drawRect(255, 185, 40, 30, getColorFromHue(backColor));
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
  SimbleeForMobile.updateValue(noise_switch, 0);

  SimbleeForMobile.drawText(201, switch_text_height, "Palette", TEXT_COLOR);
  palette_switch = SimbleeForMobile.drawSwitch(255, switch_height, BLUE);
  SimbleeForMobile.updateValue(palette_switch, 0);

  // Palette center slider
  palette_center_text = SimbleeForMobile.drawText(10, 450, "Center", TEXT_COLOR);
  drawPaletteCenterRects(70, 447, 175, 30);
  palette_center_slider = SimbleeForMobile.drawSlider(70, 445, 180, 0, 255);
  palette_center_swatch = SimbleeForMobile.drawRect(255, 445, 40, 30, getColorFromHue(foreColor));
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
  
  // update(); // is this necessary?
}

//
//  Update display with user-initiated changes
//
void update() {

  // Brightness slider
  if (eventId == brightness_slider || eventId == brightness_textfield) {
    updateBrightness();
  
  // Speed Slider
  } else if (eventId == wait_slider || eventId == wait_textfield) {  
    updateSpeed();
  
  // Forecolor slider
  } else if (eventId == foreColor_slider) {
    updateForeColor();
  
  // Backcolor slider
  } else if (eventId == backColor_slider) {
    updateBackColor();
  
  // Shows
  } else if (eventId == show_stepper || eventId == show_textfield) {
    updateShows();
  
  // Switches
  } else if (eventId == on_switch) {
    return;
  } else if (eventId == noise_switch) {
    return;
  } else if (eventId == palette_switch) {
    if (!do_palettes) {  // counter-intuitive !
      turnOnPaletteDetails();
    } else {
      turnOffPaletteDetails();
    }

  // Palette center slider
  } else if (eventId == palette_center_slider) {
    updatePaletteCenter();
    updatePaletteSwatch();
  
  // Palette width slider
  } else if (eventId == palette_width_slider) {
    updatePaletteWidth();
    updatePaletteSwatch();
  }
}

//
//  update functions: use these functions to adjust sliders
//
void updateUI() {
  updateBrightness();
  updateSpeed();
  updateForeColor();
  updateBackColor();
  
}

void updateBrightness() {
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(brightness_slider, brightness);
    SimbleeForMobile.updateValue(brightness_textfield, brightness);
  }
}

void updateSpeed() {
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(wait_slider, wait);
    SimbleeForMobile.updateValue(wait_textfield, wait);
  }
}

void updateForeColor() {
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(foreColor_slider, foreColor);
    SimbleeForMobile.updateColor(foreColor_swatch, getColorFromHue(foreColor));
  }
}

void updateBackColor() {
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(backColor_slider, backColor);
    SimbleeForMobile.updateColor(backColor_swatch, getColorFromHue(backColor));
  }
}

void updateShows() {
  if (current_show == NUM_SHOWS) {
    current_show = 0;
  } else if (current_show == 255) {
    current_show = NUM_SHOWS - 1;
  }
//  if (SimbleeForMobile.updatable) {
//    SimbleeForMobile.updateValue(show_stepper, current_show);
//  }
  if (SimbleeForMobile.updatable) {
    SimbleeForMobile.updateValue(show_textfield, current_show);
  }
  set_show(current_show);
  speak_all_commands();
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
  
  if (event.id == brightness_slider || event.id == brightness_textfield) {
    brightness = event.value;
    
  } else if (event.id == wait_slider || event.id == wait_textfield) {
    wait = event.value;
    total_frames = getNumFrames(wait);
    
  } else if (event.id == foreColor_slider) {
    foreColor = event.value;
    
  } else if (event.id == backColor_slider) {
    backColor = event.value;
    
  } else if (event.id == show_stepper) {
    current_show = event.value;
    
  } else if (event.id == on_switch) {
    is_on = event.value;
    
  } else if (event.id == noise_switch) {
    has_noise = event.value;
    
  } else if (event.id == palette_switch) {
    do_palettes = event.value;
    
  } else if (event.id == palette_center_slider) {
    palette_center = event.value;
    
  } else if (event.id == palette_width_slider || event.id == palette_width_textfield) {
    palette_width = event.value;
  }
  needsUpdate = true;
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
    if (SimbleeForMobile.updatable) {
      SimbleeForMobile.updateColor(palette_swatch[i], getColorForPaletteSwatch(i));
    }
  }
}

void turnOnPaletteDetails() {
  updatePaletteSwatch();
  updatePaletteCenter();
  updatePaletteWidth();
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
