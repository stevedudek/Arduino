#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
//
//  Feather Cape - 38 Petals on a cape
//
//  9/11/22
//
//  FastLED
//
//  Libracize everything
//
/*****************************************************************************/

#define BRIGHTNESS  255  // (0-255)

#define DELAY_TIME 60  // in milliseconds - Higher for slower shows

#define DATA_PIN 0  // 9
#define CLOCK_PIN 2  // 8

#define NUM_LEDS 38
#define NUM_SPACERS 10  // Should always be turned off
#define TOTAL_LEDS 48

#define XX  255

#define MAX_XCOORD 11
#define MAX_YCOORD 4

#define MAX_COLOR 256   // Colors are 0-255
#define MAIN_COLOR 120
#define BLACK CHSV(0, 0, 0)

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[TOTAL_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library (will need to be reinitialized)

// Shows
#define NUM_SHOWS 20
uint8_t current_show = 0; // Starting show
uint8_t current_pattern = 0;

// wait times
#define SHOW_DURATION 30  // seconds
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 2   // seconds to fade out
#define FADE_IN_TIME 2    // seconds to fade out

// Lookup tables

uint8_t coords[] PROGMEM = {
     0,  1,  2, XX,
   3,  4,  5,  6,
     7,  8,  9, XX,
  10, 11, 12, 13,
    14, 15, 16, XX,
  17, 18, 19, 20,
    21, 22, 23, XX,
  24, 25, 26, 27,
    28, 29, 30, XX,
  31, 32, 33, 34,
    35, 36, 37, XX
};

uint8_t ConeLookUp[] PROGMEM = {
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

uint8_t SpacerPixels[NUM_SPACERS] = { 3, 8, 12, 17, 21, 26, 30, 35, 39, 44 };

uint8_t neighbors[] PROGMEM = {
  XX,1,4,3,XX,XX, // 0
  XX,2,5,4,0,XX,
  XX,XX,6,5,1,XX,
  0,4,7,XX,XX,XX,
  1,5,8,7,3,0, // 4
  2,6,9,8,4,1,
  XX,XX,XX,9,5,2,
  4,8,11,10,XX,3,
  5,9,12,11,7,4, // 8
  6,XX,13,12,8,5,
  7,11,14,XX,XX,XX,
  8,12,15,14,10,7,
  9,13,16,15,11,8, // 12
  XX,XX,XX,16,12,9,
  11,15,18,17,XX,10,
  12,16,19,18,14,11,
  13,XX,20,19,15,12, // 16
  14,18,21,XX,XX,XX,
  15,19,22,21,17,14,
  16,20,23,22,18,15,
  XX,XX,XX,23,19,16, // 20
  18,22,25,24,XX,17,
  19,23,26,25,21,18,
  20,XX,27,26,22,19,
  21,25,28,XX,XX,XX, // 24
  22,26,29,28,24,21,
  23,27,30,29,25,22,
  XX,XX,XX,30,26,23,
  25,29,32,31,XX,24, // 28
  26,30,33,32,28,25,
  27,XX,34,33,29,26,
  28,32,35,XX,XX,XX,
  29,33,36,35,31,28, // 32
  30,34,37,36,32,29,
  XX,XX,XX,37,33,30,
  32,36,XX,XX,XX,31,
  33,37,XX,XX,35,32, // 36
  34,XX,XX,XX,36,33,
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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, TOTAL_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  // Set up the various mappings (1D lists in PROGMEM)
  //led.setLedMap(ConeLookUp);  // turned off mapping - it's handled explicitly here in update_leds()
  led.setCoordMap(MAX_YCOORD, coords);  // x,y grid of cones
  led.setNeighborMap(neighbors);  // 6 neighbors for every pixel
  shows = Shows(&led);  // Show library - reinitialized for led mappings
  
  led.fillBlack();
  led.push_frame();
}

void loop() { 
  
  switch(current_show) {      
    
    case 0:    
      patterns();
      break;
    case 1:
      shows.randomFill();
      break;
    case 2:
      shows.randomColors();
      break;
    case 3:
      shows.twoColor();
      break;
    case 4:
      shows.sawTooth();
      break;
    case 5:
      shows.morphChain();
      break;
    case 6:
      shows.bounce();
      break;
    case 7:
      shows.bounceGlowing();
      break;
    case 8:
      shows.plinko(1);  // 1 = starting pixel
      break;
    case 9:
      vert_back_forth_dots();
      break;
    case 10:
      vert_back_forth_bands();
      break;
    case 11:
      vert_back_forth_colors();
      break;
    case 12:
      horiz_back_forth_dots();
      break;
    case 13:
      horiz_back_forth_bands();
      break;
    case 14:
      horiz_back_forth_colors();
      break;
    case 15:
      diag_back_forth_dots();
      break;
    case 16:
      diag_back_forth_bands();
      break;
    case 17:
      diag_back_forth_colors();
      break;
    case 18:
      shows.lightWave();
      break;
    default:
      scales();
      break;
  }
  
  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay
  
  shows.advanceClock();
  
  if (shows.getSmallCycle() >= MAX_SMALL_CYCLE) { 
    next_show(); 
  }
}

//
// next_show
//
void next_show() {
  // Switch between a patterns show and all the other shows
  if (current_show == 0) {
    current_show = random8(1, NUM_SHOWS);
  } else {
    current_show = 0;
    current_pattern = random8(NUM_PATTERNS);
  }
  led.fillBlack();
  led.push_frame();
  shows.resetAllClocks();
  shows.pickRandomColorSpeeds();
  shows.pickRandomWait();
}

//// Start of specialized shows

//
// patterns shows
//
void patterns() {
  for (uint8_t i=0; i < NUM_LEDS; i++) {
    
    switch (PatternMatrix[current_pattern][i]) {
      case 0: {        // Off (black)
        led.setPixelBlack(i);
        break;
      }
      case 1: {        // MAIN_COLOR
        led.setPixelHue(i, MAIN_COLOR);
        break;
      }
      case 2: {        // The other color
        led.setPixelHue(i, shows.getForeColor());
        break;
      }
    }
  }
}

//
// vert back forth dots - vertical dots moving back and forth
//
void vert_back_forth_dots() {
  uint8_t temp_x;
  uint16_t cycle = shows.getCycle();
  
  led.fillBlack();

  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
      if ((temp_x + cycle) % MAX_XCOORD == 0) {
        shows.setPixeltoForeColor(led.getLedFromCoord(x,y));
      }
    }
  }
}

