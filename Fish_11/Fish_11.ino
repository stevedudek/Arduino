#include <Noise.h>
#include <FastLED.h>

//
//  Fish
//
//  6/6/18
//
//  FastLED
//
//  2-D
//  Noise from Library
//
#define FISH_NUM   5
#define MAX_FISH   6

#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 61
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number
#define SIMPLE_TAIL false  // simple tails have 5 segments

CRGB leds[NUM_LEDS];
CHSV current_frame[NUM_LEDS]; // framebuffers
CHSV next_frame[NUM_LEDS];  // framebuffers
CHSV interp_frame[NUM_LEDS];  // framebuffers

#define MAX_XCOORD  7
#define MAX_YCOORD 11

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
#define FISH_HUE (FISH_NUM * (FISH_NUM / MAX_COLOR)) // Main fish color: red (https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors)
uint8_t foreColor = FISH_HUE;        // Starting foreground color
uint8_t backColor = MAX_COLOR / 2;   // Starting background color
CHSV BLACK = CHSV(0, 0, 0);
#define MIN_COLOR_SPEED 6    // Higher = slower
#define MAX_COLOR_SPEED 40   // Higher = slower
uint8_t foreColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster
uint8_t backColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster

// Palettes
#define CAN_CHANGE_PALETTES true  // Has strange effects on colors - use with care
uint8_t palette_start = 0; // 0-255 (hue)
uint8_t palette_width = 255;  // 255 = full RGB
#define PALETTE_DURATION 300  // seconds between palettes

// Shows
#define NUM_SHOWS 21
uint8_t current_show = 1;
uint8_t current_pattern = 0;

#define SHOW_DURATION 20  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 1   // seconds to fade out
#define FADE_IN_TIME 1    // seconds to fade out

// Plinko
#define MAX_PLINKO 10
int8_t num_plinko = 5;
int8_t plink[MAX_PLINKO] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1  };

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
#define HAVE_NOISE true   // set to false to suppress noise
#define MAX_HUE_NOISE 128   // 255 max 
#define MAX_SAT_NOISE 128   // 255 max
Noise noise = Noise(MAX_XCOORD, MAX_YCOORD);

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
const uint8_t glow_colors[] = { 255, 105, 55 };
 
const int8_t coords[] PROGMEM = {
  -1,-1, 1, 0,-1,-1,-1,  // 4 (1) - Head
  -1, 8, 7, 6, 5, 4,-1,  // 5
   9,10,11,12,13,14,-1,  // 6
  21,20,19,18,17,16,15,  // 7
  22,23,24,25,26,27,-1,  // 6
  34,33,32,31,30,29,28,  // 7
  35,36,37,38,39,40,-1,  // 6
  -1,45,44,43,42,41,-1,  // 5
  -1,46,47,48,49,-1,-1,  // 4
  -1,-1,52,51,50,-1,-1,  // 3
  -1,53,54,56,58,59,-1,  // 8 (5) - Tail
};

