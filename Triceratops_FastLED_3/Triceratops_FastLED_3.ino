#include <FastLED.h>
//
//  Triceratops - 45 cones on a coat
//
//  10/19/17
//
//  FastLED
//
#define BRIGHTNESS  125 // (0-255)

#define DELAY_TIME 20 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define BODY_LEDS 42
#define NUM_LEDS 45
#define NUM_ROWS 17

CRGB leds[NUM_LEDS];
CRGB current_frame[NUM_LEDS]; // framebuffers
CRGB next_frame[NUM_LEDS];  // framebuffers

uint8_t shuffle[NUM_LEDS];  // For random-fill show (nasty global)

// Light colors
#define MAX_COLOR 256   // Colors are 0-255 (palette color)
uint8_t foreColor =  0;    // Starting foreground color
uint8_t backColor = MAX_COLOR / 2;   // Starting background color
#define MIN_COLOR_SPEED 2   // Higher = slower
#define MAX_COLOR_SPEED 10   // Higher = slower
uint8_t foreColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster
uint8_t backColorSpeed = MAX_COLOR_SPEED / 2; // Lower = faster

// Palettes
CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;
TBlendType    currentBlending;
#define CAN_CHANGE_PALETTES true
#define DEFAULT_PALETTE RainbowColors_p // starting palette
#define PALETTE_DURATION 300  // seconds between palettes

// Possible palettes:
// RainbowColors_p, RainbowStripeColors_p, OceanColors_p, 
// CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p

// Shows - now enumerated
typedef void (*ShowList[])();
ShowList Shows = { allOn, randomfill, randomcolors, twocolor, lightwave, lightrunup, 
                   colorsize, brightsize, stripe, alternate, diagcolor, sidesidecolor, explodecolor, 
                   diagbright, sidesidebright, explodebright, plinko, bounce, bounce_glowing, sectioncolor };

uint8_t current_show = 0;
#define SHOW_DURATION 20  // seconds
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 1000 / DELAY_TIME;
#define FADE_OUT_TIME 3   // seconds to fade out
#define FADE_IN_TIME 3    // seconds to fade out

// Clocks and time
uint8_t morph = 0;
uint32_t small_cycle = 0;
uint16_t cycle = 0;

// wait times
#define WAIT_DURATION 20 // second between increasing wait time
#define MIN_WAIT   2  // Minimum number of morph steps in a cycle
#define MAX_WAIT 100  // Maximum number of morph steps in a cycle
#define NUM_WAIT_VALUES 20 // Number of stored delay times
uint8_t wait = NUM_WAIT_VALUES / 2;
uint8_t total_frames = getNumFrames(wait);

// noise
#define HAVE_NOISE false
#define NOISE_INTENSE 40  // 0-255 with 255 = very intense
uint8_t noise_param1 = 40;
uint8_t noise_param2 = 200;

// Plinko
#define NUM_PLINKO 5
uint8_t plink_x[NUM_PLINKO] = { 0,0,0,0,0 };
uint8_t plink_y[NUM_PLINKO] = { 18,18,18,18,18 };

// Bounce
uint8_t bounce_dir = random(6);
uint8_t bounce_pos = random(NUM_LEDS);
uint8_t trail_size = 5;
int trail[] = { -1, -1, -1, -1, -1 };
uint8_t trail_colors[] = { 205, 145, 85, 45, 5 };
uint8_t glow_colors[] = { 254, 35, 9 };

// Lookup tables

