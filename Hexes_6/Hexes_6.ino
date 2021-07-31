#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include <ArduinoBlue.h>  // (ArduinoBlue)
//
//  Hexes - 3 Large hexes
//
//  6/8/20 - Time of Covid
//
//  FastLED
//
//  Large chips - dual shows - xBee - Arduino Blue
//
//  Most elaborate shows possible - hold on to your hats
//
#define HEX_NUM   0
#define MAX_HEX   1

uint8_t BRIGHTNESS = 48;  // (0-255)

#define DELAY_TIME 20 // in milliseconds

// Hex Grid - Each Hex is 11 x 11
// MAX_XGRID, MAX_YGRID must be the same for all HEX
#define HEX_XGRID  0  // x-coord (upper left)
#define HEX_YGRID  0  // y-coord (upper left)
#define MAX_XGRID  11  //
#define MAX_YGRID  33  //

#define DATA_PIN 7
#define CLOCK_PIN 8

#define CHANNEL_A  1  // Don't change these
#define CHANNEL_B  0
#define DUAL       2  // How many shows to run at once (dual = 2). Don't change.

#define ARE_CONNECTED true  // Are the HEX talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

boolean is_lost = false;
unsigned long last_connection = millis();
#define MAX_SILENT_TIME  (3 * 1000)  // Time (in sec) without communication before marked as is_lost

#define NUM_LEDS 91
#define ACTUAL_LEDS 106
#define HALF_LEDS ((NUM_LEDS / 2) + 1)  // Half that number

#define MAX_COORD 11

#define XX  255

// Light colors
#define MAX_COLOR 256   // Colors are 0-255
CHSV BLACK = CHSV(0, 0, 0);

Led led[] = { Led(NUM_LEDS), Led(NUM_LEDS) };  // Class instantiation of the 2 Led libraries
Shows shows[] = { Shows(&led[CHANNEL_A]), Shows(&led[CHANNEL_B]) };  // 2 Show libraries
CHSV leds_buffer[DUAL][NUM_LEDS];  // CHSV buffers for Channel A + B; may break the memory bank
CRGB leds[ACTUAL_LEDS];  // The Leds themselves

// Shows
#define START_SHOW_CHANNEL_A  1  // Starting show for Channels A. One of these needs to be 0.
#define START_SHOW_CHANNEL_B  6  // Starting show for Channels B. One of these needs to be 0.
uint8_t current_show[] = { START_SHOW_CHANNEL_A, START_SHOW_CHANNEL_B };
#define NUM_SHOWS 27

uint8_t show_var[] = { 0, 0 };  // generic show variable - use carefully

#define ONLY_RED true

// Clocks and time
#define SHOW_DURATION 300  // Typically 30 seconds. Size problems at 1800+ seconds.
#define FADE_TIME 1   // seconds to fade in. If FADE_TIME = SHOW_DURATION, then Always Be Fading
uint32_t MAX_SMALL_CYCLE = SHOW_DURATION * 2 * (1000 / DELAY_TIME);  // *2! 50% = all on, 50% = all off, and rise + decay on edges
#define FADE_CYCLES  (FADE_TIME * 1000 / DELAY_TIME)  // cycles to fade in + out

// xBee language
#define COMMAND_START      '+'
#define COMMAND_FORE       'F'
#define COMMAND_BACK       'B'
#define COMMAND_BRIGHTNESS 'I'
#define COMMAND_WAIT       'W'
#define COMMAND_SHOW       'S'
#define COMMAND_SM_CYCLE   'C'
#define COMMAND_NOISE      'N'
#define COMMAND_CHANNEL_A  'x'
#define COMMAND_CHANNEL_B  'y'
#define COMMAND_COMMA      ','
#define COMMAND_PERIOD     '.'
#define EMPTY_CHAR         ' '
#define MAX_MESSAGE       100
#define MAX_NUM             5  // To handle 65,535 of small_cycle
char message[MAX_MESSAGE];     // Incoming message buffer
char number_buffer[MAX_NUM];   // Incoming number buffer

// Lookup tables
//const uint8_t LED_LOOKUP[] PROGMEM = { 
//  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
// 30, 31, 32, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 
// 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 80, 81, 82, 83, 84, 
// 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 98, 99, 101, 102, 104, 105 };

#define NUMBER_SPACER_LEDS 15
uint8_t spacer_leds[] = { 13, 14, 15, 33, 34, 56, 57, 58, 78, 79, 95, 96, 97, 100, 103 };

//const uint8_t coords[] PROGMEM = {
//    XX, XX, XX, XX, XX, 50, 51, 52, 84, 83, 85,
//    XX, XX, XX, XX, 49, 48, 53, 54, 82, 81, 86,
//    XX, XX, XX, 13, 47, 46, 55, 56, 80, 79, 87,
//    XX, XX, 14, 15, 45, 44, 57, 58, 78, 77, 88,
//    XX, 12, 16, 17, 43, 42, 59, 60, 76, 75, 89,
//    11, 10, 18, 19, 41, 40, 61, 62, 74, 73, 90,
//     9, 8, 20, 21, 39, 38, 63, 64, 72, 71, XX,
//     7, 6, 22, 23, 37, 36, 65, 66, 70, XX, XX,
//     5, 4, 24, 25, 35, 34, 67, 68, XX, XX, XX,
//     3, 2, 26, 27, 33, 32, 69, XX, XX, XX, XX,
//     1, 0, 28, 29, 31, 30, XX, XX, XX, XX, XX
//};

const uint8_t coords[] PROGMEM = {
  XX, XX, XX, XX, XX,  0,  1,  2,  3,  4,  5, 
  XX, XX, XX, XX,  6,  7,  8,  9, 10, 11, 12, 
  XX, XX, XX, 13, 14, 15, 16, 17, 18, 19, 20, 
  XX, XX, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
  XX, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 
  40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 
  51, 52, 53, 54, 55, 56, 57, 58, 59, 60, XX, 
  61, 62, 63, 64, 65, 66, 67, 68, 69, XX, XX, 
  70, 71, 72, 73, 74, 75, 76, 77, XX, XX, XX, 
  78, 79, 80, 81, 82, 83, 84, XX, XX, XX, XX, 
  85, 86, 87, 88, 89, 90, XX, XX, XX, XX, XX
};


