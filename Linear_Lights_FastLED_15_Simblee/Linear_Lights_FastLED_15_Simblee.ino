#include <FastLED.h>
#include <SimbleeForMobile.h>

//
//  Linear Lights with FastLED
//
//  Simblee!!!
//
//  6/12/2018
//

#define NUM_LEDS 18

uint8_t brightness = 255; // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

CRGB leds[NUM_LEDS];
CHSV current_frame[NUM_LEDS]; // framebuffers
CHSV next_frame[NUM_LEDS];  // framebuffers

uint8_t shuffle[NUM_LEDS];  // For random-fill show (nasty global)

bool is_on = true;

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
uint8_t foreColor =  0;    // Starting foreground color
uint8_t backColor = MAX_COLOR / 2;   // Starting background color
CHSV BLACK_COLOR = CHSV( 0, 0, 0);
#define MIN_COLOR_SPEED 2   // Higher = slower
#define MAX_COLOR_SPEED 10   // Higher = slower
uint8_t foreColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster
uint8_t backColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster

// Palettes
bool do_palettes = true;
uint8_t palette_center = 0; // 0-255 (hue)
uint8_t palette_width = 255;  // 255 = full RGB
#define PALETTE_DURATION 300  // seconds between palettes

// Shows - now enumerated
#define NUM_SHOWS  9
uint8_t current_show = 0;
#define SHOW_DURATION 60  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 3   // seconds to fade out
#define FADE_IN_TIME 3    // seconds to fade out

// Clocks and time
uint8_t morph = 0;
uint32_t small_cycle = 0;
uint16_t cycle = 0;
uint8_t bands_bpm_1, bands_bpm_2;
uint8_t band_min_1, band_min_2;
#define MIN_BPM 5
#define MAX_BPM 60

// wait times
#define WAIT_DURATION 20 // second between increasing wait time
#define MIN_WAIT   2  // Minimum number of morph steps in a cycle
#define MAX_WAIT  50  // Maximum number of morph steps in a cycle
#define NUM_WAIT_VALUES 10 // Number of stored delay times
uint8_t wait = NUM_WAIT_VALUES / 2;
uint8_t total_frames;

// noise
bool has_noise = true;    // set to false to suppress noise
#define MAX_HUE_NOISE 64   // 255 max 
#define MAX_SAT_NOISE 64   // 255 max
uint8_t noise_hue_intense = MAX_HUE_NOISE / 2;
uint8_t noise_sat_intense = MAX_SAT_NOISE / 2;
static uint16_t noise_x;
static uint16_t noise_y;
static uint16_t noise_z;
uint16_t noise_speed = 20; // speed is set dynamically once we've started up
uint16_t noise_scale = 30; // scale is set dynamically once we've started up
uint8_t noise[2][NUM_LEDS];

const uint8_t noise_param_set[] PROGMEM = 
    { 0,0, 20,30, 10,50, 8,120, 4,30, 8,50, 20,90, 20,30, 20,20, 50,50, 90,90, 30,20 };

volatile bool needsUpdate;  // For Simblee

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");

  SimbleeForMobile.deviceName = "Lights";  // Big name
  SimbleeForMobile.advertisementData = "Fish";  // Small name
  SimbleeForMobile.begin();
  
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( brightness );

  //// May be necessary?
  // pinMode (DATA_PIN, OUTPUT);
  // pinMode (CLOCK_PIN, OUTPUT);
  
  noise_x = random16();
  noise_y = random16();
  noise_z = random16();
  
  set_all_black();

  total_frames = getNumFrames(wait);
}

void loop() {
  
  switch (current_show) {
  
    case 0:
      allOn();
      break;
    case 1:
      morphChain();
      break;
    case 2:
      randomfill();
      break;
    case 3:
      randomcolors();
      break;
    case 4:
      twocolor();
      break;
    case 5:
      lightwave();
      break;
    case 6:
      sawtooth();
      break;
    case 7:
      noisyshow();
      break;
    default:
      bands();
      break;
  }

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay
  
  morph++;
  small_cycle++;
  fill_noise();

  if (small_cycle % foreColorSpeed == 0) { 
    foreColor = IncColor(foreColor, 1);
    updateForeColor();
  }
  if (small_cycle % backColorSpeed == 0) { 
    backColor = IncColor(backColor, 1);
    updateBackColor();
  }
  
  if (morph >= total_frames) {  // Finished morphing
    
    morph = 0;
    
    if (cycle++ > 10000) { 
      cycle = 0;  // Advance the cycle clock
    }
    push_frame();

    change_it_up();
  }

  if (small_cycle >= MAX_SMALL_CYCLE) { 
    next_show(); 
  }

  // Simblee
  if(needsUpdate) { 
    needsUpdate = false;
    update();
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
  EVERY_N_SECONDS(WAIT_DURATION) { 
    wait = up_or_down(wait, 0, MAX_WAIT);
    updateSpeed();
    total_frames = getNumFrames(wait);
  }
}

//
// next_show
//
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void next_show() {
  set_show(random8(NUM_SHOWS));
}

void set_show(uint8_t show_num) {
  current_show = show_num;
  morph = 0;
  small_cycle = 0;
  cycle = 0;
  set_noise_parameters();
  set_all_black();
  foreColorSpeed = up_or_down(foreColorSpeed, MIN_COLOR_SPEED, MAX_COLOR_SPEED);
  backColorSpeed = up_or_down(backColorSpeed, MIN_COLOR_SPEED, MAX_COLOR_SPEED);
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
    palette_width = random8(20, 255);
    updatePaletteCenter();
    updatePaletteWidth();
  }
}

