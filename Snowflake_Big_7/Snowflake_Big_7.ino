#include <Noise.h>
#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Snowflakes
//
//  10/16/2018
//
//  xBee - Speaking & Hearing Modes
//

#define SNOWFLAKE_SIZE 0  // 0 = Small, 1 = Medium, 2 = Large
#define SPOKE_LENGTH 8  // 8 = Small, 10 = Medium, 12 = Large
#define NUM_SNOWFLAKE_SIZES 3  // How many sizes of snowflake
#define ARE_CONNECTED false  // Are Snowflakes talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

#define SPOKE_LEDS  (SPOKE_LENGTH * 6)  // Don't change
#define NUM_LEDS  (SPOKE_LEDS + 7)  // Don't change

uint8_t BRIGHTNESS = 255;  // (0-255)

#define DELAY_TIME 40 // in milliseconds (usually 40)

#define DATA_PIN 9
#define CLOCK_PIN 8

Led led = Led(NUM_LEDS);  // Class instantiation of the Led library
CRGB leds[NUM_LEDS];  // Hook for FastLED library
Shows shows = Shows(&led);  // Show library

#define CAN_CHANGE_PALETTES true  // Palettes

// Shows
#define NUM_SHOWS 14
#define SINGLE_SPOKE_SHOWS 3  // Shows here and above can have a single spoke
uint8_t current_show = 7;
uint8_t current_pattern = 0;
boolean has_black_pattern = true;

// Clocks and time
#define SHOW_DURATION 60  // seconds. Size problems at 1800+ seconds.
uint16_t MAX_SMALL_CYCLE = SHOW_DURATION * (1000 / DELAY_TIME);
#define FADE_OUT_TIME 1   // seconds to fade out
#define FADE_IN_TIME 1    // seconds to fade out

// noise
#define HAVE_NOISE false    // set to false to suppress noise
Noise noise = (HAVE_NOISE) ? Noise(NUM_LEDS) : Noise() ;  // empty Noise() is lighter with memory

// symmetries - 6 arms
#define NUM_SYMMETRIES 5
uint8_t symmetry = 0;  // 0-4
boolean invert = false;  // Whether to invert odd arms
boolean switch_axis = true;  // Whether to switch spoke + radial axes
#define NEVER_ROTATE true  // True prevents rotation
boolean is_rotating = false;  // Whether arms are currently rotating
const uint8_t symmetries[] = { 1, 2, 3, 6, 6 };
const uint8_t hub_pixels[] = { 7, 4, 3, 2, 2 };  // 1 + (6 / symmetries[n]);

// xBee language
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_FORE_SPEED 'X'
#define COMMAND_BACK_SPEED 'Y'
#define COMMAND_BRIGHTNESS 'I'
#define COMMAND_SYMMETRY   'M'
#define COMMAND_HASBLACK   'H'
#define COMMAND_SWITCHAXIS 'A'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_CYCLE      'C'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define EMPTY_CHAR         ' '
#define MAX_MESSAGE       100
#define MAX_NUM             5  // To handle 65,535 of small_cycle
char message[MAX_MESSAGE];  // Incoming message buffer
char number_buffer[MAX_NUM];  // Incoming number buffer

//
// PATTERNS
//
#define NUM_PATTERNS 6   // Total number of patterns

const uint8_t pattern_matrix[] PROGMEM = {  // A pattern is 2-byte of binary
  // Small Snowflake (8-spoke)               
  B01010111, B01000000,
  B10101101, B10000000,
  B01001111, B01000000,
  B10110111, B00000000,
  B11001101, B10000000,
  B11001010, B10000000,

  // Medium Snowflake (10-spoke)               
  B11111111, B11110000,
  B11111111, B11110000,
  B11111111, B11110000,
  B11111111, B11110000,
  B11111111, B11110000,
  B11111111, B11110000,
  
  // Largest Snowflake (12-spoke)
  B01010101, B11010100,
  B10001011, B01101000,
  B01101010, B11001100,
  B10110000, B11110000,
  B01110010, B10101100,
  B11000111, B01110000
};

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
//  Serial.println("Start");  // For debugging

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  if (HAVE_NOISE) {
    noise.turnNoiseOn();
  } else {
    noise.turnNoiseOff();
  }

  if (CAN_CHANGE_PALETTES) {
    led.setPalette();  // turns on palettes with default valets
  }

  // Set up the various mappings here (1D lists in PROGMEM)
  //  led.setLedMap(ConeLookUp);  // mapping of pixels to actual leds
  shows = Shows(&led);  // Show library - reinitialized for led mappings
  
  led.fillBlack();
  led.push_frame();

  set_symmetry();