const uint8_t corner_coords[] PROGMEM = {
  5,0, 10,0, 10,5, 5,10, 0,10, 0,5
};

const uint8_t LED_LOOKUP[] PROGMEM = {
             55, 59, 60, 94, 93, 98,
           54, 53, 61, 62, 92, 91, 99,
         16, 52, 51, 63, 64, 90, 89, 101,
       17, 18, 50, 49, 65, 66, 88, 87, 102,
     12, 19, 20, 48, 47, 67, 68, 86, 85, 104,
   11, 10, 21, 22, 46, 45, 69, 70, 84, 83, 105,
      9,  8, 23, 24, 44, 43, 71, 72, 82, 81,
        7,  6, 25, 26, 42, 41, 73, 74, 80,
          5,  4, 27, 28, 40, 39, 75, 76,
            3,  2, 29, 30, 38, 37, 77,
              1,  0, 31, 32, 36, 35
}; 

//const uint8_t LED_LOOKUP[] PROGMEM = {
//    11, 9, 7, 5, 3, 1, 12, 10, 8, 6, 4, 2, 0, 14, 16, 18, 20, 22, 
//    24, 26, 28, 13, 15, 17, 19, 21, 23, 25, 27, 29, 49, 47, 45, 43, 
//    41, 39, 37, 35, 33, 31, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 
//    30, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 52, 54, 56, 58, 60, 
//    62, 64, 66, 68, 84, 82, 80, 78, 76, 74, 72, 70, 83, 81, 79, 77, 
//    75, 73, 71, 85, 86, 87, 88, 89, 90
//}

const uint8_t led_map_edge_1[] PROGMEM = {
    11, 9, 7, 5, 3, 1, 12, 10, 8, 6, 4, 2, 0, 14, 16, 18, 20, 22, 
    24, 26, 28, 13, 15, 17, 19, 21, 23, 25, 27, 29, 49, 47, 45, 43, 
    41, 39, 37, 35, 33, 31, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 
    30, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 52, 54, 56, 58, 60, 
    62, 64, 66, 68, 84, 82, 80, 78, 76, 74, 72, 70, 83, 81, 79, 77, 
    75, 73, 71, 85, 86, 87, 88, 89, 90
};

const uint8_t led_map_edge_2[] PROGMEM = {
    50, 51, 52, 84, 83, 85, 49, 48, 53, 54, 82, 81, 86, 13, 47, 46, 
    55, 56, 80, 79, 87, 14, 15, 45, 44, 57, 58, 78, 77, 88, 12, 16, 
    17, 43, 42, 59, 60, 76, 75, 89, 11, 10, 18, 19, 41, 40, 61, 62, 
    74, 73, 90, 9, 8, 20, 21, 39, 38, 63, 64, 72, 71, 7, 6, 22, 23, 
    37, 36, 65, 66, 70, 5, 4, 24, 25, 35, 34, 67, 68, 3, 2, 26, 27, 
    33, 32, 69, 1, 0, 28, 29, 31, 30
};

const uint8_t led_map_edge_3[] PROGMEM = {
    90, 89, 88, 87, 86, 85, 71, 73, 75, 77, 79, 81, 83, 70, 72, 74, 
    76, 78, 80, 82, 84, 68, 66, 64, 62, 60, 58, 56, 54, 52, 69, 67, 
    65, 63, 61, 59, 57, 55, 53, 51, 30, 32, 34, 36, 38, 40, 42, 44, 
    46, 48, 50, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 29, 27, 25, 
    23, 21, 19, 17, 15, 13, 28, 26, 24, 22, 20, 18, 16, 14, 0, 2, 4, 
    6, 8, 10, 12, 1, 3, 5, 7, 9, 11
};

const uint8_t led_map_edge_4[] PROGMEM = {
    30, 31, 29, 28, 0, 1, 69, 32, 33, 27, 26, 2, 3, 68, 67, 34, 35, 
    25, 24, 4, 5, 70, 66, 65, 36, 37, 23, 22, 6, 7, 71, 72, 64, 63, 
    38, 39, 21, 20, 8, 9, 90, 73, 74, 62, 61, 40, 41, 19, 18, 10, 11, 
    89, 75, 76, 60, 59, 42, 43, 17, 16, 12, 88, 77, 78, 58, 57, 44, 
    45, 15, 14, 87, 79, 80, 56, 55, 46, 47, 13, 86, 81, 82, 54, 53, 
    48, 49, 85, 83, 84, 52, 51, 50
};

const uint8_t led_map_edge_5[] PROGMEM = {
    50, 49, 13, 14, 12, 11, 51, 48, 47, 15, 16, 10, 9, 52, 53, 46, 45, 
    17, 18, 8, 7, 84, 54, 55, 44, 43, 19, 20, 6, 5, 83, 82, 56, 57, 42, 
    41, 21, 22, 4, 3, 85, 81, 80, 58, 59, 40, 39, 23, 24, 2, 1, 86, 79, 
    78, 60, 61, 38, 37, 25, 26, 0, 87, 77, 76, 62, 63, 36, 35, 27, 28, 
    88, 75, 74, 64, 65, 34, 33, 29, 89, 73, 72, 66, 67, 32, 31, 90, 71, 
    70, 68, 69, 30
};