//
// PATTERNS - 8 hex bytes per pattern (see Fish.ipynb)
//
#define NUM_PATTERNS 19   // Total number of patterns
const uint8_t pattern_matrix[][8] = {
  { 0xa, 0xab, 0x56, 0xa5, 0x4a, 0xa9, 0x55, 0x50 },
  { 0xd, 0x99, 0x8c, 0xcc, 0x66, 0x6d, 0xaa, 0xd0 },
  { 0xf2, 0x18, 0x20, 0xc1, 0x6, 0x11, 0x97, 0xf8 },
  { 0xf2, 0x18, 0x71, 0xe5, 0x56, 0xbb, 0xd1, 0xe0 },
  { 0xf2, 0x18, 0x51, 0x24, 0x50, 0x91, 0xa9, 0x20 },
  { 0xf0, 0x7e, 0x3, 0xf0, 0x1f, 0x83, 0xc4, 0xc8 },
  { 0xf, 0xc3, 0x26, 0xd9, 0x30, 0xc6, 0x7f, 0x38 },
  { 0x2, 0x25, 0x25, 0x29, 0x29, 0x12, 0x54, 0xc8 },
  { 0x8, 0xa4, 0x50, 0xc1, 0x6, 0x2a, 0x42, 0x10 },
  { 0x8, 0x99, 0x4, 0xc8, 0x29, 0x12, 0x43, 0x30 },
  
  { 0xfd, 0xdb, 0x7, 0xf8, 0x36, 0xee, 0x46, 0xd8 },
  { 0xf2, 0x19, 0xdf, 0x3e, 0xf9, 0xee, 0x6a, 0x10 },
  { 0xd, 0xe7, 0xdf, 0x3e, 0xf9, 0xee, 0x68, 0x0 },
  { 0xf0, 0x43, 0x8f, 0x3e, 0xff, 0xc6, 0x6c, 0xc8 },
  { 0xf3, 0xf8, 0xf, 0xc, 0xfc, 0x80, 0xe3, 0x30 },
  { 0x3, 0x9d, 0xc7, 0x11, 0xc7, 0x62, 0x19, 0xf8 },
  { 0xff, 0xe7, 0x8e, 0x1c, 0x79, 0xec, 0x7, 0x38 },
  { 0xf7, 0x5b, 0x8c, 0xc, 0x76, 0xb9, 0xbb, 0xf0 },
  { 0xfd, 0xdb, 0x6d, 0xbb, 0x76, 0xed, 0x9c, 0x88 },
};

//
// COLORED_PATTERNS - 16 hex bytes per pattern (see Fish.ipynb)
//
#define NUM_COLORED_PATTERNS 8   // Total number of colored patterns
const uint8_t colored_pattern_matrix[][16] = {
  { 0xff, 0x63, 0x67, 0x25, 0x8d, 0x82, 0x72, 0x8d, 0x8f, 0x27, 0x36, 0x3c, 0x98, 0xc9, 0xc9, 0xc0 },
  { 0xaa, 0x1b, 0xe, 0x40, 0x6, 0xce, 0x40, 0x1, 0xb0, 0xe4, 0x6, 0xc3, 0x9b, 0x0, 0xb9, 0x0 },
  { 0x55, 0xff, 0xb7, 0x7f, 0xbb, 0xf7, 0xff, 0xe7, 0xbe, 0xfb, 0xef, 0x7f, 0x7f, 0xa5, 0xff, 0x80 },
  { 0xaa, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xb9, 0xbe, 0xbf, 0xff, 0x40 },
  { 0x55, 0x0, 0x0, 0x20, 0x0, 0x40, 0xcc, 0x80, 0x3, 0x1, 0x0, 0x2, 0x4, 0x8, 0x53, 0x0 },
  { 0xff, 0xd5, 0xf6, 0x9f, 0x62, 0x76, 0x9, 0x60, 0x26, 0xaa, 0x95, 0x5b, 0xeb, 0x9c, 0xa3, 0x40 },
  { 0xff, 0x55, 0x5f, 0xf7, 0xea, 0xfa, 0xa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1a, 0xa, 0x40 },
  { 0xff, 0xd1, 0xf4, 0x1f, 0x48, 0x7d, 0x7, 0xd2, 0x1f, 0x41, 0xf4, 0x7d, 0x7e, 0xf5, 0xa5, 0x80 },
};

#define NUM_RINGS 6   // Total number of rings
const uint8_t ring_matrix[][8] = {
  { 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0 },
  { 0x0, 0x0, 0x0, 0xc2, 0x86, 0x0, 0x0, 0x0 },
  { 0x0, 0x0, 0x71, 0x24, 0x49, 0x38, 0x0, 0x0 },
  { 0x0, 0x3c, 0x8a, 0x18, 0x30, 0xc7, 0xc0, 0x0 },
  { 0xf, 0xc3, 0x4, 0x0, 0x0, 0x0, 0x38, 0x0 },
  { 0xf0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7, 0xf8 },
};

