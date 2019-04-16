#include <FastLED.h>
//
//  Turtle! - 29 Lights in a hexagonal grid
//
/*****************************************************************************/
//
// 2/3/18
//
// FastLED
//

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 10 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 29
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number

CRGB leds[NUM_LEDS];
CHSV current_frame[NUM_LEDS]; // framebuffers
CHSV next_frame[NUM_LEDS];  // framebuffers

uint8_t shuffle[NUM_LEDS];  // For random-fill show (nasty global)

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
uint8_t foreColor =  0;    // Starting foreground color
uint8_t backColor = MAX_COLOR / 2;   // Starting background color
CHSV BLACK = CHSV(0, 0, 0);
#define MIN_COLOR_SPEED 2   // Higher = slower
#define MAX_COLOR_SPEED 10   // Higher = slower
uint8_t foreColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster
uint8_t backColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster

// Palettes
#define CAN_CHANGE_PALETTES false
uint8_t palette_start = 0; // 0-255 (hue)
uint8_t palette_width = 255;  // 255 = full RGB
#define PALETTE_DURATION 300  // seconds between palettes

// Shows
#define NUM_SHOWS 10
uint8_t current_show = 0;
uint8_t current_pattern = 1;

#define SHOW_DURATION 20  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 2   // seconds to fade out
#define FADE_IN_TIME 2    // seconds to fade out

// Plinko
#define NUM_PLINKO 5
int8_t plink[NUM_PLINKO] = { -1,-1,-1,-1,-1 };

// Clocks and time
uint8_t morph = 0;
uint32_t small_cycle = 0;
uint16_t cycle = 0;
uint8_t bands_bpm_1, bands_bpm_2;
uint8_t band_min_1, band_min_2;
#define MIN_BPM 5
#define MAX_BPM 100

// wait times
#define WAIT_DURATION 4 // second between increasing wait time
#define MIN_WAIT   2  // Minimum number of morph steps in a cycle
#define MAX_WAIT  20  // Maximum number of morph steps in a cycle
#define NUM_WAIT_VALUES 10 // Number of stored delay times
uint8_t wait = NUM_WAIT_VALUES / 2;
uint8_t total_frames;

// noise
#define HAVE_NOISE true    // set to false to suppress noise
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

// Bounce
uint8_t bounce_dir = random(6);
uint8_t bounce_pos = random(NUM_LEDS);
uint8_t trail_size = 5;
int8_t trail[] = { -1, -1, -1, -1, -1 };
const uint8_t trail_colors[] = { 205, 145, 85, 45, 5 };
const uint8_t glow_colors[] = { 255, 75, 35 };

int8_t neighbors[][6] = {
  {-1,1,8,9,-1,-1},
  {-1,2,7,8,0,-1},
  {-1,-1,3,7,1,-1},
  {-1,-1,4,6,7,2},
  {-1,-1,-1,5,6,3}, // 4
  {4,-1,-1,14,13,6},
  {3,4,5,13,12,7},
  {2,3,6,12,8,1},
  {1,7,12,11,9,0},
  {0,8,11,10,-1,-1},
  {9,11,18,19,-1,-1},
  {8,12,17,18,10,9},
  {7,6,13,17,11,8}, // 12
  {6,5,14,16,17,12},
  {5,-1,-1,15,16,13},
  {14,-1,-1,24,23,16},
  {13,14,15,23,22,17}, // 16
  {12,13,16,22,18,11},
  {11,17,22,21,19,10},
  {10,18,21,20,-1,-1},
  {19,21,27,-1,-1,-1},
  {18,22,26,27,20,19},
  {17,16,23,26,21,18}, // 22
  {16,15,24,25,26,22},
  {15,-1,-1,-1,25,23},
  {23,24,-1,-1,28,26},
  {22,23,25,28,27,21},
  {21,26,28,-1,-1,20},
  {26,25,-1,-1,-1,27} // 28
};

uint8_t rings[][12] = {
  { 17, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },
  { 12, 13, 16, 22, 18, 11, -1,-1,-1,-1,-1,-1 },
  { 7, 6, 5, 14, 15, 23, 26, 21, 19, 10, 9, 8 },
  { 0, 1, 2, 3, 4, 24, 25, 28, 27, 20, -1, -1 } 
};

#define NUM_PATTERNS 9   // Total number of patterns

