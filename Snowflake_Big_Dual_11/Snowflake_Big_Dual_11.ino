#include <FastLED.h>
#include <Led.h>
#include <Shows.h>

//
//  Snowflakes
//
//  11/17/2018
//
//  FOR TEENSY ONLY
//
//  Dual shows - Blend together 2 shows running at once
//
//  Removed: Noise, palettes, switch_axis
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
#define SNOWFLAKE_NUMBER 3  // 0 = Small Simple, 1 = Medium Pointy, 2 = Large Complex, 3 = Medium Inward

#define SPOKE_LENGTH 10  // [8, 10, 12, 10]
#define HUB_LENGTH 1  // [1, 2, 1, 1]

#define ARE_CONNECTED false  // Are Snowflakes talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

#define SPOKE_LEDS  (SPOKE_LENGTH * 6)  // Don't change
#define HUB_LEDS  (HUB_LENGTH * 6)  // Don't change
#define NUM_LEDS  (SPOKE_LEDS + HUB_LEDS + 1)  // Don't change

uint8_t BRIGHTNESS = 255;  // (0-255)

#define VARIABLE_SPOKE_BRIGHTNESS true
uint8_t spoke_brightness = 96;  // Dim the spokes (not the center or hub) by * X / 256
uint8_t brightness_clock = 0;  // Don't change
// 96: center bright, 128: center a little bright 192: even lighting, 256: center dim

#define DELAY_TIME 40  // in milliseconds (usually 40)

#define DATA_PIN 9
#define CLOCK_PIN 8

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[NUM_LEDS];  // The Leds themselves

// Shows
#define START_SHOW_CHANNEL_A  0  // Starting show for Channels A
#define START_SHOW_CHANNEL_B  1  // Starting show for Channels B
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 1 };
boolean has_black_pattern[] = { false, false };
#define NUM_SHOWS 14
#define SINGLE_SPOKE_SHOWS 3  // Shows here and above can have a single spoke

// Clocks and time
#define SHOW_DURATION 30  // 30 seconds. Size problems at 1800+ seconds.
#define FADE_TIME 30   // seconds to fade in. If FADE_TIME = SHOW_DURATION, Always Be Fading
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

boolean is_lost = false;
unsigned long last_connection = millis();
#define MAX_SILENT_TIME  (3 * 1000)  // Time (in sec) without communication before marked as is_lost

// symmetries - 6 arms
#define NUM_SYMMETRIES 5
uint8_t symmetry[] = { 3, 3 };  // 0-4
boolean invert[] = { false, false };  // Whether to invert (0 <-> 1) patterned shows
const uint8_t symmetries[] = { 1, 2, 3, 6, 6 };  // Don't change

// xBee language
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_FORE_SPEED 'X'
#define COMMAND_BACK_SPEED 'Y'
#define COMMAND_BRIGHTNESS 'I'
#define COMMAND_SYMMETRY   'M'
#define COMMAND_HASBLACK   'H'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_CYCLE      'C'
#define COMMAND_CHANNEL_A  'x'
#define COMMAND_CHANNEL_B  'y'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define EMPTY_CHAR         ' '
#define MAX_MESSAGE       200
#define MAX_NUM             5  // To handle 65,535 of small_cycle
char message[MAX_MESSAGE];     // Incoming message buffer
char number_buffer[MAX_NUM];   // Incoming number buffer

//
// PATTERNS
//
#define NUM_PATTERNS 6   // Total number of patterns