//
// Pixel neighbors
//
// DIRECTIONS - If looking from head to tail
//
// 0 = Up, 1 = up-right, 2 = down-right, 3 = Down, 4 = down-left, 5 = up-left
//
const int8_t neighbors[] PROGMEM = {
  -1,6,7,-1,-1,-1, // 0 - Same as 1
  -1,6,7,-1,-1,-1, // 1
  -1,6,7,-1,-1,-1, // 2 - Same as 1
  -1,6,7,-1,-1,-1, // 3 - Same as 1
  -1,14,13,5,-1,-1, // 4
  4,13,12,6,-1,-1, // 5
  5,12,11,7,1,-1, // 6
  6,11,10,8,-1,1, // 7
  7,10,9,-1,-1,-1, // 8
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
  -1,58,56,51,48,49, // 50
  50,56,54,52,47,48, // 51
  51,54,-1,-1,46,47, // 52
  60,-1,-1,-1,54,56, // 53
  56,57,55,-1,52,51, // 54
  57,-1,-1,53,-1,54, // 55
  58,59,57,54,51,50, // 56
  59,-1,-1,55,54,56, // 57
  -1,60,59,56,50,-1, // 58
  60,-1,-1,57,56,58, // 59
  -1,-1,-1,59,58,-1, // 60
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
  
  if (HAVE_NOISE) {
    noise.turnNoiseOn();
    noise.setMaxNoise(MAX_HUE_NOISE, MAX_SAT_NOISE);
  } else {
    noise.turnNoiseOff();
  }
  
  set_all_black();

  total_frames = getNumFrames(wait);
}

//
// loop
//
void loop() { 

  switch (current_show) {
  
    case 0:
      patternsBW();
      break;
    case 1:
      patterns2();
      break;
    case 2:
      patterns_four_color();
      break;
    case 3:
      warp1();
      break;
    case 4:
      warp2();
      break;
    case 5:
      sawtooth();
      break;
    case 6:
      color_gradient_show();
      break;
    case 7:
      bounce();
      break;
    case 8:
      bounce_glowing();
      break;
    case 9:
      plinko();
      break;
    case 10:
      bands();
      break;
    case 11:
      vert_back_forth_dots();
      break;
    case 12:
      vert_back_forth_bands();
      break;
    case 13:
      vert_back_forth_colors();
      break;
    case 14:
      horiz_back_forth_dots();
      break;
    case 15:
      horiz_back_forth_bands();
      break;
    case 16:
      horiz_back_forth_colors();
      break;
    case 17:
      diag_back_forth_dots();
      break;
    case 18:
      diag_back_forth_bands();
      break;
    case 19:
      diag_back_forth_colors();
      break;
    default:
      noisy_show();
      break;
  }

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay
  
  morph++;
  small_cycle++;
  noise.fillNoise();

  if (small_cycle % foreColorSpeed == 0) { foreColor = IncColor(foreColor, 1); }
  if (small_cycle % backColorSpeed == 0) { backColor = IncColor(backColor, 1); }
  
  if (morph >= total_frames) {  // Finished morphing
    
    morph = 0;
    
    cycle++;  // Advance the cycle clock
    
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
}

//
// next_show
//
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void next_show() {
  // Switch between a patterns show and all the other shows
  current_pattern = random8(NUM_PATTERNS);
  
  if (current_show < 2) {
    current_show++;
  } else if (current_show == 2) {
    current_show = random8(3, NUM_SHOWS);
  } else {
    current_show = 0;
  }

  morph = 0;
  small_cycle = 0;
  cycle = 0;
  wait = random8(NUM_WAIT_VALUES);
  total_frames = getNumFrames(wait);
  noise.setRandomNoiseParams();
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
void patternsBW() {
  for (int i=0; i < NUM_LEDS; i++) {
    if (get_bit_from_pattern_number(i, current_pattern)) {
      setPixelColor(i, Wheel(FISH_HUE));
    } else {
      setPixelColor(i, Wheel(backColor));
    }
  }
}

void patterns2() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_pattern_number(i, current_pattern)) {
      setPixelColor(i, Wheel(foreColor));
    } else {
      setPixelColor(i, BLACK);
    }
  }
}

