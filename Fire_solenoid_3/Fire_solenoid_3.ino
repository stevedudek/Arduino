#include "FastLED.h"
#include "painlessMesh.h"
#include <Arduino_JSON.h>

//
//  Dragon flame solenoild - includes 2 LEDs for eye lights
//
//  8/6/22
//
//  FastLED - On the Adafruit Feather
//
//  MESH network!
//
//  Listens to the Network and an IR Controller
//
//
//  2 pins for clock and data of lights.
// 
#define CLOCK_FREQ 4    // 1 / CLOCK_FREQ  seconds

#define FLAME_WAIT_MIN (5 * CLOCK_FREQ)   // seconds
#define FLAME_WAIT_MAX (30 * CLOCK_FREQ)  // seconds

#define NUM_FLAME_SHOWS 7
uint8_t flame_show = random(NUM_FLAME_SHOWS);

#define DATA_PIN   0
#define CLOCK_PIN  2

#define NUM_LIGHTS 2

#define COLOR_MIN 10  // hue
#define COLOR_MAX 60  // hue
uint8_t hue = COLOR_MIN;
boolean lights_on = true;
#define FLICKER_CHANCE  20  // lower = faster flicked

CRGB leds[NUM_LIGHTS];  // The Leds themselves

uint8_t light_timer = 0;
uint8_t clock_timer = 0;  // in 0.25 seconds
boolean in_flame_show = false;
boolean flame_on = false;
uint8_t flame_timer = 0;  // in 0.25 seconds
uint8_t flame_off_time = 0;
uint8_t flame_on_time = 0;
#define MAX_FLAME_ON_TIME  (5 * CLOCK_FREQ)  // seconds

#define DELAY_TIME 30  // in milliseconds. A cycle takes 3-5 milliseconds. 15+ is safe.

#define FIRE_PIN  4

// User stub
void checkEveything();

Task taskCheckEverything(TASK_MILLISECOND * 1000 / CLOCK_FREQ, TASK_FOREVER, &checkEveything);

//// Mesh parameters

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

#define ARE_CONNECTED true  // Are the dragons talking to each other?
#define IS_SPEAKING true  // true = speaking, false = hearing

boolean is_lost = false;
uint16_t last_connection = 0;
#define MAX_SILENT_TIME  (CLOCK_FREQ * 60 * 2)  // Seconds without communication before marked as is_lost

String message;  // String to send to other displays
uint8_t msg_count = 0;
#define MESSAGE_REPEAT 2   // send this many duplicate messages

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

// User stub
String getReadings();  // Prototype for reading state of LEDs

#define IR_MESSAGE  0
#define FIRE_MESSAGE  3
#define FIRE_COMMAND 7

//// End Mesh parameters

//
// Setup
//
void setup() {

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(FIRE_PIN, OUTPUT);

  digitalWrite(FIRE_PIN, LOW);  // Turn flames off

  randomSeed(analogRead(0));
  
  delay( 3000 ); // power-up safety delay

  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN> (leds, NUM_LIGHTS);
  FastLED.setBrightness( 255 );  // ws2811 oddly is BRG
  
  Serial.begin(115200);
  Serial.println("Start");
  
  flame_off_time = random(FLAME_WAIT_MIN, FLAME_WAIT_MAX);

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  userScheduler.addTask(taskCheckEverything);
  taskCheckEverything.enable();
}

//
// loop
//
void loop() {
  userScheduler.execute();  // Essential!
  if (ARE_CONNECTED) {
    sendFlameStatus();
    mesh.update();
  }
}

void checkEveything() {
  adjust_hue();  // For the LED eye
  update_lights();
  checkFlame();

  if (ARE_CONNECTED && !IS_SPEAKING && !is_lost) {
    if (last_connection++ > MAX_SILENT_TIME) {
      is_lost = true;
    }
  }
}

void checkFlame() {
  adjust_hue();  // For the LED eye
  update_lights();

  if (in_flame_show) {
    run_flame_show();
    
  } else {
    clock_timer++;
    if (clock_timer == flame_off_time) {
      flame_show = random(NUM_FLAME_SHOWS);
      Serial.println(flame_show);
      in_flame_show = true;
      flame_timer = 0;
    }
  }
}

