#include <FastLED.h>
//
//  Fish
//
//  2/9/18
//
//  FastLED
//
#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 20 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 57
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number

CRGB leds[NUM_LEDS];
CHSV current_frame[NUM_LEDS]; // framebuffers
CHSV next_frame[NUM_LEDS];  // framebuffers

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
uint8_t current_pattern = 0;

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
#define MIN_BPM 10
#define MAX_BPM 50

// wait times
#define WAIT_DURATION 4 // second between increasing wait time
#define MIN_WAIT   4  // Minimum number of morph steps in a cycle
#define MAX_WAIT  20  // Maximum number of morph steps in a cycle
#define NUM_WAIT_VALUES 50 // Number of stored delay times
uint8_t wait = NUM_WAIT_VALUES / 2;
uint8_t total_frames;

// noise
#define HAVE_NOISE true   // set to false to suppress noise
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
#define MAX_BALLS  5
uint8_t num_balls = 2;
uint8_t bounce_dir[] = { random(6), random(6), random(6), random(6), random(6) };
uint8_t bounce_pos[] = { random(NUM_LEDS), random(NUM_LEDS), random(NUM_LEDS), random(NUM_LEDS), random(NUM_LEDS) };
#define trail_size  5
int8_t trail[][5] = {
    { -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1 },
};
const uint8_t trail_colors[] = { 205, 145, 85, 45, 5 };
const uint8_t glow_colors[] = { 255, 125, 55 };

//
// PATTERNS - 8 hex bytes per pattern (see Fish.ipynb)
//
#define NUM_PATTERNS 9   // Total number of patterns
const uint8_t pattern_matrix[][8] = {
  { 0xa, 0xab, 0x56, 0xa5, 0x4a, 0xa9, 0x57, 0x80 },
  { 0xd, 0x99, 0x8c, 0xcc, 0x66, 0x6d, 0xaf, 0x80 },
  { 0xf2, 0x18, 0x20, 0xc1, 0x6, 0x11, 0x97, 0x80 },
  { 0xf2, 0x18, 0x51, 0x24, 0x50, 0x91, 0xa8, 0x0 },
  { 0xf0, 0x6a, 0x3, 0xf0, 0x1f, 0x83, 0xc7, 0x80 },
  { 0xf, 0xc3, 0x26, 0xd9, 0x30, 0xc6, 0x78, 0x0 },
  { 0x2, 0x24, 0x21, 0x21, 0x9, 0x12, 0x50, 0x0 },
  { 0x8, 0xa4, 0x50, 0xc1, 0x6, 0x2a, 0x40, 0x0 },
  { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
};

#define NUM_RINGS 6   // Total number of rings
const uint8_t ring_matrix[][8] = {
  { 0x0, 0x0, 0x20, 0xc1, 0x0, 0x0, 0x0, 0x0 },
  { 0x0, 0x18, 0x51, 0x22, 0x86, 0x0, 0x0, 0x0 },
  { 0x7, 0x24, 0x8a, 0x14, 0x49, 0x38, 0x0, 0x0 },
  { 0xf8, 0xc3, 0x4, 0x8, 0x30, 0xc7, 0xc0, 0x0 },
  { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x38, 0x0 },
  { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7, 0x80 }
};

//
// Pixel neighbors
//
// DIRECTIONS - If looking from head to tail
//
// 0 = Up, 1 = up-right, 2 = down-right, 3 = Down, 4 = down-left, 5 = up-left
//
const int8_t neighbors[] PROGMEM = {
  1,7,8,-1,-1,-1, // 0
  2,6,7,0,-1,-1, // 1
  3,5,6,1,-1,-1, // 2
  -1,4,5,2,-1,-1, // 3
  -1,14,13,5,3,-1, // 4
  4,13,12,6,2,3, // 5
  5,12,11,7,1,2, // 6
  6,11,10,8,0,1, // 7
  7,10,9,-1,-1,0, // 8
  10,20,21,-1,-1,8, // 9
  11,19,20,9,8,7, // 10
  12,18,19,10,7,6, // 11
  13,17,18,11,6,5, // 12
  14,16,17,12,5,4, // 13
  -1,15,16,13,4,-1, // 14
  -1,-1,27,16,14,-1, // 15
  15,27,26,17,13,14, // 16
  16,26,25,18,12,13, // 17
  17,25,24,19,11,12, // 18
  18,24,23,20,10,11, // 19
  19,23,22,21,9,10, // 20
  20,22,-1,-1,-1,9, // 21
  23,33,34,-1,21,20, // 22
  24,32,33,22,20,19, // 23
  25,31,32,23,19,18, // 24
  26,30,31,24,18,17, // 25
  27,29,30,25,17,16, // 26
  -1,28,29,26,16,15, // 27
  -1,-1,40,29,27,-1, // 28
  28,40,39,30,26,27, // 29
  29,39,38,31,25,26, // 30
  30,38,37,32,24,25, // 31
  31,37,36,33,23,24, // 32
  32,36,35,34,22,23, // 33
  33,35,-1,-1,-1,22, // 34
  36,45,-1,-1,34,33, // 35
  37,44,45,35,33,32, // 36
  38,43,44,36,32,31, // 37
  39,42,43,37,31,30, // 38
  40,41,42,38,30,29, // 39
  -1,-1,41,39,29,28, // 40
  -1,-1,49,42,39,40, // 41
  41,49,48,43,38,39, // 42
  42,48,47,44,37,38, // 43
  43,47,46,45,36,37, // 44
  44,46,-1,-1,35,36, // 45
  47,52,-1,-1,45,44, // 46
  48,51,52,46,44,43, // 47
  49,50,51,47,43,42, // 48
  -1,-1,50,48,42,41, // 49
  -1,56,55,51,48,49, // 50
  50,55,54,52,47,48, // 51
  51,54,53,-1,46,47, // 52
  54,-1,-1,-1,-1,52, // 53
  55,-1,-1,53,52,51, // 54
  56,-1,-1,54,51,50, // 55
  -1,-1,-1,55,50,-1, // 56
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
      sawtooth();
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
    
    if (cycle++ > 10000) { cycle = 0; }  // Advance the cycle clock
    
    push_frame();

    change_it_up();
  }

  if (small_cycle >= MAX_SMALL_CYCLE) { next_show(); }
  
}