const uint8_t PatternMatrix[NUM_PATTERNS][29] = {
    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1 },
    { 1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,1 },
    { 1,1,1,1,1,1,2,2,2,1,1,2,1,2,1,1,2,2,2,1,1,2,1,2,1,1,2,1,1 },
    { 1,1,1,1,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,1,1 },
    { 1,1,1,1,1,2,2,2,2,2,2,1,1,1,2,2,1,1,1,2,1,2,1,2,1,1,2,1,1 },
    { 2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,2 },
    { 1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,1,1,1,2,1,2,1,2,1,1,2,1,1 },
    { 1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,2,1,2,1 } 
};   

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  noise_x = random16();
  noise_y = random16();
  noise_z = random16();
  
  set_all_black();

  total_frames = getNumFrames(wait);
}

//
// loop
//
void loop() { 
                   
  switch (current_show) {
  
    case 0:
      patterns();
      break;
    case 1:
      warp1();
      break;
    case 2:
      warp2();
      break;
    case 3:
      rainbow_show();
      break;
    case 4:
      color_gradient_show();
      break;
    case 5:
      bounce();
      break;
    case 6:
      bounce_glowing();
      break;
    case 7:
      plinko();
      break;
    case 8:
      bands();
      break;
    default:
      noisy_show();
      break;
  }

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay
  
  morph++;
  small_cycle++;
  fill_noise();

  if (small_cycle % foreColorSpeed == 0) { foreColor = IncColor(foreColor, 1); }
  if (small_cycle % backColorSpeed == 0) { backColor = IncColor(backColor, 1); }
  
  if (morph >= total_frames) {  // Finished morphing
    
    morph = 0;
    
    cycle++;  // Advance the cycle clock
    
    push_frame();

//    change_it_up();
  }

  if (small_cycle >= MAX_SMALL_CYCLE) { next_show(); }
  
}

//
// change_it_up
//
void change_it_up() {
//  EVERY_N_SECONDS(PALETTE_DURATION) { change_palette(); }
  EVERY_N_SECONDS(WAIT_DURATION) { 
//    wait = up_or_down(wait, 0, MAX_WAIT);
    wait = (wait + 1) % MAX_WAIT;
    total_frames = getNumFrames(wait);
  }
}

//
// next_show
//
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void next_show() {
  // Switch between a patterns show and all the other shows
  
  if (current_show == 0) {
    current_show = random8(1, NUM_SHOWS);
  } else {
    current_show = 0;
    current_pattern = random8(NUM_PATTERNS);
  }

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
  if (CAN_CHANGE_PALETTES) {
    palette_start = random8(255);
    palette_width = random8(10, 255);
  }
}
  
void set_all_black() {
  for (int i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = BLACK;
    next_frame[i] = BLACK;
    leds[i] = BLACK;
  }
  FastLED.show();
}

void fill(CHSV color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, color);
  }
}

//
// patterns shows
//
void patterns() {
  for (int i=0; i < NUM_LEDS; i++) {
    switch (PatternMatrix[current_pattern][i]) {
      case 0: {        // Off
        setPixelColor(i, BLACK);
        break;
      }
      case 1: {        // always green
        setPixelColor(i, CHSV(96, 255, 255));
        break;
      }
      case 2: {        // the other color
        setPixelColor(i, Wheel(backColor));
        break;
      }
    }
  }
}

//
// unit_test()
//
// visit each light in order and illuminate its neighbors
// ignores morphing and usual show progression
//
void unit_test() {
  for (int i = 0; i < NUM_LEDS; i++) {
    set_all_black();
    leds[i] = Wheel(0);
    
    int8_t *n = neighbors[i];
    for (int j = 0; j < 6; j++) {
      if (n[j] != -1) {
        leds[n[j]] = Wheel(125);
        
        FastLED.show();
        delay(100);
      }
    }
    delay(500);    
  }
}

//
// bounce
//
void bounce() {

  wait = wait % (MAX_WAIT / 4);
  if (morph != 0) {
    return;
  }
  
  fill(CHSV(170,255,2));
  
  for (int i = trail_size - 1; i >= 0; i--) {
    if (trail[i] == -1) continue;
    setPixelColor(trail[i], rgb_to_hsv(CRGB(0, 0, trail_colors[i])));
  }
  setPixelColor(bounce_pos, rgb_to_hsv(CRGB(200, 0, 128)));
  
  int8_t *n = neighbors[bounce_pos];
  uint8_t old_dir = bounce_dir;
  while (n[bounce_dir] == -1 || (bounce_dir + 3) % 6 == old_dir) {
    bounce_dir = random(0,6);
  }
  for (int i = trail_size - 1; i >= 0; i--) {
    trail[i] = trail[i - 1];
  }
  trail[0] = bounce_pos;
  bounce_pos = n[bounce_dir];
}