const uint8_t pattern_matrix[] PROGMEM = {  // A pattern is 2-byte of binary
  // Small Simple Snowflake (8-spoke)               
  B01010111, B01000000,
  B10101101, B10000000,
  B01001111, B01000000,
  B10110111, B00000000,
  B11001101, B10000000,
  B11001010, B10000000,

  // Medium Pointy Snowflake (10-spoke)
  // C=center, I=inner hub, O=outer hub, 1=next outer, 2=next outer, S=squashed Tip, T=Pointy tip,
  // BCIO1xxSx, Bx2xTx000,
  B01000010, B00101000,
  B10010111, B01010000,
  B01101010, B10000000,
  B10110000, B11100000,
  B01101010, B10101000,
  B11010101, B01111000,
  
  // Large Complex Snowflake (12-spoke)
  B01010101, B11010100,
  B10001011, B01101000,
  B01101010, B11001100,
  B10110000, B11110000,
  B01110010, B10101100,
  B11000111, B01110000,

  // Medium Inward Snowflake (10-spoke)
  B01001010, B10010000,
  B11100010, B10000000,
  B10110000, B01100000,
  B01110011, B01000000,
  B10101010, B10100000,
  B10110101, B01100000,
};

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");  // For debugging

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show
  
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].fillBlack();
    led[i].push_frame();
    set_symmetry(i);
    shows[i].setColorSpeedMinMax(2, 10); // Speed up color changing
  }
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
        case 11:
          shows[i].randomColors();
          break;
        case 12:
          shows[i].randomFill();
          break;
        default:
          shows[i].bands();
          break;
      }
  
      shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
    }
  }

  update_leds();  // push the interp_frames on to the leds
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
  speak_and_hear();  // speak out or hear in signals
  delay(DELAY_TIME); // The only delay

  if (VARIABLE_SPOKE_BRIGHTNESS) { 
    EVERY_N_SECONDS(1) { advance_brightness_clock(); }  // in -> out takes 2 minutes
  };
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE && !is_listening()) { 
      next_show(i);
      pick_next_show(i);
    }
  }
}

// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  led[i].fillBlack();
  led[i].push_frame();
  
  shows[i].resetAllClocks();

//  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  // Switch between a patterns show and all the other shows
  current_pattern[i] = random8(NUM_PATTERNS);
  has_black_pattern[i] = (random8(2) == 0) ? true : false ;
  current_show[i] = is_other_channel_show_zero(i) ? random8(2, NUM_SHOWS) : 0 ;
  //  current_show = (current_show + 1) % NUM_SHOWS;  // For debugging
  pick_symmetry(i);
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
}

