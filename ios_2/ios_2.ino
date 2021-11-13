#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

//
//  Adafruit Feather Huzzah ESP8266 Server
//
//  Router-free | No interaction with LEDs
//
//  Sends commands to a Mesh network
//
//  9/15/21
//

// MESH Details
#define MESH_PREFIX     "ROARY-LED" // name for your MESH
#define MESH_PASSWORD   "roarroar" // password for your MESH
#define MESH_PORT       5555 //default port

#define LED_PIN  2  // blue LED. We will use this LED as a status light.

//ESP8266WebServer server(80);
WiFiServer server(80);

//
// setup
//
void setup() {

  Serial.begin(115200);  // Feather has a higher baud rate
  Serial.println("");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.softAP(MESH_PREFIX, MESH_PASSWORD);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("ESP8266 Access Point IP address: ");
  Serial.println(IP);

  server.begin();

  // Associate the URLs with the functions that will be handling the requests
//  server.on("/", HTTP_GET, handleRoot);
//  server.on("/setleds", HTTP_GET, handleSetLeds);
//  server.onNotFound(handleNotFound);
}

//
// loop
//
void loop(void) {
  
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    // Server Response
 
    // Header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println(""); // header end marker
   
    // Content
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
   
    client.print(request);
  }
}

/*

// User-defined function that will be called when a client accesses the root
// directory path of the ESP8266 host
void handleRoot() {
  // Simply sends an 'OK' (200) response to the client, and a plain text
  // string with usage.
  server.send(200, "text/plain", String("Hello from esp8266! Usage: navigate to") +
              String(" /setleds?led1=XX&led2=YY changing XX/YY to ON or OFF."));
}

// User-defined function that will be called when a client accesses the /setleds
// path of the ESP8266 host
void handleSetLeds() {
  // We can read the desired status of the LEDs from the expected parameters that
  // should be passed in the URL.  We expect two parameters "led1" and "led2".
  String led1_status = server.arg("led1");
  String led2_status = server.arg("led2");
  
  // Check if the URL include a change of the LED status
  bool url_check = false;
  if((led1_status == "ON")||(led1_status == "OFF")||(led2_status == "ON")||(led2_status == "OFF"))
    url_check = true;

  // It's not required to pass them both, so we check that they're exactly equal to
  // the strings ON or OFF by our design choice (this can be changed if you prefer
  // a different behavior)
  if (led1_status == "ON")
    digitalWrite(led1_pin, HIGH);
  else if (led1_status == "OFF")
    digitalWrite(led1_pin, LOW);
  if (led2_status == "ON")
    digitalWrite(led2_pin, HIGH);
  else if (led2_status == "OFF")
    digitalWrite(led2_pin, LOW);
  if (url_check)
    // If we've set the LEDs to the requested status, we have the webserver
    // return an "OK" (200) response.  We also include the number of milliseconds
    // since the program started running.
    // Note: This number will overflow (go back to zero), after approximately 50 days.
    server.send(200, "text/plain", "LEDs' status changed! (" + String(millis()) + ")");
  else
    server.send(200, "text/plain", "LEDs' status unchanged! (" + String(millis()) + ")");
}

// If the client requests any other URL than the root directory or the /setled path:
void handleNotFound() {
  // We construct a message to be returned to the client
  String message = "File Not Found\n\n";
  // which includes what URI was requested
  message += "URI: ";
  message += server.uri();
  // what method was used
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  // and what parameters were passed
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  // the response, as expected, is a "Not Found" (404) error
  server.send(404, "text/plain", message);
}

*/
