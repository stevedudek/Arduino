#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>

//
//  Sphere
//
//  ArduinoBlue wireless bluetooth controller (ArduinoBlue)
//
//  Two hemispheres that talk to each other
//
//  Dual shows - Blend together 2 shows running at once
//
//  2 CHSV buffers for Channel A + B
//  interpolate the two buffers on to the CRGB leds
//
//  6/26/2019
//

#define NUM_LEDS 144
#define NUM_FACES 12
#define NUM_SPOKES 10

#define ARE_CONNECTED true  // Are Spheres talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

uint8_t BRIGHTNESS = 255;  // (0-255) (ArduinoBlue)

uint8_t DELAY_TIME = 40;  // in milliseconds (ArduinoBlue)

#define DATA_PIN 8
#define CLOCK_PIN 7

#define CHANNEL_A  0  // Don't change these
#define CHANNEL_B  1
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[NUM_LEDS];  // Hook for FastLED library

// Shows
#define START_SHOW_CHANNEL_A  12  // Channels A starting show
#define START_SHOW_CHANNEL_B  0  // Channels B starting show
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 21

// ArduinoBlue
uint8_t hue_start = 0;
uint8_t hue_width = 255;
uint8_t curr_lightning = 0;
#define MAX_COLOR 256   // Colors are 0-255
#define WHITE  CHSV(0, 0, 255)
#define BLACK  CHSV(0, 0, 0)

// Clocks and time
#define SHOW_DURATION 30  // seconds
uint8_t FADE_TIME = 3;  // seconds to fade in + out (Arduino Blue)
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

boolean is_lost = false;
unsigned long last_connection = millis();
#define MAX_SILENT_TIME  (3 * 1000)  // Time (in sec) without communication before marked as is_lost

// Spherical Effect
uint8_t show_freq;
uint8_t center_pixel;
float rotation_speed;

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

const uint8_t TRIANGLES[] PROGMEM = {
1, 0, 0, 2, 2, 1, 1, 0, 0, 1,
2, 0, 0, 1, 1, 2, 2, 1, 1, 2,
1, 0, 0, 2, 2, 1, 1, 2, 2, 1,
2, 0, 0, 1, 1, 2, 2, 1, 1, 2,
1, 2, 2, 1, 1, 0, 0, 2, 2, 1,
0, 1, 1, 2, 2, 1, 1, 2, 2, 0,
};

const uint8_t DIAMONDS[] PROGMEM = {
1, 1, 2, 2, 1, 1, 2, 2, 0, 0,
2, 2, 1, 1, 2, 2, 0, 0, 1, 1,
1, 1, 2, 2, 0, 0, 1, 1, 2, 2,
2, 2, 1, 1, 0, 0, 2, 2, 0, 0,
1, 1, 2, 2, 0, 0, 1, 1, 0, 0,
2, 2, 0, 0, 1, 1, 2, 2, 1, 1,
};