void set_all_black() {
  for (int i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = BLACK_COLOR;
    next_frame[i] = BLACK_COLOR;
    leds[i] = BLACK_COLOR;
  }
  FastLED.show();
}

void fill(CHSV color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, color);
  }
}

//
// All On - turns all the pixels on the foreColor
// 
void allOn() {
   fill(Wheel(foreColor));
}

//
// randomly fill in pixels from blank to all on, then takes them away
//
void randomfill() {
  int i, j, save, pos;
  
  pos = cycle % (NUM_LEDS*2);  // Where we are in the show
  
  if (pos == 0) {  // Start of show
    shuffle_leds();
  }

  if (pos > NUM_LEDS) {
    pos = (2 * NUM_LEDS) - pos;
  }
  
  for (i=0; i < NUM_LEDS; i++) {
    if (i < pos) {  
      setPixelColor(shuffle[i], Wheel(foreColor));  // Turning on lights one at a time
    } else { 
      setPixelColor(shuffle[NUM_LEDS-(i % NUM_LEDS)-1], BLACK_COLOR);  // Turning off lights one at a time
    }
  }
}

//
// Shuffle LEDS
//
void shuffle_leds() {
  int i, j, save;
  
  for (i=0; i < NUM_LEDS; i++) {
    shuffle[i] = i; // before shuffle
  }
  for (i=0; i < NUM_LEDS; i++) {  // here's position
    j = random8(NUM_LEDS);         // there's position
    save = shuffle[i];
    shuffle[i] = shuffle[j];       // first swap
    shuffle[j] = save;             // second swap
  }
}

//    
// random colors - turns each pixel on to a random color
//
void randomcolors() {
  if (is_show_start()) {  // Start of show: assign lights to random colors
    for (int i=0; i < NUM_LEDS; i++) {
      shuffle[i] = random8(MAX_COLOR);
    }
  }
  
  // Otherwise, fill lights with their color
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(shuffle[i]));
  }
}

//
// two color - alternates the color of pixels between two colors
//
void twocolor() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % 2) {
      setPixelColor(i, Wheel(foreColor));
    } else {
      setPixelColor(i, Wheel(backColor));
    }
  }
}

//
// Morph Chain - morphs color 1 from position x to color 2 at position x+n
//
void morphChain() {
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t fract = map((i+(cycle % NUM_LEDS)) % NUM_LEDS, 0, NUM_LEDS, 0, 255);
    setPixelColor(NUM_LEDS-i-1, Wheel(interpolate_wrap(foreColor, backColor, fract)));
  }
}

//
// Saw tooth - Fills in pixels with a sawtooth of intensity
//
void sawtooth() {
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint16_t fract = map((i+(cycle % NUM_LEDS)) % NUM_LEDS, 0, NUM_LEDS, 0, 512);
    if (fract >= 256) {
      fract = 512 - fract;  // the subtraction creates the sawtooth
    }
    // "i" will have pattern move up; "NUM_LEDS-i-1'' will have pattern move down
    setPixelColor(NUM_LEDS-i-1, Gradient_Wheel(foreColor, fract));
  }
}