void patterns_four_color() {
  for (int i = 0; i < NUM_LEDS; i++) {
    switch (get_two_bits_from_pattern_number(i, current_pattern)) {
      case 0:
        setPixelColor(i, BLACK);
        break;
      case 1:
        setPixelColor(i, Wheel(foreColor));
        break;
      case 2:
        setPixelColor(i, Wheel(backColor));
        break;
      default:
        setPixelColor(i, Wheel(FISH_HUE));
        break;
    }
  }
}

//
// get LED from coord
//
int8_t get_led_from_coord(uint8_t x, uint8_t y) {
  return pgm_read_byte_near(coords + ((y % MAX_YCOORD) * MAX_XCOORD) + (x % MAX_XCOORD));
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
// vert back forth dots - vertical dots moving back and forth
//
void vert_back_forth_dots() {
  int8_t i;
  uint8_t temp_x;

  fill(BLACK);
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      int8_t i = get_led_from_coord(x, y);
      if (i != -1) {
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        if ((temp_x + cycle) % MAX_XCOORD == 0) {
          setPixelColor(i, Wheel(foreColor));
        }
      }
    }
  }
  setPixelColor(1, Wheel(foreColor));  // Light up the head
}

//
// vert back forth bands - vertical bands moving back and forth
//
void vert_back_forth_bands() {
  int8_t i;
  uint8_t temp_x, intensity;
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      int8_t i = get_led_from_coord(x, y);
      if (i != -1) {
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        temp_x = (temp_x + cycle) % MAX_XCOORD;
        intensity = sin8_avr(map(temp_x, 0, MAX_XCOORD, 0, 255) + (cycle % (255 / MAX_XCOORD)) );
        setPixelColor(i, Gradient_Wheel(backColor, intensity));
      }
    }
  }
  setPixelColor(1, Wheel(backColor));  // Light up the head
}

//
// vert back forth colors - vertical colors moving back and forth
//
void vert_back_forth_colors() {
  int8_t i;
  uint8_t temp_x, hue;
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      int8_t i = get_led_from_coord(x, y);
      if (i != -1) {
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        temp_x = (temp_x + cycle) % MAX_XCOORD;
        hue = sin8_avr(map(temp_x, 0, MAX_XCOORD, foreColor, backColor) + (cycle % (255 / MAX_XCOORD)) );
        setPixelColor(i, Wheel(hue));
      }
    }
  }
}

//
// horiz back forth dots - horizontal dots moving back and forth
//
void horiz_back_forth_dots() {
  int8_t i;
  uint8_t temp_y;

  fill(BLACK);
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      int8_t i = get_led_from_coord(x, y);
      if (i != -1) {
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        if ((temp_y + cycle) % MAX_YCOORD == 0) {
          setPixelColor(i, Wheel(foreColor));
        }
      }
    }
  }
  setPixelColor(1, Wheel(foreColor));  // Light up the head
}

//
// horiz back forth bands - horizontal bands moving back and forth
//
void horiz_back_forth_bands() {
  int8_t i;
  uint8_t temp_y, intensity;
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      int8_t i = get_led_from_coord(x, y);
      if (i != -1) {
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        temp_y = (temp_y + cycle) % MAX_YCOORD;
        intensity = sin8_avr(map(temp_y, 0, MAX_YCOORD, 0, 255) + (cycle % (255 / MAX_YCOORD)) );
        setPixelColor(i, Gradient_Wheel(backColor, intensity));
      }
    }
  }
  setPixelColor(1, Wheel(backColor));  // Light up the head
}

