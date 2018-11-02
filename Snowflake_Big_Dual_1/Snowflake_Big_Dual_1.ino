#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Snowflakes
//
//  10/16/2018
//
//  Dual shows - Blend together 2 shows running at once
//
//  Try to save on memory
//
//  Removed: Noise, Shuffle shows, rotating, xBee (restore later?)
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

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // Hook for FastLED library

#define CAN_CHANGE_PALETTES false  // Palettes

// Shows
#define START_SHOW_CHANNEL_A  0  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  1
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 1 };
boolean has_black_pattern[] = { true, false };
#define NUM_SHOWS 12
#define SINGLE_SPOKE_SHOWS 3  // Shows here and above can have a single spoke

// Clocks and time
#define SHOW_DURATION 30  // seconds. Size problems at 1800+ seconds.
#define FADE_TIME 10   // seconds to fade in + out
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// symmetries - 6 arms
#define NUM_SYMMETRIES 5
uint8_t symmetry[] = { 0, 1 };  // 0-4
boolean invert[] = { true, false };  // Whether to invert odd arms
boolean switch_axis[] = { true, false };  // Whether to switch spoke + radial axes
const uint8_t symmetries[] = { 1, 2, 3, 6, 6 };
const uint8_t hub_pixels[] = { 7, 4, 3, 2, 2 };  // 1 + (6 / symmetries[n]);

/*
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
*/

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

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);  // Only 1 leds object
  FastLED.setBrightness( BRIGHTNESS );
  
  if (CAN_CHANGE_PALETTES) {
    for (uint8_t i = 0; i < DUAL; i++) {
      led[i].setPalette();  // turns on palettes with default values
    }
  }

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show
  
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].fillBlack();
    led[i].push_frame();
    set_symmetry(i);
    shows[i].setColorSpeedMinMax(2, 10); // Speed up color changing
  }

  // if (ARE_CONNECTED && IS_SPEAKING) { speak_all_commands(); }
}

void loop() {
  
  for (uint8_t i = 0; i < DUAL; i++) {

    if (get_intensity(i) > 0) {
      
      switch (current_show[i]) {
  
        case 0:
          patternsBW(i);
          break;
        case 1:
          shows[i].twoColor();
          break;
        case 2:
          shows[i].morphChain();
          break;
        case 3:
          shows[i].lightWave(shows[i].getForeColorSpeed());
          break;
        case 4:
          shows[i].sawTooth();
          break;
        case 5:
          shows[i].lightRunUp();
          break;
        case 6:
          shows[i].packets();
          break;
        case 7:
          shows[i].packets_two();
          break;
        case 8:
          shows[i].sinelon_fastled();
          break;
        case 9:
          shows[i].bpm_fastled();
          break;
        case 10:
          shows[i].juggle_fastled();
          break;
        default:
          shows[i].bands();
          break;
      }
  
      shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
    }
  }

  update_leds();  // morph together the 2 chanels & push the interp_frame on to the leds
  FastLED.show();  // Update the display

  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { 
      next_show(i); 
    }
  }
  
  delay(DELAY_TIME); // The only delay

  // if (ARE_CONNECTED && !IS_SPEAKING) { HearSignal(); }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  led[i].fillBlack();
  led[i].push_frame();
  led[i].randomizePalette();
  
  shows[i].resetAllClocks();

  pick_next_show();

  //  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  // Switch between a patterns show and all the other shows
  current_pattern[i] = random8(NUM_PATTERNS);
  has_black_pattern[i] = random8(2) ? true : false;
  current_show[i] = (current_show[i] == 0) ? random8(2, NUM_SHOWS) : false;
  //  current_show = (current_show + 1) % NUM_SHOWS;  // For debugging
  pick_symmetry(i);
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
}

/*
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
*/

////// Specialized shows

//
// patterns shows
//
void patternsBW(uint8_t channel) {
  map_pattern(get_bit_from_pattern_number(0, current_pattern[channel]), 0, channel);  // Center
  map_pattern(get_bit_from_pattern_number(1, current_pattern[channel]), 1, channel);  // Start
  
  for (int i = 0; i < SPOKE_LENGTH; i++) {
    map_pattern(get_bit_from_pattern_number(i+2, current_pattern[channel]), i + hub_pixels[symmetry[channel]]);
  }
}

void map_pattern(boolean isOn, uint8_t i, uint8_t channel) {
  if (isOn) {  // For pattern table
    if (has_black_pattern[channel]) {
      shows[channel].setPixeltoHue(i, shows[channel].getForeColor() + i);  // 1 = foreColor
    } else {
      shows[channel].setPixeltoForeColor(i);  // 1 = foreColor
    }
  } else {
    if (has_black_pattern[channel]) {
      shows[channel].setPixeltoBlack(i);  // 0 = either Black or backColor
    } else {
      shows[channel].setPixeltoBackColor(i);
    }
  }
}