const uint8_t ConeLookUp[45] = {
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

uint8_t ConeGrid[17][4] = {
 {45,    44,   45,45},
 {45,  42, 43,    45},
 {   41, 40, 32,  45},
 {45,  39, 33,    45},
 {   38, 34, 31,  45},
 {45,  35, 30,    45},
 {   36, 29, 20,  45},
 { 37, 28, 21, 19,  },
 {   27, 22, 18,  45},
 { 26, 23, 17,  9,  },
 {   24, 16,  8,  45},
 { 25, 15,  7, 10,  },
 {   14,  6, 11,  45},
 {45,   5, 12,    45},
 {    4, 13,  0,  45},
 {45,   3,  1,    45},
 {45,     2,   45,45},
};

uint8_t neighbors[][6] = {
  {88,88,88,1,13,12}, // 0
  {0,88,88,2,3,13},
  {1,88,88,88,88,3},
  {13,1,2,88,88,4},
  {5,13,3,88,88,88}, // 4
  {6,12,13,4,88,14},
  {7,11,12,5,14,15},
  {8,10,11,6,15,16},
  {9,88,10,7,16,17}, // 8
  {88,88,88,8,17,18},
  {88,88,88,11,7,8},
  {10,88,88,12,6,7},
  {11,88,0,13,5,6}, // 12
  {12,0,1,3,4,5},
  {15,6,5,88,88,25},
  {16,7,6,14,25,24},
  {17,8,7,15,24,23}, // 16
  {18,9,8,16,23,22},
  {19,88,9,17,22,21},
  {88,88,88,18,21,20},
  {88,88,19,21,29,30}, // 20
  {20,19,18,22,28,29},
  {21,18,17,23,27,28},
  {22,17,16,24,26,27},
  {23,16,15,25,88,26}, // 24
  {24,15,14,88,88,88},
  {27,23,24,88,88,88},
  {28,22,23,26,88,37},
  {29,21,22,27,37,36}, // 28
  {30,20,21,28,36,35},
  {31,88,20,29,35,34},
  {88,88,88,30,34,33},
  {88,88,88,33,40,43}, // 32
  {32,88,31,34,39,40},
  {33,31,30,35,38,39},
  {34,30,29,36,88,38},
  {35,29,28,37,88,88}, // 36
  {36,28,27,88,88,88},
  {39,34,35,88,88,88},
  {40,33,34,38,88,41},
  {43,32,33,39,41,42}, // 40
  {42,40,39,88,88,88},
  {44,43,40,41,88,88},
  {88,88,32,40,42,44},
  {88,88,43,42,88,88}, // 44
};

const uint8_t PatternLookUp[45] = { 41,43,44,42,39,37,35,32,29,26,
                           33,36,38,40,34,31,28,25,22,19,
                           15,18,21,24,27,30,23,20,17,14,
                           12,10,5,7,9,11,13,16,8,6,
                           4,3,2,1,0 };

const uint8_t Stripe_Pattern[45] PROGMEM = {
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

const uint8_t Section_Pattern[45] PROGMEM = {
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

const uint8_t Explode_Pattern[45] PROGMEM = {
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

const uint8_t Alternate_Pattern[45] PROGMEM = {
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

const uint8_t SideSide_Pattern[45] PROGMEM = {
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

const uint8_t Diag_Pattern[45] PROGMEM = {
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

const uint8_t ConeSize[45]  = { 5,5,5,5,5,3,5,1,3,5,1,1,5,4,3,4,5,5,6,3,6,2,1,6,6,4,2,6,2,1,3,4,2,4,4,5,3,2,2,3,1,6,3,3,1 };

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

  currentPalette = DEFAULT_PALETTE;
  targetPalette = DEFAULT_PALETTE;
  currentBlending = LINEARBLEND;
  
  set_all_black();
}

void loop() { 

  Shows[current_show]();  // Function call to the current show

  morph_frame();  // Morph the display and update the LEDs

  blend_palette();
  
  delay(DELAY_TIME); // The only delay

  morph++;
  small_cycle++;
  
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
  EVERY_N_SECONDS(PALETTE_DURATION) { change_palette(); }
  EVERY_N_SECONDS(WAIT_DURATION) { 
    wait = up_or_down(wait, 0, MAX_WAIT);
    total_frames = getNumFrames(wait);
  }
}

//
// Blend Palette
//
void blend_palette() {
  uint8_t maxChanges = 24; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);
}

//
// next_show
//
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void next_show() {
  current_show = random8(ARRAY_SIZE( Shows));
  morph = 0;
  small_cycle = 0;
  cycle = 0;
  set_all_black();
  foreColorSpeed = up_or_down(foreColorSpeed, MIN_COLOR_SPEED, MAX_COLOR_SPEED);
  backColorSpeed = up_or_down(backColorSpeed, MIN_COLOR_SPEED, MAX_COLOR_SPEED);
}

//
// change_palette
//
void change_palette() {
  if (!CAN_CHANGE_PALETTES) return;
  
  switch (random(6)) {
    case 0:
      targetPalette = RainbowColors_p;
      break;
    case 1:
      targetPalette = RainbowStripeColors_p;
      break;
    case 2:
      targetPalette = OceanColors_p;
      break;
    case 3:
      targetPalette = CloudColors_p;
      break;
    case 4:
      targetPalette = LavaColors_p;
      break;
    case 5:
      targetPalette = ForestColors_p;
      break;
    default:
      targetPalette = PartyColors_p;
      break;
  }
}

void set_all_black() {
  for (int i = 0; i < NUM_LEDS; i++) {
    current_frame[i] = CRGB::Black;
    next_frame[i] = CRGB::Black;
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void fill(CRGB color) {
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
// random fill - fill randomly and then take away
//
void randomfill() {
  int i, j, save, pos;
  
  pos = cycle % (BODY_LEDS*2);  // Where we are in the show

  if (pos == 0) {  // Start of show
    shuffle_leds();
  }
  
  if (pos >= BODY_LEDS) {
    pos = (BODY_LEDS * 2) - pos;  // For a sawtooth effect
  }
  
  for (i=0; i < BODY_LEDS; i++) {
    if (i < pos) {  
      setPixelColor(shuffle[i], Wheel(foreColor));  // Turning on lights one at a time
    } else { 
      setPixelColor(shuffle[i], CRGB::Black);  // Turning off lights one at a time
    }
  }
  setHead(Wheel(foreColor));
}  

//
// Shuffle LEDS
//
void shuffle_leds() {
  int i, j, save;
  
  for (i=0; i < BODY_LEDS; i++) {
    shuffle[i] = i; // before shuffle
  }
  for (i=0; i < BODY_LEDS; i++) {  // here's position
    j = random8(BODY_LEDS);         // there's position
    save = shuffle[i];
    shuffle[i] = shuffle[j];       // first swap
    shuffle[j] = save;             // second swap
  }
}

//
// random colors - turns each pixel on to a random color
//
void randomcolors() {
  if (cycle == 0 && morph == 0) {  // Start of show: assign lights to random colors
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
// colorsize - light each cone according to its cone size
//
void colorsize() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(IncColor(foreColor, ((ConeSize[i]-1) * backColor) % MAX_COLOR)));
  }
  foreColor = IncColor(foreColor, 1);
}

//
// calcIntensity - use sine wave + cycle + variable to calculate intensity
//
uint8_t calcIntensity(uint8_t x, uint8_t max_x) {
  return sin8_C( 255 * ((cycle + x) % max_x) / (max_x - 1));
}

//
// brightsize - light just one cone size
//
void brightsize() {
  for (int i=0; i < BODY_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(ConeSize[i]-1, 5)));
  }
  setHead(Wheel(foreColor));
}

//
// stripe
//
void stripe() {
  for (int i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Stripe_Pattern + PatternLookUp[i]) == 0) {
      setPixelColor(i, Wheel(foreColor));
    } else {
      setPixelColor(i, Wheel(backColor));
    }
  }
  backColor = IncColor(backColor, -3);
}

//
// alternate
//
void alternate() {
  for (int i=0; i < NUM_LEDS; i++) {
    if (pgm_read_byte_near(Alternate_Pattern + PatternLookUp[i])) {
      setPixelColor(i, Wheel(foreColor));
    } else {
      setPixelColor(i, Wheel(backColor));
    }
  }
  //foreColor = IncColor(foreColor, 75);
}

void diagcolor() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(IncColor(foreColor, (backColor * pgm_read_byte_near(Diag_Pattern + PatternLookUp[ConeLookUp[i]])) / 5)));
  }
}