boolean is_other_channel_show_zero(uint8_t channel) {
  if (channel == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

////// Speaking

void speak_and_hear() {
  if (ARE_CONNECTED && IS_SPEAKING) { speak_all_commands(); }
  if (ARE_CONNECTED && !IS_SPEAKING) { hear_signal(); }
}

boolean is_listening() {
  return (ARE_CONNECTED && !IS_SPEAKING && !is_lost);
}

void speak_all_commands() {
  for (uint8_t i = 0; i < DUAL; i++) {  // Send one channel, then the next
    speak_channel(i);  // Send the channel A + B prompt
    speak_command(COMMAND_FORE, shows[i].getForeColor());
    speak_command(COMMAND_BACK, shows[i].getBackColor());
    speak_command(COMMAND_FORE_SPEED, shows[i].getForeColorSpeed());
    speak_command(COMMAND_BACK_SPEED, shows[i].getBackColorSpeed());
    speak_command(COMMAND_BRIGHTNESS, BRIGHTNESS);
    speak_command(COMMAND_WAIT, shows[i].getWait());
    speak_command(COMMAND_CYCLE, shows[i].getSmallCycle());
    speak_command(COMMAND_SYMMETRY, symmetry[i]);
    speak_command(COMMAND_HASBLACK, has_black_pattern[i]);
  }
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

void speak_channel(int i) {
  char command_channel = (i == 0) ? COMMAND_CHANNEL_A : COMMAND_CHANNEL_B ;
  Serial.print(command_channel);
}


////// Hearing

void hear_signal() {
  if (Serial.available()) {  // Heard a signal!
    ResetLostCounter();
    GetMessage(message);  // Load global message buffer by pointer
    ProcessMessage(message);
  } else {
    if (!is_lost && (millis() - last_connection > MAX_SILENT_TIME) ) {
      is_lost = true;
    }
  }
}

//
// ResetLostCounter - board is not lost
//
void ResetLostCounter() {
  is_lost = false;
  last_connection = millis();
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
void ProcessMessage(char* message) {
  uint8_t i = 0;
  uint8_t channel = 0;  // Default to channel 0
  
  while (i < MAX_MESSAGE) {
    char command = message[i];  // Get one-letter command
    
    if (command == COMMAND_PERIOD) { return; }  // end of message

    if (command == COMMAND_CHANNEL_A) {
      i++;
      channel = 0;
    } else if  (command == COMMAND_CHANNEL_B) {
      i++;
      channel = 1;
    } else {
      for (uint8_t j = 0; j < MAX_NUM; j++) { 
        number_buffer[j] = EMPTY_CHAR;  // Clear number buffer
      }
    
      int numsiz = ReadNum(message, i+1, number_buffer);  // Read in the number
      i = i + 2 + numsiz; // 2 = leap beginning command and trailing comma
      
      ExecuteOrder(command, atoi(number_buffer), channel);
    }
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
void ExecuteOrder(char command, uint16_t value, uint8_t i) {
      
  switch (command) {
    
    case COMMAND_FORE:
      shows[i].setForeColor(value);
      break;
    
    case COMMAND_BACK:
      shows[i].setBackColor(value);
      break;

    case COMMAND_FORE_SPEED:
      shows[i].setForeColorSpeed(value);
      break;
    
    case COMMAND_BACK_SPEED:
      shows[i].setBackColorSpeed(value);
      break;
    
    case COMMAND_BRIGHTNESS:
      if (BRIGHTNESS != value) {
        BRIGHTNESS = value;
        FastLED.setBrightness( BRIGHTNESS );
      }
      break;
    
    case COMMAND_WAIT:
      shows[i].setWait(value);
      break;
    
    case COMMAND_SHOW:  // Show
      if (current_show[i] != value) {
        current_show[i] = value;
        next_show(i);
      }
      break;
    
    case COMMAND_CYCLE: // small_cycle
      shows[i].setSmallCycle(value);
      break;

    case COMMAND_SYMMETRY:  // symmetry
      if (symmetry[i] != value) {
        symmetry[i] = value;
        set_symmetry(i);
      }
      break;

    case COMMAND_HASBLACK:  // has_black_pattern
      has_black_pattern[i] = value;
      break;
  }
}

////// End Speaking & Hearing

//
// advance_brightness_clock
//
void advance_brightness_clock() {
  // Clock goes up by evens to 254
  if (brightness_clock % 2 == 0) {
    brightness_clock += 2;
    if (brightness_clock == 254) {
      brightness_clock = 255;
    }
  } else {
    // Clock goes down by odds to 1
    brightness_clock -= 2;
    if (brightness_clock == 1) {
      brightness_clock = 0;
    }
  }
  spoke_brightness =  map8(sin8_C(brightness_clock), 96, 255);
}

////// Specialized shows

//
// patterns shows
//
void patternsBW(uint8_t channel) {
  map_pattern(get_bit_from_pattern_number(0, current_pattern[channel]), 0, channel);  // Center

  for (int i = 0; i < HUB_LENGTH; i++) {
    map_pattern(get_bit_from_pattern_number(i+1, current_pattern[channel]), i+1, channel);  // Hub
  }
  
  for (int i = 0; i < SPOKE_LENGTH; i++) {
    if (i == SPOKE_LENGTH - 1 && symmetry[channel] < 3) {
      shows[channel].setPixeltoBlack(i + get_hub_pixels(symmetry[channel]));  // Hack: strip off the last odd pixel
    } else {
      map_pattern(get_bit_from_pattern_number(i+1+HUB_LENGTH, current_pattern[channel]), i + get_hub_pixels(symmetry[channel]), channel);
    }
  }
}

void map_pattern(boolean isOn, uint8_t i, uint8_t channel) {
  if (invert[channel]) { 
    isOn = (isOn == 1) ? 0 : 1 ;  // invert pattern (0 <-> 1)
    if (!has_black_pattern[channel]) { shows[channel].fillForeColor(); }  // Test this
  }
  
  if (isOn) {  // For pattern table
    if (has_black_pattern[channel]) {
      shows[channel].setPixeltoHue(i, shows[channel].getForeColor() + i);  // 1 = foreColor
    } else {
      shows[channel].setPixeltoForeColor(i);  // 1 = foreColor
    }
  } else {
    if (has_black_pattern[channel]) {
      shows[channel].setPixeltoBlack(i);  // 0 = Black or backColor
    } else {
      shows[channel].setPixeltoBackColor(i);  // 0 = backColor
    }
  }
}

///////// DUAL SHOW LOGIC

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][i], 
                                          leds_buffer[CHANNEL_A][i], 
                                          fract);  // interpolate a + b channels
  }
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
  invert[i] = (random8(2) == 0) ? true : false;
  set_symmetry(i);
}

//
// set_symmetry
//
void set_symmetry(uint8_t i) {
  led[i].fillBlack();  // clear leds before symmetry change
  uint8_t numSymLeds = ((6 / symmetries[symmetry[i]]) * SPOKE_LENGTH) + get_hub_pixels(symmetry[i]);
  shows[i].resetNumLeds(numSymLeds);  // resets the virtual number of LEDs
  led[i].fillBlack();  // and clear leds after symmetry change
  led[i].push_frame();
}

//
// update_leds - push the interp_frames on to the leds - adjust for SYMMETRY!
//
void update_leds() {
  for (uint8_t i = 0; i < DUAL; i++) {
    draw_center(i);
    draw_hub(i);
    draw_spokes(i);
  }
}

//
// draw_center - fill in the center circle - Virtual led 0 maps to the last pixel
//
void draw_center(uint8_t channel) {
  leds_buffer[channel][NUM_LEDS-1] = led[channel].getInterpFrameColor(0);
}

//
// draw_hub - fill in the 6 petals around the center with virtual leds (1+)
//            Virtual leds 1+ map to end -6:-1 pixels 
//
void draw_hub(uint8_t channel) {
  uint8_t trans_i, pixel;
  uint8_t pitch = HUB_LEDS / symmetries[symmetry[channel]];
  
  for (uint8_t i = 0; i < pitch; i++) {
    for (uint8_t j = 0; j < symmetries[symmetry[channel]]; j++) {
      trans_i = i;  // This is where switch_axis could happen
      pixel = SPOKE_LEDS + trans_i + (j * pitch);

      if (SNOWFLAKE_NUMBER == 1 || SNOWFLAKE_NUMBER == 3) { pixel = shift_one_hub(pixel); }
      
      leds_buffer[channel][pixel] = led[channel].getInterpFrameColor(1 + i);
    }
  }
}

//
// draw_spokes - draw the six spokes
//
// spoke virtual leds (2-7:SPOKE_LEDS) map to (0:SPOKE_LEDS) pixels
//
void draw_spokes(uint8_t channel) {
  uint8_t symm_type = symmetry[channel];  // 0-4
  uint8_t symm = symmetries[symm_type];  // 1, 2, 3, 6, 6
  uint8_t hub_pixels = get_hub_pixels(symm_type);
  uint8_t pitch = SPOKE_LEDS / symm;
  uint8_t trans_i, pixel;
  
  CHSV color;
  
  if (symm_type < 4) {
    for (uint8_t i = 0; i < pitch; i++) {  // SPOKE_LEDS / symm;  
      for (uint8_t j = 0; j < symm; j++) {  // 1, 2, 3, 6, 6
        trans_i = i;  // This is where switch_axis could happen
        pixel = shift_one(trans_i + (j * pitch));
        if (SNOWFLAKE_NUMBER == 3) {
          pixel = shift_one(pixel);  // Hack for Medium Inward Snowfalke
        }
        color = led[channel].getInterpFrameColor(hub_pixels + i);
        leds_buffer[channel][pixel] = dim_spoke(color);
      }
    }
  } else {  // For 12-fold symmetry
    for (uint8_t i = 0; i < (SPOKE_LENGTH / 2) + 1; i++) {
      for (uint8_t spoke = 0; spoke < 6; spoke++) {
        color = led[channel].getInterpFrameColor(hub_pixels + i);
        leds_buffer[channel][shift_one((SPOKE_LENGTH - i) + (spoke * SPOKE_LENGTH))] = dim_spoke(color);
        leds_buffer[channel][shift_one(i + (spoke * SPOKE_LENGTH))] = dim_spoke(color);
      }
    }
  }
}

//
// dim_spoke - dim the value part of the CHSV color
//
CHSV dim_spoke(CHSV color) {
  return CHSV(color.h, color.s, scale8(color.v, spoke_brightness));
}

//
// shift_one - shift the LED over one in software to fix the hardware layout
//
uint8_t shift_one(uint8_t i) {
  return (i + SPOKE_LEDS - 1) % SPOKE_LEDS;
}

uint8_t shift_one_hub(uint8_t i) {
  return ((i - SPOKE_LEDS + 1) % HUB_LEDS) + SPOKE_LEDS;
}

//
// get_hub_pixels - Get the number of symmetric hub pixels
//
uint8_t get_hub_pixels(uint8_t n) {
  return (1 + (HUB_LENGTH * (6 / symmetries[n])));
}

//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern_byte = pgm_read_byte_near(pattern_matrix + 
                                            (SNOWFLAKE_NUMBER * (NUM_PATTERNS * 2)) + 
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
  Serial.print(", Show: ");
  Serial.print(current_show[i]);
  Serial.print(", Symmetry: ");
  Serial.print(symmetry[i]);
  Serial.print(", Invert: ");
  Serial.print(invert[i]);
  Serial.println(".");
}