const uint8_t COORDS[] PROGMEM = {
215, 86, 215, 231, 113, 205, 231, 141, 205, 215, 168, 215, 
192, 176, 229, 165, 166, 246, 151, 143, 255, 151, 111, 255, 
165, 88, 246, 192, 78, 229, 195, 128, 236, 195, 128, 236,   // End of face 0
176, 229, 192, 166, 246, 165, 143, 255, 151, 111, 255, 151, 
88, 246, 165, 78, 229, 192, 86, 215, 215, 113, 205, 231, 
141, 205, 231, 168, 215, 215, 128, 236, 195, 128, 236, 195,   // End of face 1
49, 231, 113, 39, 215, 86, 25, 192, 78, 8, 165, 88, 
0, 151, 111, 0, 151, 143, 8, 165, 166, 25, 192, 176, 
39, 215, 168, 49, 231, 141, 19, 195, 127, 19, 195, 127,   // End of face 2
8, 89, 88, 25, 62, 78, 39, 39, 86, 49, 23, 113, 
49, 23, 141, 39, 39, 168, 25, 62, 176, 8, 89, 166, 
0, 103, 143, 0, 103, 111, 19, 60, 127, 19, 60, 127,   // End of face 3
111, 0, 151, 143, 0, 151, 166, 8, 165, 176, 25, 192, 
168, 39, 215, 141, 49, 231, 113, 49, 231, 86, 39, 215, 
78, 25, 192, 88, 8, 165, 128, 19, 195, 128, 19, 195,   // End of face 4
103, 111, 255, 103, 143, 255, 89, 166, 246, 62, 176, 229, 
39, 168, 215, 23, 141, 205, 23, 113, 205, 39, 86, 215, 
62, 78, 229, 89, 88, 246, 60, 128, 236, 60, 128, 236,   // End of face 5
205, 231, 141, 215, 215, 168, 229, 192, 176, 246, 165, 166, 
255, 151, 143, 255, 151, 111, 246, 165, 88, 229, 192, 78, 
215, 215, 86, 205, 231, 113, 236, 195, 127, 236, 195, 127,   // End of face 6
246, 89, 166, 229, 62, 176, 215, 39, 168, 205, 23, 141, 
205, 23, 113, 215, 39, 86, 229, 62, 78, 246, 89, 88, 
255, 103, 111, 255, 103, 143, 236, 60, 127, 236, 60, 127,   // End of face 7
143, 0, 103, 111, 0, 103, 88, 8, 89, 78, 25, 62, 
86, 39, 39, 113, 49, 23, 141, 49, 23, 168, 39, 39, 
176, 25, 62, 166, 8, 89, 128, 19, 60, 128, 19, 60,   // End of face 8
39, 86, 39, 23, 113, 49, 23, 141, 49, 39, 168, 39, 
62, 176, 25, 89, 166, 8, 103, 143, 0, 103, 111, 0, 
89, 88, 8, 62, 78, 25, 60, 128, 19, 60, 128, 19,   // End of face 9
78, 229, 62, 88, 246, 89, 111, 255, 103, 143, 255, 103, 
166, 246, 89, 176, 229, 62, 168, 215, 39, 141, 205, 23, 
113, 205, 23, 86, 215, 39, 128, 236, 60, 128, 236, 60,   // End of face 10
215, 168, 39, 231, 141, 49, 231, 113, 49, 215, 86, 39, 
192, 78, 25, 165, 88, 8, 151, 111, 0, 151, 143, 0, 
165, 166, 8, 192, 176, 25, 195, 128, 19, 195, 128, 19,   // End of face 11
};

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");
  Serial1.begin(9600);  // Serial1: xBee port
  Serial2.begin(9600);  // Serial2: Bluetooth serial (ArduinoBlue)

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS / 2);  // Only 1 leds object
  FastLED.setBrightness( BRIGHTNESS );

  setBothChannelsSmallCycles(0);
  
  for (uint8_t i = 0; i < DUAL; i++) {
    led[i].fillBlack();
    led[i].push_frame();
  }
}

void loop() {

  for (uint8_t i = 0; i < DUAL; i++) {

    switch (current_show[i]) {

      case 0:
        propellor(i);
        break;
      case 1:
        propellor_two_color(i);
        break;
      case 2:
        fan(i);
        break;
      case 3:
        fan_two_color(i);
        break;
      case 4:
        hub_two_color(i);
        break;
      case 5:
        hub_only(i);
        break;
      case 6:
        shows[i].twoColor();
        break;
      case 7:
        shows[i].packets();
        break;
      case 8:
        shows[i].packets_two();
        break;
      case 9:
        shows[i].sinelon_fastled();
        break;
      case 10:
        shows[i].juggle_fastled();
        break;
      case 11:
        shows[i].bands();
        break;
      case 12:
        draw_objects_static(i, 0);  // 0 = TRIANGLES
        break;
      case 13:
        draw_objects_static(i, 1);  // 1 = DIAMONDS
        break;
      case 14:
        rotating_white_moon(i, false);
        break;
      case 15:
        rotating_dark_moon(i, false);
        break;
      case 16:
        rotating_two_color_moon(i, false);
        break;
      case 17:
        rotating_white_moon(i, true);
        break;
      case 18:
        rotating_dark_moon(i, true);
        break;
      case 19:
        rotating_two_color_moon(i, true);
        break;
      default:
        shows[i].allOn();
        break;
    }
  
    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  check_phone();  // Check the phone settings (ArduinoBlue)
  update_leds();  // morph together the 2 chanels & push the interp_frame on to the leds
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
  advance_clocks();  // advance the cycle clocks and check for next show
  speak_and_hear();  // speak out or hear in signals
  
  delay(DELAY_TIME); // The only delay  
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { 
      next_show(i);
      pick_next_show(i);
    }
  }
  
  if (curr_lightning > 0 ) {  // (ArduinoBlue)
    curr_lightning--;  // Reduce the current bolt
  }
}

