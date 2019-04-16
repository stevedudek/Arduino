#include <FastLED.h>
//
//  Red Dinosaur
//
//  29 LEDs = 7 left side + 15 center + 7 right side
//
//  10/21/17
//
//  FastLED
//
#define BRIGHTNESS  255 // (0-255)

#define DELAY_TIME 40 // in milliseconds

#define DATA_PIN 9
#define CLOCK_PIN 8

#define NUM_LEDS 29
#define SYM_LEDS 22
#define SPINE_LEDS 14

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
#define CAN_CHANGE_PALETTES false
#define DEFAULT_PALETTE RainbowColors_p // starting palette
#define PALETTE_DURATION 300  // seconds between palettes

const TProgmemPalette16 dinosaurPalette_p PROGMEM =
{
    CRGB::Blue, CRGB::Crimson, CRGB::Gold, CRGB::Magenta,
    CRGB::DarkOrange, CRGB::Gray, CRGB::FireBrick, CRGB::MediumVioletRed,
    CRGB::Red, CRGB::DeepPink, CRGB::HotPink, CRGB::Indigo,
    CRGB::Orange, CRGB::OrangeRed, CRGB::Orchid, CRGB::Yellow
};

// Possible palettes:
// RainbowColors_p, RainbowStripeColors_p, OceanColors_p, 
// CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p

// Shows - now enumerated
typedef void (*ShowList[])();
ShowList Shows = { allOn, morphChain, randomfill, randomcolors, twocolor, lightwave, lightrunup, 
                   sawtooth, colorsize, brightsize, colorarrow, brightarrow };

uint8_t current_show = 0;
#define SHOW_DURATION 30  // seconds (don't go over 40)
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 1000 / DELAY_TIME;
#define FADE_OUT_TIME 3   // seconds to fade out
#define FADE_IN_TIME 3    // seconds to fade outs

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

// center column | left side | right side
const uint8_t ConeLookUp[NUM_LEDS]  = { 0,1,3,5,6,7,8,19,18,17,16,15,26,27,28,
                                2,9,10,11,12,13,14,
                                4,20,21,22,23,24,25 };

const uint8_t ConeSize[NUM_LEDS] PROGMEM = { 2,1,2,3,2,1,1,1,1,1,1,2,3,4,5,
                      1,0,0,0,0,0,0,
                      1,0,0,0,0,0,0 };

const uint8_t ArrowPattern[NUM_LEDS] PROGMEM = 
{ 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                  2, 7, 6, 5, 4, 3, 2,
                  2, 7, 6, 5, 4, 3, 2 };

#define NUM_PATTERNS 6   // Total number of patterns

uint8_t PatternMatrix[NUM_PATTERNS][NUM_LEDS] = {
{ 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2, 1, 2, 1,
                  1, 2, 1, 2, 1, 2, 1,
                  1, 2, 1, 2, 1, 2, 1 },
                
{ 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
                  1, 2, 1, 2, 1, 2, 1,
                  2, 1, 2, 1, 2, 1, 2 },

{ 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
                  2, 1, 2, 1, 2, 1, 2,
                  1, 2, 1, 2, 1, 2, 1 },

{ 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 1, 2, 1, 2,
                1, 2, 1, 2, 1, 2, 1,
                1, 2, 1, 2, 1, 2, 1 },

{ 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 2,
                  1, 2, 2, 1, 2, 2, 1,
                  1, 2, 2, 1, 2, 2, 1 },

{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                  1, 1, 1, 1, 1, 1, 1,
                  1,  1, 1, 1, 1, 1, 1 },
                
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
  patterns(1000);
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
      setPixelColor(shuffle[NUM_LEDS-(i % NUM_LEDS)-1], CRGB::Black);  // Turning off lights one at a time
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
// two color - alternates the color of pixels between two colors
//
void twocolor() {
  for (int i = 0; i < SYM_LEDS; i++) {
    if (i % 2) {
      setSymPixelColor(i, Wheel(foreColor));
    }
    else {
      setSymPixelColor(i, Wheel(backColor));
    }
  }
}

//
// patterns shows - not part of the usual show loop
//
void patterns(int cycles) {
  int pattern = random(NUM_PATTERNS);    // Pick a random pattern
  int i, ticks = 0;
  CRGB color;
  
  while (ticks++ < cycles) {
      if (!random(10)) {       // The not-MAIN_COLOR color
        foreColor = IncColor(foreColor, 10); 
      }
        
      for (i=0; i < NUM_LEDS; i++) {
        switch (PatternMatrix[pattern][i]) {
          case 0: {        // Off (black)
            color = CRGB::Black;
            break;
          }
          case 1: {        // backColor
            color = Wheel(backColor);
            break;
          }
          case 2: {        // foreColor
            color = Wheel(foreColor);
            break;
          }
        }
        leds[ConeLookUp[i]] = color;
      }
      FastLED.show();  // Update the display 
      delay(DELAY_TIME);
  }  
}

//
// Morph Chain - morphs color 1 from position x to color 2 at position x+n
//
void morphChain() {
  for (int i=0; i < NUM_LEDS; i++) {
    uint8_t fract = map((i+(cycle % NUM_LEDS)) % NUM_LEDS, 0, NUM_LEDS-1, 0, 255);
//    fract = ((i+(cycle % NUM_LEDS)) % NUM_LEDS)/(float)(NUM_LEDS-1);
    setPixelColor(NUM_LEDS-i-1, Wheel(interpolate_wrap(foreColor, backColor, fract)));
  }
}

//
// Saw tooth - Fills in pixels with a sawtooth of intensity
//
void sawtooth() {
  
  for (int i=0; i < NUM_LEDS; i++) {
    uint16_t fract = map((i+(cycle % NUM_LEDS)) % NUM_LEDS, 0, NUM_LEDS-1, 0, 512);
//    attenuation = 2*(((i+(cycle % NUM_LEDS)) % NUM_LEDS)/(float)(NUM_LEDS-1));
    if (fract >= 256) {
      fract = 512 - fract;  // the subtraction creates the sawtooth
    }
    // "i" will have pattern move up; "NUM_LEDS-i-1'' will have pattern move down
    setPixelColor(NUM_LEDS-i-1, Gradient_Wheel(foreColor, fract));
  }
}

//
// colorsize - light each cone according to its cone size
//
void colorsize() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(IncColor(foreColor, pgm_read_byte_near(ArrowPattern + i) * backColor)));
  }
}

