#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>  // (ArduinoBlue)
//
//  Turtle! - 29 Lights in a hexagonal grid
//
//  3 Turtle Shells with a different wiring layout
//
//  10/24/20 - FINAL
//
//  Dual shows - Blend together 2 shows running at once
//

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 15;  // in milliseconds (ArduinoBlue)
long last_time;
#define SMOOTHING 0   // 0 = no smooth, lower the number = more smoothing

#define DATA_PIN 0  // 11 // 7  // Changed!
#define CLOCK_PIN 2  // 13 // 8

#define NUM_LEDS 29
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number

#define ACTUAL_LEDS 37  // There are 8 dummy spacer LEDs

#define XX  255

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define ARE_CONNECTED false  // Are the HEX talking to each other?
#define IS_SPEAKING false  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = millis();
#define MAX_SILENT_TIME  (3 * 1000)  // Time (in sec) without communication before marked as is_lost

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV led_buffer[NUM_LEDS];  // For smoothing
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

#define ONLY_RED false  // (ArduinoBlue)
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;

#define MAX_COLOR 256   // Colors are 0-255

// Shows
#define NUM_SHOWS 13
#define START_SHOW_CHANNEL_A  1  // Startings shows for Channels A + B
#define START_SHOW_CHANNEL_B  0
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };

uint8_t current_pattern[] = { 0, 0 };

// wait times
#define SHOW_DURATION 45  // seconds
uint8_t FADE_TIME = 45;  // seconds to fade in + out (Arduino Blue)
#define MAX_SMALL_CYCLE  (SHOW_DURATION * 2 * (1000 / DELAY_TIME))  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

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
#define COMMAND_PALETTE    'H'
#define COMMAND_PAL_WIDTH  'J'
#define COMMAND_SPEED      'Y'
#define COMMAND_FADE       'R'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_SM_CYCLE   'C'
#define COMMAND_PATTERN    'P'
#define COMMAND_BOLT       'X'
#define COMMAND_CHANNEL_A  'x'
#define COMMAND_CHANNEL_B  'y'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define EMPTY_CHAR         ' '
#define MAX_MESSAGE       100
#define MAX_NUM             6  // To handle 65,535 of small_cycle
char message[MAX_MESSAGE];     // Incoming message buffer
char number_buffer[MAX_NUM];   // Incoming number buffer

// Lookup tables

const uint8_t rewire[] PROGMEM = {
  30, 26, 27,  0,  1,
   3,  2, 25, 24, 32,
  33, 22, 23,  4,  5,
   7,  6, 21, 20, 35,
  36, 18, 19,  8,  9,
  10, 17, 16, 15
};

const uint8_t neighbors[] PROGMEM = {
  XX,1,8,9,XX,XX,
  XX,2,7,8,0,XX,
  XX,XX,3,7,1,XX,
  XX,XX,4,6,7,2,
  XX,XX,XX,5,6,3, // 4
  4,XX,XX,14,13,6,
  3,4,5,13,12,7,
  2,3,6,12,8,1,
  1,7,12,11,9,0,  // 8
  0,8,11,10,XX,XX,
  9,11,18,19,XX,XX,
  8,12,17,18,10,9,
  7,6,13,17,11,8, // 12
  6,5,14,16,17,12,
  5,XX,XX,15,16,13,
  14,XX,XX,24,23,16,
  13,14,15,23,22,17, // 16
  12,13,16,22,18,11,
  11,17,22,21,19,10,
  10,18,21,20,XX,XX,
  19,21,27,XX,XX,XX,
  18,22,26,27,20,19,
  17,16,23,26,21,18, // 22
  16,15,24,25,26,22,
  15,XX,XX,XX,25,23,
  23,24,XX,XX,28,26,
  22,23,25,28,27,21,
  21,26,28,XX,XX,20,
  26,25,XX,XX,XX,27 // 28
};

#define NUM_PATTERNS 8   // Total number of patterns 

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
//  Serial.begin(9600);
  Serial1.begin(9600);  // Serial1: xBee port
  Serial2.begin(9600);  // Serial2: Bluetooth serial (ArduinoBlue)