void run_flame_show() {
  
  switch (flame_show) {

      case 0:  // 1 second burst
        steady_flame(1);
        break;
      case 1:  // 2 second burst
        steady_flame(2);
        break;
      case 2:  // 3 second burst
        steady_flame(3);
        break;
      case 3:  // 0.5 second burst
        steady_flame(0.5);
        break;
      case 4:
        spurt_flame(2, 2);  // 2 second burst of 50% flames
        break;
      case 5:
        spurt_flame(4, 1);  // 4 second burst of 25% flames
        break;
      case 6:
        spurt_flame(8, 1);  // 8 second burst of 25% flames
        break;
      case 7:
        spurt_flame(3, 3);  // 3 second burst of 75% flames
        break;
      default:  // 0.25 second burst
        steady_flame(0.25);
        break;
  }
  flame_timer++;
}

void spurt_flame(float time_on, float fract_on) {
  if (flame_timer < time_on * CLOCK_FREQ) {
    if (flame_timer % 4 < fract_on) {
      start_flame_counter();
    } else {
      stop_flame_counter();
    }
  } else {
    end_flame_show();
  }
}

void steady_flame(float time_on) {
  if (flame_timer < time_on * CLOCK_FREQ) {
    start_flame_counter();
  } else {
    stop_flame_counter();
    end_flame_show();
  }
}

void end_flame_show() {
  digitalWrite(FIRE_PIN, LOW);  // Turn flames off 
  in_flame_show = false;
  clock_timer = 0;
  msg_count = 0;
}

void start_flame_counter() {
  if (!flame_on) {
    digitalWrite(FIRE_PIN, HIGH);  // Turn flames on 
    flame_on = true;
    flame_on_time = 0;
    msg_count = 0;
  }
}


void stop_flame_counter() {
  if (flame_on) {
    digitalWrite(FIRE_PIN, LOW);  // Turn flames on 
    flame_on = false;
    flame_on_time = 0;
    msg_count = 0;
  }
}

void check_flame_counter() {
  if (flame_on) {
    if (flame_on_time++ > MAX_FLAME_ON_TIME) {
      stop_flame_counter();  // flames went on too long!
    }
  }
}

void adjust_hue() {
  light_timer++;
  if (hue > COLOR_MAX) {
    hue = map(sin8(light_timer), 0, 255, COLOR_MIN, COLOR_MAX);
  }
  if (random(FLICKER_CHANCE) == 1) {
    lights_on = !lights_on;
  }
}

void update_lights() {
  leds[0] = CRGB::Black;
  if (lights_on) {
    leds[1] = CHSV(hue, 255, 255);
  } else {
    leds[1] = CRGB::Black;
  }
  FastLED.show();
}


////// Speaking and Hearing

void sendFlameStatus () {
  if (!IS_SPEAKING && msg_count < MESSAGE_REPEAT) {
    msg_count++;
    JSONVar jsonReadings;
  
    jsonReadings["type"] = FIRE_MESSAGE;
    jsonReadings["fire"] = flame_on;
  
    message = JSON.stringify(jsonReadings);
  
    mesh.sendBroadcast(message);
  }
}

void receivedCallback( uint32_t from, String &msg ) {
  // Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  JSONVar myObject = JSON.parse(msg.c_str());

  if (int(myObject["type"]) == FIRE_MESSAGE && !IS_SPEAKING) {
    last_connection = 0;
    is_lost = false;
    boolean flame_on = boolean(myObject["fire"]);
    
    if (flame_on) {
      digitalWrite(FIRE_PIN, HIGH);  // Turn flames on 
      flame_on = true;
      flame_on_time = 0;
    } else {
      digitalWrite(FIRE_PIN, LOW);  // Turn flames off 
      flame_on = false;
    }
    
  } else if (int(myObject["type"]) == IR_MESSAGE && int(myObject["param1"]) == FIRE_COMMAND) {
    flame_show = 0;
    in_flame_show = true;
    flame_timer = 0;
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

////// End Speaking and Hearing
