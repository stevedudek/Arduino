#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>

//
//  Snowflakes
//
//  4/5/2021
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  FOR TEENSY ONLY
//
//  Complex Switch Axis
//
//  White Lightning
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  Some big updates: smoothing
//
#define SNOWFLAKE_NUMBER 0  // 0 = Small Simple, 1 = Medium Pointy, 
                            // 2 = Large Complex, 3 = Medium Inward

#define SPOKE_LENGTH 8  // [8, 10, 12, 10]
#define HUB_LENGTH 1     // [1,  2,  1,  1]

#define ARE_CONNECTED false  // Are Snowflakes talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 20;  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.
uint32_t timing[] = { millis(), millis() };  // last_time, last_connection
#define SMOOTHING 1   // 0 = no smooth, lower the number = more smoothing

#define SPOKE_LEDS  (SPOKE_LENGTH * 6)  // Don't change
#define HUB_LEDS  (HUB_LENGTH * 6)  // Don't change
#define NUM_LEDS  (SPOKE_LEDS + HUB_LEDS + 1)  // Don't change

#define VARIABLE_SPOKE_BRIGHTNESS true
uint8_t spoke_brightness = 96;  // Dim the spokes (not the center or hub) by * X / 256
uint8_t brightness_clock = 0;  // Don't change
// 96: center bright, 128: center a little bright 192: even lighting, 256: center dim

#define DATA_PIN 7  // CHANGED!
#define CLOCK_PIN 8  // CHANGED!

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CRGB leds[NUM_LEDS];  // The Leds themselves
CHSV led_buffer[NUM_LEDS];  // For smoothing

// Shows
#define START_SHOW_CHANNEL_A  1  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B  0  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
uint8_t current_pattern[] = { 0, 1 };
boolean has_black_pattern[] = { false, false };
#define NUM_SHOWS 12
#define SINGLE_SPOKE_SHOWS 4  // Shows here and above can have a single spoke

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;
#define MAX_COLOR 256   // Colors are 0-255
#define BLACK  CHSV(0, 0, 0)

// Clocks and time
#define SHOW_DURATION 40  // Typically 30 seconds. Size problems at 1800+ seconds.
uint8_t FADE_TIME = 40;  // seconds to fade in + out (Arduino Blue)
#define MAX_SMALL_CYCLE  (SHOW_DURATION * 2 * (1000 / DELAY_TIME))  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

boolean is_lost = false;
#define MAX_SILENT_TIME  (3 * 1000)  // Time (in sec) without communication before marked as is_lost

// symmetries - 6 arms
#define NUM_SYMMETRIES 5
uint8_t symmetry[] = { 2, 2 };  // 0-4
boolean invert[] = { false, false };  // Whether to invert (0 <-> 1) patterned shows
boolean switch_axis[] = { false, false };
const uint8_t symmetries[] = { 1, 2, 3, 6, 6 };  // Don't change

// ArduinoBlue
ArduinoBlue phone(Serial2); // Blue Tx = pin 9; Blue Rx = pin 10
#define HUE_SLIDER        0
#define HUE_WIDTH_SLIDER  1
#define SPEED_SLIDER      2
#define BRIGHTNESS_SLIDER 3
#define FADE_TIME_SLIDER  4
#define BAM_BUTTON        0
#define BOLT_TIME        20

// xBee language
#define COMMAND_START      '+'
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_BRIGHTNESS 'I'
#define COMMAND_PALETTE    'U'
#define COMMAND_PAL_WIDTH  'J'
#define COMMAND_SPEED      'Y'
#define COMMAND_FADE       'R'
#define COMMAND_BOLT       'X'
#define COMMAND_SYMMETRY   'M'
#define COMMAND_HASBLACK   'H'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_SM_CYCLE   'C'
#define COMMAND_CHANNEL_A  'x'
#define COMMAND_CHANNEL_B  'y'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define EMPTY_CHAR         ' '
#define MAX_MESSAGE       100
#define MAX_NUM             6  // To handle 65,535 of small_cycle
char message[MAX_MESSAGE];     // Incoming message buffer
char number_buffer[MAX_NUM];   // Incoming number buffer