//  log_status();  // For debugging
  if (ARE_CONNECTED && IS_SPEAKING) { speak_all_commands(); }
  
  shows.setColorSpeedMinMax(2, 10); // Speed up color changing
}

void loop() {
  
  switch (current_show) {
  
    case 0:
      patternsBW();
      break;
    case 1:
      shows.randomColors();
      break;
    case 2:
      shows.randomFill();
      break;
    case 3:
      shows.twoColor();
      break;
    case 4:
      shows.morphChain();
      break;
    case 5:
      shows.lightWave(shows.getForeColorSpeed());
      break;
    case 6:
      shows.sawTooth();
      break;
    case 7:
      shows.lightRunUp();
      break;
    case 8:
      shows.packets();
      break;
    case 9:
      shows.packets_two();
      break;
    case 10:
      shows.sinelon_fastled();
      break;
    case 11:
      shows.bpm_fastled();
      break;
    case 12:
      shows.juggle_fastled();
      break;
    default:
      shows.bands();
      break;
  }

  morph_frame();  // Morph the display and update the LEDs

  delay(DELAY_TIME); // The only delay

  if (ARE_CONNECTED && !IS_SPEAKING) { HearSignal(); }
  
  if (HAVE_NOISE) { noise.fillNoise(); }
  
  shows.advanceClock();
  
  if ((shows.getSmallCycle() >= MAX_SMALL_CYCLE) &&
      (!ARE_CONNECTED || (ARE_CONNECTED && IS_SPEAKING))) {
    next_show(); 
  }
  
  // If the Snowflake isn't hearing, force the next show
  // Might remove this
  if (shows.getSmallCycle() >= MAX_SMALL_CYCLE * 2) { 
    pick_next_show(); 
    next_show();
  }
}

//
// next_show - adjust variables that are not communicated
//
void next_show() {
  led.fillBlack();
  led.push_frame();
  shows.resetAllClocks();
  led.randomizePalette();
  if (HAVE_NOISE) { noise.setRandomNoiseParams(); }

  if (!ARE_CONNECTED || IS_SPEAKING) { pick_next_show(); }
  if (ARE_CONNECTED && IS_SPEAKING) { speak_all_commands(); }

  //  log_status();  // For debugging
}

//
// next_show - adjust variables that are communicated
//
void pick_next_show() {
  // Switch between a patterns show and all the other shows
  current_pattern = random8(NUM_PATTERNS);
  has_black_pattern = random8(2) ? true : false;
  is_rotating = (random8(8) == 1) ? true : false;
  current_show = (current_show == 0) ? random8(2, NUM_SHOWS) : false;
  //  current_show = (current_show + 1) % NUM_SHOWS;  // For debugging
  pick_symmetry();
  shows.pickRandomColorSpeeds();
  shows.pickRandomWait();
}

////// Speaking

void speak_all_commands() {
  // Send all commands at once - order probably matters
  speak_command(COMMAND_SHOW, current_show);
  speak_command(COMMAND_FORE, shows.getForeColor());
  speak_command(COMMAND_BACK, shows.getBackColor());
  speak_command(COMMAND_FORE_SPEED, shows.getForeColorSpeed());
  speak_command(COMMAND_BACK_SPEED, shows.getBackColorSpeed());
  speak_command(COMMAND_BRIGHTNESS, BRIGHTNESS);
  speak_command(COMMAND_WAIT, shows.getWait());
  speak_command(COMMAND_CYCLE, shows.getSmallCycle());
  speak_command(COMMAND_SYMMETRY, symmetry);
  speak_command(COMMAND_HASBLACK, has_black_pattern);
  speak_command(COMMAND_SWITCHAXIS, switch_axis);
  speak_end_command();
}