// 
// bands
//
void bands() {
  if (is_show_start()) {
    noise_hue_intense = 0;  // Turn off regular noise
    noise_sat_intense = 0;
    bands_bpm_1 = random8(MIN_BPM, MAX_BPM);
    bands_bpm_2 = random8(MIN_BPM, MAX_BPM);
    band_min_1 = random8(64, 192);
    band_min_2 = random8(64, 192);
  }

  uint8_t wave;
  
  for (int i=0; i < NUM_LEDS; i++) {
    wave = beatsin8(bands_bpm_1, 0, 255, 0, map(i, 0, NUM_LEDS, 0, 255) );
    wave = (wave > band_min_1) ? map(wave, band_min_1, 255, 0, 255) : 0;
    if (wave > 0) {
      setPixelColor(i, CHSV(foreColor, 255, wave) );
    } else {
      setPixelColor(i, BLACK_COLOR);
    }

    wave = beatsin8(bands_bpm_2, 0, 255, 0, map(i, 0, NUM_LEDS, 0, 255) );
    wave = (wave > band_min_2) ? map(wave, band_min_2, 255, 0, 255) : 0;
    if (wave > 0) {
      addPixelColor(NUM_LEDS - i - 1, CHSV(backColor, 255, wave) );
    }
  }
}

//
// noisyshow
//
void noisyshow() {
  if (is_show_start()) {
    noise_hue_intense = 0;  // Turn off regular noise
    noise_sat_intense = 0;
  }
  
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, CHSV(noise[0][i], 255, noise[1][NUM_LEDS-i-1]));
  }
}

//
// lightwave - just one pixel traveling along the chain
//
void lightwave() {
  for (int i=0; i < NUM_LEDS; i++) {
     if (i == cycle % NUM_LEDS) {
       setPixelColor(i, Wheel(foreColor));
     } else {
       setPixelColor(i, BLACK_COLOR);
     }
  }
}

//
// lightrunup -lights fill up one at time
// 
void lightrunup() {
  int pos = cycle % (NUM_LEDS*2);  // Where we are in the show
  if (pos >= NUM_LEDS) {
    pos = (NUM_LEDS*2) - pos;
  }
  
  for (int i=0; i < NUM_LEDS; i++) {
    if (i < pos) {
      setPixelColor(i, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setPixelColor(i, BLACK_COLOR);   // black
    }
  }
}
    
//
// morph_frame
//
void morph_frame() {
   uint8_t fract = map(morph, 0, total_frames-1, 0, 255);  // 0 - 255
   
   for (int i = 0; i < NUM_LEDS; i++) {
     setLEDinterpHSV(i, current_frame[i], next_frame[i], fract);
   }
   check_fades();
   FastLED.show();  // Update the display 
}

//
// push_frame
//
void push_frame() {
  for (int i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = next_frame[i];
  }
}

//
// constrain_palette
//
CHSV constrain_palette(uint8_t i, CHSV color) {
  if (has_noise) {
    color.s = add_noise_to_channel(noise[1][NUM_LEDS - i - 1], color.s, noise_sat_intense);
    color.h = add_noise_to_channel(noise[0][i], color.h, noise_hue_intense);
  }
  color.h = map8(sin8(color.h), palette_center, (palette_center + palette_width) % 256);
  return color;
}

//
// add_noise_to_channel
//
uint8_t add_noise_to_channel(uint8_t noise_amount, uint8_t value, uint8_t noise_intense) {
  int new_value = value + (map8(noise_amount, 0, noise_intense) / 2) - (noise_intense / 2);
  return new_value % 256;
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
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(fade_amount);
    }
  }
}

void setPixelColor(int pos, CHSV color) {
  next_frame[pos] = color;
}

//
// addPixelColor - add color to the existing color in next_frame
//
void addPixelColor(int pos, CHSV c2) {
  CHSV c1 = next_frame[pos];
  
  if (c1.v > c2.v) {
      next_frame[pos] = c1;
    } else {
      next_frame[pos] = c2;
    }
}