//
// next_show - dependent on channel i
//
void next_show(uint8_t i) {
  led[i].fillBlack();
  led[i].push_frame(); 
  shows[i].resetAllClocks();
  
  shows[i].turnOnMorphing();
  shows[i].setWaitRange(20, 100, 80);
}

void pick_next_show(uint8_t i) {
  if (i == CHANNEL_A) { setBothChannelsSmallCycles(0); }
  
  // Switch between a patterns show and all the other shows
  current_show[i] = random8(NUM_SHOWS);
  
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
      setBothChannelsSmallCycles(value);
      break;
  }
}

////// End Speaking & Hearing


//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (uint8_t channel = 0; channel < DUAL; channel++) {
    for (uint8_t i = 0; i < shows[channel].getNumLeds(); i++) {
      leds_buffer[channel][i] = led[channel].getInterpFrameColor(i);
    }
  }
}

///// SPECIALIZED SHOWS

//
// pattern shows
//
void draw_objects_static(uint8_t i, uint8_t obj) {
  uint8_t c1 = shows[i].getForeColor();
  uint8_t c2 = shows[i].getBackColor();
  uint8_t c3 = led[i].interpolate_wrap(c1, c2, 128);
  
  draw_objects(led[i].wheel(c1), led[i].wheel(c2), led[i].wheel(c3), i, obj);
}

void draw_objects_pulsing(uint8_t i, uint8_t obj) {
  if (shows[i].isShowStart()) {
    shows[i].turnOffMorphing();
    show_freq = random8(3,30);
  }
  uint8_t c1 = shows[i].getForeColor();
  uint8_t c2 = shows[i].getBackColor();
  uint8_t c3 = led[i].interpolate_wrap(c1, c2, 128);
  
  CHSV color1 = led[i].gradient_wheel(c1, beatsin8(show_freq, 0, 255, 0, 0));
  CHSV color2 = led[i].gradient_wheel(c2, beatsin8(show_freq, 0, 255, 0, 128));
  CHSV color3 = led[i].gradient_wheel(c3, beatsin8(show_freq * 1.5, 0, 255, 0, 0));
  
  draw_objects(color1, color2, color3, i, obj);
}

void draw_objects(CHSV color1, CHSV color2, CHSV color3, uint8_t channel, uint8_t obj) {
  uint8_t hue;
  CHSV color = BLACK;
  
  turn_off_hubs(channel);  
  for (uint8_t i = 0; i < NUM_FACES / 2; i++) {
    for (uint8_t j = 0; j < NUM_SPOKES; j++) {
      if (obj == 0) {
        hue = pgm_read_byte_near(TRIANGLES + (i * NUM_SPOKES) + j);
      } else {
        hue = pgm_read_byte_near(DIAMONDS + (i * NUM_SPOKES) + j);
      }
      
      switch (hue) {
        case 0:
          color = color3;
          break;
        case 1:
          color = color1;
          break;
        case 2:
          color = color2;
          break;
      }
      shows[channel].setPixeltoColor(      (i * (NUM_SPOKES + 2)) + j, color);  // top hemisphere
      shows[channel].setPixeltoColor(((6 + i) * (NUM_SPOKES + 2)) + j, color);  // bottom hemisphere
    }
  }
}

//
// propellor shows
//
void propellor(uint8_t i) {
  set_propellor_two_color(led[i].wheel(shows[i].getForeColor()), 
                          led[i].wheel(shows[i].getBackColor()), 
                          CHSV(0,0,0), i);
}

void propellor_two_color(uint8_t i) {
  set_propellor_two_color(led[i].wheel(shows[i].getForeColor()), 
                          led[i].wheel(shows[i].getForeColor()), 
                          led[i].wheel(shows[i].getBackColor()), i);
}

