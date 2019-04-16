#include <FastLED.h>
//
//  Feather Cape - 38 Petals on a cape
//
//  4/13/18
//
//  FastLED
//
//  Has a brightness setting: BRIGHTNESS
//
/*****************************************************************************/

#define BRIGHTNESS  255  // (0-255)
boolean ONLY_RED = false; // true = use only red colors

#define DELAY_TIME 40  // in milliseconds - Higher for slower shows

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 38
#define MAX_XCOORD 11
#define MAX_YCOORD 4

CRGB leds[NUM_LEDS];
CHSV current_frame[NUM_LEDS]; // framebuffers
CHSV next_frame[NUM_LEDS];  // framebuffers
CHSV interp_frame[NUM_LEDS];  // framebuffers

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
#define MAIN_COLOR 120
uint8_t foreColor = MAIN_COLOR;  // Starting foreground color
uint8_t backColor = 0;  // Starting background color
CHSV BLACK = CHSV(0, 0, 0);
#define MIN_COLOR_SPEED 6    // Higher = slower
#define MAX_COLOR_SPEED 40   // Higher = slower
uint8_t foreColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster
uint8_t backColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster

// Palettes
#define CAN_CHANGE_PALETTES false  // Has strange effects on colors - use with care
uint8_t palette_start = 0; // 0-255 (hue)
uint8_t palette_width = 255;  // 255 = full RGB
#define PALETTE_DURATION 300  // seconds between palettes

// Shows
#define NUM_SHOWS 21
uint8_t curr_show = 0; // Starting show
uint8_t current_pattern = 0;

#define SHOW_DURATION 20  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 2   // seconds to fade out
#define FADE_IN_TIME 2    // seconds to fade out

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
uint8_t noise_hue_intense = MAX_HUE_NOISE / 2;
uint8_t noise_sat_intense = MAX_SAT_NOISE / 2;
static uint16_t noise_x;
static uint16_t noise_y;
static uint16_t noise_z;
uint16_t noise_speed = 20; // speed is set dynamically once we've started up
uint16_t noise_scale = 30; // scale is set dynamically once we've started up
uint8_t noise[MAX_XCOORD][MAX_YCOORD];
uint8_t shuffle[NUM_LEDS];  // Will contain a shuffle of lights

const uint8_t noise_param_set[] PROGMEM = 
    { 0,0, 20,30, 10,50, 8,120, 4,30, 8,50, 20,90, 20,30, 20,20, 50,50, 90,90, 30,20 };

// Plinko
#define MAX_PLINKO 10
int8_t num_plinko = 5;
int8_t plink[MAX_PLINKO] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1  };

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

  
// Lookup tables

const int8_t coords[] PROGMEM = {
     0,  1,  2, -1,
   3,  4,  5,  6,
     7,  8,  9, -1,
  10, 11, 12, 13,
    14, 15, 16, -1,
  17, 18, 19, 20,
    21, 22, 23, -1,
  24, 25, 26, 27,
    28, 29, 30, -1,
  31, 32, 33, 34,
    35, 36, 37, -1
};

const int8_t PetalLookUp[] PROGMEM = {
     0,  1,  2,
   7,  6,  5,  4,
     9, 10, 11,
  16, 15, 14, 13,
    18, 19, 20,
  25, 24, 23, 22,
    27, 28, 29,
  34, 33, 32, 31,
    36, 37, 38,
  43, 42, 41, 40,
    45, 46, 47
};

#define NUM_SPACERS 10
uint8_t SpacerPixels[NUM_SPACERS] = { 3, 8, 12, 17, 21, 26, 30, 35, 39, 44 };

const int8_t neighbors[] PROGMEM = {
  -1,1,4,3,-1,-1, // 0
  -1,2,5,4,0,-1,
  -1,-1,6,5,1,-1,
  0,4,7,-1,-1,-1,
  1,5,8,7,3,0, // 4
  2,6,9,8,4,1,
  -1,-1,-1,9,5,2,
  4,8,11,10,-1,3,
  5,9,12,11,7,4, // 8
  6,-1,13,12,8,5,
  7,11,14,-1,-1,-1,
  8,12,15,14,10,7,
  9,13,16,15,11,8, // 12
  -1,-1,-1,16,12,9,
  11,15,18,17,-1,10,
  12,16,19,18,14,11,
  13,-1,20,19,15,12, // 16
  14,18,21,-1,-1,-1,
  15,19,22,21,17,14,
  16,20,23,22,18,15,
  -1,-1,-1,23,19,16, // 20
  18,22,25,24,-1,17,
  19,23,26,25,21,18,
  20,-1,27,26,22,19,
  21,25,28,-1,-1,-1, // 24
  22,26,29,28,24,21,
  23,27,30,29,25,22,
  -1,-1,-1,30,26,23,
  25,29,32,31,-1,24, // 28
  26,30,33,32,28,25,
  27,-1,34,33,29,26,
  28,32,35,-1,-1,-1,
  29,33,36,35,31,28, // 32
  30,34,37,36,32,29,
  -1,-1,-1,37,33,30,
  32,36,-1,-1,-1,31,
  33,37,-1,-1,35,32, // 36
  34,-1,-1,-1,36,33,
};

