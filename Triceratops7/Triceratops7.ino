#include <FastLED.h>
//
//  Triceratops - 45 cones on a coat
//
//  5/13/17
//
//  FastLED
//

#define BRIGHTNESS  128 // (0-255)
boolean ONLY_RED = false; // true = use only red colors

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define BODY_LEDS 42
#define NUM_LEDS 45
#define NUM_ROWS 17
#define MAX_XCOORD 4
#define MAX_YCOORD 17

CRGB leds[NUM_LEDS];
CHSV current_frame[NUM_LEDS]; // framebuffers
CHSV next_frame[NUM_LEDS];  // framebuffers
CHSV interp_frame[NUM_LEDS];  // framebuffers
uint8_t shuffle[NUM_LEDS];  // Will contain a shuffle of lights
// Light colors
#define MAX_COLOR 256   // Colors are 0-255
uint8_t foreColor = 0;        // Starting foreground color
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
#define NUM_PLINKO 5
uint8_t plink_x[NUM_PLINKO] = { 0,0,0,0,0 };
uint8_t plink_y[NUM_PLINKO] = { 18,18,18,18,18 };

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
uint8_t noise_speed_coeff = 1; // divider coefficient to slow down noise speed; Jason thinks we're too fast
uint16_t noise_speed = 5; // speed is set dynamically once we've started up
uint8_t noise_scale_coeff = 1; // multiplier coefficient to increase noise volume
uint16_t noise_scale = 50; // scale is set dynamically once we've started up
uint8_t noise[MAX_XCOORD][MAX_YCOORD];
const uint8_t noise_param_set[] PROGMEM = 
    { 0,0, 20,30, 10,50, 8,120, 4,30, 8,50, 20,90, 20,30, 20,20, 50,50, 90,90, 30,20 };

// Bounce
uint8_t bounce_dir = random(0,6);
uint8_t bounce_pos = random(0,NUM_LEDS);
#define trail_size  5
int8_t trail[] = { -1, -1, -1, -1, -1 };
const uint8_t trail_colors[] = { 205, 145, 85, 45, 5 };
const uint8_t glow_colors[] = { 255, 105, 55 };

// Lookup tables

const uint8_t ConeLookUp[45] PROGMEM = {
      44,
    42, 43,
  41, 40, 32,
    39, 33,
  38, 34, 31,
    35, 30,
  36, 29, 20,
37, 28, 21, 19,
  27, 22, 18,
26, 23, 17,  9,
  24, 16,  8,
25, 15,  7, 10,
  14,  6, 11,
     5, 12,
   4, 13,  0,
     3,  1,
       2
};

const uint8_t ConeGrid[] PROGMEM = {
 45,    44,   45,45,
 45,  42, 43,    45,
    41, 40, 32,  45,
 45,  39, 33,    45,
    38, 34, 31,  45,
 45,  35, 30,    45,
    36, 29, 20,  45,
  37, 28, 21, 19,
    27, 22, 18,  45,
  26, 23, 17,  9,
    24, 16,  8,  45,
  25, 15,  7, 10,
    14,  6, 11,  45,
 45,   5, 12,    45,
     4, 13,  0,  45,
 45,   3,  1,    45,
 45,     2,   45,45,
};

const int8_t neighbors[] PROGMEM = {
  88,88,88,1,13,12, // 0
  0,88,88,2,3,13,
  1,88,88,88,88,3,
  13,1,2,88,88,4,
  5,13,3,88,88,88, // 4
  6,12,13,4,88,14,
  7,11,12,5,14,15,
  8,10,11,6,15,16,
  9,88,10,7,16,17, // 8
  88,88,88,8,17,18,
  88,88,88,11,7,8,
  10,88,88,12,6,7,
  11,88,0,13,5,6, // 12
  12,0,1,3,4,5,
  15,6,5,88,88,25,
  16,7,6,14,25,24,
  17,8,7,15,24,23, // 16
  18,9,8,16,23,22,
  19,88,9,17,22,21,
  88,88,88,18,21,20,
  88,88,19,21,29,30, // 20
  20,19,18,22,28,29,
  21,18,17,23,27,28,
  22,17,16,24,26,27,
  23,16,15,25,88,26, // 24
  24,15,14,88,88,88,
  27,23,24,88,88,88,
  28,22,23,26,88,37,
  29,21,22,27,37,36, // 28
  30,20,21,28,36,35,
  31,88,20,29,35,34,
  88,88,88,30,34,33,
  88,88,88,33,40,43, // 32
  32,88,31,34,39,40,
  33,31,30,35,38,39,
  34,30,29,36,88,38,
  35,29,28,37,88,88, // 36
  36,28,27,88,88,88,
  39,34,35,88,88,88,
  40,33,34,38,88,41,
  43,32,33,39,41,42, // 40
  42,40,39,88,88,88,
  44,43,40,41,88,88,
  88,88,32,40,42,44,
  88,88,43,42,88,88, // 44
};