void sectioncolor() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(IncColor(foreColor, (backColor * pgm_read_byte_near(Section_Pattern + PatternLookUp[ConeLookUp[i]])) / 10)));
  }
}

void sidesidecolor() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(IncColor(backColor, (foreColor * pgm_read_byte_near(SideSide_Pattern + PatternLookUp[ConeLookUp[i]])))));
  }
}

void explodecolor() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(IncColor(backColor, (foreColor * pgm_read_byte_near(Explode_Pattern + PatternLookUp[ConeLookUp[i]])))));
  }
}

void diagbright() {
  for (int i=0; i < BODY_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(pgm_read_byte_near(Diag_Pattern + PatternLookUp[i]), 9)));
  }
  setHead(Wheel(foreColor));
}

void sidesidebright() {
  for (int i=0; i < BODY_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(foreColor, calcIntensity(pgm_read_byte_near(SideSide_Pattern + PatternLookUp[i]), 7)));
  }
  setHead(Wheel(foreColor));
}

void explodebright() {
  for (int i=0; i < BODY_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(foreColor, calcIntensity(5 - pgm_read_byte_near(Explode_Pattern + PatternLookUp[i]), 6)));
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
// lightwave - just one pixel traveling along the chain
//
void lightwave() {
  for (int i=0; i < BODY_LEDS; i++) {
     if (i == BODY_LEDS-(cycle % BODY_LEDS)-1) {
       setPixelColor(ConeLookUp[i], Wheel(foreColor));
     } else if (i == cycle % BODY_LEDS) {
       setPixelColor(ConeLookUp[i], Wheel(backColor));
     } else {
       setPixelColor(ConeLookUp[i], CRGB::Black);
     }
  }
  setHead(Wheel(foreColor));
}

//
// lightrunup - wave filling in and out
//
void lightrunup() {
  int pos = cycle % (BODY_LEDS*2);  // Where we are in the show
  if (pos >= BODY_LEDS) {
    pos = (BODY_LEDS*2) - pos;  // For a sawtooth effect
  }
  
  for (int i=0; i < BODY_LEDS; i++) {
    if (i < pos) {
      setPixelColor(ConeLookUp[i], CRGB::Black);
    } else {
      setPixelColor(ConeLookUp[i], foreColor);
    }
  }
  setHead(foreColor);
}

//
// bounce
//
void bounce() {
  if (morph != 0) return;
  
  fill(CRGB(0,0,2));
  
  for (int i = trail_size - 1; i >= 0; i--) {
    if (trail[i] == 88) continue;
    setPixelColor(trail[i], CRGB(0, 0, trail_colors[i]));
  }
  setPixelColor(bounce_pos, CRGB(200, 0, 128));
  
  uint8_t *n = neighbors[bounce_pos];
  uint8_t old_dir = bounce_dir;
  while (n[bounce_dir] == 88 || (bounce_dir + 3) % 6 == old_dir) {
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
  if (morph != 0) { return; }
  
  fill(CRGB(0, 2, 2));
  
  for (int i = 0; i < 6; i++) {
    int x = neighbors[bounce_pos][i];
    if (x == 88) continue;
    for (int j = 0; j < 6; j++) {
      int xx = neighbors[x][j];
      if (xx == 88) continue;
      setPixelColor(xx, CRGB(0, glow_colors[2], glow_colors[2]));
    }
  }
  for (int i = 0; i < 6; i++) {
    int x = neighbors[bounce_pos][i];
    if (x == 88) continue;
    setPixelColor(x, CRGB(0, glow_colors[1], glow_colors[1]));
  }
  setPixelColor(bounce_pos, CRGB(0, glow_colors[0], glow_colors[0]));
  
  uint8_t *n = neighbors[bounce_pos];
  int old_dir = bounce_dir;
  while (n[bounce_dir] == 88 || (bounce_dir + 3) % 6 == old_dir) {
    bounce_dir = random(0,6);
  }
  bounce_pos = n[bounce_dir]; 
}

//
// plinko
//
void plinko() {
  if (morph != 0) { return; }
  
  // Start a new plinko
  if (!random8(8)) {
    for (int i = 0; i < NUM_PLINKO; i++) {
      if (plink_y[i] >= NUM_ROWS) {  // off the board
        plink_x[i] = 1;  // Start at center
        plink_y[i] = 0;  // Start at top
        break;
      }
    }
  }
  
  // Move existing plinko
  set_all_black();
  
  for (int i = 0; i < NUM_PLINKO; i++) {
    if (plink_y[i] < NUM_ROWS) {  // is on the board?
      if (ConeGrid[plink_y[i]][plink_x[i]] != 88) {
        setPixelColor(ConeGrid[plink_y[i]][plink_x[i]], Wheel(IncColor(foreColor, 100 * i))); // Draw the plinko
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
// getRowWidth
//
uint8_t getRowWidth(uint8_t row) {
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
   uint8_t fract = map(morph, 0, total_frames-1, 0, 255);  // 0 - 255
   
   for (int i = 0; i < NUM_LEDS; i++) {
     setLEDinterpCRGB(i, current_frame[i], next_frame[i], fract);
   }
   add_noise();
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
// add_noise
//
void add_noise() {
  if (!HAVE_NOISE) { return; }

  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t data = inoise8_raw ((small_cycle * noise_param1) + (i * noise_param2));
    data = qsub8(data,16);
    data = qadd8(data,scale8(data,39));
    data = map8(data, 0, NOISE_INTENSE);
    leds[i].fadeToBlackBy(data);
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

void setPixelColor(int pos, CRGB color) {
  next_frame[pos] = color;
}

void setHead(CRGB color) {
  next_frame[42] = color;
  next_frame[43] = color;
  next_frame[44] = color;
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
CRGB Wheel(uint8_t hue)
{
  return Gradient_Wheel(hue, 255);  // 255 = full brightness
}

//
//  Gradient_Wheel - Input a hue and intensity (0-255) to get a CHSV from the palette
//
CRGB Gradient_Wheel(uint8_t hue, uint8_t intensity)
{
  return ColorFromPalette( currentPalette, hue, intensity, currentBlending);
}

//
//  setLEDinterpCRGB - Set LED i to the interpolate of two CRGB colors 
//
void setLEDinterpCRGB(int i, CRGB c1, CRGB c2, uint8_t fract)
{  
  if (c1 == c2) {
    leds[i] = c1;
    return;
  } else if (fract == 0) {
    leds[i] = c1;
    return;
  } else if (fract == 255) { 
    leds[i] = c2;
    return;
  } else if (is_black(c1)) {
    leds[i] = c2;
    leds[i].fadeToBlackBy(255 - fract);
    return;
  } else if (is_black(c2)) {
    leds[i] = c1;
    leds[i].fadeToBlackBy(fract);
    return;
  } else {
    leds[i] = c1.lerp8(c2, fract);
    // leds[i] = HSVinterp(c1, c2, fract); // flickers at 255,0,0
    return;
  }
}

// is_black
boolean is_black(CRGB color) {
  return (color.r == 0 && color.g == 0 && color.b == 0);
}

//
// Interpolate between 2 CRGB colors using HSV
//
CRGB HSVinterp(CRGB a, CRGB b, uint8_t fract) {
  Serial.println("interpolating");
  CRGB new_color;
  CHSV c1 = rgb2hsv_approximate(a); // convert to HSV
  CHSV c2 = rgb2hsv_approximate(b); // convert to HSV
  
  uint8_t h = interpolate_wrap(c1.h, c2.h, fract);
  uint8_t s = lerp8by8(c1.s, c2.s, fract);
  uint8_t v = lerp8by8(c1.v, c2.v, fract);
  
  return new_color.setHSV(h,s,v);
}

//
// Interpolate Wrap - returns the fractional point from a to b, checking both ways around a circle
//
uint8_t interpolate_wrap(uint8_t a, uint8_t b, uint8_t fract)
{
  if (should_wrap(a,b)) {
    if (a < b) a += 255;
    else b += 255;
  }
  return lerp8by8(a, b, fract);
}

//
// should_wrap - determines whether the negative way  around the circle is shorter
//
boolean should_wrap(uint8_t a, uint8_t b) {
  if (a > b) {
    uint8_t temp = a;  // basic a <-> b swap
    a = b;
    b = temp;
  }
  return(abs(b-a) > abs((a+255) - b));
}