//
// vert back forth bands - vertical bands moving back and forth
//
void vert_back_forth_bands() {
  uint8_t temp_x, intensity;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
      temp_x = (temp_x + cycle) % MAX_XCOORD;
      intensity = sin8_avr(map(temp_x, 0, MAX_XCOORD, 0, 255) + (cycle % (255 / MAX_XCOORD)) );
      led.setPixelColor(led.getLedFromCoord(x,y), led.gradient_wheel(shows.getBackColor(), intensity));
    }
  }
}

//
// vert back forth colors - vertical colors moving back and forth
//
void vert_back_forth_colors() {
  uint8_t temp_x, hue;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
      temp_x = (temp_x + cycle) % MAX_XCOORD;
      hue = sin8_avr(map(temp_x, 0, MAX_XCOORD, shows.getForeColor(), shows.getBackColor()) + (cycle % (255 / MAX_XCOORD)) );
      led.setPixelHue(led.getLedFromCoord(x,y), hue);
    }
  }
}

//
// horiz back forth dots - horizontal dots moving back and forth
//
void horiz_back_forth_dots() {
  uint8_t temp_y;
  uint16_t cycle = shows.getCycle();

  led.fillBlack();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
      if ((temp_y + cycle) % MAX_YCOORD == 0) {
        shows.setPixeltoForeColor(led.getLedFromCoord(x,y));
      }
    }
  }
}