void set_propellor_two_color(CHSV hub_color, CHSV color1, CHSV color2, uint8_t channel) {
  if (shows[channel].isShowStart()) { shows[channel].makeWaitFaster(6); }
  
  CHSV color;
  set_hubs(hub_color, channel);
  set_spokes(color2, channel);  // Set the background

  uint8_t rot = shows[channel].getCycle() % NUM_SPOKES;
  set_blade(color1, (rot + 0) % NUM_SPOKES, channel);
  set_blade(color1, (rot + 1) % NUM_SPOKES, channel);
  set_blade(color1, (rot + 6) % NUM_SPOKES, channel);
  set_blade(color1, (rot + 7) % NUM_SPOKES, channel);
  
}

void fan(uint8_t i) {
  set_fan_two_color(led[i].wheel(shows[i].getForeColor()), 
                    led[i].wheel(shows[i].getBackColor()), 
                    CHSV(0,0,0), i);
}

void fan_two_color(uint8_t i) {
  set_fan_two_color(led[i].wheel(shows[i].getForeColor()), 
                    led[i].wheel(shows[i].getForeColor()), 
                    led[i].wheel(shows[i].getBackColor()), i);
}

void set_fan_two_color(CHSV hub_color, CHSV color1, CHSV color2, uint8_t channel) {
  if (shows[channel].isShowStart()) { shows[channel].makeWaitFaster(4); }
  
  CHSV color;
  set_hubs(hub_color, channel);
  set_spokes(color2, channel);  // Set the background

  uint8_t rot = shows[channel].getCycle() % NUM_SPOKES;
  for (uint8_t j = 0; j < 4; j++) {
    set_blade(color1, (rot + j) % NUM_SPOKES, channel);
  }
}

void set_blade(CHSV color, uint8_t blade, uint8_t channel) {
  uint8_t blade_num;
  for (uint8_t i = 0; i < NUM_FACES; i++) {
    blade_num = (blade + NUM_SPOKES - (i / 2)) % NUM_SPOKES;
    blade_num = (i % 2 == 0) ? blade_num : NUM_SPOKES - blade_num - 1;  // adjacent faces spin in opposite directions
    shows[channel].setPixeltoColor((i * (NUM_SPOKES + 2)) + blade_num, color);
  }
}

//
// hub_two_color - several kinds of hub & spoke shows
//
void hub_two_color(uint8_t i) {
  set_hubs(led[i].wheel(shows[i].getForeColor()), i);
  set_spokes(led[i].wheel(shows[i].getBackColor()), i);
}

void hub_only(uint8_t i) {
  set_hubs(led[i].wheel(shows[i].getForeColor()), i);
  set_spokes(CHSV(0,0,0), i);
}

void hub_pulse_two_color(uint8_t i) {
  if (shows[i].isShowStart()) {
    shows[i].turnOffMorphing();
    show_freq = random8(3,30);
  }
  set_hubs(led[i].gradient_wheel(shows[i].getForeColor(), beatsin8(show_freq, 0, 255, 0, 0)), i);
  set_spokes(led[i].gradient_wheel(shows[i].getBackColor(), beatsin8(show_freq, 0, 255, 0, 128)), i);
}

void spinners(uint8_t channel) {
  if (shows[channel].isShowStart()) {
    shows[channel].turnOffMorphing();
    show_freq = random8(20, 60);
  }
  set_hubs(led[channel].wheel(shows[channel].getForeColor()), channel);

  for (uint8_t j = 0; j < NUM_SPOKES; j++) {
    CHSV color = led[channel].gradient_wheel(shows[channel].getBackColor(), beatsin8(show_freq, 0, 255, 0, j * 255 / NUM_SPOKES));
    for (uint8_t i = 0; i < NUM_FACES; i++) {
      shows[channel].setPixeltoColor((i * (NUM_SPOKES + 2)) + j, color);
    }
  }
}

void spinners_whole_sphere(uint8_t channel) {
  if (shows[channel].isShowStart()) {
    shows[channel].turnOffMorphing();
    show_freq = random8(3, 30);
  }
  set_hubs(led[channel].wheel(shows[channel].getForeColor()), channel);

  uint8_t pixel;
  CHSV color;
  for (uint8_t i = 0; i < NUM_FACES; i++) {
    for (uint8_t j = 0; j < NUM_SPOKES; j++) {
      pixel = (i * (NUM_SPOKES + 2)) + j;
      color = led[channel].gradient_wheel(shows[channel].getBackColor(), beatsin8(show_freq, 0, 255, 0, pixel * 255 / NUM_LEDS));
      shows[channel].setPixeltoColor(pixel, color);
    }
  }
}