void speak_command(char command, int value) {
  Serial.print(command);
  Serial.print(value);
  Serial.print(COMMAND_COMMA);
}

void speak_end_command() {
  Serial.println(COMMAND_PERIOD);  // Speak terminal period
}

void speak_cycle() {
  speak_command(COMMAND_CYCLE, shows.getSmallCycle());
  speak_end_command();
}


////// Hearing

void HearSignal() {
  if (Serial.available()) {  // Heard a signal!
    GetMessage(message);  // Load global message buffer by pointer
    ProcessMessage(message);
  }
}

//
// GetMessage - pulls the whole serial buffer until a period
//
void GetMessage(char* message) {
  uint8_t i = 0;
  
  while (i < MAX_MESSAGE) {
    char tmp = Serial.read();
    message[i] = tmp;
    i++;
    if (tmp == COMMAND_PERIOD) {
      return;  // Hit the end of the message
    }
  }
  //Serial.println("Message too long!");  // Ran out of message space
}

//
// ProcessMessage
//
boolean ProcessMessage(char* message) {
  uint8_t i = 0;
  
  while (i < MAX_MESSAGE) {
    char command = message[i];  // Get one-letter command
    
    if (command == COMMAND_PERIOD) { 
      return;  // end of message
    }   
    for (uint8_t j = 0; j < MAX_NUM; j++) { 
      number_buffer[j] = EMPTY_CHAR;  // Clear number buffer
    }
    
    int numsiz = ReadNum(message, i+1, number_buffer);  // Read in the number
    i = i + 2 + numsiz; // 2 = leap beginning command and trailing comma
    
    ExecuteOrder(command, atoi(number_buffer));
  }
}

//
// ReadNum - reads numbers in a string
//
uint8_t ReadNum(char* msg, uint8_t place, char* number) {
  uint8_t i = 0; // Start of number
  char tmp;
  
  while (i < MAX_NUM) {
    tmp = msg[place];
    if (tmp == COMMAND_COMMA) {
      return (i);
    } else {
      number[i] = msg[place];
      i++;
      place++;
      if (place >= MAX_MESSAGE) { 
        break;  
      }
    }
  }
  //Serial.println("Number too long");
  return (0); // Number too long
}

//
// Execute Order - execute a letter command followed by a number
//
void ExecuteOrder(char command, uint16_t value) {
      
  switch (command) {
    
    case COMMAND_FORE:
      shows.setForeColor(value);
      break;
    
    case COMMAND_BACK:
      shows.setBackColor(value);
      break;

    case COMMAND_FORE_SPEED:
      shows.setForeColorSpeed(value);
      break;
    
    case COMMAND_BACK_SPEED:
      shows.setBackColorSpeed(value);
      break;
    
    case COMMAND_BRIGHTNESS:
      if (BRIGHTNESS != value) {
        BRIGHTNESS = value;
        FastLED.setBrightness( BRIGHTNESS );
      }
      break;
    
    case COMMAND_WAIT:
      shows.setWait(value);
      break;
    
    case COMMAND_SHOW:  // Show
      if (current_show != value) {
        current_show = value;
        next_show();
      }
      break;
    
    case COMMAND_CYCLE: // small_cycle
      shows.setSmallCycle(value);
      break;

    case COMMAND_SYMMETRY:  // symmetry
      if (symmetry != value) {
        symmetry = value;
        set_symmetry();
      }
      break;

    case COMMAND_HASBLACK:  // has_black_pattern
      has_black_pattern = value;
      break;

    case COMMAND_SWITCHAXIS:
      switch_axis = value;
      break;
  }
}


////// Specialized shows

//
// patterns shows
//
void patternsBW() {
  map_pattern(get_bit_from_pattern_number(0, current_pattern), 0);  // Center
  map_pattern(get_bit_from_pattern_number(1, current_pattern), 1);  // Start
  
  for (int i = 0; i < SPOKE_LENGTH; i++) {
    map_pattern(get_bit_from_pattern_number(i+2, current_pattern), i + hub_pixels[symmetry]);
  }
}