const uint8_t led_map_edge_6[] PROGMEM = {
    30, 69, 68, 70, 71, 90, 31, 32, 67, 66, 72, 73, 89, 29, 33, 34, 65, 
    64, 74, 75, 88, 28, 27, 35, 36, 63, 62, 76, 77, 87, 0, 26, 25, 37, 
    38, 61, 60, 78, 79, 86, 1, 2, 24, 23, 39, 40, 59, 58, 80, 81, 85, 3, 
    4, 22, 21, 41, 42, 57, 56, 82, 83, 5, 6, 20, 19, 43, 44, 55, 54, 84, 
    7, 8, 18, 17, 45, 46, 53, 52, 9, 10, 16, 15, 47, 48, 51, 11, 12, 14, 
    13, 49, 50
};

#define LIFE_MAP_SIZE 15
#define LIFE_NUM_CELLS  (LIFE_MAP_SIZE * LIFE_MAP_SIZE)
boolean *game_of_life_curr_map = (boolean *)calloc(LIFE_NUM_CELLS * DUAL, sizeof(boolean));
boolean *game_of_life_next_map = (boolean *)calloc(LIFE_NUM_CELLS * DUAL, sizeof(boolean));

const int8_t life_neighbors[] PROGMEM = { 1, 0, 1, -1, 0, -1, -1, 0, -1, 1, 0, 1 };
const int8_t life_next_neighbors[] PROGMEM = { 1, -2, 2, -1, 1, 1, -1, 2, -2, 1, -1, -1 };