byte PatternLookUp[45] = { 41,43,44,42,39,37,35,32,29,26,
                           33,36,38,40,34,31,28,25,22,19,
                           15,18,21,24,27,30,23,20,17,14,
                           12,10,5,7,9,11,13,16,8,6,
                           4,3,2,1,0 };

byte Stripe_Pattern[45] = {
       0,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
     0,  0,
   0,  1,  0,
     0,  0,
       0
};

byte Section_Pattern[45] = {
       2,
     2,  2,
   0,  0,  0,
     0,  0,
   0,  0,  0,
     0,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
 0,  1,  1,  0,
   0,  1,  0,
     0,  0,
   2,  0,  2,
     2,  2,
       2
};

byte Explode_Pattern[45] = {
       5,
     5,  5,
   5,  4,  5,
     4,  4,
   4,  3,  4,
     3,  3,
   3,  2,  3,
 3,  1,  1,  3,
   2,  0,  2,
 3,  1,  1,  3,
   3,  2,  3,
 4,  3,  3,  4,
   4,  3,  4,
     4,  4,
   5,  4,  5,
     5,  5,
       5
};

byte Alternate_Pattern[45] = {
       0,
     1,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
     0,  1,
   1,  0,  0,
 0,  0,  1,  0,
   1,  0,  0,
 1,  0,  1,  0,
   1,  0,  0,
 0,  0,  0,  1,
   0,  1,  0,
     0,  0,
   1,  0,  1,
     0,  0,
       1
};

byte SideSide_Pattern[45] = {
       3,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
 0,  2,  4,  6,
   1,  3,  5,
     2,  4,
   1,  3,  5,
     2,  4,
       3
};

byte Diag_Pattern[45] = {
       0,
     1,  0,
   2,  1,  0,
     2,  1,
   3,  2,  1,
     3,  2,
   4,  3,  2,
 5,  4,  3,  2,
   5,  4,  3,
 6,  5,  4,  3,
   6,  5,  4,
 7,  6,  5,  4,
   7,  6,  5,
     7,  6,
   8,  7,  6,
     8,  7,
       8
};

byte ConeSize[45] = { 5,5,5,5,5,3,5,1,3,5,1,1,5,4,3,4,5,5,6,3,6,2,1,6,6,4,2,6,2,1,3,4,2,4,4,5,3,2,2,3,1,6,3,3,1 };

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

void loop() { 
   
  switch(current_show) {
    
    case 0:    
      allOn();
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
      lightwave();
      break;
    case 5:
      lightrunup();
      break;
    case 6:
      colorsize();
      break;
    case 7:
      brightsize();
      break;
    case 8:
      stripe();
      break;
    case 9:
      alternate();
      break;
    case 10:
      diagcolor();
      break;
    case 11:
      sidesidecolor();
      break;
    case 12:
      explodecolor();
      break;
    case 13:
      diagbright();
      break;
    case 14:
      sidesidebright();
      break;
    case 15:
      explodebright();
      break;
    case 16:
      plinko();
      break;
    case 17:
      bounce();
      break;
    case 18:
      bounce_glowing();
      break;    
    default:
      sectioncolor();
      break;
  }
  
  // Morph the display
  
  morph_frame();
  
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
  current_show = random(NUM_SHOWS);

  morph = 0;
  small_cycle = 0;
  cycle = 0;
  wait = random(NUM_WAIT_VALUES);
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
    palette_start = random(255);
    palette_width = random(10, 255);
  }
}
  
void set_all_black() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = BLACK;
    next_frame[i] = BLACK;
    leds[i] = BLACK;
  }
  FastLED.show();
}

void fill(CHSV color) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, color);
  }
}

//
// All On - turns all the pixels on to one color
// 
void allOn() {
   fill(Wheel(foreColor));
}

//
// random fill - randomly fills in pixels from blank to all on
// then takes them away random until blank
//
void randomfill() {
  uint8_t i, j, save, pos;
  
  pos = cycle % (BODY_LEDS*2);  // Where we are in the show

  if (pos == 0) {  // Start of show
    shuffle_leds();
  }
  
  if (pos > BODY_LEDS) {
    pos = (2 * BODY_LEDS) - pos;  // For a sawtooth effect
  }
  
  for (i = 0; i < BODY_LEDS; i++) {
    if (i < pos) {  
      setPixel(shuffle[i], foreColor);  // Turning on lights one at a time
    } else { 
      setPixelBlack(shuffle[i]);  // Turning off lights one at a time
    }
  }
  setHead(foreColor);
}  