void map_pattern(boolean isOn, uint8_t i) {
  if (isOn) {  // For pattern table
    if (has_black_pattern) {
      shows.setPixeltoHue(i, shows.getForeColor() + i);  // 1 = foreColor
    } else {
      shows.setPixeltoForeColor(i);  // 1 = foreColor
    }
  } else {
    if (has_black_pattern) {
      shows.setPixeltoBlack(i);  // 0 = either Black or backColor
    } else {
      shows.setPixeltoBackColor(i);
    }
  }
}

//
// morph_frame
//
void morph_frame() {
  shows.morphFrame();  // 1. calculate interp_frame 2. adjust palette
  add_noise();   // Use the noise library
  update_leds();  // push the interp_frame on to the leds
  check_fades();  // Fade start and end of shows
  FastLED.show();  // Update the display 
}

//
// add_noise - from library - uses led.library getters and setters
//
void add_noise() {
  if (HAVE_NOISE) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      led.setInterpFrameHue(i, noise.addNoiseAtoValue(i, led.getInterpFrameHue(i))); 
      led.setInterpFrameSat(i, noise.addNoiseBtoValueNoWrap(i, led.getInterpFrameSat(i)));
    }
  }
}

//
// pick_symmetry  - pick a random symmetry for the duration of the show
//
void pick_symmetry() {
  symmetry = random8(NUM_SYMMETRIES); // 0-4
  if (symmetry == 0 && current_show < SINGLE_SPOKE_SHOWS) {
    symmetry = random(1, NUM_SYMMETRIES);  // don't allow symmetry 0 (single spoke) for shows < SINGLE_SPOKE_SHOWS
  }
  invert = random8(2) ? true : false;
  switch_axis = (random8(3) == 0 && current_show != 0) ? true : false;
  set_symmetry();
}

//
// set_symmetry
//
void set_symmetry() {
  led.fillBlack();  // clear leds before symmetry change
  uint8_t numSymLeds = ((6 / symmetries[symmetry]) * SPOKE_LENGTH) + hub_pixels[symmetry];
  shows.resetNumLeds(numSymLeds);  // resets the virtual number of LEDs
  led.fillBlack();  // and clear leds after symmetry change
  led.push_frame();
}

//
// update_leds - push the interp_frame on to the leds - adjust for SYMMETRY!
//
void update_leds() {
  draw_center();
  draw_hub();
  draw_spokes();
}

//
// draw_center - fill in the center circle - Virtual led 0 maps to the last pixel
//
void draw_center() {
  leds[NUM_LEDS-1] = led.getInterpFrameColor(0);
}

//
// draw_hub - fill in the 6 petals around the center with virtual leds (1+)
//            Virtual leds 1+ map to end -6:-1 pixels 
//
void draw_hub() {
  uint8_t pixel;
  uint8_t pitch = 6 / symmetries[symmetry];
  for (uint8_t i = 0; i < pitch; i++) {
    for (uint8_t j = 0; j < symmetries[symmetry]; j++) {
      pixel = try_rotate(SPOKE_LEDS + i + (j * pitch));
      if (switch_axis) {
        pixel = 5 - pixel + (2 * SPOKE_LEDS);  // Invert the hub to get orientation correct
      }
      leds[pixel] = led.getInterpFrameColor(1 + i);
    }
  }
}

//
// draw_spokes - draw the six spokes
//
// spoke virtual leds (2-7:SPOKE_LEDS) map to (0:SPOKE_LEDS) pixels
//
void draw_spokes() {
  uint8_t adj_i, pixel, pitch;

  if (symmetry < 4) {
    pitch = SPOKE_LEDS / symmetries[symmetry];
    
    for (uint8_t i = 0; i < pitch; i++) {      
      for (uint8_t j = 0; j < symmetries[symmetry]; j++) {
        adj_i = (invert && !switch_axis && j % 2 && symmetry == 3) ? pitch - i : i;

        if (switch_axis) {
          pixel = try_axis_switch((adj_i + (j * pitch)) % SPOKE_LEDS) ;
        } else {
          pixel = try_rotate(shift_one(adj_i + (j * pitch)));
        }
        leds[pixel] = led.getInterpFrameColor(hub_pixels[symmetry] + i);
      }
    }
  } else {  // For 12-fold symmetry
    for (uint8_t i = 0; i < ((SPOKE_LENGTH/2)+1); i++) {
      for (uint8_t spoke = 0; spoke < 6; spoke++) {
        leds[shift_one((SPOKE_LENGTH - i) + (spoke * SPOKE_LENGTH))] = led.getInterpFrameColor(hub_pixels[symmetry] + i);
        leds[shift_one(i + (spoke * SPOKE_LENGTH))] = led.getInterpFrameColor(hub_pixels[symmetry] + i);        
      }
    }
  }
}