//
// bounce_glowing
//
void bounce_glowing() {
  
  wait = wait % (MAX_WAIT / 4);
  if (morph != 0) {
    return;
  }
  
  fill(rgb_to_hsv(CRGB(0, 2, 2)));
  
  for (int i = 0; i < 6; i++) {
    int x = neighbors[bounce_pos][i];
    if (x == -1) continue;
    for (int j = 0; j < 6; j++) {
      int xx = neighbors[x][j];
      if (xx == -1) continue;
      setPixelColor(xx, rgb_to_hsv(CRGB(0, glow_colors[2], glow_colors[2])));
    }
  }
  for (int i = 0; i < 6; i++) {
    int x = neighbors[bounce_pos][i];
    if (x == -1) continue;
    setPixelColor(x, rgb_to_hsv(CRGB(0, glow_colors[1], glow_colors[1])));
  }
  setPixelColor(bounce_pos, rgb_to_hsv(CRGB(0, glow_colors[0], glow_colors[0])));
  
  int8_t *n = neighbors[bounce_pos];
  int old_dir = bounce_dir;
  while (n[bounce_dir] == -1 || (bounce_dir + 3) % 6 == old_dir) {
    bounce_dir = random(0,6);
  }
  bounce_pos = n[bounce_dir]; 
}

//
// draw_ring
//
void draw_ring(uint8_t i, CHSV color) {
  uint8_t *r = rings[i];
  for (int j = 0; j < 12; j++) {
    setPixelColor(r[j], color);
  }
}

//
// tunnel vision
//
// Colored ring animating outwards
// color1 is the primary color, colo22 is a trail color
// background is the background color
//
void tunnelvision(CHSV color1, CHSV color2, CHSV background) {  
  int i = cycle % 5;
  if (i < 4) { draw_ring(i, color1); }      
  if (i != 0) { draw_ring(i-1, color2); }
}

//
// warp1 - colors on a black field
// 
void warp1() {
  switch ((cycle / 5) % 6) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0, 255, 0)), rgb_to_hsv(CRGB(0,40,0)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,0,255)), rgb_to_hsv(CRGB(0,0,40)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,255,255)), rgb_to_hsv(CRGB(0,40,40)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(40,40,0)), rgb_to_hsv(CRGB(0,0,0)));  
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,0,0)), rgb_to_hsv(CRGB(40,0,0)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(255,0,255)), rgb_to_hsv(CRGB(40,0,40)), rgb_to_hsv(CRGB(0,0,0)));  
      break;
  }
}

//
// warp2 - colors on a green field
//
void warp2() {
  switch ((cycle / 5) % 8) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0,255,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,200,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,150,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(0,100,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 5:
      tunnelvision(rgb_to_hsv(CRGB(200,200,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    case 6:
      tunnelvision(rgb_to_hsv(CRGB(150,150,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)));
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(100,100,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)));  
      break;
  }
}

//
// rainbow show - distribute a rainbow wheel equally distributed along the chain
//
void rainbow_show() {
  for (uint8_t i = 0; i < HALF_LEDS; i++) {
    uint8_t hue = ((foreColorSpeed * i) + ((backColorSpeed / 4) * cycle)) % MAX_COLOR;
    setPixelColor(i, Wheel(hue));
    setPixelColor(NUM_LEDS - i, Wheel(hue));
  }  
}

//
// color gradient show
//
void color_gradient_show() {
  uint8_t hue = (foreColorSpeed + ((backColorSpeed / 4) * cycle)) % MAX_COLOR;
  uint8_t i = cycle % NUM_LEDS;
  
  setPixelColor(i, Wheel(hue));
  setPixelColor(NUM_LEDS - i, Wheel(hue));
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

  fill(BLACK);
  
  for (int i=0; i < NUM_LEDS; i++) {
    wave = beatsin8(bands_bpm_1, 0, 255, 0, map(i, 0, NUM_LEDS-1, 0, 255) );
    wave = (wave > band_min_1) ? map(wave, band_min_1, 255, 0, 255) : 0;
    if (wave > 0) {
      setPixelColor(i, CHSV(foreColor, 255, wave) );
    }

    wave = beatsin8(bands_bpm_2, 0, 255, 0, map(i, 0, NUM_LEDS-1, 0, 255) );
    wave = (wave > band_min_2) ? map(wave, band_min_2, 255, 0, 255) : 0;
    if (wave > 0) {
//      setPixelColor(NUM_LEDS - i - 1, CHSV(backColor, 255, wave) );
      addPixelColor(NUM_LEDS - i - 1, CHSV(backColor, 255, wave) );
    }
  }
}

//
// noisy_show
//
void noisy_show() {
  if (is_show_start()) {
    noise_hue_intense = 0;  // Turn off regular noise
    noise_sat_intense = 0;
  }
  
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, CHSV(noise[0][i], 255, noise[1][NUM_LEDS-i-1]));
  }
}