void hue_spinners(uint8_t channel) {
  if (shows[channel].isShowStart()) {
    show_freq = random8(3,30);
    shows[channel].turnOffMorphing();
  }
  set_hubs(led[channel].wheel(shows[channel].getForeColor()), channel);

  for (uint8_t j = 0; j < NUM_SPOKES; j++) {
    uint8_t hue = beatsin8(show_freq, 0, 255, 0, j * 255 / (NUM_SPOKES * 5) );  // * X etc will make hues less rainbow
    for (uint8_t i = 0; i < NUM_FACES; i++) {
      shows[channel].setPixeltoHue((i * (NUM_SPOKES + 2)) + j, hue);
    }
  }
}

void hue_spinners_whole_sphere(uint8_t channel) {
  if (shows[channel].isShowStart()) {
    shows[channel].turnOffMorphing();
    show_freq = random8(3,30);
  }
  set_hubs(led[channel].wheel(shows[channel].getForeColor()), channel);

  uint8_t pixel;
  for (uint8_t i = 0; i < NUM_FACES; i++) {
    for (uint8_t j = 0; j < NUM_SPOKES; j++) {
      pixel = (i * (NUM_SPOKES + 2)) + j;
      shows[channel].setPixeltoHue(pixel, beatsin8(show_freq, 0, 255, 0, pixel * 255 / (NUM_LEDS * 2)) );
    }
  }
}

void turn_off_hubs(uint8_t channel) {
  set_hubs(CHSV(0,0,0), channel);
}

void set_hubs(CHSV color, uint8_t channel) {
  uint8_t hub_pixel;
  for (uint8_t i = 0; i < NUM_FACES; i++) {
    hub_pixel = (i * (NUM_SPOKES + 2)) + NUM_SPOKES + 1;
    shows[channel].setPixeltoColor(hub_pixel, color);
    shows[channel].setPixeltoColor(hub_pixel + 1, color);
  }
}

void set_spokes(CHSV color, uint8_t channel) {
  for (uint8_t i = 0; i < NUM_FACES; i++) {
    for (uint8_t j = 0; j < NUM_SPOKES; j++) {
      shows[channel].setPixeltoColor((i * (NUM_SPOKES + 2)) + j, color);
    }
  }
}

//
// bands - several kinds of traveling bands
//
void color_band(uint8_t i) {
  band_effect(i, led[i].wheel(shows[i].getForeColor()), BLACK );
}

void dark_color_band(uint8_t i) {
  band_effect(i, BLACK, led[i].wheel(shows[i].getBackColor()) );
}

void two_color_band(uint8_t i) {
  band_effect(i, led[i].wheel(shows[i].getForeColor()), led[i].wheel(shows[i].getBackColor()) );
}

void hue_band(uint8_t channel) {
  if (shows[channel].isShowStart()) { initialize_effect_band(); }
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t hue = beatsin8(show_freq, 0, 255, 0, get_distance(i, center_pixel) );
    shows[channel].setPixeltoHue(i, hue);
  }
}

void band_effect(uint8_t channel, CHSV foreColor, CHSV backColor) {
  if (shows[channel].isShowStart()) { initialize_effect_band(); }
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t fract = beatsin8(show_freq, 0, 255, 0, get_distance(i, center_pixel) );
    CHSV color = led[CHANNEL_A].getInterpHSV(backColor, foreColor, fract);
    shows[channel].setPixeltoColor(i, color);
  }
}

//
// initialize_effect_band - reset effect parameters
//
void initialize_effect_band() {
  center_pixel = random8(NUM_LEDS);
  show_freq = random8(3,30);
  rotation_speed = random8(1, 9) / 4.0;
}

//
// Rotating moon
//
void rotating_dark_moon(uint8_t i, boolean fade) {
  rotating_moon(led[i].wheel(shows[i].getForeColor()), BLACK, i, fade);
}

void rotating_white_moon(uint8_t i, boolean fade) {
  rotating_moon(WHITE, BLACK, i, fade);
}