//
// change_it_up
//
void change_it_up() {
  EVERY_N_SECONDS(PALETTE_DURATION) { 
    change_palette();
  }
  EVERY_N_SECONDS(WAIT_DURATION) { 
    wait = up_or_down(wait, 0, NUM_WAIT_VALUES);
//    wait = (wait + 1) % NUM_WAIT_VALUES;
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
    if (get_bit_from_pattern_number(i, current_pattern)) {
      setPixelColor(i, Wheel(140)); // Green
    } else {
      setPixelColor(i, Wheel(backColor));
    }
  }
}

//
// get_neighbor
//

int8_t get_neighbor(uint8_t i, uint8_t dir) {
  return pgm_read_byte_near(neighbors + (i * 6) + dir);
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
    
    for (int j = 0; j < 6; j++) {
      int8_t neighbor = get_neighbor(i, j);
      if (neighbor != -1) {
        leds[neighbor] = Wheel(125);
        
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

  if (is_show_start()) {
    num_balls = random(1, MAX_BALLS);
  }
  wait = wait % (MAX_WAIT / 4);
  if (morph != 0) {
    return;
  }
  
  fill(CHSV(170,255,20));  // changed v=2 to v=20

  for (int n = 0; n < num_balls; n++) {
    for (int i = trail_size - 1; i >= 0; i--) {
      if (trail[n][i] == -1) continue;
      addPixelColor(trail[n][i], rgb_to_hsv(CRGB(0, 0, trail_colors[i])));
    }
    addPixelColor(bounce_pos[n], rgb_to_hsv(CRGB(200, 0, 128)));
    
    uint8_t old_dir = bounce_dir[n];
    while (get_neighbor(bounce_pos[n], bounce_dir[n]) == -1 || (bounce_dir[n] + 3) % 6 == old_dir) {
      bounce_dir[n] = random(0,6);
    }
    for (int i = trail_size - 1; i >= 0; i--) {
      trail[n][i] = trail[n][i - 1];
    }
    trail[n][0] = bounce_pos[n];
    bounce_pos[n] = get_neighbor(bounce_pos[n], bounce_dir[n]);
  }
}

//
// bounce_glowing
//
void bounce_glowing() {

  if (is_show_start()) {
    num_balls = random(1, MAX_BALLS);
  }
  wait = wait % (MAX_WAIT / 4);
  if (morph != 0) {
    return;
  }
  
  fill(rgb_to_hsv(CRGB(0, 20, 20)));  // changed g,b=2 to g,b=20

  for (int n = 0; n < num_balls; n++) {
    for (int i = 0; i < 6; i++) {
      int x = get_neighbor(bounce_pos[n], i);
      if (x == -1) continue;
      for (int j = 0; j < 6; j++) {
        int xx = get_neighbor(x, j);
        if (xx == -1) continue;
        addPixelColor(xx, rgb_to_hsv(CRGB(0, glow_colors[2], glow_colors[2])));
      }
    }
    for (int i = 0; i < 6; i++) {
      int x = get_neighbor(bounce_pos[n], i);
      if (x == -1) continue;
      addPixelColor(x, rgb_to_hsv(CRGB(0, glow_colors[1], glow_colors[1])));
    }
    addPixelColor(bounce_pos[n], rgb_to_hsv(CRGB(0, glow_colors[0], glow_colors[0])));
    
    int old_dir = bounce_dir[n];
    while (get_neighbor(bounce_pos[n], bounce_dir[n]) == -1 || (bounce_dir[n] + 3) % 6 == old_dir) {
      bounce_dir[n] = random(0,6);
    }
    bounce_pos[n] = get_neighbor(bounce_pos[n], bounce_dir[n]);
  }
}


//
// draw_ring
//
void draw_ring(uint8_t ring_n, CHSV color) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_ring(i, ring_n % NUM_RINGS) == true) {
      setPixelColor(i, color);
    }
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
  uint8_t i = cycle % NUM_RINGS;
  if (i < NUM_RINGS) { draw_ring(i, color1); }      
  if (i != 0) { draw_ring(i-1, color2); }
}