//
// shift_one - shift the LED one over to fix in software the hardware layout
//
uint8_t shift_one(uint8_t i) {
  return (i + SPOKE_LEDS - 1) % SPOKE_LEDS;
}

//
// try_rotate
//
uint8_t try_rotate(uint8_t i) {
  // Rotate only if NEVER_ROTATE is off, is_rotating is on, and not in switch_axis mode
  if (!NEVER_ROTATE && is_rotating && !switch_axis) {
    return rotate(shows.getCycle() % 6, i);
  } else {
    return i;
  }
}

//
// try_axis_switch - switch the radial and spoke axes
//
uint8_t try_axis_switch(uint8_t i) {
  uint8_t adj_i, offset;

  if (i < 6) {
    adj_i = i * SPOKE_LENGTH;  // innermost symmetric spokes
    
  } else if (i >= SPOKE_LEDS - 6) {
    adj_i = ((i + 6 - SPOKE_LEDS) * SPOKE_LENGTH) + (SPOKE_LENGTH / 2);  // outer tips
    
  } else {
    i -= 6;
    offset = (i / 12) + 1;  // 12 might mean SPOKE_LENGTH and vice versa
    i = i % 12;
    adj_i = ((i / 2) * SPOKE_LENGTH) + offset;
    if (i % 2) {
      adj_i += (SPOKE_LENGTH - (adj_i % SPOKE_LENGTH) - offset);
    }
  }
  return SPOKE_LEDS - adj_i - 1;  // Oddly backwards
}

//
// rotate - rotate a pixel (i) by 0-5 arms (r)
//
uint8_t rotate(uint8_t r, uint8_t i) {
  if (r == 0 || i == (NUM_LEDS-1)) {  // not rotated or center
    return i;
  } else if (i >= SPOKE_LEDS) {  // spoke hub
    return (((i-SPOKE_LEDS) + r) % 6) + SPOKE_LEDS;
  } else {  // spoke
    return (i + (r * SPOKE_LENGTH)) % SPOKE_LEDS;
  }
}

//
// check_fades - check the fade-to-blacks at beginning and end of show
//
void check_fades() {
  uint8_t fade_amount = 0;
  uint16_t small_cycle = shows.getSmallCycle();
  
  if (small_cycle <= (FADE_IN_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map(small_cycle, 0, (FADE_IN_TIME * 1000 / DELAY_TIME), 255, 0);
  } else if ((MAX_SMALL_CYCLE - small_cycle) <= (FADE_OUT_TIME * 1000 / DELAY_TIME)) {
    fade_amount = map((MAX_SMALL_CYCLE - small_cycle), 0, (FADE_OUT_TIME * 1000 / DELAY_TIME), 255, 0);
  }
  
  // Fade lights
  if (fade_amount > 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(fade_amount);
    }
  }

  // Speak
  if (ARE_CONNECTED && IS_SPEAKING && shows.getMorph() == 0) {
    if (fade_amount > 0) {
      speak_all_commands();
    } else {
      speak_cycle();
    }
  }
}

//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern_byte = pgm_read_byte_near(pattern_matrix + 
                                            (SNOWFLAKE_SIZE * (NUM_PATTERNS * 2)) + 
                                            (pattern_number * 2) +
                                            (n / 8)
                                            );
  return (pattern_byte & (1 << (7 - (n % 8)) ));
}

//
// log status
//
void log_status() {
  Serial.print("Show: ");
  Serial.print(current_show);
  Serial.print(", Symmetry: ");
  Serial.print(symmetry);
  Serial.print(", Invert: ");
  Serial.print(invert);
  Serial.print(", Rotate: ");
  Serial.print(is_rotating);
  Serial.println(".");
}