#define NUM_PATTERNS 8   // Total number of patterns
// Patterns
//
// 0 = Off
// 1 = Color 1
// 2 = Color 2

uint8_t PatternMatrix[NUM_PATTERNS][38] = {
  {  1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1,
   2,  2,  2,  2,
     1,  1,  1    },
  
  {  1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1,
   1,  2,  1,  2,
     1,  2,  1    },
     
  {  2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2    },
  
  {  1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2,
   2,  1,  1,  2,
     1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2,
   2,  1,  1,  2,
     1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2    },
  
  {  1,  1,  1,
   1,  1,  1,  1,
     1,  2,  1,
   1,  2,  2,  1,
     2,  1,  2,
   2,  1,  1,  2,
     2,  1,  2,
   1,  2,  2,  1,
     1,  2,  1,
   1,  1,  1,  1,
     1,  1,  1    },
     
  {  1,  2,  1,
   2,  1,  2,  1,
     2,  1,  2,
   1,  2,  1,  2,
     1,  2,  1,
   2,  1,  2,  1,
     2,  1,  2,
   1,  2,  1,  2,
     1,  2,  1,
   2,  1,  2,  1,
     2,  1,  2    },
   
  {  1,  2,  1,
   1,  2,  1,  2,
     2,  1,  2,
   2,  1,  2,  1,
     1,  2,  1,
   1,  2,  1,  2,
     2,  1,  2,
   2,  1,  2,  1,
     1,  2,  1,
   1,  2,  1,  2,
     2,  1,  2    },
  
  {  1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1,
   1,  1,  1,  1,
     1,  1,  1    },
     
};

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");  

  // 10 spacer pixels + 38 petals
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, 48);
  FastLED.setBrightness( BRIGHTNESS );
  
  noise_x = random16();
  noise_y = random16();
  noise_z = random16();
  
  set_all_black();

  total_frames = getNumFrames(wait);
}

void loop() { 
  
  switch(curr_show) {      
    
    case 0:    
      patterns();
      break;
    case 1:
      randomfill();
      break;
    case 2:
      randomcolors();
      break;
    case 3:
      twocolor();
      break;
    case 4:
      sawtooth();
      break;
    case 5:
      color_gradient_show();
      break;
    case 6:
      bounce();
      break;
    case 7:
      bounce_glowing();
      break;
    case 8:
      plinko();
      break;
    case 9:
      bands();
      break;
    case 10:
      vert_back_forth_dots();
      break;
    case 11:
      vert_back_forth_bands();
      break;
    case 12:
      vert_back_forth_colors();
      break;
    case 13:
      horiz_back_forth_dots();
      break;
    case 14:
      horiz_back_forth_bands();
      break;
    case 15:
      horiz_back_forth_colors();
      break;
    case 16:
      diag_back_forth_dots();
      break;
    case 17:
      diag_back_forth_bands();
      break;
    case 18:
      diag_back_forth_colors();
      break;
    case 19:
      noisy_show();
      break;
    default:
      scales();
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
  if (curr_show == 0) {
    curr_show = random8(1, NUM_SHOWS);
  } else {
    curr_show = 0;
    current_pattern = random8(NUM_PATTERNS);
  }

  morph = 0;
  small_cycle = 0;
  cycle = 0;
  wait = random8(NUM_WAIT_VALUES);
  total_frames = getNumFrames(wait);
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
    //leds[lookup_petal(i)] = BLACK;
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
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    
    switch (PatternMatrix[current_pattern][i]) {
      case 0: {        // Off (black)
        setPixelColor(i, BLACK);
        break;
      }
      case 1: {        // MAIN_COLOR
        setPixelColor(i, Wheel(MAIN_COLOR));
        break;
      }
      case 2: {        // The other color
        setPixelColor(i, Wheel(foreColor));
        break;
      }
    }
  }
}

//
// get LED from coord
//
int8_t get_led_from_coords(uint8_t x, uint8_t y) {
  return pgm_read_byte_near(coords + ((y % MAX_YCOORD) * MAX_XCOORD) + (x % MAX_XCOORD));
}