//
// horiz back forth colors - horizontal colors moving back and forth
//
void horiz_back_forth_colors() {
  int8_t i;
  uint8_t temp_y, hue;
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      int8_t i = get_led_from_coord(x, y);
      if (i != -1) {
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        temp_y = (temp_y + cycle) % MAX_YCOORD;
        hue = sin8_avr(map(temp_y, 0, MAX_YCOORD, foreColor, backColor) + (cycle % (255 / MAX_YCOORD)) );
        setPixelColor(i, Wheel(hue));
      }
    }
  }
}

//
// diag back forth dots - diagonal dots moving back and forth
//
void diag_back_forth_dots() {
  int8_t i;
  uint8_t temp_x, temp_y;

  fill(BLACK);
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      int8_t i = get_led_from_coord(x, y);
      if (i != -1) {
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        if ((temp_x + temp_y + cycle) % MAX_YCOORD == 0) {
          setPixelColor(i, Wheel(foreColor));
        }
      }
    }
  }
  setPixelColor(1, Wheel(foreColor));  // Light up the head
}

//
// diag back forth bands - diagonal bands moving back and forth
//
void diag_back_forth_bands() {
  int8_t i;
  uint8_t temp_x, temp_y, temp, intensity;
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 1; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      int8_t i = get_led_from_coord(x, y);
      if (i != -1) {
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        temp = (temp_x + temp_y + cycle) % MAX_YCOORD;
        intensity = sin8_avr(map(temp, 0, MAX_YCOORD, 0, 255) + (cycle % (255 / MAX_YCOORD)) );
        setPixelColor(i, Gradient_Wheel(backColor, intensity));
      }
    }
  }
  setPixelColor(1, Wheel(backColor));  // Light up the head
}

//
// diag back forth colors - diagonal colors moving back and forth
//
void diag_back_forth_colors() {
  int8_t i;
  uint8_t temp_x, temp_y, temp, hue;
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      int8_t i = get_led_from_coord(x, y);
      if (i != -1) {
        temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
        temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
        temp = (temp_x + temp_y + cycle) % MAX_YCOORD;
        hue = sin8_avr(map(temp, 0, MAX_YCOORD, foreColor, backColor) + (cycle % (255 / MAX_YCOORD)) );
        setPixelColor(i, Wheel(hue));
      }
    }
  }
}

//
// bounce
//
void bounce() {

  if (is_show_start()) {
    num_balls = random(1, MAX_BALLS);
    wait = wait % (MAX_WAIT / 4);
    total_frames = getNumFrames(wait);
  }
  if (morph != 0) {
    return;
  }
  
  fill(CHSV(170,255,40));  // changed color.v from 2 to 40

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
    wait = wait % (MAX_WAIT / 4);
    total_frames = getNumFrames(wait);
  }
  
  if (morph != 0) {
    return;
  }
  
  fill(rgb_to_hsv(CRGB(0, 40, 40)));  // changed g,b=2 to g,b=40

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
// color1 is the primary color, color2 is a trail color
// background is the background color
//
void tunnelvision(CHSV color1, CHSV color2, CHSV background) {  
  fill(background);
  uint8_t ring = cycle % NUM_RINGS;
  draw_ring(ring, color1);
  if (ring != 0) {
    draw_ring(ring - 1, color2);
  }
}

//
// warp1 - colors on a black field
// 
void warp1() {
  switch ((cycle / NUM_RINGS) % 6) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0, 255, 0)), rgb_to_hsv(CRGB(0,100,0)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,0,255)), rgb_to_hsv(CRGB(0,0,100)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,255,255)), rgb_to_hsv(CRGB(0,100,100)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(100,100,0)), rgb_to_hsv(CRGB(0,0,0)));  
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,0,0)), rgb_to_hsv(CRGB(100,0,0)), rgb_to_hsv(CRGB(0,0,0)));
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(255,0,255)), rgb_to_hsv(CRGB(100,0,100)), rgb_to_hsv(CRGB(0,0,0)));  
      break;
  }
}