//
// Pixel neighbors
//
const uint8_t neighbors[] PROGMEM = {
7, 6, XX, XX, XX, 1,
8, 7, 0, XX, XX, 2,
9, 8, 1, XX, XX, 3,
10, 9, 2, XX, XX, 4,
11, 10, 3, XX, XX, 5,
12, 11, 4, XX, XX, XX,
14, 13, XX, XX, 0, 7,
15, 14, 6, 0, 1, 8,
16, 15, 7, 1, 2, 9,
17, 16, 8, 2, 3, 10,
18, 17, 9, 3, 4, 11,
19, 18, 10, 4, 5, 12,
20, 19, 11, 5, XX, XX,
22, 21, XX, XX, 6, 14,
23, 22, 13, 6, 7, 15,
24, 23, 14, 7, 8, 16,
25, 24, 15, 8, 9, 17,
26, 25, 16, 9, 10, 18,
27, 26, 17, 10, 11, 19,
28, 27, 18, 11, 12, 20,
29, 28, 19, 12, XX, XX,
31, 30, XX, XX, 13, 22,
32, 31, 21, 13, 14, 23,
33, 32, 22, 14, 15, 24,
34, 33, 23, 15, 16, 25,
35, 34, 24, 16, 17, 26,
36, 35, 25, 17, 18, 27,
37, 36, 26, 18, 19, 28,
38, 37, 27, 19, 20, 29,
39, 38, 28, 20, XX, XX,
41, 40, XX, XX, 21, 31,
42, 41, 30, 21, 22, 32,
43, 42, 31, 22, 23, 33,
44, 43, 32, 23, 24, 34,
45, 44, 33, 24, 25, 35,
46, 45, 34, 25, 26, 36,
47, 46, 35, 26, 27, 37,
48, 47, 36, 27, 28, 38,
49, 48, 37, 28, 29, 39,
50, 49, 38, 29, XX, XX,
51, XX, XX, XX, 30, 41,
52, 51, 40, 30, 31, 42,
53, 52, 41, 31, 32, 43,
54, 53, 42, 32, 33, 44,
55, 54, 43, 33, 34, 45,
56, 55, 44, 34, 35, 46,
57, 56, 45, 35, 36, 47,
58, 57, 46, 36, 37, 48,
59, 58, 47, 37, 38, 49,
60, 59, 48, 38, 39, 50,
XX, 60, 49, 39, XX, XX,
61, XX, XX, 40, 41, 52,
62, 61, 51, 41, 42, 53,
63, 62, 52, 42, 43, 54,
64, 63, 53, 43, 44, 55,
65, 64, 54, 44, 45, 56,
66, 65, 55, 45, 46, 57,
67, 66, 56, 46, 47, 58,
68, 67, 57, 47, 48, 59,
69, 68, 58, 48, 49, 60,
XX, 69, 59, 49, 50, XX,
70, XX, XX, 51, 52, 62,
71, 70, 61, 52, 53, 63,
72, 71, 62, 53, 54, 64,
73, 72, 63, 54, 55, 65,
74, 73, 64, 55, 56, 66,
75, 74, 65, 56, 57, 67,
76, 75, 66, 57, 58, 68,
77, 76, 67, 58, 59, 69,
XX, 77, 68, 59, 60, XX,
78, XX, XX, 61, 62, 71,
79, 78, 70, 62, 63, 72,
80, 79, 71, 63, 64, 73,
81, 80, 72, 64, 65, 74,
82, 81, 73, 65, 66, 75,
83, 82, 74, 66, 67, 76,
84, 83, 75, 67, 68, 77,
XX, 84, 76, 68, 69, XX,
85, XX, XX, 70, 71, 79,
86, 85, 78, 71, 72, 80,
87, 86, 79, 72, 73, 81,
88, 87, 80, 73, 74, 82,
89, 88, 81, 74, 75, 83,
90, 89, 82, 75, 76, 84,
XX, 90, 83, 76, 77, XX,
XX, XX, XX, 78, 79, 86,
XX, XX, 85, 79, 80, 87,
XX, XX, 86, 80, 81, 88,
XX, XX, 87, 81, 82, 89,
XX, XX, 88, 82, 83, 90,
XX, XX, 89, 83, 84, XX,
};
/*
 * Old neighbors
 * 
 
  XX, XX, 1, 2, 26, 28,
  XX, XX, XX, 3, 2, 0,
  0, 1, 3, 4, 24, 26,
  1, XX, XX, 5, 4, 2,
  2, 3, 5, 6, 22, 24,
  3, XX, XX, 7, 6, 4,
  4, 5, 7, 8, 20, 22,
  5, XX, XX, 9, 8, 6,
  6, 7, 9, 10, 18, 20,
  7, XX, XX, 11, 10, 8,
  8, 9, 11, 12, 16, 18,
  9, XX, XX, XX, 12, 10,
  10, 11, XX, XX, 14, 16,
  15, 14, XX, XX, 49, 47,
  16, 12, XX, XX, 13, 15,
  17, 16, 14, 13, 47, 45,
  18, 10, 12, 14, 15, 17,
  19, 18, 16, 15, 45, 43,
  20, 8, 10, 16, 17, 19,
  21, 20, 18, 17, 43, 41,
  22, 6, 8, 18, 19, 21,
  23, 22, 20, 19, 41, 39,
  24, 4, 6, 20, 21, 23,
  25, 24, 22, 21, 39, 37,
  26, 2, 4, 22, 23, 25,
  27, 26, 24, 23, 37, 35,
  28, 0, 2, 24, 25, 27,
  29, 28, 26, 25, 35, 33,
  XX, XX, 0, 26, 27, 29,
  XX, XX, 28, 27, 33, 31,
  XX, XX, 31, 32, 69, XX,
  XX, XX, 29, 33, 32, 30,
  30, 31, 33, 34, 67, 69,
  31, 29, 27, 35, 34, 32,
  32, 33, 35, 36, 65, 67,
  33, 27, 25, 37, 36, 34,
  34, 35, 37, 38, 63, 65,
  35, 25, 23, 39, 38, 36,
  36, 37, 39, 40, 61, 63,
  37, 23, 21, 41, 40, 38,
  38, 39, 41, 42, 59, 61,
  39, 21, 19, 43, 42, 40,
  40, 41, 43, 44, 57, 59,
  41, 19, 17, 45, 44, 42,
  42, 43, 45, 46, 55, 57,
  43, 17, 15, 47, 46, 44,
  44, 45, 47, 48, 53, 55,
  45, 15, 13, 49, 48, 46,
  46, 47, 49, 50, 51, 53,
  47, 13, XX, XX, 50, 48,
  48, 49, XX, XX, XX, 51,
  53, 48, 50, XX, XX, 52,
  54, 53, 51, XX, XX, 84,
  55, 46, 48, 51, 52, 54,
  56, 55, 53, 52, 84, 82,
  57, 44, 46, 53, 54, 56,
  58, 57, 55, 54, 82, 80,
  59, 42, 44, 55, 56, 58,
  60, 59, 57, 56, 80, 78,
  61, 40, 42, 57, 58, 60,
  62, 61, 59, 58, 78, 76,
  63, 38, 40, 59, 60, 62,
  64, 63, 61, 60, 76, 74,
  65, 36, 38, 61, 62, 64,
  66, 65, 63, 62, 74, 72,
  67, 34, 36, 63, 64, 66,
  68, 67, 65, 64, 72, 70,
  69, 32, 34, 65, 66, 68,
  XX, 69, 67, 66, 70, XX,
  XX, 30, 32, 67, 68, XX,
  XX, 68, 66, 72, 71, XX,
  XX, 70, 72, 73, 90, XX,
  70, 66, 64, 74, 73, 71,
  71, 72, 74, 75, 89, 90,
  72, 64, 62, 76, 75, 73,
  73, 74, 76, 77, 88, 89,
  74, 62, 60, 78, 77, 75,
  75, 76, 78, 79, 87, 88,
  76, 60, 58, 80, 79, 77,
  77, 78, 80, 81, 86, 87,
  78, 58, 56, 82, 81, 79,
  79, 80, 82, 83, 85, 86,
  80, 56, 54, 84, 83, 81,
  81, 82, 84, XX, XX, 85,
  82, 54, 52, XX, XX, 83,
  86, 81, 83, XX, XX, XX,
  87, 79, 81, 85, XX, XX,
  88, 77, 79, 86, XX, XX,
  89, 75, 77, 87, XX, XX,
  90, 73, 75, 88, XX, XX,
  XX, 71, 73, 89, XX, XX,
};
*/


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial.println("Start");  // Serial: For debugging

  Serial1.begin(9600);  // Serial1: xBee port
  
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, ACTUAL_LEDS);
  FastLED.setBrightness( BRIGHTNESS );

  for (uint8_t i = 0; i < DUAL; i++) {
//    set_random_led_map(i);  // bug here I do not understand
    led[i].setCoordMap(MAX_COORD, coords);  // x, y grid of hexes
    led[i].setNeighborMap(neighbors);  // 6 neighbors for every pixel
    led[i].fillBlack();
    led[i].push_frame();
    shows[i] = Shows(&led[i]);  // Show library - reinitialized for led mappings
    shows[i].setColorSpeedMinMax(2, 10); // Speed up color changing
    shows[i].setWaitRange(4, 25, 21);
    set_show_variables(i);
  }

  shows[CHANNEL_B].setSmallCycle(MAX_SMALL_CYCLE / 2);  // Start Channel B offset at halfway through show
}