//
// shuffle_leds
//
void shuffle_leds() {
  uint8_t i, j, save;
  
  for (i=0; i < BODY_LEDS; i++) {
    shuffle[i] = i; // before shuffle
  }
  for (i=0; i < BODY_LEDS; i++) {  // here's position

    j = random(BODY_LEDS);         // there's position
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
    for (uint8_t i=0; i < NUM_LEDS; i++) {
      shuffle[i] = random(MAX_COLOR);
    }
  }
  
  // Otherwise, fill lights with their color
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    setPixel(i, shuffle[i]);
  }
}

//
// colorsize - light each cone according to its cone size
//
void colorsize() {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    setPixel(i, IncColor(foreColor, ((ConeSize[i]-1) * backColor) % MAX_COLOR));
  }
}

// calcIntensity
//
// Use sine wave + cycle + variable to calculate intensity
float calcIntensity(uint8_t x, uint8_t max_x) {
  return sin(3.14 * ((cycle + x) % max_x) / (max_x - 1));
}

//
// brightsize - light just one cone size
//
void brightsize() {
  for (uint8_t i=0; i < BODY_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(ConeSize[i]-1, 5)));
  }
  setHead(foreColor);
}

void stripe() {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (Stripe_Pattern[PatternLookUp[i]] == 0) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, backColor);
    }
  }
}

void alternate() {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (Alternate_Pattern[PatternLookUp[i]]) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, backColor);
    }
  }
}

void diagcolor() {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    setPixel(i, IncColor(foreColor, (backColor * Diag_Pattern[PatternLookUp[get_led_from_coord(i)]] / 5) % MAX_COLOR));
  }
}

void sectioncolor() {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    setPixel(i, IncColor(foreColor, (backColor * Section_Pattern[PatternLookUp[get_led_from_coord(i)]] / 10) % MAX_COLOR));
  }
}

void sidesidecolor() {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    setPixel(i, IncColor(backColor, (foreColor * SideSide_Pattern[PatternLookUp[get_led_from_coord(i)]]) % MAX_COLOR));
  }
}

void explodecolor() {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    setPixel(i, IncColor(backColor, (foreColor * Explode_Pattern[PatternLookUp[get_led_from_coord(i)]]) % MAX_COLOR));
  }
}

void diagbright() {
  for (uint8_t i=0; i < BODY_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(Diag_Pattern[PatternLookUp[i]], 9)));
  }
  setHead(foreColor);
}

void sidesidebright() {
  for (uint8_t i=0; i < BODY_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(foreColor, calcIntensity(SideSide_Pattern[PatternLookUp[i]], 7)));
  }
  setHead(foreColor);
}

void explodebright() {
  for (uint8_t i=0; i < BODY_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(foreColor, calcIntensity(5 - Explode_Pattern[PatternLookUp[i]], 6)));
  }
}

//
// two color - alternates the color of pixels between two colors
//
void twocolor() {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    if (i % 2) {
      setPixel(i, foreColor);
    } else {
      setPixel(i, backColor);
    }
  }
}

//
// lightwave - just one pixel traveling along the chain
//
void lightwave() {
  for (uint8_t i=0; i < BODY_LEDS; i++) {
     if (i == BODY_LEDS-(cycle % BODY_LEDS)-1) {
       setPixel(get_led_from_coord(i), foreColor);
     } else if (i == cycle % BODY_LEDS) {
       setPixel(get_led_from_coord(i), backColor);
     } else {
       setPixelBlack(get_led_from_coord(i));
     }
  }
  setHead(foreColor);
}

//
// lightrunup wave filling in and out
//
void lightrunup() {
  uint8_t i, pos;
  
  pos = cycle % (BODY_LEDS*2);  // Where we are in the show
  if (pos >= BODY_LEDS) {
    pos = (BODY_LEDS*2) - pos;  // For a sawtooth effect
  }
  
  for (i=0; i < BODY_LEDS; i++) {
    if (i < pos) {
      setPixelBlack(get_led_from_coord(i));
    } else {
      setPixel(get_led_from_coord(i), foreColor);
    }
  }
  setHead(foreColor);
}

//
// bounce
//
void bounce() {  
  if (morph != 0) {
    return;
  }
  
  fill(CHSV(170,255,40));  // changed color.v from 2 to 40
  
  for (int i = trail_size - 1; i >= 0; i--) {
    if (trail[i] == 88) continue;
    setPixelColor(trail[i], rgb_to_hsv(CRGB(0, 0, trail_colors[i])));
  }
  setPixelColor(bounce_pos, rgb_to_hsv(CRGB(200, 0, 128)));
  
  uint8_t old_dir = bounce_dir;
    while (get_neighbor(bounce_pos, bounce_dir) == -1 || (bounce_dir + 3) % 6 == old_dir) {
      bounce_dir = random(0,6);
    }
  for (int i = trail_size - 1; i >= 0; i--) {
    trail[i] = trail[i - 1];
  }
  trail[0] = bounce_pos;
  bounce_pos = get_neighbor(bounce_pos, bounce_dir);
}

