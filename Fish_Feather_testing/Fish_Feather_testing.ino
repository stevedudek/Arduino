#include <FastLED.h>
#include <Led.h>
#include <Shows.h>
#include "painlessMesh.h"
#include <ESP8266WebServer.h>
#include <Arduino_JSON.h>

//
//  Fish
//
//  9/20/21
//
//  On the Adafruit Feather
//
//  MESH network!
//
//  Speaking & Hearing
//

uint8_t BRIGHTNESS = 255;  // (0-255)

// Server
#define APSSID  "LED Controller"
#define APPASSWORD "neon"
ESP8266WebServer server(80);  // Usually 80

// MESH Details
#define   MESH_PREFIX     "ROARY" // name for your MESH
#define   MESH_PASSWORD   "roarroar" // password for your MESH
#define   MESH_PORT       5555 //default port

String message;  // String to send to other displays

//#define MSG_FREQUENCY  50  // send message every X milliseconds
//#define MESSAGE_REPEAT 3   // send this many duplicate messages
//#define MESSAGE_SPACING 3   // wait this many cycles
Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

// User stub
//void sendMessage();
//String getReadings();  // Prototype for reading state of LEDs

//Task taskSendMessage(MSG_FREQUENCY, TASK_FOREVER, &sendMessage);

void handleRoot() {
  sendRedMessage(0);
  sendRedMessage(1);
  server.send(200, "text/html", "<h1>You are connected</h1>");
  Serial.println("Got a client! Sent red messages on the mesh"); //added so see client connect
} 

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
  WiFi.softAP(APSSID);

  // by default the local IP address of will be 192.168.4.1
  // you can override it with the following:
  // WiFi.config(IPAddress(10, 0, 0, 1));

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
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

////// Speaking and Hearing

/*
String getReadings (uint8_t c) {
  JSONVar jsonReadings;

  jsonReadings["c"] = c;
  jsonReadings["cycle"] = (const double)shows[c].getSmallCycle();
  jsonReadings["f"] = shows[c].getForeColor();
  jsonReadings["fspd"] = shows[c].getForeColorSpeed();
  jsonReadings["b"] = shows[c].getBackColor();
  jsonReadings["bspd"] = shows[c].getBackColorSpeed();
  jsonReadings["s"] = current_show[c];
  jsonReadings["w"] = shows[c].getWait();
  jsonReadings["p"] = current_pattern[c];

  message = JSON.stringify(jsonReadings);
  return message;
}

void sendMessage (uint8_t c) {
  String msg = getReadings(c);
  mesh.sendBroadcast(msg);
}
*/

String makeRedMessage (uint8_t c) {
  JSONVar jsonReadings;

  jsonReadings["c"] = c;
  jsonReadings["f"] = 128;
  jsonReadings["b"] = 128;

  message = JSON.stringify(jsonReadings);
  return message;
}

void sendRedMessage (uint8_t c) {
  String msg = makeRedMessage(c);
  mesh.sendBroadcast(msg);
  Serial.println(msg);
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  /* 
  JSONVar myObject = JSON.parse(msg.c_str());

  int c = int(myObject["c"]);
  shows[c].setSmallCycle(double(myObject["cycle"]));
  shows[c].setForeColor(int(myObject["f"]));
  shows[c].setForeColorSpeed(int(myObject["fspd"]));
  shows[c].setBackColor(int(myObject["b"]));
  shows[c].setBackColorSpeed(int(myObject["bspd"]));
  shows[c].setWait(int(myObject["w"]));
  current_pattern[c] = int(myObject["p"]);
  
  int show_number = int(myObject["s"]);
  if (current_show[c] != show_number) {
    current_show[c] = show_number % NUM_SHOWS;
    next_show(c);
  }
  
  last_connection = 0;
  is_lost = false;
  */
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

////// End Speaking & Hearing