#define MAX_GROUP_LENGTH 5  // How many pixels in a group (row-width)
#define MAX_GROUPS 5  // How many groups in a Snowflake (column-length)
#define END_GRP 99  // Don't change any of these

#define NUM_PATTERNS 6   // Total number of patterns

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
//  Serial.begin(9600);
//  Serial.println("Start");  // Serial: For debugging
  Serial1.begin(9600);  // Serial1: xBee port
  Serial2.begin(9600);  // Serial2: Bluetooth serial (ArduinoBlue)
  
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].fillForeBlack();
    led[i].push_frame();
    set_symmetry(i);
    shows[i].setWaitRange(5, 30);  // How fast animation happens
    shows[i].setColorSpeedMinMax(4, 40); // Speed up color changing
    shows[i].turnOnBlur();  // And leave it on
  }
  
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);
  timing[0] = millis();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = BLACK;
  }

  if (ONLY_RED) {  // (ArduinoBlue)
    hue_start = 192;
    hue_width = 124;
  }
}

//
// loop
//
void loop() {
  
  for (uint8_t i = 0; i < DUAL; i++) {
      
    switch (current_show[i]) {

      case 0:
        patternsBW(i);
        break;
      case 1:
        shows[i].twoColor();
        break;
      case 2:
        shows[i].randomTwoColorBlack();
        break;
      case 3:
        shows[i].morphChain();
        break;
      case 4:
        shows[i].lightWave();
        break;
      case 5:
        shows[i].sawTooth();
        break;
      case 6:
        shows[i].lightRunUp();
        break;
      case 7:
        shows[i].sinelon_fastled();
        break;
      case 8:
        shows[i].juggle_fastled();
        break;
      case 9:
        shows[i].randomColors();
        break;
      case 10:
        shows[i].randomFill();
        break;
      default:
        shows[i].bands();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  check_phone();  // Check the phone settings
  update_leds();
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
  fixed_delay();
  speak_and_hear();  // speak out or hear in signals
  
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
  
  if (curr_lightning > 0 ) {  // (ArduinoBlue)
    curr_lightning--; // Reduce the current bolt
  }
}

//
// fixed_delay - make every cycle the same time
//
void fixed_delay() {
  long new_time = millis();
  long time_delta = new_time - timing[0];  // how much time has elapsed? Usually 3-5 milliseconds
  timing[0] = new_time;  // update the counter
  if (time_delta < DELAY_TIME) {  // if we have excess time,
    delay(DELAY_TIME - time_delta);  // delay for the excess
  }
}

// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  led[i].fillBlack();
  led[i].push_frame();
  shows[i].resetAllClocks();
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(2, 22, 20);
  
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

  // Set new show's colors to those of the other channel
  uint8_t other_channel = (i == 0) ? 1 : 0 ;
  shows[i].setForeColor(shows[other_channel].getForeColor());
  shows[i].setBackColor(shows[other_channel].getBackColor());
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
  if (ARE_CONNECTED && IS_SPEAKING) { speak_commands(); }
  if (ARE_CONNECTED && !IS_SPEAKING) { hear_signal(); }  
}

boolean is_listening() {
  return (ARE_CONNECTED && !IS_SPEAKING && !is_lost);
}

void speak_commands() {
  for (uint8_t i = 0; i < DUAL; i++) {  // Send one channel, then the next
    if (shows[i].getMorph() == 0) {
      speak_channel_commands(i);
    }
  }
}

void speak_channel_commands(uint8_t i) {
  // Speak all the commands for a particular channel
  uint8_t m = 0;  // where we are in the send-out message string

  m = speak_start(m);
  m = speak_channel(i, m);  // Send the channel A or B prompt

  m = speak_command(COMMAND_SM_CYCLE, shows[CHANNEL_A].getSmallCycle(), m);
  m = speak_command(COMMAND_FORE, shows[i].getForeColor(), m);
  m = speak_command(COMMAND_BACK, shows[i].getBackColor(), m);
  m = speak_command(COMMAND_SHOW, current_show[i], m);
  m = speak_command(COMMAND_SYMMETRY, symmetry[i], m);
  m = speak_command(COMMAND_HASBLACK, has_black_pattern[i], m);
  m = speak_command(COMMAND_WAIT, shows[i].getWait(), m);
    
  message[m++] = COMMAND_PERIOD;  // Terminate message with null character
  message[m++] = '\0';  // Terminate message with null character
  Serial1.print(message);  // Only done once
//  Serial.println(message);  // For debugging
}

void speak_arduinoblue_commands() {
  uint8_t m = 0;  // where we are in the send-out message string

  m = speak_start(m);
  m = speak_command(COMMAND_BRIGHTNESS, BRIGHTNESS, m);
  m = speak_command(COMMAND_PALETTE, hue_start, m);
  m = speak_command(COMMAND_PAL_WIDTH, hue_width, m);
  m = speak_command(COMMAND_SPEED, DELAY_TIME, m);
  m = speak_command(COMMAND_FADE, FADE_TIME, m);
  m = speak_command(COMMAND_BOLT, curr_lightning, m);
  
  message[m++] = COMMAND_PERIOD;  // Terminate message with null character
  message[m++] = '\0';  // Terminate message with null character
  Serial1.print(message);  // Only done once
//  Serial.println(message);  // For debugging
}

uint8_t speak_start(uint8_t m) {
  message[m++] = COMMAND_START;
  return m;
}

uint8_t speak_command(char cmd, int value, uint8_t m) {
  message[m++] = cmd;

  String value_str = String(value);
  for (uint8_t i = 0; i < value_str.length(); i++) {
    message[m++] = value_str[i];
  }

  message[m++] = COMMAND_COMMA;
  return m;
}

uint8_t speak_channel(int i, uint8_t m) {
  char command_channel = (i == 0) ? COMMAND_CHANNEL_A : COMMAND_CHANNEL_B ;
  message[m++] = command_channel;
  return m;
}

////// Hearing

void hear_signal() {
  if (Serial1.available()) {  // Heard a signal!
    ResetLostCounter();
    boolean msg_received = GetMessage(message);  // Load global message buffer by pointer
    if (msg_received) {
      // Serial.println(message);  // For debugging
      ProcessMessage(message);
    }
  } else {
    if (!is_lost && (millis() - timing[1] > MAX_SILENT_TIME) ) {
      is_lost = true;
    }
  }
}

//
// ResetLostCounter - board is not lost
//
void ResetLostCounter() {
  is_lost = false;
  timing[1] = millis();
}

//
// GetMessage - pulls the whole serial buffer until a period
//
boolean GetMessage(char* message) {
  char tmp;
  boolean have_start_signal = false;
  uint8_t tries = 0;
  uint16_t MAX_TRIES = 2000;
  uint8_t i = 0;

  while (tries++ < MAX_TRIES) {
    if (Serial1.available()) {
      tries = 0;
      tmp = Serial1.read();
//      Serial.print(tmp);  // For debugging

      if (!have_start_signal) {
        if (tmp == COMMAND_START) { 
          have_start_signal = true;  // Start of message
        }
      } else {
        if (tmp == COMMAND_PERIOD) {
          message[i++] = tmp;  // End of message
          message[i++] = '\0';  // End of string
          return true;
        }  
        if (isAscii(tmp)) {
          message[i++] = tmp;
          if (i >= MAX_MESSAGE) {
            Serial.println("Message too long!");  // Ran out of message space
            return false;
          }
        }
      }
    }
  }
  Serial.println("Ran out of tries");
  return false;  // discard message
}

//
// ProcessMessage
//
void ProcessMessage(char* message) {
  uint8_t i = 0;
  uint8_t channel = 0;  // Default to channel 0
  char cmd;
  
  while (i < MAX_MESSAGE) {
    
    cmd = message[i];  // Get one-letter command

    if (cmd == COMMAND_PERIOD) {
      return;
    } else if (!isAscii(cmd)) {
      return;
    } else if (cmd == COMMAND_CHANNEL_A) {
      i++;
      channel = 0;
    } else if (cmd == COMMAND_CHANNEL_B) {
      i++;
      channel = 1;
    } else if (isAlpha(cmd)) {
      for (uint8_t j = 0; j < MAX_NUM; j++) { 
        number_buffer[j] = EMPTY_CHAR;  // Clear number buffer
      }
      
      int numsiz = ReadNum(message, i+1, number_buffer);  // Read in the number
      i = i + 2 + numsiz; // 2 = leap beginning command and trailing comma
      
      ExecuteOrder(cmd, atoi(number_buffer), channel);
    } else {
      return;  // Garbage
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
  Serial.println("Number too long");
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

    case COMMAND_BRIGHTNESS:
      BRIGHTNESS = value;
      FastLED.setBrightness( BRIGHTNESS );
      break;
    
    case COMMAND_PALETTE:
      hue_start = value;
      break;
    
    case COMMAND_PAL_WIDTH:
      hue_width = value;
      break;

    case COMMAND_SPEED:
      DELAY_TIME = value;
      break;

    case COMMAND_FADE:
      FADE_TIME = value;
      break;

    case COMMAND_BOLT:
      curr_lightning = value;
      break;
    
    case COMMAND_WAIT:
      if (shows[i].getWait() != value) { 
        shows[i].setWait(value);
      }
      break;
    
    case COMMAND_SHOW:  // Show
      if (current_show[i] != value) {
        current_show[i] = value;
        next_show(i);
      }
      break;

    case COMMAND_SM_CYCLE: // small_cycle - always channel 0
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
    map_pattern(get_bit_from_pattern_number(i+1+HUB_LENGTH, current_pattern[channel]), i + get_hub_pixels(symmetry[channel]), channel);
    if (symmetry[channel] < 3 && invert[channel] == 0 && i == SPOKE_LENGTH - 1 && SNOWFLAKE_NUMBER != 1) {
      shows[channel].setPixeltoForeBlack(i + get_hub_pixels(symmetry[channel]));  // Hack: strip off the last odd pixel
    } else {
      map_pattern(get_bit_from_pattern_number(i+1+HUB_LENGTH, current_pattern[channel]), i + get_hub_pixels(symmetry[channel]), channel);
    }
  }
}

void map_pattern(boolean isOn, uint8_t i, uint8_t channel) {
  if (invert[channel]) { 
    isOn = (isOn == 1) ? 0 : 1 ;  // invert pattern (0 <-> 1)
  }
  
  if (isOn) {  // For pattern table
    if (has_black_pattern[channel]) {
      shows[channel].setPixeltoHue(i, shows[channel].getForeColor() + i);  // 1 = foreColor
    } else {
      shows[channel].setPixeltoForeColor(i);  // 1 = foreColor
    }
  } else {
    if (has_black_pattern[channel]) {
      shows[channel].setPixeltoBackBlack(i);  // 0 = Black or backColor
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
    CHSV color = led[CHANNEL_A].getInterpHSV(led[CHANNEL_B].getInterpFrameColor(i),
                                             led[CHANNEL_A].getInterpFrameColor(i),
                                             fract);  // interpolate a + b channels
    color = lightning(narrow_palette(color));  // (ArduinoBlue)

    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }
    
    leds[i] = color;  // these are the leds themselves
    led_buffer[i] = color;
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
  return ease8InOutQuad(intensity);
}

//// End DUAL SHOW LOGIC

//
// pick_symmetry  - pick a random symmetry for the duration of the show
//
void pick_symmetry(uint8_t i) {
  symmetry[i] = random8(NUM_SYMMETRIES); // 0-4
  if (current_show[i] < SINGLE_SPOKE_SHOWS) {
    symmetry[i] = random8(1, 4);  // don't allow symmetry 0 or 4 for shows < SINGLE_SPOKE_SHOWS
  }
  invert[i] = (random8(3) == 0) ? true : false;
  switch_axis[i] = (random8(3) == 0 && current_show[i] != 0) ? true : false;
  set_symmetry(i);
}

//
// set_symmetry
//
void set_symmetry(uint8_t i) {
  led[i].fillBlack();  // clear leds before symmetry change
  uint8_t numSymLeds = ((6 / symmetries[symmetry[i]]) * SPOKE_LENGTH) + get_hub_pixels(symmetry[i]);
  if (symmetry[i] == 4) {
    numSymLeds = (SPOKE_LENGTH / 2) + 1 + get_hub_pixels(symmetry[i]);  // Hack for Symmetry 4
  }
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
    if (symmetry[i] == 4) {
      draw_spokes_symm_4(i);  // Hack for Symmetry 4
    } else {
      draw_spokes(i);
    }
  }
}

//
// draw_center - fill in the center circle - Virtual led 0 maps to the last pixel
//
void draw_center(uint8_t channel) {
  //// An intermediate buffer might be necessary. Showing older code below.
  // leds_buffer[channel][NUM_LEDS-1] = led[channel].getInterpFrameColor(0);
  led[channel].setInterpFrame(NUM_LEDS-1, led[channel].getInterpFrameColor(0));
}

//
// draw_hub - fill in the 6 petals around the center with virtual leds (1+)
//            Virtual leds 1+ map to end -6:-1 pixels 
//
void draw_hub(uint8_t channel) {
  uint8_t trans_i, pixel;
  uint8_t pitch = HUB_LEDS / symmetries[symmetry[channel]];
  uint8_t num_arms = 6 / symmetries[symmetry[channel]];
  CHSV color;
  
  for (uint8_t i = 0; i < pitch; i++) {
    for (uint8_t j = 0; j < symmetries[symmetry[channel]]; j++) {
      trans_i = i;
      
      if (SNOWFLAKE_NUMBER == 1 && switch_axis[channel]) {
        trans_i = ((i % num_arms) * HUB_LENGTH) + (i / num_arms);  // Hack for SNOWFLAKE 1
      }

      pixel = SPOKE_LEDS + trans_i + (j * pitch);

      if (SNOWFLAKE_NUMBER == 1 || SNOWFLAKE_NUMBER == 3) { pixel = shift_one_hub(pixel); }
      
      color = led[channel].getInterpFrameColor(1 + i);

      //// An intermediate buffer might be necessary. Showing older code below.
//      leds_buffer[channel][pixel] = color;
      led[channel].setInterpFrame(pixel, color);
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
  uint8_t symm = symmetries[symm_type];  // Number of repeated arms: 1, 2, 3, 6, 6
  uint8_t num_arms = 6 / symm;  // How many spokes per repeat
  uint8_t arm_length = SPOKE_LENGTH * num_arms;  // unique pixels in an arm
  uint8_t hub_pixels = get_hub_pixels(symm_type);
  uint8_t i, trans_i, pixel, switch_pixel;
  
  // For switch_axis
  uint8_t switch_arm = 0;  
  uint8_t switch_group_num = 0;
  uint8_t switch_item = 0;
  uint8_t switch_group[MAX_GROUP_LENGTH];
  
  CHSV color;

  if (switch_axis[channel]) { 
    fill_group(switch_group, switch_group_num);  // Seed first group
  }

  for (i = 0; i < arm_length; i++) {
    trans_i = i; // default is easy for no switch_axis

    // SWITCH AXIS algorithm
    if (switch_axis[channel]) {
      // Headache here to parse an array of variable-length arrays (switch_matrix)
      switch_pixel = switch_group[switch_item++];  // Pull the pixel from the 5-item array
      
      if (switch_pixel == END_GRP) {
        switch_item = 0;
        switch_arm++;
        
        if (switch_arm >= num_arms) {
          switch_arm = 0;
          switch_group_num++;  // Danger: not checking if this goes over 5 groups
          fill_group(switch_group, switch_group_num);
        }
        switch_pixel = switch_group[switch_item++];  // Try again
      }
      trans_i = switch_pixel + (switch_arm * SPOKE_LENGTH);
    }
    // End SWITCH AXIS algorithm
    
    color = led[channel].getInterpFrameColor(hub_pixels + i);
    color = dim_spoke(color);
    
    // Repeat the pixel on to each symmetric arm
    for (uint8_t j = 0; j < symm; j++) {  // 1, 2, 3, 6, 6
      pixel = trans_i + (j * arm_length);  // repeat
      pixel = shift_pixel(pixel, SNOWFLAKE_NUMBER, channel);  // shift pixel by snowflake type
      
      //// An intermediate buffer might be necessary. Showing older code below.
//       leds_buffer[channel][pixel] = color;
      led[channel].setInterpFrame(pixel, color);
    }
  }
}

//
// fill_group - load switch_group[5] with the 5 pixel row from the switch_matrix
//
void fill_group(uint8_t *switch_group, uint8_t switch_group_num) {
  
  uint8_t switch_matrix[] = {
    // Small Simple Snowflake (8-spoke): Start at left-most of 3-set pixel
    0, 1, 2, END_GRP, END_GRP,
    3, 7, END_GRP, END_GRP, END_GRP,
    4, 6, END_GRP, END_GRP, END_GRP,
    5, END_GRP, END_GRP, END_GRP, END_GRP,
    END_GRP, END_GRP, END_GRP, END_GRP, END_GRP,
  
    // Medium Pointy Snowflake (10-spoke): Start at left-most of middle 4-set pixel
    1, 7, END_GRP, END_GRP, END_GRP,
    0, 2, 6, 8, END_GRP,
    3, 5, 9, END_GRP, END_GRP,
    4, END_GRP, END_GRP, END_GRP, END_GRP,
    END_GRP, END_GRP, END_GRP, END_GRP, END_GRP,
  
    // Large Complex Snowflake (12-spoke): Start at left-most of 3-set pixel
     0, 1, 2, END_GRP, END_GRP,
     3,11, END_GRP, END_GRP, END_GRP,
     4,10, END_GRP, END_GRP, END_GRP,
     5, 6, 8, 9, END_GRP,
     7, END_GRP, END_GRP, END_GRP, END_GRP,
  
    // Medium Inward Snowflake (10-spoke): Start at left-most inward pixel
     0, 2, 4, END_GRP, END_GRP,
     1, 3, 5, 9, END_GRP,
     6, 8, END_GRP, END_GRP, END_GRP,
     7, END_GRP, END_GRP, END_GRP, END_GRP,
     END_GRP, END_GRP, END_GRP, END_GRP, END_GRP,
  };
  
  for (uint8_t i = 0; i < MAX_GROUP_LENGTH; i++) {
    switch_group[i] = switch_matrix[(SNOWFLAKE_NUMBER * (MAX_GROUPS * MAX_GROUP_LENGTH)) +
                                    (switch_group_num * MAX_GROUP_LENGTH) + i];
  }
}

//
// draw_spokes_symm_4 - special case for 12-fold symmetry. No switch_axis possible
//
void draw_spokes_symm_4(uint8_t channel) {
  uint8_t symm_type = 4;
  uint8_t hub_pixels = get_hub_pixels(symm_type);
  uint8_t i, pixel;
  
  CHSV color;

  for (i = 0; i < (SPOKE_LENGTH / 2) + 1; i++) {
    
    color = led[channel].getInterpFrameColor(hub_pixels + i);
    color = dim_spoke(color);
    
    // Repeat the pixel on to each symmetric arm
    for (uint8_t j = 0; j < 6; j++) {
      pixel = i + (j * SPOKE_LENGTH);  // left arm
      pixel = shift_pixel(pixel, SNOWFLAKE_NUMBER, channel);  // shift pixel by snowflake type
      
      //// An intermediate buffer might be necessary. Showing older code below.
      // leds_buffer[channel][pixel] = color;
      led[channel].setInterpFrame(pixel, color);

      if (i > 0 && i < SPOKE_LENGTH / 2) {  // Do not double-up first and last pixels
        pixel = SPOKE_LENGTH - i + (j * SPOKE_LENGTH);  // right arm
        pixel = shift_pixel(pixel, SNOWFLAKE_NUMBER, channel);  // shift pixel by snowflake type

        //// An intermediate buffer might be necessary. Showing older code below.
        // leds_buffer[channel][pixel] = color;
        led[channel].setInterpFrame(pixel, color);
      }
    }
  }
}

//
// dim_spoke - dim the value part of the CHSV color
//
CHSV dim_spoke(CHSV color) {
  color.v = scale8(color.v, spoke_brightness);
  return color;
}

//
// shift_pixel - shift the pixel in software to fix the hardware layout
//
uint8_t shift_pixel(uint8_t i, uint8_t snowflake_type, uint8_t channel) {
  int8_t shift_amount;  // Three kinds of shift matrices. Pick the right one

  int8_t shift_matrix_regular[] =  { -1, -1, -1, -2 };
  int8_t shift_matrix_sym4[] =     { -1, -5, -1, -3 };
  int8_t shift_matrix_switched[] = { -2, -2, -2,  5 };
  
  if (symmetry[channel] == 4) {
    shift_amount = shift_matrix_sym4[snowflake_type];  // For Sym 4
  } else if (switch_axis[channel]) {
    shift_amount = shift_matrix_switched[snowflake_type];  // For switch_axis
  } else {
    shift_amount = shift_matrix_regular[snowflake_type];  // Regular
  }
  return (i + SPOKE_LEDS + shift_amount) % SPOKE_LEDS;
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
// narrow_palette - confine the color range (ArduinoBlue)
//
CHSV narrow_palette(CHSV color) {
  color.h = map8(color.h, hue_start, (hue_start + hue_width) % MAX_COLOR );
  return color;
}

//
// lightning - ramp all pixels quickly up to white (down sat & up value) and back down
//
CHSV lightning(CHSV color) {  // (ArduinoBlue)
  if (curr_lightning > 0) {
    uint8_t increase = 255 - cos8( map(curr_lightning, 0, BOLT_TIME, 0, 255));
    color.s -= increase;
    color.v += increase;
  }
  return color;
}

//
// check_phone - poll the phone for updated values  (ArduinoBlue)
//
void check_phone() {
  int8_t sliderId = phone.getSliderId();  // ID of the slider moved
  int8_t buttonId = phone.getButton();  // ID of the button

  if (sliderId != -1) {
    int16_t sliderVal = phone.getSliderVal();  // Slider value goes from 0 to 200
    sliderVal = map(sliderVal, 0, 200, 0, 255);  // Recast to 0-255

    switch (sliderId) {
      case BRIGHTNESS_SLIDER:
        BRIGHTNESS = sliderVal;
        FastLED.setBrightness( BRIGHTNESS );
        break;
      case HUE_SLIDER:
        hue_start = sliderVal;
        break;
      case HUE_WIDTH_SLIDER:
        hue_width = sliderVal;
        break;
      case SPEED_SLIDER:
        DELAY_TIME = map8(sliderVal, 10, 100);
        break;
      case FADE_TIME_SLIDER:
        FADE_TIME = map8(sliderVal, 0, SHOW_DURATION);
        break;
    }
    speak_arduinoblue_commands();
  }
  
  if (buttonId == BAM_BUTTON) { 
    curr_lightning = BOLT_TIME;
    speak_arduinoblue_commands();
  }
}

//
//
// Unpack hex into bits
//
boolean get_bit_from_pattern_number(uint8_t n, uint8_t pattern_number) {
  uint8_t pattern_matrix[] = {  // A pattern is 2-byte of binary
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

  uint8_t pattern_byte = pattern_matrix[(SNOWFLAKE_NUMBER * (NUM_PATTERNS * 2)) + 
                                        (pattern_number * 2) + (n / 8)];
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
  Serial.print(", Switch Axis: ");
  Serial.print(switch_axis[i]);
  Serial.println(".");
}