//
// loop
//
void loop() { 

  for (uint8_t i = 0; i < DUAL; i++) {
    
    switch (current_show[i]) {

      case 0:
        shows[i].randomFill();  // Vetted
        break;
      case 1:
        shows[i].juggle_fastled();
        break;
      case 2:
        shows[i].randomTwoColorBlack();
        break;
      case 3:
        shows[i].randomOneColorBlack();
        break;
      case 4:
        shows[i].sawTooth();  // Vetted
        break;
      case 5:
        pendulum_wave(i);  // Need work
        break;
      case 6:
        shows[i].bounce();  // Vetted
        break;
      case 7:
        shows[i].bounceGlowing();  // Vetted
        break;
      case 8:
        shows[i].plinko(90);  // Vetted
        break;
      case 9:
        shows[i].bands();  // Vetted
        break;
      case 10:
        vert_back_forth_dots(i);  // grided
        break;
      case 11:
        vert_back_forth_bands(i);  // grided
        break;
      case 12:
        vert_back_forth_colors(i);  // grided
        break;
      case 13:
        horiz_back_forth_dots(i);  // grided
        break;
      case 14:
        horiz_back_forth_bands(i);  // grided
        break;
      case 15:
        horiz_back_forth_colors(i);  // grided
        break;
      case 16:
        diag_back_forth_dots(i);  // grided
        break;
      case 17:
        diag_back_forth_bands(i);  // grided
        break;
      case 18:
        shows[i].lightWave();  // Vetted - better without led_map
        break;
      case 19:
        shows[i].packets();  // Vetted
        break;
      case 20:
        shows[i].sinelon_fastled();  // Vetted
        break;
      case 21:
        center_ring(i);
        break;
      case 22:
        corner_ring(i);
        break;
      case 23:
        game_of_life_one_color(i);
        break;
      case 24:
        game_of_life_two_color(i);
        break;
      default:
        diag_back_forth_colors(i);  // grided
        break;
    }

    shows[i].morphFrame();  // 1. calculate interp_frame 2. adjust palette
  }

  update_leds();  // push the interp_frames on to the leds
  morph_channels(get_intensity(CHANNEL_A));  // morph together the 2 leds channels and deposit on to Channel_A
  FastLED.show();  // Update the display
//  speak_and_hear();  // speak out or hear in signals
  advance_clocks();  // advance the cycle clocks and check for next show
  delay(DELAY_TIME); // The only delay
}

//
// advance_clocks
//
void advance_clocks() {
  for (uint8_t i = 0; i < DUAL; i++) {
    shows[i].advanceClock();
    if (shows[i].getSmallCycle() >= MAX_SMALL_CYCLE) { // && !is_listening()) { 
      next_show(i);
      pick_next_show(i);
    }
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

  // Set new show's colors to those of the other channel
  uint8_t other_channel = (i == 0) ? 1 : 0 ;
  shows[i].setForeColor(shows[other_channel].getForeColor());
  shows[i].setBackColor(shows[other_channel].getBackColor());

//  log_status(i);  // For debugging
}

//
// pick_next_show - dependent on channel i
//
void pick_next_show(uint8_t i) {
//  current_show[i] = (current_show[i] + 1) % NUM_SHOWS;  // For debugging
  current_show[i] = random8(1, NUM_SHOWS);
  shows[i].pickRandomColorSpeeds();
  shows[i].pickRandomWait();
//  set_random_led_map(i);
  set_show_variables(i);
//  speak_all_commands();

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
// set_show_variables - depends on the show
//
void set_show_variables(uint8_t i) {
  
  switch (current_show[i]) {

    case 0:  // randomFill
      shows[i].setWaitRange(1, 6, 7);
      break;
    case 5:
      show_var[i] = random8(5);
      break;
    case 19:
      shows[i].setWaitRange(10, 50, 7);
      break;
    case 11:
      break;
    case 22:
      break;
    case 23:
      show_var[i] = random8(6);
      break;
    default:
      shows[i].setWaitRange(4, 25, 21);
      shows[i].setColorSpeedMinMax(2, 10);  // morphChain
      break;
  }
}

//
// set_random_led_map - straight edge from 1 of 6 hexagonal edges
//
void set_random_led_map(uint8_t i) {
  
  switch(random(6)) {
    case 0:
      led[i].setLedMap(led_map_edge_1);
      break;
    case 1:
      led[i].setLedMap(led_map_edge_2);
      break;
    case 2:
      led[i].setLedMap(led_map_edge_3);
      break;
    case 3:
      led[i].setLedMap(led_map_edge_4);
      break;
    case 4:
      led[i].setLedMap(led_map_edge_5);
      break;
    default:
      led[i].setLedMap(led_map_edge_6);
      break;
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
  uint8_t m = 0;  // where we are in the send-out message string

  if (shows[0].getMorph() == 0 || shows[1].getMorph() == 0) {
    m = speak_start(m);
    m = speak_command(COMMAND_BRIGHTNESS, BRIGHTNESS, m);
    
    for (uint8_t i = 0; i < DUAL; i++) {  // Send one channel, then the next
      if (shows[i].getMorph() == 0) {
        m = speak_channel(i, m);  // Send the channel A or B prompt
        m = speak_command(COMMAND_SM_CYCLE, shows[i].getSmallCycle(), m);

        if (shows[i].getCycle() < 3) {
          m = speak_command(COMMAND_SHOW, current_show[i], m);
          m = speak_command(COMMAND_WAIT, shows[i].getWait(), m);
          m = speak_command(COMMAND_FORE, shows[i].getForeColor(), m);
          m = speak_command(COMMAND_BACK, shows[i].getBackColor(), m);
        }
      }
    }
    
    message[m++] = COMMAND_PERIOD;  // Terminate message with null character
    message[m++] = '\0';  // Terminate message with null character
    Serial1.print(message);  // Only done once
//    Serial.println(message);  // For debugging
  }
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
        current_show[i] = value;
        next_show(i);
      }
      break;

    case COMMAND_SM_CYCLE: // small_cycle
      shows[i].setSmallCycle(value);
      break;
  }
}

////// End Speaking & Hearing


////// Specialized shows

//
// pendulum_wave
//
void pendulum_wave(uint8_t c) {
  uint8_t side_bands = show_var[c] % 6; // how many side bands
  shows[c].fillBlack();
  
  for (uint8_t y = 0; y < MAX_COORD; y++) {
    uint8_t x = beatsin8(30 + y) / 23;
    shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x, y));

    if (side_bands > 0) {
      for (uint8_t i = 0; i < side_bands; i++) {
        CHSV color = led[c].gradient_wheel(shows[c].getForeColor(), (side_bands - i) * (255 / (side_bands + 1)));
        if (x - i - 1 > 0) {
          shows[c].setPixeltoColor(led[c].getLedFromCoord(x - i - 1, y), color);
        }
        if (x + i + 1 < MAX_COORD) {
          shows[c].setPixeltoColor(led[c].getLedFromCoord(x + i + 1, y), color);
        }
      }
    }
  }
}

