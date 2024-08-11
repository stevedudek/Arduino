#include "painlessMesh.h"

//
//  Dragon flame solenoild
//
//  8/1/22
//
//  FastLED - On the Adafruit Feather
//
//  MESH network!
//
//  Listens to the Network and an IR Controller
//

#define IGNITER_TIME 4  // how many seconds to heat the igniter
#define FLAME_FREQ 4  // seconds

#define FLAME_WAIT_MIN 4   // in 0.25 seconds
#define FLAME_WAIT_MAX 20  // in 0.25 seconds

#define FLAME_ON_MIN 1  // in 0.25 seconds
#define FLAME_ON_MAX 4  // in 0.25 seconds

#define CLOCK_FREQ 4    // 1 / CLOCK_FREQ  seconds

uint32_t clock_timer = 0;  // in 0.25 seconds
uint8_t next_flame_timer = 0;  // in 0.25 seconds
uint8_t flame_timer = 0;  // in 0.25 seconds
uint8_t next_flame = 0;
uint8_t flame_duration = 0;

#define DELAY_TIME 50  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.

#define IGNITER_PIN  4
#define PILOT_PIN  0
#define FIRE_PIN  2

// User stub
void checkFlame();

Scheduler userScheduler; // to control your personal task
Task taskCheckFlame(TASK_MILLISECOND * 1000 / CLOCK_FREQ, TASK_FOREVER, &checkFlame);


//
// Setup
//
void setup() {

  pinMode(IGNITER_PIN, OUTPUT);
  pinMode(PILOT_PIN, OUTPUT);
  pinMode(FIRE_PIN, OUTPUT);

  digitalWrite(IGNITER_PIN, LOW);  // Turn igniter off
  digitalWrite(FIRE_PIN, LOW);  // Turn flames off
  digitalWrite(PILOT_PIN, LOW);  // Turn pilot off
  
  delay( 3000 ); // power-up safety delay
  
  digitalWrite(IGNITER_PIN, HIGH);  // Turn igniter on
  
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
  clock_timer++;
    
  if (clock_timer < IGNITER_TIME * CLOCK_FREQ) {
    return; // digitalWrite(IGNITER_PIN, HIGH);  // Turn igniter on (should already be on)
    
  } else if (clock_timer == IGNITER_TIME * CLOCK_FREQ) {
    digitalWrite(PILOT_PIN, HIGH);  // Turn pilot on and will stay on
    digitalWrite(IGNITER_PIN, LOW);  // Turn igniter off
    return;
    
  } else {
    // We are in pilot flame mode
    if (flame_timer == 0) {
      next_flame = random(FLAME_WAIT_MIN, FLAME_WAIT_MAX);  // Queue up a flame
      flame_duration = random(FLAME_ON_MIN, FLAME_ON_MAX);  // How long the flame will last
      return;
    
    } else if (flame_timer == next_flame) {
      digitalWrite(FIRE_PIN, HIGH);  // Turn flames on!
      flame_timer++;
      return;
    
    } else if (flame_timer >= next_flame + flame_duration) {
      digitalWrite(FIRE_PIN, LOW);  // Turn flames off
      flame_timer = 0;
    }
  }
}
