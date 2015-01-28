#include <TimerOne.h>

//
//  Microhpone - Can I beat match?
//
/*****************************************************************************/
//
// Set Up Pins
//

#define soundPin 5   // analog-in pin for microphone
#define SAMPLE_TIME  2  // Time window in seconds that will be FFT
#define SAMPLE_FREQ  0.1  // How often in seconds a sound reading is taken
#define TOTAL_POINTS 20

int gain = 10;  // Will get adjusted later
char sound_array[TOTAL_POINTS];
char im_array[TOTAL_POINTS];
int curr_place = 0;
boolean full_flag = false;

char sound;

void setup() {
  
  Timer1.initialize(SAMPLE_FREQ * 1000000);  // Microseconds: 10000 = 0.1s = 10Hz
  Timer1.attachInterrupt(hear);
  
  Serial.begin(9600);  // For debugging
}


void loop() {
  
  if (full_flag) {
    full_flag = false;  // Reset
    print_sound_array();
    gain = recalc_gain();
    //fix_fft(sound_array, im, 14, 0);
    //updateData();
  }
}

void hear() {
  sound_array[curr_place] = ((analogRead(soundPin) - 512) * gain) / 4;
  im_array[curr_place] = 0;
  
  //sound = ((analogRead(soundPin) - 512) * gain) / 4;
  
  //print_sound(sound);
  
  if (curr_place++ >= TOTAL_POINTS) {
    curr_place = 0;
    full_flag = true;
  }
}

int recalc_gain() {
  char curr_max = get_max();
  Serial.println((int)curr_max);
  //if (curr_max < 10) return gain;
  //int new_gain = 256 * gain / curr_max;

  //Serial.println(new_gain);
  return gain;
}

char get_max() {
  char sound;
  char max_sound = 0;
  for (int i = 0; i < TOTAL_POINTS; i++) {
    sound = sound_array[i];
    if (sound > max_sound) {
      max_sound = sound;
    }
  }
  return(max_sound);
}

void print_sound_array() {
  for (int i = 0; i < TOTAL_POINTS; i++) {
    //Serial.print((int)sound_array[i]);
    //Serial.print(" ");
    //print_sound(sound_array[i]);
  }
}

void print_sound(char intense) {
  byte xs, os, i;
  
  if (intense >= 0) {
    os = 32;
    xs = intense / 8;
  } else {
    xs = 0;
    os = (256 + intense) / 8;
  }
  
  for (i = 0; i < (32 - os); i++) {
    Serial.print(" ");
  }  
  for (i = 0; i < os; i++) {
    Serial.print("O");
  }
  for (i = 0; i < xs; i++) {
    Serial.print("X");
  }
  Serial.println("");
  }

void updataData() {
  for (int i = 0; i < TOTAL_POINTS; i++) {
    sound_array[i] = sqrt(sound_array[i] * sound_array[i] + im_array[i] * im_array[i]);
  }
}