//
// plinko
//
void plinko() {
  wait = wait % (MAX_WAIT / 4);
  if (morph != 0) {
    return;
  }
  
  // Move plinko
  uint8_t i, num_choices;
  int8_t choices[3] = { -1, -1, -1 };
  
  for (i = 0; i < NUM_PLINKO; i++) {
    if (plink[i] != -1) {  // is on the board?
      if (true) { // lower the faster the plinko
        num_choices = 0;
  
        for (int dir = 2; dir < 5; dir++) {
          if (neighbors[plink[i]][dir] != -1) {
            choices[num_choices] = neighbors[plink[i]][dir];
            num_choices++;
          }
        }
        
        if (num_choices == 0) {
          plink[i] = -1;  // nowhere to go; put off board
        } else {
          plink[i] = choices[random(num_choices)];
        }
      }
    }
  }

  
  // Start a new plinko
  i = 0;
  while (i < NUM_PLINKO) {
    if (plink[i] == -1) {  // off the board
      plink[i] = random(1,4);  // Start at top
      i = NUM_PLINKO; // Just one at a time
    }
    i++;
  }
  
  // Draw existing plinko
  fill(BLACK);

  for (i = 0; i < NUM_PLINKO; i++) {
    if (plink[i] != -1) {
      setPixelColor(plink[i], Wheel(IncColor(foreColor, 100 * i))); // Draw the plinko
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
  if (HAVE_NOISE) {
    color.s = add_noise_to_channel(noise[1][NUM_LEDS - i - 1], color.s, noise_sat_intense);
    color.h = add_noise_to_channel(noise[0][i], color.h, noise_hue_intense);
  }
  if (CAN_CHANGE_PALETTES) {
    color.h = map8(sin8(color.h), palette_start, (palette_start + palette_width) % 256);
  }
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
  return easeInQuad(wait_value, NUM_WAIT_VALUES-1, MIN_WAIT, MAX_WAIT);
}

//
//  easeInQuad - convert (input - max_input) to (min_output - max_output)
//
uint8_t easeInQuad(uint8_t input, uint8_t max_input, uint8_t min_output, uint8_t max_output){
  float ratio = float(input) / max_input;
  return min_output + (ratio * ratio * (max_output - min_output));
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
    if (c1.v > c2.v) {
      // Can also try: leds[i] = constrain_palette(i, c1);
      leds[i] = constrain_palette(i, CHSV(c1.h, interpolate(c1.s, c2.s, fract), interpolate(c1.v, c2.v, fract)));
    } else {
      // Can also try: leds[i] = constrain_palette(i, c2);
      leds[i] = constrain_palette(i, CHSV(c2.h, interpolate(c1.s, c2.s, fract), interpolate(c1.v, c2.v, fract)));
    }
    return;
  }
}

// is_black
boolean is_black(CHSV color) {
  return color == BLACK;
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
    return a + lerp8by8 (0, distCW, fract);
  } else {
    return a - lerp8by8 (0, distCCW, fract);
  }
}

//
// set_noise_parameters - at the end of each show, change the noise parameters
//
void set_noise_parameters() {
  if (!HAVE_NOISE) { return; }

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

//
// RGB to HSV - save having to translate RGB colors by hand
//
CHSV rgb_to_hsv( CRGB color) {
  float h,s,v;
  float* p_h;
  float* p_s;
  float* p_v;
  p_h = &h;
  p_s = &s;
  p_v = &v;
  
  RGBtoHSV(color.r, color.g, color.b, p_h, p_s, p_v);
  
  return CHSV(byte(h * 255 / 360), byte(s * 255), byte(v * 255));
}

// r,g,b values are from 0 to 255
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
// 
// code from http://www.cs.rit.edu/~ncs/color/t_convert.html

void RGBtoHSV( uint8_t red, uint8_t green, uint8_t blue, float *h, float *s, float *v )
{
  float r = red/float(255);
  float g = green/float(255);
  float b = blue/float(255);
  
  float MIN = min(r, min(g,b));  // min(r,g,b)
  float MAX = max(r, max(g,b));  // max(r,g,b)
 
  *v = MAX;            // v

  float delta = MAX - MIN;

  if (MAX != 0 ) *s = delta / MAX;  // s
  else { // r = g = b = 0   // s = 0, v is undefined
    *s = 0;
    *h = -1;
    return;
  }
  if( r == MAX ) *h = 60.0 * ( g - b ) / delta; // between yellow & magenta
  else {
    if( g == MAX ) {
      *h = 120.0 + 60.0 * ( b - r ) / delta; // between cyan & yellow
    } else {
      *h = 240.0 + 60.0 * ( r - g ) / delta;  // between magenta & cyan
    }
  }
  if( *h < 0 ) *h += 360;
}