//
// warp2 - colors on a green field
//
void warp2() {
  switch ((cycle / NUM_RINGS) % 8) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0,255,100)), rgb_to_hsv(CRGB(0,160,100)), rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,200,100)), rgb_to_hsv(CRGB(0,160,100)), rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,150,100)), rgb_to_hsv(CRGB(0,160,100)), rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(0,100,100)), rgb_to_hsv(CRGB(0,160,100)), rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(160,160,0)), rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 5:
      tunnelvision(rgb_to_hsv(CRGB(200,200,0)), rgb_to_hsv(CRGB(160,160,0)), rgb_to_hsv(CRGB(0,100,0)));
      break;
    case 6:
      tunnelvision(rgb_to_hsv(CRGB(150,150,0)), rgb_to_hsv(CRGB(160,160,0)), rgb_to_hsv(CRGB(0,100,0)));
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(100,100,0)), rgb_to_hsv(CRGB(160,160,0)), rgb_to_hsv(CRGB(0,100,0)));  
      break;
  }
}

//
// Saw tooth - Fills in pixels with a sawtooth of intensity
//
void sawtooth() {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t fract = map(((i * num_balls) + cycle) % NUM_LEDS, 0, NUM_LEDS, 0, 512);
    if (fract >= 256) { fract = 512 - fract; }  // the subtraction creates the sawtooth
    // "i" will have pattern move up; "NUM_LEDS-i-1'' will have pattern move down
    setPixelColor(NUM_LEDS - i - 1, Gradient_Wheel(foreColor, fract));
  }  
}

//
// color gradient show
//
void color_gradient_show() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t fract = map(((i * num_balls) + (cycle % NUM_LEDS)) % NUM_LEDS, 0, NUM_LEDS, 0, 255);
    // "i" will have pattern move up; "NUM_LEDS-i-1'' will have pattern move down
    setPixelColor(i, Wheel(interpolate_wrap(foreColor, backColor, fract)));
  }
}

// 
// bands
//
void bands() {
  if (is_show_start()) {
    bands_bpm_1 = random8(MIN_BPM, MAX_BPM);
    bands_bpm_2 = random8(MIN_BPM, MAX_BPM);
    band_min_1 = random8(64, 192);
    band_min_2 = random8(64, 192);
  }

  uint8_t wave;

  fill(Wheel(backColor));
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    wave = beatsin8(bands_bpm_1, 0, 255, 0, map(i, 0, NUM_LEDS, 0, 255) );
    wave = (wave > band_min_1) ? map(wave, band_min_1, 255, 0, 255) : 0;
    if (wave > 0) {
      setPixelColor(NUM_LEDS-i-1, CHSV(foreColor, 255, wave) );
    }
    wave = beatsin8(bands_bpm_2, 0, 255, 0, map(i, 0, NUM_LEDS, 0, 255) );
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
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      int8_t i = get_led_from_coord(x, y);
      if (i != -1) {    
        setPixelColor(i, CHSV(noise.getScaledNoiseA(x, y), 255, noise.getScaledNoiseB(x, y)));
      }
    }
  }
}

//
// plinko
//
void plinko() {
  if (is_show_start()) {
    num_plinko = random(1, MAX_PLINKO);
    wait = wait % (MAX_WAIT / 3);
  }
  if (morph > 0) {
    return;
  }
  
  // Move plinko
  uint8_t i, num_choices;
  int8_t choices[2] = { -1, -1 };
  
  for (i = 0; i < num_plinko; i++) {
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
  while (i < num_plinko) {
    if (plink[i] == -1) {  // off the board
      plink[i] = 1;   // The fish head
      i = num_plinko; // Just one at a time
    }
    i++;
  }
  
  // Draw existing plinko
  fill(BLACK);

  for (i = 0; i < num_plinko; i++) {
    setPixelColor(plink[i], Wheel(IncColor(foreColor, 5 * i))); // Draw the plinko
  }
}

//
// morph_frame
//
void morph_frame() {
   uint8_t fract = map(morph, 0, total_frames, 0, 255);  // 0 - 255
   
   for (int i = 0; i < NUM_LEDS; i++) {
     interp_frame[i] = getInterpHSV(i, current_frame[i], next_frame[i], fract);
   }

   add_noise();  // Add noise
   update_leds();  // push the interp_frame on to the leds
   check_fades();  // Dim at start + end of shows
   unify_head_tail(); // Make all 4 head pixels the same color
   FastLED.show();  // Update the display 
}

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = interp_frame[i];
  }
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
CHSV constrain_palette(CHSV color) {
  CHSV new_color;
  uint8_t hue;
  if (CAN_CHANGE_PALETTES) {
    hue = map8(sin8(color.h), palette_start, (palette_start + palette_width) % 256);
    new_color = CHSV(hue, color.s, color.v);
  } else {
    new_color = color;
  }
  return new_color;
}