//// Start Game of Life

void game_of_life_one_color(uint8_t c) {
  game_of_life(c, false);
}

void game_of_life_two_color(uint8_t c) {
  game_of_life(c, true);
}


//
// game_of_life - Conway's Game of Life for Hexes
//
void game_of_life(uint8_t c, boolean two_color) {
  if (shows[c].isShowStart()) {
    shows[c].setWait(40);
    seed_game_of_life_map(c);
  }
  
  if (shows[c].getMorph() == 0) {
    calculate_life(c);
    send_game_of_life_to_leds(c, two_color);
    push_game_of_life_frame(c);
  }
}

void calculate_life(uint8_t c) {
  boolean fate;
  
  for (uint8_t x = 0; x < LIFE_MAP_SIZE; x++) {
    for (uint8_t y = 0; y < LIFE_MAP_SIZE; y++) {
      float life_quantity = get_life_quantity(c, x, y);
      
      if (get_game_of_life_cell(c, x, y)) {
        fate = (life_quantity >= 2.0 && life_quantity <= 3.3) ? true : false;
      } else {
        fate = (life_quantity >= 2.3 && life_quantity <= 2.9) ? true : false;
      }
      set_game_of_next_life_cell(c, x, y, fate);
    }
  }
}

float get_life_quantity(uint8_t c, int8_t x, int8_t y) {
  return num_alive_neighbors(c, x, y) + (num_alive_next_neighbors(c, x, y) * 0.3);
}

uint8_t num_alive_neighbors(uint8_t c, int8_t x, int8_t y) {
  float life = 0;
  int8_t dx;
  int8_t dy;
  
  for (uint8_t dir = 0; dir < 6; dir++) {
    dx = pgm_read_byte_near(life_neighbors + (dir * 2));
    dy = pgm_read_byte_near(life_neighbors + (dir * 2) + 1);
    
    if (out_of_bounds(x + dx, y + dy)) {
      life += 0.5;
    } else {
      if (get_game_of_life_cell(c, x + dx, y + dy)) {
        life += 1.0;
      }
    }
  }
  return life;
}

uint8_t num_alive_next_neighbors(uint8_t c, int8_t x, int8_t y) {
  float life = 0;
  int8_t dx;
  int8_t dy;
  
  for (uint8_t dir = 0; dir < 6; dir++) {
    dx = pgm_read_byte_near(life_next_neighbors + (dir * 2));
    dy = pgm_read_byte_near(life_next_neighbors + (dir * 2) + 1);
    
    if (out_of_bounds(x + dx, y + dy)) {
      life += 0.5;
    } else {
      if (get_game_of_life_cell(c, x + dx, y + dy)) {
        life += 1.0;
      }
    }
  }
  return life;
}

void seed_game_of_life_map(uint8_t c) {
  // 20% chance of a life in a starting cell
  for (uint8_t x = 0; x < LIFE_MAP_SIZE; x++) {
    for (uint8_t y = 0; y < LIFE_MAP_SIZE; y++) {
      boolean outcome = (random8(0, 10) < 2) ? true : false;
      set_game_of_curr_life_cell(c, x, y, outcome);
      set_game_of_curr_life_cell(c, x, y, false);
    }
  }
}

void push_game_of_life_frame(uint8_t c) {
  for (uint8_t i = 0; i < LIFE_MAP_SIZE * LIFE_MAP_SIZE; i++) {
    game_of_life_curr_map[i + (c * LIFE_NUM_CELLS)] = game_of_life_next_map[i + (c * LIFE_NUM_CELLS)];
  }
}

void send_game_of_life_to_leds(uint8_t c, boolean two_color) {
  uint8_t foreColor = shows[c].getForeColor();
  uint8_t backColor = shows[c].getBackColor();
  uint8_t off_set = (LIFE_MAP_SIZE - MAX_COORD) / 2;
  boolean curr_life, next_life;
  
  for (uint8_t x = 0; x < LIFE_MAP_SIZE; x++) {
    for (uint8_t y = 0; y < LIFE_MAP_SIZE; y++) {
      uint8_t pixel = led[c].getLedFromCoord(x, y);
      if (pixel != XX) {
        curr_life = get_game_of_life_cell(c, x + off_set, y + off_set);
        next_life = get_game_of_life_next_cell(c, x + off_set, y + off_set);
        shows[c].setPixeltoColor(pixel, get_game_of_life_color(curr_life, next_life, two_color, foreColor, backColor));
      }
    }
  }
}

CHSV get_game_of_life_color(boolean curr_life, boolean next_life, boolean two_color, uint8_t foreColor, uint8_t backColor) {
  // One and two color versions depending on two_color boolean
  CHSV color;
  
  if (two_color) {
    if (curr_life) {
      color = (next_life) ? led[CHANNEL_A].wheel(foreColor) : led[CHANNEL_A].gradient_wheel(backColor, 128) ;
    } else {
      color = (next_life) ? led[CHANNEL_A].gradient_wheel(foreColor, 128) : led[CHANNEL_A].wheel(backColor) ;
    }
  
  } else {
    color = (curr_life) ? led[CHANNEL_A].wheel(foreColor) : BLACK ;  // one color version is either foreColor or Black
  }
  
  return color;
}

void set_game_of_curr_life_cell(uint8_t c, int8_t x, int8_t y, boolean value) {
  if (out_of_bounds(x, y) == false) {
    game_of_life_curr_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)] = value;
  }
}

void set_game_of_next_life_cell(uint8_t c, int8_t x, int8_t y, boolean value) {
  if (out_of_bounds(x, y) == false) {
    game_of_life_next_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)] = value;
  }
}

boolean get_game_of_life_cell(uint8_t c, int8_t x, int8_t y) {
  // if coordinates are out of bounds, default to false;
  if (out_of_bounds(x, y)) {
    return false;
  }
  return game_of_life_curr_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)];
}

