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
//  10/9/21
//
//  On the Adafruit Feather
//
//  MESH network!
//
//  Send a message from a soft access point ESP3266 server
//

// Server
#define APSSID  "LED Controller"
#define APPASSWORD "neon"
ESP8266WebServer server(80);  // Usually 80

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

String message;  // String to send to other displays

#define PHONE_MESSAGE  0
#define MESH_MESSAGE  1

#define BRIGHTNESS_NAME  "brightness"
#define HUE_NAME  "hue"
#define HUE_WIDTH_NAME "width"
#define SPEED_NAME "speed"
#define SHOW_DURATION_NAME "duration"
#define FADE_NAME "fade"

#define BRIGHTNESS_COMMAND  0
#define HUE_COMMAND  1
#define HUE_WIDTH_COMMAND  2
#define SPEED_COMMAND  3
#define SHOW_DURATION_COMMAND  4
#define FADE_COMMAND 5

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

// User stub
//void sendMessage();
//String getReadings();  // Prototype for reading state of LEDs

//Task taskSendMessage(MSG_FREQUENCY, TASK_FOREVER, &sendMessage);
void handleRoot();
void handleControl();

//
// Setup
//
void setup() {

  delay( 3000 ); // power-up safety delay

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

  // Soft AP
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(APSSID, APPASSWORD);

  // by default the local IP address of will be 192.168.4.1
  // you can override it with the following:
  // WiFi.config(IPAddress(10, 0, 0, 1));

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/control", HTTP_GET, handleControl);
  server.begin();
  Serial.println("HTTP server started");
  Serial.println(WiFi.softAPIP());
}

//
// loop
//
void loop() {
  server.handleClient();
  mesh.update() ;
}

void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
  Serial.println("Got a client!");
}

void handleControl() {
  if (server.hasArg(BRIGHTNESS_NAME)) {
    sendParameterMessage(BRIGHTNESS_COMMAND, server.arg(BRIGHTNESS_NAME).toInt());
  
  } else if (server.hasArg(HUE_NAME)) {
    sendParameterMessage(HUE_COMMAND, server.arg(HUE_NAME).toInt());
  
  } else if (server.hasArg(HUE_WIDTH_NAME)) {
    sendParameterMessage(HUE_WIDTH_COMMAND, server.arg(HUE_WIDTH_NAME).toInt());
  
  } else if (server.hasArg(SPEED_NAME)) {
    sendParameterMessage(SPEED_COMMAND, server.arg(SPEED_NAME).toInt());
  
  } else if (server.hasArg(SHOW_DURATION_NAME)) {
    sendParameterMessage(SHOW_DURATION_COMMAND, server.arg(SHOW_DURATION_NAME).toInt());
  
  } else if (server.hasArg(FADE_NAME)) {
    sendParameterMessage(FADE_COMMAND, server.arg(FADE_NAME).toInt());
  }
}

String sendParameterMessage (uint8_t param, uint8_t value) {
  JSONVar jsonReadings;

  jsonReadings["type"] = PHONE_MESSAGE;
  jsonReadings["param"] = param;
  jsonReadings["value"] = value;

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