//
// lookup_petal - convert petlas into LEDs
//
int8_t lookup_petal(uint8_t i) {
  return pgm_read_byte_near(PetalLookUp + i);
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
    leds[lookup_petal(i)] = Wheel(0);
    
    for (int j = 0; j < 6; j++) {
      int8_t neighbor = get_neighbor(i, j);
      if (neighbor != -1) {
        leds[lookup_petal(neighbor)] = Wheel(125);
        
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
      int8_t i = get_led_from_coords(x, y);
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
      int8_t i = get_led_from_coords(x, y);
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
      int8_t i = get_led_from_coords(x, y);
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
      int8_t i = get_led_from_coords(x, y);
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
      int8_t i = get_led_from_coords(x, y);
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
      int8_t i = get_led_from_coords(x, y);
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
      int8_t i = get_led_from_coords(x, y);
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
      int8_t i = get_led_from_coords(x, y);
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
      int8_t i = get_led_from_coords(x, y);
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
    noise_hue_intense = 0;  // Turn off regular noise
    noise_sat_intense = 0;
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

// random fill
//
// randomfill: randomly fills in pixels from blank to all on
// then takes them away random until blank
//
void randomfill() {
  uint8_t i, j, save, pos;
  
  pos = cycle % (NUM_LEDS * 2);  // Where we are in the show
  if (pos >= NUM_LEDS) {
    pos = (NUM_LEDS * 2) - pos;  // For a sawtooth effect
  }
  
  if (pos == 0) {  // Start of show
    shuffle_lights();
  }
  
  for (i=0; i < NUM_LEDS; i++) {
    if (i < pos) {  
      setPixelColor(shuffle[i], Wheel(foreColor));  // Turning on lights one at a time
    } else { 
      setPixelColor(shuffle[i], BLACK);  // Turning off lights one at a time
    }
  }
}

//
// scales
//
void scales() {
  if (cycle == 0) {
    shuffle_lights();
  }
  fill(Wheel(MAIN_COLOR));
  
  int num_scales = (shuffle[0] / 3) + 3;
  for (int i=0; i < num_scales; i++) {
    setPixelColor(shuffle[i], Wheel(foreColor));
  }
}

//
// shuffle_lights
//
void shuffle_lights() {
  // Shuffle sort to determine order to turn on lights
  uint8_t i,j, save;
  
  for (i=0; i < NUM_LEDS; i++) {
    shuffle[i] = i; // before shuffle
  }
  for (i=0; i < NUM_LEDS; i++) {  // here's position
    j = random(NUM_LEDS);         // there's position
    save = shuffle[i];
    shuffle[i] = shuffle[j];       // first swap
    shuffle[j] = save;             // second swap
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
  }
  wait = wait % (MAX_WAIT / 4);
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
// random colors - turns each pixel on to a random color
//
void randomcolors() {
  int i;
  
  if (cycle == 0) {  // Start of show: assign lights to random colors
    for (i=0; i < NUM_LEDS; i++) {
      shuffle[i] = random(MAX_COLOR);
    }
  }
  
  // Otherwise, fill lights with their color
  for (i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(shuffle[i]));
  }
}

//
// two color - alternates the color of pixels between two colors
//
void twocolor() {
  for (int i=0; i < NUM_LEDS; i++) {
    if (i % 2) {
      setPixelColor(i, Wheel(foreColor));
    } else {
      setPixelColor(i, Wheel(backColor));
    }
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
   turn_off_spacers();  // Make sure spacer pixels are off
   FastLED.show();  // Update the display 
}

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // leds[i] = interp_frame[lookup_petal(i)]; // Might need this one instead
    leds[lookup_petal(i)] = interp_frame[i]; // Might need this one instead
  }
}

//
// turn_off_spacers
//
void turn_off_spacers() {
  for (uint8_t i = 0; i < NUM_SPACERS; i++) {
    leds[SpacerPixels[i]] = BLACK;
  }
}

//
// push_frame
//
void push_frame() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = next_frame[i];
  }
}

//
// constrain_palette - strange side effects - avoid until better tested
//
CHSV constrain_palette(CHSV color) {
  CHSV new_color;
  uint8_t hue;
  if (ONLY_RED) {
    hue = map8(sin8(color.h), 200, 70);
    new_color = CHSV(hue, color.s, color.v);
  } else if (CAN_CHANGE_PALETTES) {
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
      int8_t i = get_led_from_coords(x, y);
      if (i != -1) {
        interp_frame[i].s = add_noise_to_channel(noise[x][y], interp_frame[i].s, noise_sat_intense);
        interp_frame[i].h = add_noise_to_channel(noise[MAX_XCOORD-x-1][MAX_YCOORD-y-1], interp_frame[i].h, noise_hue_intense);
      } 
    }
  }
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
      leds[lookup_petal(i)].fadeToBlackBy(fade_amount);
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
  uint8_t v;
  
  if (c1 == c2) {
    color = c1;
  } else if (fract == 0) {
    color = c1;
  } else if (fract == 255) { 
    color = c2;
  } else if (is_black(c1)) {
    color = CHSV(c2.h, c2.s, interpolate(c1.v, c2.v, fract));  // safe approach
//    color = CHSV(c2.h, c2.s, dim8_raw(scale8(c2.v, fract)); // try this too
  } else if (is_black(c2)) {
    color = CHSV(c1.h, c1.s, interpolate(c1.v, c2.v, fract));  // safe approach
//    color = CHSV(c1.h, c1.s, dim8_raw(scale8(c1.v, 255 - fract)); // try this too
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
float interpolate(uint8_t a, uint8_t b, uint8_t fract) {
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

  for(int i = 0; i < MAX_XCOORD; i++) {
    int ioffset = noise_scale * i;
    for(int j = 0; j < MAX_YCOORD; j++) {
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
