#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <ESP8266WebServer.h>
#include <Arduino_JSON.h>
#include <IRremote.h>

//
//  LED Bridge ESP3266
//
//  10/29/21
//
//  On the Adafruit Feather
//
//  MESH network!
//
//  Send a message from an IR receiver to a MESH network
//

#define IR_RECEIVE_PIN  2  // the pin where you connect the output pin of IR sensor

long last_time;
#define DEBOUNCE_TIME 200

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

String message;  // String to send to other displays

uint32_t brightness = 255;

#define PHONE_MESSAGE  0
#define MESH_MESSAGE  1

#define BRIGHTNESS_NAME  "brightness"
#define HUE_NAME  "hue"
#define HUE_WIDTH_NAME "width"
#define SPEED_NAME "speed"
#define SHOW_DURATION_NAME "duration"
#define FADE_NAME "fade"

#define EMPTY_COMMAND  0
#define BRIGHTNESS_COMMAND  1
#define HUE_COMMAND  2
#define HUE_WIDTH_COMMAND  3
#define SPEED_COMMAND  4
#define SHOW_DURATION_COMMAND  5
#define FADE_COMMAND 6

#define BRIGHT_UP  92
#define BRIGHT_DOWN  93
#define PLAY  65
#define STOP  64  // Don't key off this one
#define RED_1  88  
#define RED_2  84
#define RED_3  80
#define RED_4  28
#define RED_5  24
#define GREEN_1  89
#define GREEN_2  85
#define GREEN_3  81
#define GREEN_4  29
#define GREEN_5  25
#define BLUE_1  69
#define BLUE_2  73
#define BLUE_3  77
#define BLUE_4  30
#define BLUE_5  26
#define WHITE_1  68
#define WHITE_2  72
#define WHITE_3  76
#define WHITE_4  31
#define WHITE_5  27
#define RED_UP  20
#define RED_DOWN  16
#define GREEN_UP  21
#define GREEN_DOWN  17
#define BLUE_UP  22
#define BLUE_DOWN  18
#define QUICK  23
#define SLOW  19
#define DIY1  12
#define DIY2  13
#define DIY3  14
#define DIY4  8
#define DIY5  9
#define DIY6  10
#define AUTO  15
#define FLASH  11
#define JUMP3  4
#define JUMP7  5
#define FADE3  6
#define FADE7  7

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

// User stub
//void sendMessage();
//String getReadings();  // Prototype for reading state of LEDs

//Task taskSendMessage(MSG_FREQUENCY, TASK_FOREVER, &sendMessage);


//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay
  
  IrReceiver.begin(IR_RECEIVE_PIN); // Start the receiver, enable feedback LED, take LED feedback pin from the internal boards definition
  
  randomSeed(analogRead(0));
  
  Serial.begin(115200);
  Serial.println("Start");
  

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
}

//
// loop
//
void loop() {
  mesh.update();
  check_ir();
}

void check_ir() {
  
  if (IrReceiver.decode()) {
    IrReceiver.resume(); // Enable receiving of the next value
    if (millis() - last_time > DEBOUNCE_TIME) {
      last_time = millis();
      uint8_t ir_command = IrReceiver.decodedIRData.command;
      Serial.println(ir_command);

      execute_ir_command(ir_command);

      if (ir_command == 92) {
        brightness = (brightness + 10) % 255;
        sendParameterMessage(BRIGHTNESS_COMMAND, brightness); // brightness up
      }
      if (ir_command == 93) {
        if (brightness > 11) {
          brightness -= 10;
        }
        sendParameterMessage(BRIGHTNESS_COMMAND, brightness); // brightness down
      }
    }
  }
}

void execute_ir_command(uint8_t ir_command) {

  switch (ir_command) {

    case BRIGHT_UP:
      brightness = (brightness + 10) % 255;
      sendParameterMessage(BRIGHTNESS_COMMAND, brightness);
      break;
    case BRIGHT_DOWN:
      if (brightness > 11) {
        brightness -= 10;
      }
      sendParameterMessage(BRIGHTNESS_COMMAND, brightness);
      break;
    case RED_1:
      sendHueMessage(0, 128);
      break;
    case RED_2:
      sendHueMessage(0, 64);
      break;
    case RED_3:
      sendHueMessage(34, 128);
      break;
    case RED_4:
      sendHueMessage(34, 64);
      break;
    case RED_5:
      sendHueMessage(68, 128);
      break;
    case GREEN_1:
      sendHueMessage(96, 128);
      break;
    case GREEN_2:
      sendHueMessage(96, 64);
      break;
    case GREEN_3:
      sendHueMessage(130, 128);
      break;
    case GREEN_4:
      sendHueMessage(130, 64);
      break;
    case GREEN_5:
      sendHueMessage(145, 128);
      break;
    case BLUE_1:
      sendHueMessage(160, 128);
      break;
    case BLUE_2:
      sendHueMessage(160, 64);
      break;
    case BLUE_3:
      sendHueMessage(190, 128);
      break;
    case BLUE_4:
      sendHueMessage(190, 64);
      break;
    case BLUE_5:
      sendHueMessage(223, 128);
      break;
    case WHITE_1:
      sendParameterMessage(HUE_WIDTH_COMMAND, 255);
      break;
    case WHITE_2:
      sendParameterMessage(HUE_WIDTH_COMMAND, 196);
      break;
    case WHITE_3:
      sendParameterMessage(HUE_WIDTH_COMMAND, 128);
      break;
    case WHITE_4:
      sendParameterMessage(HUE_WIDTH_COMMAND, 64);
      break;
    case WHITE_5:
      sendParameterMessage(HUE_WIDTH_COMMAND, 32);
      break;

/*
#define RED_UP  20
#define RED_DOWN  16
#define GREEN_UP  21
#define GREEN_DOWN  17
#define BLUE_UP  22
#define BLUE_DOWN  18
#define QUICK  23
#define SLOW  19
#define DIY1  12
#define DIY2  13
#define DIY3  14
#define DIY4  8
#define DIY5  9
#define DIY6  10
#define AUTO  15
#define FLASH  11
#define JUMP3  4
#define JUMP7  5
#define FADE3  6
#define FADE7  7
*/
  }
}

void sendHueMessage (uint8_t hue, uint8_t hue_width) {
  JSONVar jsonReadings;

  jsonReadings["type"] = PHONE_MESSAGE;
  jsonReadings["param1"] = HUE_COMMAND;
  jsonReadings["value1"] = hue;
  jsonReadings["param2"] = HUE_WIDTH_COMMAND;
  jsonReadings["value2"] = hue_width;

  message = JSON.stringify(jsonReadings);
  mesh.sendBroadcast(message);
  Serial.println(message);
}

void sendParameterMessage (uint8_t param, uint8_t value) {
  JSONVar jsonReadings;

  jsonReadings["type"] = PHONE_MESSAGE;
  jsonReadings["param1"] = param;
  jsonReadings["value1"] = value;
  jsonReadings["param2"] = EMPTY_COMMAND;
  jsonReadings["value2"] = 0;

  message = JSON.stringify(jsonReadings);
  mesh.sendBroadcast(message);
  Serial.println(message);
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
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