void rotating_two_color_moon(uint8_t i, boolean fade) {
  rotating_moon(led[i].wheel(shows[i].getForeColor()), led[i].wheel(shows[i].getBackColor()), i, fade);
}

void rotating_moon(CHSV lightColor, CHSV darkColor, uint8_t channel, boolean fade) {
  if (shows[channel].isShowStart()) {
    initialize_effect_band();
    shows[channel].setWait(rotation_speed * 4);
  }

  // Only rotates around the z-axis
  // Equator is a unit circle on the xy plane: x(2) + y(2) = 1
  // Points are determined by: x = sin(i), y = cos(i)
  uint8_t x = sin8_C(shows[channel].getSmallCycle() * rotation_speed);
  uint8_t y = cos8(shows[channel].getSmallCycle() * rotation_speed);
  uint8_t z = 128;
  
  light_half_moon_from_point(x,y,z, lightColor, darkColor, channel, fade);
}

void light_half_moon_from_point(uint8_t x, uint8_t y, uint8_t z, CHSV lightColor, CHSV darkColor, uint8_t channel, boolean fade) {
  CHSV color;
  uint8_t dist;
  
  for (uint8_t i = 0; i < NUM_LEDS; i++) {  // 180 = 255 / sqrt(2)
    dist = get_distance_pixel_point(i, x,y,z);
    if (fade) {
      color = led[CHANNEL_A].getInterpHSV(lightColor, darkColor, dist);
    } else {
      color = dist ? lightColor : darkColor ;
    }
    shows[channel].setPixeltoColor(i, color);
  }
}

///// END SPECIALIZED SHOWS


//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    CHSV color = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][i], 
                                             leds_buffer[CHANNEL_A][i], 
                                             fract);  // interpolate a + b channels
    color = lightning(narrow_palette(color));  // (ArduinoBlue)
    
    if (IS_SPEAKING && i < 72) {
      leds[i] = color;
    }
    if (!IS_SPEAKING && i >= 72) { 
      leds[i - 72] = color;
    }
  }
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
        Serial.print(BRIGHTNESS);
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
// get_intensity - calculate a channel's intensity based on its cycle clock
//
uint8_t get_intensity(uint8_t i) {
  uint16_t small_cycle = shows[i].getSmallCycle();
  uint8_t intensity;  // 0 = Off, 255 = full On

  if (small_cycle < FADE_CYCLES) {
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

void setBothChannelsSmallCycles(uint16_t channel_a_small_cycle) {
  // Start Channel B offset at halfway through show
  uint32_t channel_b_small_cycle = (channel_a_small_cycle + (MAX_SMALL_CYCLE / 2)) % MAX_SMALL_CYCLE;
  shows[CHANNEL_A].setSmallCycle(channel_a_small_cycle);
  shows[CHANNEL_B].setSmallCycle(channel_b_small_cycle);
}

//
// get_distance
//
uint8_t get_distance_pixel_point(uint8_t i, uint8_t x, uint8_t y, uint8_t z) {
  // 3D distance = sqrt(x2 + y2 + z2) scaled to 0-255
  uint16_t d = sqrt(get_diff_sq_pixel_value_by_index(i, 0, x) +
                    get_diff_sq_pixel_value_by_index(i, 1, y) +
                    get_diff_sq_pixel_value_by_index(i, 2, z));
  return map(d, 0, 262, 0, 255);
}

uint8_t get_distance(uint8_t c1, uint8_t c2) {
  // 3D distance = sqrt(x2 + y2 + z2) scaled to 0-255
  uint16_t d = sqrt(get_diff_sq_by_index(c1, c2, 0) +
                    get_diff_sq_by_index(c1, c2, 1) +
                    get_diff_sq_by_index(c1, c2, 2));
  return map(d, 0, 262, 0, 255);
}

uint16_t get_diff_sq_pixel_value_by_index(uint8_t i, uint8_t index, uint8_t value) {
  return sq( get_coord_value(i, index) - value );
}

uint16_t get_diff_sq_by_index(uint8_t c1, uint8_t c2, uint8_t index) {
  return sq( get_coord_value(c2, index) - get_coord_value(c1, index) );
}

//
// get_coord_value
//
uint8_t get_coord_value(uint8_t coord, uint8_t index) {
  return pgm_read_byte_near(COORDS + (coord * 3) + index);
}