//  Serial.println("Start");

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS);  // only use of ACTUAL_LEDS
  FastLED.setBrightness( BRIGHTNESS );
  
  // Set up the various mappings (1D lists in PROGMEM)
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    shows[i].fillForeBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].setColorSpeedMinMax(4, 20);  // Make colors change faster (lower = faster) Usually 8, 30
    shows[i].setWaitRange(30, 120, 90);
    shows[i].setBandsBpm(10, 30);
  }
  // Start Channel B offset at halfway through show
  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);
  last_time = millis();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    led_buffer[i] = CHSV(0, 0, 0);
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
        patterns(i);
        break;
      case 1:
        warp1(i);
        break;
      case 2:
        warp2(i);
        break;
      case 3:
        rainbow_show(i);
        break;
      case 4:
        shows[i].morphChain();
        break;
      case 5:
        shows[i].bounce();
        break;
      case 6:
        shows[i].bounceGlowing();
        break;
      case 7:
        shows[i].plinko(2);
        break;
      case 8:
        shows[i].randomFill();  // Vetted
        break;
      case 10:
        shows[i].sawTooth();  // Vetted
        break;
      case 11:
        shows[i].sinelon_fastled();  // Vetted
        break;
      default:
        shows[i].bands();
        break;
    }
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }
  check_phone();  // Check the phone settings (ArduinoBlue)
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
  fixed_delay();
  speak_and_hear();  // speak out or hear in signals
}

//
// fixed_delay - make every cycle the same time
//
void fixed_delay() {
  long new_time = millis();
  long time_delta = new_time - last_time;  // how much time has elapsed? Usually 3-5 milliseconds
  last_time = new_time;  // update the counter
  if (time_delta < DELAY_TIME) {  // if we have excess time,
    delay(DELAY_TIME - time_delta);  // delay for the excess
  }
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { 
      pick_next_show(i);
      next_show(i);
    }
  }
  
  if (curr_lightning > 0 ) {  // (ArduinoBlue)
    curr_lightning--; // Reduce the current bolt
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
//  led[i].push_frame();  // Doesn't work well with speaking & hearing
  shows[i].turnOnMorphing();
  shows[i].resetAllClocks();
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
  current_show[i] = is_other_channel_show_zero(i) ? random8(1, NUM_SHOWS) : 0 ;
  current_pattern[i] = random8(NUM_PATTERNS);
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  shows[i].setWaitRange(30, 120, 90);
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
  speak_and_hear();
//  log_status(i);  // For debugging
}

boolean is_other_channel_show_zero(uint8_t channel) {
  if (channel == 0) {
    return (current_show[CHANNEL_B] == 0);
  } else {
    return (current_show[CHANNEL_A] == 0);
  }
}

//
// patterns shows
//
void patterns(uint8_t c) {
  uint8_t PatternMatrix[] = {
    1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,
    1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,1,
    1,1,1,1,1,1,2,2,2,1,1,2,1,2,1,1,2,2,2,1,1,2,1,2,1,1,2,1,1,
    1,1,1,1,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,1,2,1,1,
    1,1,1,1,1,2,2,2,2,2,2,1,1,1,2,2,1,1,1,2,1,2,1,2,1,1,2,1,1,
    2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,1,1,2,1,2,1,2,1,1,1,2,
    1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,1,1,1,2,1,2,1,2,1,1,2,1,1,
    1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,2,1,2,1 
  }; 

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t pattern = PatternMatrix[(current_pattern[c] * 29) + i];
 
    switch (pattern) {
      case 0: {        // Off
        led[c].setPixelBlack(i);
        break;
      }
      case 1: {        // always green
        led[c].setPixelColor(i, CHSV(96, 255, 255));
        break;
      }
      case 2: {        // the other color
        led[c].setPixelHue(i, shows[c].getForeColor());
        break;
      }
    }
  }
}

//
// draw_ring
//
void draw_ring(uint8_t i, CHSV color, uint8_t c) {
  uint8_t rings[] = {
    17, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX,
    12, 13, 16, 22, 18, 11, XX, XX, XX, XX, XX, XX,
     7,  6,  5, 14, 15, 23, 26, 21, 19, 10,  9,  8,
     0,  1,  2,  3,  4, 24, 25, 28, 27, 20, XX, XX 
  };

  for (uint8_t j = 0; j < 12; j++) {
    uint8_t r = rings[(i * 12) + j];
    if (r != XX) {
      led[c].setPixelColor(r, color);
    }
  }
}

//
// tunnel vision
//
// Colored ring animating outwards
// color1 is the primary color, colo22 is a trail color
// background is the background color
//
void tunnelvision(CHSV color1, CHSV color2, CHSV background, uint8_t c) {
  led[c].fill(background);  
  uint8_t i = shows[c].getCycle() % 5;
  if (i < 4) { 
    draw_ring(i, color1, c); 
  }      
  if (i != 0) { 
    draw_ring(i-1, color2, c); 
  }
}

//
// warp1 - colors on a black field
// 
void warp1(uint8_t c) {
  switch ((shows[c].getCycle() / 5) % 6) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0, 255, 0)), rgb_to_hsv(CRGB(0,40,0)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,0,255)), rgb_to_hsv(CRGB(0,0,40)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,255,255)), rgb_to_hsv(CRGB(0,40,40)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(40,40,0)), rgb_to_hsv(CRGB(0,0,0)), c);  
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,0,0)), rgb_to_hsv(CRGB(40,0,0)), rgb_to_hsv(CRGB(0,0,0)), c);
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(255,0,255)), rgb_to_hsv(CRGB(40,0,40)), rgb_to_hsv(CRGB(0,0,0)), c);  
      break;
  }
}