boolean get_game_of_life_next_cell(uint8_t c, int8_t x, int8_t y) {
  // if coordinates are out of bounds, default to false;
  if (out_of_bounds(x, y)) {
    return false;
  }
  return game_of_life_next_map[x + (y * LIFE_MAP_SIZE)  + (c * LIFE_NUM_CELLS)];
}

boolean out_of_bounds(int8_t x, int8_t y) {
  // Is (x,y) on the game of life board between (0 ,0) and (LIFE_MAP_SIZE, LIFE_MAP_SIZE)?
  return (x < 0 || x >= LIFE_MAP_SIZE || y < 0 || y >= LIFE_MAP_SIZE);
}

//// End Game of Life

//
// morph_chain - grided! not the standard: shows[i].morphChain()
//
void morph_chain(uint8_t c) {
  if (shows[c].isShowStart()) {
    shows[c].makeWaitFaster(4);  // Divide wait value by 4 (make much faster)
  }
  uint8_t i, adj_i, fract;
  for (i = 0; i < NUM_LEDS; i++) {
    adj_i = i + (HEX_NUM * NUM_LEDS);
    fract = map((adj_i + shows[c].getCycle()) % NUM_LEDS, 0, NUM_LEDS, 0, 255);
    led[c].setPixelHue(i, led[c].interpolate_wrap(shows[c].getForeColor(), shows[c].getBackColor(), fract));
  }
}

//
// vert back forth dots - vertical dots moving back and forth
//
void vert_back_forth_dots(uint8_t c) {
  uint8_t temp_x;
  uint16_t cycle = shows[c].getCycle();

  led[c].fillBlack();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_x = x + HEX_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      if ((temp_x + cycle) % MAX_XGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
}

//
// vert back forth bands - vertical bands moving back and forth
//
void vert_back_forth_bands(uint8_t c) {
  uint8_t temp_x, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_x = x + HEX_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_x = (temp_x + cycle) % MAX_XGRID;
      intensity = sin8(map(temp_x, 0, MAX_XGRID, 0, 255) + (cycle % (255 / MAX_XGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
}

//
// vert back forth colors - vertical colors moving back and forth
//
void vert_back_forth_colors(uint8_t c) {
  uint8_t temp_x, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_x = x + HEX_XGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_x = (temp_x + cycle) % MAX_XGRID;
      hue = sin8(map(temp_x, 0, MAX_XGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_XGRID)) );
      led[c].setPixelHue(led[c].getLedFromCoord(x,y), hue);
    }
  }
}

//
// horiz back forth dots - horizontal dots moving back and forth
//
void horiz_back_forth_dots(uint8_t c) {
  uint8_t temp_y;
  uint16_t cycle = shows[c].getCycle();

  led[c].fillBlack();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_y = y + HEX_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      if ((temp_y + cycle) % MAX_XGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
}

//
// horiz back forth bands - horizontal bands moving back and forth
//
void horiz_back_forth_bands(uint8_t c) {
  uint8_t temp_y, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_y = y + HEX_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp_y = (temp_y + cycle) % MAX_YGRID;
      intensity = sin8(map(temp_y, 0, MAX_YGRID, 0, 255) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
}

//
// horiz back forth colors - horizontal colors moving back and forth
//
void horiz_back_forth_colors(uint8_t c) {
  uint8_t temp_y, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_y = y + HEX_YGRID;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp_y = (temp_y + cycle) % MAX_YGRID;
      hue = sin8(map(temp_y, 0, MAX_YGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelHue(led[c].getLedFromCoord(x,y), hue);
    }
  }
}

//
// diag back forth dots - diagonal dots moving back and forth
//
void diag_back_forth_dots(uint8_t c) {
  uint8_t temp_x, temp_y;
  uint16_t cycle = shows[c].getCycle();

  led[c].fillBlack();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_x = x + HEX_XGRID;
      temp_y = y + HEX_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      if ((temp_x + temp_y + cycle) % MAX_YGRID == 0) {
        shows[c].setPixeltoForeColor(led[c].getLedFromCoord(x,y));
      }
    }
  }
}

//
// diag back forth bands - diagonal bands moving back and forth
//
void diag_back_forth_bands(uint8_t c) {
  uint8_t temp_x, temp_y, temp, intensity;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {  // Don't light the head (row 0)
      temp_x = x + HEX_XGRID;
      temp_y = y + HEX_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp = (temp_x + temp_y + cycle) % MAX_YGRID;
      intensity = sin8(map(temp, 0, MAX_YGRID, 0, 255) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelColor(led[c].getLedFromCoord(x,y), led[c].gradient_wheel(shows[c].getBackColor(), intensity));
    }
  }
}

//
// diag back forth colors - diagonal colors moving back and forth
//
void diag_back_forth_colors(uint8_t c) {
  uint8_t temp_x, temp_y, temp, hue;
  uint16_t cycle = shows[c].getCycle();
  
  for (uint8_t x = 0; x < MAX_COORD; x++) {
    for (uint8_t y = 0; y < MAX_COORD; y++) {
      temp_x = x + HEX_XGRID;
      temp_y = y + HEX_YGRID;
      temp_x = (y % 2) ? temp_x : MAX_XGRID - temp_x - 1;
      temp_y = (x % 2) ? temp_y : MAX_YGRID - temp_y - 1;
      temp = (temp_x + temp_y + cycle) % MAX_YGRID;
      hue = sin8(map(temp, 0, MAX_YGRID, shows[c].getForeColor(), shows[c].getBackColor()) + (cycle % (255 / MAX_YGRID)) );
      led[c].setPixelHue(led[c].getLedFromCoord(x,y), hue);
    }
  }
}

/*

//
// draw_ring
//
void draw_ring(uint8_t ring_n, CHSV color, uint8_t c) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (get_bit_from_ring(i, ring_n % NUM_RINGS) == true) {
        led[c].setPixelColor(i, color);
    }
  }
}

*/

//
// tunnel vision
//
// Colored ring animating outwards
// color1 is the primary color, color2 is a trail color
// background is the background color
//
void center_ring(uint8_t c) {
  ring(c, shows[c].getForeColor(), BLACK, 5, 5, 8, 64, 128);
}

void corner_ring(uint8_t c) {
  uint8_t corner = show_var[c] % 6;
  uint8_t x = pgm_read_byte_near(corner_coords + (corner * 2));
  uint8_t y = pgm_read_byte_near(corner_coords + (corner * 2) + 1);
  ring(c, shows[c].getForeColor(), BLACK, x, y, 8, 64, 255);
}

void ring(uint8_t c, uint8_t color, CHSV background, uint8_t center_x, uint8_t center_y, uint8_t ring_speed, uint8_t cutoff, uint8_t ring_freq) {
  // center_x, center_y = ring center coordinates; (5,5) is hex centers
  // ring_speed = 1+. Higher = faster.
  // cutoff = ring thickness with higher values = thicker
  // ring_freq = (255: 1 ring at a time; 128: 2 rings at a time)
  led[c].fill(background);
  uint8_t value = (shows[c].getCycle() * ring_speed) % 256;

  for (uint8_t y = 0; y < MAX_COORD; y++) {
    for (uint8_t x = 0; x < MAX_COORD; x++) {
      uint8_t pixel = led[c].getLedFromCoord(x, y);
      if (pixel != XX) {
        uint8_t delta = abs(get_distance(x, y, center_x, center_y) - value) % ring_freq;
        if (delta < cutoff) {
          uint8_t intensity = map(delta, 0, cutoff, 255, 0);
          shows[c].setPixeltoColor(pixel, led[c].gradient_wheel(color, intensity));
        }
      }
    }
  }
}

/*

//
// warp1 - colors on a black field
// 
void warp1(uint8_t i) {
  
  switch ((shows[i].getCycle() / NUM_RINGS) % 6) {
    case 0:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0, 255, 0)), led[i].rgb_to_hsv(CRGB(0,100,0)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    case 1:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,0,255)), led[i].rgb_to_hsv(CRGB(0,0,100)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    case 2:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,255,255)), led[i].rgb_to_hsv(CRGB(0,100,100)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    case 3:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,255,0)), led[i].rgb_to_hsv(CRGB(100,100,0)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);  
      break;
    case 4:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,0,0)), led[i].rgb_to_hsv(CRGB(100,0,0)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);
      break;
    default:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,0,255)), led[i].rgb_to_hsv(CRGB(100,0,100)), led[i].rgb_to_hsv(CRGB(0,0,0)), i);  
      break;
  }
}

//
// warp2 - colors on a green field
//
void warp2(uint8_t i) {
  
  switch ((shows[i].getCycle() / NUM_RINGS) % 8) {
    case 0:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,255,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 1:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,200,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 2:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,150,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 3:
      tunnelvision(led[i].rgb_to_hsv(CRGB(0,100,100)), led[i].rgb_to_hsv(CRGB(0,160,100)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 4:
      tunnelvision(led[i].rgb_to_hsv(CRGB(255,255,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 5:
      tunnelvision(led[i].rgb_to_hsv(CRGB(200,200,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    case 6:
      tunnelvision(led[i].rgb_to_hsv(CRGB(150,150,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);
      break;
    default:
      tunnelvision(led[i].rgb_to_hsv(CRGB(100,100,0)), led[i].rgb_to_hsv(CRGB(160,160,0)), led[i].rgb_to_hsv(CRGB(0,100,0)), i);  
      break;
  }
}
*/

///////// DUAL SHOW LOGIC

//
// morph_channels - morph together the 2 chanels and update the LEDs
//
void morph_channels(uint8_t fract) {
  uint8_t led_number;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    led_number = convert_pixel_to_led(i);
    if (led_number != XX) {
      leds[led_number] = led[CHANNEL_A].getInterpHSV(leds_buffer[CHANNEL_B][i], 
                                                     leds_buffer[CHANNEL_A][i], 
                                                     fract);  // interpolate a + b channels
    }                                                 
  }
  turn_off_spacer_leds();
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
//  return ease8InOutQuad(intensity);
  return intensity;
}