//
// brightsize -Light just one cone size at a time
//
void brightsize() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(pgm_read_byte_near(ArrowPattern + i), 9)));
  }
}

//
// colorarrow - light each cone according to its arrow position
//
void colorarrow() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Wheel(IncColor(foreColor, pgm_read_byte_near(ConeSize + i) * backColor)));
  }
}

//
// brightarrow -Light just one arrow piece at a time
//
void brightarrow() {
  for (int i=0; i < NUM_LEDS; i++) {
    setPixelColor(i, Gradient_Wheel(backColor, calcIntensity(pgm_read_byte_near(ConeSize + i), 5)));
  }
}

//
// calcIntensity - use sine wave + cycle + variable to calculate intensity
//
uint8_t calcIntensity(uint8_t x, uint8_t max_x) {
  return sin8_C( 255 * ((cycle + x) % max_x) / (max_x - 1));
}

//
// rainbowshow - fills with a gradient rainbow; over time, the starting color changes
//
void rainbowshow(int cycles) {
  uint8_t diff = abs(foreColor - backColor);
  
  for (int i = 0; i < SYM_LEDS; i++) {
    setSymPixelColor( (i+(cycle % SYM_LEDS)) % SYM_LEDS, Wheel(((i*diff / SYM_LEDS)+foreColor) % diff) );
  }
}

//
// lightwave - just one pixel traveling along the chain
//
void lightwave() {
  for (int i = 0; i < SYM_LEDS; i++) {
     if (i == SYM_LEDS - (cycle % SYM_LEDS) - 1) {
      setSymPixelColor(i, Wheel(foreColor));
     }
     else {
      setSymPixelColor(i,CRGB::Black);
     }
  }
}

//
// lightrunup - lights fill up one at time
// 
void lightrunup() {
  int pos = cycle % (SYM_LEDS * 2);  // Where we are in the show
  if (pos >= SYM_LEDS) {
    pos = (SYM_LEDS*2) - pos;
  }
  
  for (int i=0; i < SYM_LEDS; i++) {
    if (i < pos) {
      setSymPixelColor(i, Wheel(foreColor));  // Turning on lights one at a time
    } else {
      setSymPixelColor(i, CRGB::Black);   // black
    }
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

//
// set SymPixelColor - forces the two sides to have symmetric pixels
//
void setSymPixelColor(int pos, CRGB color) {
  // 0 - 13 : just light the center spine
  // 14 - 20: light the left side and light the right side the same
  // 20 - 25: ignore the right side
  
  if (pos < SYM_LEDS) {
    next_frame[ConeLookUp[pos]] = color;

    if (pos >= SPINE_LEDS) {
      next_frame[ConeLookUp[pos + 7]] = color;
    }
  } 
}

void setPixelColor(int pos, CRGB color) {
  next_frame[ConeLookUp[pos]] = color;
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
//  Gradient_Wheel - Input a hue and intensity to get a CHSV from the palette
//
//  Intensity must be 0 <= intensity <= 255
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