//
// warp1 - colors on a black field
// 
void warp1() {
  switch ((cycle / NUM_RINGS) % 6) {
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
  switch ((cycle / NUM_RINGS) % 8) {
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
// Saw tooth - Fills in pixels with a sawtooth of intensity
//
void sawtooth() {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t fract = map((i+(cycle % NUM_LEDS)) % NUM_LEDS, 0, NUM_LEDS - 1, 0, 512);
    if (fract >= 256) {
      fract = 512 - fract;  // the subtraction creates the sawtooth
    }
    // "i" will have pattern move up; "NUM_LEDS-i-1'' will have pattern move down
    setPixelColor(NUM_LEDS-i-1, Gradient_Wheel(foreColor, fract));
  }  
}

//
// color gradient show - fixed?
//
void color_gradient_show() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t fract = map((i+(cycle % NUM_LEDS)) % NUM_LEDS, 0, NUM_LEDS-1, 0, 255);
    setPixelColor(i, Wheel(interpolate_wrap(foreColor, backColor, fract)));
  }
}

// 
// bands - FIX! play with parameters
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
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    wave = beatsin8(bands_bpm_1, 0, 255, 0, map(i, 0, NUM_LEDS-1, 0, 255) );
    wave = (wave > band_min_1) ? map(wave, band_min_1, 255, 0, 255) : 0;
    if (wave > 0) {
      setPixelColor(i, CHSV(foreColor, 255, wave) );
    } else {
      setPixelColor(i, BLACK);
    }

    wave = beatsin8(bands_bpm_2, 0, 255, 0, map(i, 0, NUM_LEDS-1, 0, 255) );
    wave = (wave > band_min_2) ? map(wave, band_min_2, 255, 0, 255) : 0;
    if (wave > 0) {
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
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
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
  int8_t choices[2] = { -1, -1 };
  
  for (i = 0; i < NUM_PLINKO; i++) {
    int8_t pos = plink[i];
    if (pos != -1) {  // is on the board?
      if (true) { // lower the faster the plinko
        num_choices = 0;
  
        for (int dir = 1; dir < 3; dir++) {
          if (get_neighbor(pos, dir) != -1) {
            choices[num_choices] = get_neighbor(pos, dir);
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
      plink[i] = random(0,4);  // Start at top
      i = NUM_PLINKO; // Just one at a time
    }
    i++;
  }
  
  // Draw existing plinko
  fill(BLACK);

  for (i = 0; i < NUM_PLINKO; i++) {
    if (plink[i] != -1) {
      setPixelColor(plink[i], Wheel(IncColor(foreColor, 5 * i))); // Draw the plinko
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
// constrain_palette - strange side effects - avoid until better tested
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
  return map(wait_value, 0, NUM_WAIT_VALUES-1, MIN_WAIT, MAX_WAIT);
}

//
//  Wheel - Input a hue (0-255) to get a color
//
CHSV Wheel(uint8_t hue) {
  return Gradient_Wheel(hue, 255);  // 255 = full brightness
}

//
//  Gradient_Wheel - Input a hue and intensity (0-255) to get a CHSV from the palette
//
CHSV Gradient_Wheel(uint8_t hue, uint8_t intensity) {
  return CHSV(hue, 255, intensity);
}

//
//  setLEDinterpHSV - Set LED i to the interpolate of two HSV colors 
//
void setLEDinterpHSV(int i, CHSV c1, CHSV c2, uint8_t fract) {
   
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
float interpolate(uint8_t a, uint8_t b, uint8_t fract) {
  return lerp8by8(a, b, fract);
}

//
// Interpolate Wrap - returns the fractional point from a to b, checking both ways around a circle
//
uint8_t interpolate_wrap(uint8_t a, uint8_t b, uint8_t fract) {
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
// rgb_to_hsv - save having to translate RGB colors by hand
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

//
// RGBtoHSV
// 
// r,g,b values are from 0 to 255
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
// 
// code from http://www.cs.rit.edu/~ncs/color/t_convert.html
//
void RGBtoHSV( uint8_t red, uint8_t green, uint8_t blue, float *h, float *s, float *v ) {
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

boolean get_bit_from_ring(uint8_t n, uint8_t ring_number) {
  uint8_t pattern_byte = ring_matrix[ring_number][(n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern_byte = pattern_matrix[pattern_number][(n / 8)];
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

