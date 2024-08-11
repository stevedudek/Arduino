#include "painlessMesh.h"

//
//  Test of the Dragon flame solenoild
//
//  7/24/22
//
//  FastLED - On the Adafruit Feather
//
//  MESH network!
//
//  Listens to the Network and an IR Controller
//
//  For Shows & LED Libraries: Look in Documents/Arduino/libraries/Shows
//
//  Check the library code for anything that stores arrays
//  Additional smoothing function for RGB / RGB smoothing
//  No rotating nor symmetry easily possible
//  Masks and wipes should be possible
//
//

#define FLAME_FREQ 4  // seconds
#define FLAME_ON_SEC 2
uint8_t flame_timer = 0;

#define DELAY_TIME 20  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.

#define FIRE_PIN  2

// User stub
void checkFlame();

Scheduler userScheduler; // to control your personal task
Task taskCheckFlame(TASK_SECOND * 1, TASK_FOREVER, &checkFlame);


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

  pinMode(FIRE_PIN, OUTPUT);
  digitalWrite(FIRE_PIN, LOW);  // Turn flames off
  
  Serial.begin(115200);
  Serial.println("Start");
  
  userScheduler.addTask(taskCheckFlame);
  taskCheckFlame.enable();
}

//
// loop
//
void loop() {
  userScheduler.execute();  // Essential!
}

void checkFlame() {
  flame_timer++;
  Serial.println(flame_timer);
  if (flame_timer >= FLAME_FREQ + FLAME_ON_SEC) {
    digitalWrite(FIRE_PIN, LOW);  // Turn flames off
    flame_timer = 0;
    return;
  } else if (flame_timer == FLAME_FREQ) {
    digitalWrite(FIRE_PIN, HIGH);  // Turn flames off
    return;
  }
}