//
// add_noise - add 2D noise across all channels
//
void add_noise() {
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      int8_t i = get_led_from_coord(x, y);
      if (i != -1) {    
        interp_frame[i].h = noise.addNoiseAtoValue(x, y, interp_frame[i].h); 
        interp_frame[i].s = noise.addNoiseBtoValueNoWrap(x, y, interp_frame[i].s);
      }
    }
  }
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

void setPixelColor(int8_t pos, CHSV color) {
  if (pos != -1) {
    next_frame[pos] = color;
  }
}

CHSV getPixelColor(int pos) {
  return next_frame[pos];
}

// 
//  unify head tail - Make the same color all pixels of the head
//
void unify_head_tail() {
  leds[0] = leds[1];  // head
  leds[2] = leds[1];
  leds[3] = leds[1];
  
  if (SIMPLE_TAIL) {
    leds[55] = leds[54]; // tail
    leds[57] = leds[56];
    leds[58] = leds[58];
  }
}

//
// addPixelColor - add color to the existing color in next_frame
//
void addPixelColor(int i, CHSV c2) {
  CHSV c1 = next_frame[i];
  
  if (c1.v > c2.v) {
    setPixelColor(i, c1);
  } else {
    setPixelColor(i, c2);
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
//  getInterpHSV - get the HSV interpolation of two HSV colors 
//
CHSV getInterpHSV(int i, CHSV c1, CHSV c2, uint8_t fract) {
  CHSV color;
  
  if (c1 == c2) {
    color = c1;
  } else if (fract == 0) {
    color = c1;
  } else if (fract == 255) { 
    color = c2;
  } else if (is_black(c1)) {
    color = CHSV(c2.h, c2.s, interpolate(c1.v, c2.v, fract));  // safe approach
  } else if (is_black(c2)) {
    color = CHSV(c1.h, c1.s, interpolate(c1.v, c2.v, fract));  // safe approach
  } else {
    color = CHSV(interpolate_wrap(c1.h, c2.h, fract), interpolate(c1.s, c2.s, fract), interpolate(c1.v, c2.v, fract));
  }
  return constrain_palette(color);
}

// is_black
boolean is_black(CHSV color) {
  return color == BLACK;
}

//
// Interpolate - returns the fractional point from a to b
//
uint8_t interpolate(uint8_t a, uint8_t b, uint8_t fract) {
  return lerp8by8(a, b, fract);
}

//
// Interpolate Wrap - returns the fractional point from a to b, checking both ways around a circle
//
uint8_t interpolate_wrap(uint8_t a, uint8_t b, uint8_t fract) {
  uint8_t distCCW, distCW, answer;

  if (a >= b) {
    distCW = 256 + b - a;
    distCCW = a - b;
  } else {
    distCW = b - a;
    distCCW = 256 + a - b;
  }
  if (distCW <= distCCW) {
    answer = a + map8(fract, 0, distCW);
  } else {
    answer = a - map8(fract, 0, distCCW);
  }
  return answer;
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

uint8_t get_two_bits_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern = pattern_number % NUM_COLORED_PATTERNS;
  uint8_t pattern_byte = colored_pattern_matrix[pattern][(n / 4)];
  return ( (pattern_byte >> (2 * (3 - (n % 4)))) & B00000011 );
}