//
// horiz back forth bands - horizontal bands moving back and forth
//
void horiz_back_forth_bands() {
  uint8_t temp_y, intensity;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
      temp_y = (temp_y + cycle) % MAX_YCOORD;
      intensity = sin8_avr(map(temp_y, 0, MAX_YCOORD, 0, 255) + (cycle % (255 / MAX_YCOORD)) );
      led.setPixelColor(led.getLedFromCoord(x,y), led.gradient_wheel(shows.getBackColor(), intensity));
    }
  }
}

//
// horiz back forth colors - horizontal colors moving back and forth
//
void horiz_back_forth_colors() {
  uint8_t temp_y, hue;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
      temp_y = (temp_y + cycle) % MAX_YCOORD;
      hue = sin8_avr(map(temp_y, 0, MAX_YCOORD, shows.getForeColor(), shows.getBackColor()) + (cycle % (255 / MAX_YCOORD)) );
      led.setPixelHue(led.getLedFromCoord(x,y), hue);
    }
  }
}

//
// diag back forth dots - diagonal dots moving back and forth
//
void diag_back_forth_dots() {
  uint8_t temp_x, temp_y;
  uint16_t cycle = shows.getCycle();

  led.fillBlack();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
      temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
      if ((temp_x + temp_y + cycle) % MAX_YCOORD == 0) {
        shows.setPixeltoForeColor(led.getLedFromCoord(x,y));
      }
    }
  }
}

//
// diag back forth bands - diagonal bands moving back and forth
//
void diag_back_forth_bands() {
  uint8_t temp_x, temp_y, temp, intensity;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {  // Don't light the head (row 0)
      temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
      temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
      temp = (temp_x + temp_y + cycle) % MAX_YCOORD;
      intensity = sin8_avr(map(temp, 0, MAX_YCOORD, 0, 255) + (cycle % (255 / MAX_YCOORD)) );
      led.setPixelColor(led.getLedFromCoord(x,y), led.gradient_wheel(shows.getBackColor(), intensity));
    }
  }
}

//
// diag back forth colors - diagonal colors moving back and forth
//
void diag_back_forth_colors() {
  uint8_t temp_x, temp_y, temp, hue;
  uint16_t cycle = shows.getCycle();
  
  for (uint8_t x = 0; x < MAX_XCOORD; x++) {
    for (uint8_t y = 0; y < MAX_YCOORD; y++) {
      temp_x = (y % 2) ? x : MAX_XCOORD - x - 1;
      temp_y = (x % 2) ? y : MAX_YCOORD - y - 1;
      temp = (temp_x + temp_y + cycle) % MAX_YCOORD;
      hue = sin8_avr(map(temp, 0, MAX_YCOORD, shows.getForeColor(), shows.getBackColor()) + (cycle % (255 / MAX_YCOORD)) );
      led.setPixelHue(led.getLedFromCoord(x,y), hue);
    }
  }
}

//
// scales
//
void scales() {
  if (shows.isShowStart()) {
    shows.shuffleLeds();
  }
  led.fillHue(MAIN_COLOR);

  uint8_t num_scales = (shows.getShuffleValue(0) / 3) + 3;
  for (uint8_t i=0; i < num_scales; i++) {
    led.setPixelHue(shows.getShuffleValue(i), shows.getForeColor());
  }
}

////  Utility Functions

//
// lookup_petal - convert petals into LEDs - this is where mapping happens
//
int8_t lookup_petal(uint8_t i) {
  return pgm_read_byte_near(ConeLookUp + i);
}

//
// morph_frame
//
void morph_frame() {
  shows.morphFrame();  // 1. calculate interp_frame 2. adjust palette
  update_leds();  // push the interp_frame on to the leds
  check_fades();  // Fade start and end of shows
  //turn_off_spacers();  // Black the spacer pixels
  FastLED.show();  // Update the display 
}

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[lookup_petal(i)] = led.getInterpFrameColor(i);
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
      leds[lookup_petal(i)].fadeToBlackBy(fade_amount);
    }
  }
}