///////// DUAL SHOW LOGIC

//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  uint8_t intensity_a = get_intensity(CHANNEL_A);
  uint8_t intensity_b = get_intensity(CHANNEL_B);
  
  if (intensity_a == 0) {
    update_leds_one_channel(CHANNEL_B);  // Channel A is off. Use only Channel B.
  } else if (intensity_b == 0) {
    update_leds_one_channel(CHANNEL_A);  // Channel B is off. Use only Channel A.
  } else {
    uint8_t fract = get_weighted_fract(intensity_a, intensity_b);
    morph_channels(fract);
  }
}

//
// update_leds_one_channel - push the a single channel's interp_frame on to the leds
//
void update_leds_one_channel(uint8_t channel) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = led[channel].getInterpFrameColor(i);
  }
}

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  CHSV led_a, led_b;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    led_a = led[CHANNEL_A].getInterpFrameColor(i);
    led_b = led[CHANNEL_B].getInterpFrameColor(i);
    leds[i] = led[CHANNEL_A].getInterpHSV(led_a, led_b, fract);  // interpolate a + b channels
  }
}

//
// Calculate the amount of b (0-255) from the intensities of A and B
//
uint8_t get_weighted_fract(uint8_t a, uint8_t b) {
  if (a == 0 && b == 0) { return 0; }  // Prevent divide-by-0
  
  return b * 255 / (a + b);
}

//
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint8_t intensity;  // 0 = Off, 255 = full On
  uint16_t small_cycle = shows[i].getSmallCycle();

  // Similar logic to check_fades (deprecated)
  if (small_cycle <= FADE_CYCLES) {
    intensity = map(small_cycle, 0, FADE_CYCLES, 0, 255);  // rise
  } else if (small_cycle <= (MAX_SMALL_CYCLE / 2)) {
    intensity = 255;  // Show is 100%
  } else if (small_cycle <= ((MAX_SMALL_CYCLE / 2) + FADE_CYCLES)) {
    intensity = map(small_cycle - (MAX_SMALL_CYCLE / 2), 0, FADE_CYCLES, 255, 0);  // decay
  } else {
    intensity = 0;
  }
  return intensity;
}

//// End DUAL SHOW LOGIC

//
// pick_symmetry  - pick a random symmetry for the duration of the show
//
void pick_symmetry(uint8_t i) {
  symmetry[i] = random8(NUM_SYMMETRIES); // 0-4
  if (symmetry[i] == 0 && current_show[i] < SINGLE_SPOKE_SHOWS) {
    symmetry[i] = random(1, NUM_SYMMETRIES);  // don't allow symmetry 0 (single spoke) for shows < SINGLE_SPOKE_SHOWS
  }
  invert[i] = random8(2) ? true : false;
  switch_axis[i] = (random8(3) == 0 && current_show[i] != 0) ? true : false;
  set_symmetry(i);
}

//
// set_symmetry
//
void set_symmetry(uint8_t i) {
  led[i].fillBlack();  // clear leds before symmetry change
  uint8_t numSymLeds = ((6 / symmetries[symmetry[i]]) * SPOKE_LENGTH) + hub_pixels[symmetry[i]];
  shows[i].resetNumLeds(numSymLeds);  // resets the virtual number of LEDs
  led[i].fillBlack();  // and clear leds after symmetry change
  led[i].push_frame();
}

//
// update_leds - push the interp_frame on to the leds - adjust for SYMMETRY!
//
void update_leds(uint8_t i) {
  draw_center(i);
  draw_hub(i);
  draw_spokes(i);
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
      leds[pixel] = led[channel].getInterpFrameColor(1 + i);
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
        leds[pixel] = led[channel].getInterpFrameColor(hub_pixels[symmetry] + i);
      }
    }
  } else {  // For 12-fold symmetry
    for (uint8_t i = 0; i < ((SPOKE_LENGTH/2)+1); i++) {
      for (uint8_t spoke = 0; spoke < 6; spoke++) {
        leds[shift_one((SPOKE_LENGTH - i) + (spoke * SPOKE_LENGTH))] = led[channel].getInterpFrameColor(hub_pixels[symmetry] + i);
        leds[shift_one(i + (spoke * SPOKE_LENGTH))] = led[channel].getInterpFrameColor(hub_pixels[symmetry] + i);        
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
void log_status(uint8_t i) {
  Serial.print("Channel: ");
  Serial.print(i);
  Serial.print("Show: ");
  Serial.print(current_show[i]);
  Serial.print(", Symmetry: ");
  Serial.print(symmetry[i]);
  Serial.print(", Invert: ");
  Serial.print(invert[i]);
  Serial.println(".");
}