//
// bounce_glowing
//
void bounce_glowing() {  
  if (morph != 0) {
    return;
  }
  
  fill(rgb_to_hsv(CRGB(0, 40, 40)));  // changed g,b=2 to g,b=40
  
  for (int i = 0; i < 6; i++) {
    int x = get_neighbor(bounce_pos, i);
    if (x == -1) continue;
    for (int j = 0; j < 6; j++) {
      int xx = get_neighbor(x, j);
      if (xx == -1) continue;
      setPixelColor(xx, rgb_to_hsv(CRGB(0, glow_colors[2], glow_colors[2])));
    }
  }
  for (int i = 0; i < 6; i++) {
    int x = get_neighbor(bounce_pos, i);
    if (x == -1) continue;
    setPixelColor(x, rgb_to_hsv(CRGB(0, glow_colors[1], glow_colors[1])));
  }
  setPixelColor(bounce_pos, rgb_to_hsv(CRGB(0, glow_colors[0], glow_colors[0])));
  
  int old_dir = bounce_dir;
  while (get_neighbor(bounce_pos, bounce_dir) == -1 || (bounce_dir + 3) % 6 == old_dir) {
    bounce_dir = random(0,6);
  }
  bounce_pos = get_neighbor(bounce_pos, bounce_dir);
}

//
// plinko
//
void plinko() {
  if (morph > 0) {
    return;
  }
  
  // Start a new plinko
  if (!random(8)) {
    for (int i = 0; i < NUM_PLINKO; i++) {
      if (plink_y[i] >= NUM_ROWS) {  // off the board
        plink_x[i] = 1;  // Start at center
        plink_y[i] = 0;  // Start at top
        break;
      }
    }
  }
  
  // Move existing plinko
  fill(BLACK);
  
  for (int i = 0; i < NUM_PLINKO; i++) {
    if (plink_y[i] < NUM_ROWS) {  // is on the board?
      if (get_led_from_coords(plink_y[i], plink_x[i]) != 88) {
        setPixel(get_led_from_coords(plink_y[i], plink_x[i]), IncColor(foreColor, 100 * i)); // Draw the plinko
      }
      
      if (getRowWidth(plink_y[i]) == 3) {
        plink_x[i] = plink_x[i] + random(0,2);
      } else {
        switch (plink_x[i]) {
          case 0:
            plink_x[i] = 0;
            break;
          case 3:
            plink_x[i] = 2;
            break;
          default:
            plink_x[i] = plink_x[i] - random(0,2);
            break;
        }     
      }
      plink_y[i] = plink_y[i] + 1;
    }
  }
}

//
// get LED from coord
//
int8_t get_led_from_coord(uint8_t i ) {
  return pgm_read_byte_near(ConeLookUp + i);
}

//
// get LED from coords
//
int8_t get_led_from_coords(uint8_t x,  uint8_t y) {
  return pgm_read_byte_near(ConeGrid + ((y % MAX_YCOORD) * MAX_XCOORD) + (x % MAX_XCOORD));
}

//
// get_neighbor
//
int8_t get_neighbor(uint8_t i, uint8_t dir) {
  return pgm_read_byte_near(neighbors + (i * 6) + dir);
}

//
// getRowWidth
//
byte getRowWidth(byte row) {
  if (row % 2 == 0) {
    return 3;
  } else {
    return 4;
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
      int8_t i = get_led_from_coords(x, y);      
      interp_frame[i].s = add_noise_to_channel(noise[x][y], interp_frame[i].s, noise_sat_intense);
      interp_frame[i].h = add_noise_to_channel(noise[MAX_XCOORD-x-1][MAX_YCOORD-y-1], interp_frame[i].h, noise_hue_intense); 
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
      leds[i].fadeToBlackBy(fade_amount);
    }
  }
}

//
// setPixel
//
void setPixel(int8_t pos, uint8_t color) {
  setPixelColor(pos, Wheel(color));
}

void setPixelColor(int8_t pos, CHSV color) {
  if (pos != -1) {
    next_frame[pos] = color;
  }
}

void setPixelBlack(int8_t pos) {
  next_frame[pos] = BLACK;
}

CHSV getPixelColor(int8_t pos) {
  return next_frame[pos];
}

void setHead(uint8_t color) {
  next_frame[42] = Wheel(color);
  next_frame[43] = Wheel(color);
  next_frame[44] = Wheel(color);
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
  counter += (random(2) * 2 - 1);  // 0-1 -> 0,2 -> -1 or +1
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

  noise_hue_intense = random(MAX_HUE_NOISE);
  noise_sat_intense = random(MAX_SAT_NOISE);
  uint8_t noise_set = random(ARRAY_SIZE(noise_param_set) / 2);
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