//
// IncColor - adds amount to color
//
uint8_t IncColor(uint8_t c, uint8_t amount) {
  return (c + amount ) % MAX_COLOR;
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

//
//  Wheel - Input a hue (0-255) to get a color
//
CHSV Wheel(uint8_t hue)
{
  return Gradient_Wheel(hue, 255);  // 255 = full brightness
}

//
//  Gradient_Wheel - Input a hue and intensity (0-255) to get a CHSV from the palette
//
CHSV Gradient_Wheel(uint8_t hue, uint8_t intensity)
{
  return CHSV(hue, 255, intensity);
}

//
//  setLEDinterpHSV - Set LED i to the interpolate of two HSV colors 
//
void setLEDinterpHSV(int i, CHSV c1, CHSV c2, uint8_t fract)
{ 
  if (c1 == c2) {
    leds[i] = constrain_palette(i, c1);
  } else if (fract == 0) {
    leds[i] = constrain_palette(i, c1);
    return;
  } else if (fract == 255) { 
    leds[i] = constrain_palette(i, c2);
    return;
  } else if (is_black(c1)) {
    leds[i] = constrain_palette(i, c2);
    leds[i].fadeToBlackBy(255 - fract);
    return;
  } else if (is_black(c2)) {
    leds[i] = constrain_palette(i, c1);
    leds[i].fadeToBlackBy(fract);
    return;
  } else {
    leds[i] = constrain_palette(i, CHSV(interpolate_wrap(c1.h, c2.h, fract), 
                                        interpolate(c1.s, c2.s, fract), 
                                        interpolate(c1.v, c2.v, fract) ));
    return;
  }
}

// is_black
boolean is_black(CHSV color) {
  return color == BLACK_COLOR;
}

//
// Interpolate - returns the fractional point from a to b
//
float interpolate(uint8_t a, uint8_t b, uint8_t fract)
{
  return lerp8by8(a, b, fract);
}

//
// Interpolate Wrap - returns the fractional point from a to b, checking both ways around a circle
//
uint8_t interpolate_wrap(uint8_t a, uint8_t b, uint8_t fract)
{
  uint8_t distCCW, distCW;

  if (a >= b) {
    distCW = 256 + b - a;
    distCCW = a - b; 
  } else {
    distCW = b - a;
    distCCW = 256 + a - b;
  }
  if (distCW <= distCCW) {
    return a + map8(fract, 0, distCW);
  } else {
    return a - map8(fract, 0, distCCW);
  }
}

//
// set_noise_parameters - at the end of each show, change the noise parameters
//
void set_noise_parameters() {
  if (!has_noise) { return; }

  noise_hue_intense = random8(MAX_HUE_NOISE);
  noise_sat_intense = random8(MAX_SAT_NOISE);
  uint8_t noise_set = random8(ARRAY_SIZE(noise_param_set) / 2);
  noise_speed = pgm_read_byte_near(noise_param_set + (noise_set * 2));
  noise_scale = pgm_read_byte_near(noise_param_set + (noise_set * 2) + 1);
  
  return;
}

//
// fill_noise - see Examples: FastLed / NoisePlusPalette
// 
void fill_noise() {
  uint8_t dataSmoothing = 0;
  if( noise_speed < 50) {
    dataSmoothing = 200 - (noise_speed * 4);
  }

  for(int i = 0; i < 2; i++) {  // 0 = hue, 1 = sat
    int ioffset = noise_scale * i;
    for(int j = 0; j < NUM_LEDS; j++) {
      int joffset = noise_scale * j;
        
      uint8_t data = inoise8(noise_x + ioffset, noise_y + joffset, noise_z);
  
      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data,16);
      data = qadd8(data,scale8(data,39));
  
      if( dataSmoothing ) {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }
      
      noise[i][j] = data;
    }
  }
  
  noise_z += noise_speed;
  
  // apply slow drift to X and Y, just for visual variation.
  noise_x += noise_speed / 8;
  noise_y -= noise_speed / 16;
}

///////////////
//
//  Simblee portion
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
    return;

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
void updateBrightness() {
  SimbleeForMobile.updateValue(brightness_slider, brightness);
  SimbleeForMobile.updateValue(brightness_textfield, brightness);
}

void updateSpeed() {
  SimbleeForMobile.updateValue(wait_slider, wait);
  SimbleeForMobile.updateValue(wait_textfield, wait);
}

void updateForeColor() {
  SimbleeForMobile.updateValue(foreColor_slider, foreColor);
  SimbleeForMobile.updateColor(foreColor_swatch, getColorFromHue(foreColor));
}

void updateBackColor() {
  SimbleeForMobile.updateValue(backColor_slider, backColor);
  SimbleeForMobile.updateColor(backColor_swatch, getColorFromHue(backColor)); 
}

void updateShows() {
  if (current_show == NUM_SHOWS) {
    current_show = 0;
  } else if (current_show == 255) {
    current_show = NUM_SHOWS - 1;
  }
  SimbleeForMobile.updateValue(show_stepper, current_show);
  SimbleeForMobile.updateValue(show_textfield, current_show);
  set_show(current_show);
}

void updatePaletteCenter() {
  SimbleeForMobile.updateValue(palette_center_slider, palette_center);
  SimbleeForMobile.updateColor(palette_center_swatch, getColorFromHue(palette_center));  
}

void updatePaletteWidth() {
  SimbleeForMobile.updateValue(palette_width_slider, palette_width);
  SimbleeForMobile.updateValue(palette_width_textfield, palette_width);
}

//
//  ui_event - define callbacks
//
void ui_event(event_t &event) {
  eventId = event.id;
  
  if (event.id == brightness_slider || event.id == brightness_textfield) {
    brightness = event.value;
    FastLED.setBrightness( brightness );
    
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