//
// warp2 - colors on a green field
//
void warp2(uint8_t c) {
  switch ((shows[c].getCycle() / 5) % 8) {
    case 0:
      tunnelvision(rgb_to_hsv(CRGB(0,255,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 1:
      tunnelvision(rgb_to_hsv(CRGB(0,200,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 2:
      tunnelvision(rgb_to_hsv(CRGB(0,150,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 3:
      tunnelvision(rgb_to_hsv(CRGB(0,100,100)), rgb_to_hsv(CRGB(0,80,40)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 4:
      tunnelvision(rgb_to_hsv(CRGB(255,255,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 5:
      tunnelvision(rgb_to_hsv(CRGB(200,200,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    case 6:
      tunnelvision(rgb_to_hsv(CRGB(150,150,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);
      break;
    default:
      tunnelvision(rgb_to_hsv(CRGB(100,100,0)), rgb_to_hsv(CRGB(80,80,0)), rgb_to_hsv(CRGB(0,40,0)), c);  
      break;
  }
}

//
// rainbow show - distribute a rainbow wheel equally distributed along the chain
//
void rainbow_show(uint8_t c) {
  for (uint8_t i = 0; i < HALF_LEDS; i++) {
    uint8_t hue = ((shows[c].getForeColorSpeed() * i) + shows[c].getBackColorSpeed() + (10 * shows[c].getCycle())) % MAX_COLOR;
    led[c].setPixelHue(i, hue);
    led[c].setPixelHue(NUM_LEDS - i - 1, hue);
  }  
}

//// End specialized shows

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
    speak_channel_commands(i);
//    if (shows[i].getMorph() == 0) {
//      speak_channel_commands(i);
//    }
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
  m = speak_command(COMMAND_WAIT, shows[i].getWait(), m);
  
  message[m++] = COMMAND_PERIOD;  // Terminate message with period character
  message[m++] = '\0';  // Terminate message with null (new-line) character
  Serial1.print(message);  // Only done once
  // Serial.println(message);  // For debugging
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
        if (tmp == COMMAND_START) { have_start_signal = true; }  // Start of message
        else { } // discard prefix garbage
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
      if (BRIGHTNESS != value) {
        BRIGHTNESS = value;
        FastLED.setBrightness( BRIGHTNESS );
      }
      break;
    
    case COMMAND_WAIT:
      if (shows[i].getWait() != value) { 
        shows[i].setWait(value);
      }
      break;
    
    case COMMAND_SHOW:  // Show
      if (current_show[i] != value) {
        current_show[i] = value % NUM_SHOWS;
        next_show(i);
      }
      break;

    case COMMAND_SM_CYCLE: // small_cycle
      shows[i].setSmallCycle(value);
      break;
  }
}

////// End Speaking & Hearing

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color_b = led[CHANNEL_B].getInterpFrameColor(i);
    CHSV color_a = led[CHANNEL_A].getInterpFrameColor(i);
    CHSV color = led[CHANNEL_A].getInterpHSV(color_b, color_a, fract);  // interpolate a + b channels
    color = lightning(narrow_palette(color));  // (ArduinoBlue)
    
    if (SMOOTHING > 0) {
      color = led[CHANNEL_A].smooth_color(led_buffer[i], color, SMOOTHING);  // smoothing
    }
    
    leds[pgm_read_byte_near(rewire + i)] = color;
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
  return intensity;  // do not use easing
}

//// End DUAL SHOW LOGIC

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
  
  if (buttonId == BAM_BUTTON) { curr_lightning = BOLT_TIME; }
}


void turn_off_spacer_leds() {
  uint8_t spacer_leds[] = { 11, 12, 13, 14, 28, 29, 31, 34 };
  
  for (uint8_t i = 0; i < 8; i++) {
    leds[spacer_leds[i]] = CRGB(0, 0, 0);
  }
}

//
// RGB to HSV - save having to translate RGB colors by hand
//
CHSV rgb_to_hsv( CRGB color) {
  return led[0].rgb_to_hsv(color);  // static method
}

//
// log status
//
void log_status(uint8_t i) {
  Serial.print("Channel: ");
  Serial.print(i);
  Serial.print(", Show: ");
  Serial.print(current_show[i]);
  Serial.print(", Wait: ");
  Serial.print(shows[i].getNumFrames());
  Serial.print(", Delay: ");
  Serial.print(DELAY_TIME);
  Serial.println(".");
}