//// End DUAL SHOW LOGIC


//
// update_leds - push the interp_frame on to the leds
//
void update_leds() {
  for (uint8_t c = 0; c < DUAL; c++) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      leds_buffer[c][i] = redden(led[c].getInterpFrameColor(i));
    }
  }
}

//
// get_distance - calculate distance between two coordinates (x1, y1) - (x2, y2)
//
uint8_t get_distance(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  // distance = sqrt(3 * (x^2 * xy * y^2))  - remove the 3
  // answer should span 0-250
  int16_t dx = (x2 - x1) * 25;
  int16_t dy = (y2 - y1) * 25;
  return sqrt16((dx * dx) + (dx * dy) + (dy * dy));
}

//
// convert pixel to led: account for the spacer pixels
//
uint8_t convert_pixel_to_led(uint8_t i) {
  if (i == XX) {
    return XX;
  }
  return pgm_read_byte_near(LED_LOOKUP + (i % NUM_LEDS));
}

//
// turn off spacer leds - blacken the 16 space pixels
//
void turn_off_spacer_leds() {
  for (uint8_t i = 0; i < NUMBER_SPACER_LEDS; i++) {
    leds[spacer_leds[i]] = CRGB(0, 0, 0);
  }
}

//
// redden - if ONLY_RED, turn hues to red, yellow, blue
//
CHSV redden(CHSV color) {  
  if (ONLY_RED) { color.h = map8( sin8(color.h), 192, 60); }
  return color;
}

//
// log status
//
void log_status(uint8_t i) {
  Serial.print("Channel: ");
  Serial.print(i);
  Serial.print(", Show: ");
  Serial.print(current_show[i]);
  Serial.println(".");
}
