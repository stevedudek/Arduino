
#define NO_DATA 9999
// xBee language
#define COMMAND_PROMPT     '#'
#define COMMAND_WHITE      'W'
#define COMMAND_COLOR      'C'
#define COMMAND_DELAY      'D'
#define COMMAND_SHOW       'S'
#define COMMAND_COLORSENSE 'X'
#define COMMAND_END        '$'

#define COST_NUM  '1'    // Which number costume this is. Quite important!

#define NO_CHANGE 0
#define MINOR_CHANGE 1  // Colors have changed
#define MAJOR_CHANGE 2  // Show has changed


void setup() {
    
  Serial.begin(9600);
  
}

void loop() {
   
}

//
// HearBox
//
// Listen for the Box
//

byte HearBox() {
  char incoming;
  long value;
  byte flag = NO_CHANGE;
  
  if (Serial.available()) {
    
    incoming = Serial.read();
    Serial.print(incoming);
    if (incoming == COMMAND_PROMPT) {  // Heard from the box
       incoming = Serial.read();       // For which number costume?
       Serial.print(incoming);
       if (incoming == COST_NUM || incoming == '9') {  // Talking to me!
       
           incoming = Serial.read();  // Get that one-letter command
           Serial.print(incoming);
           value = HearNum();         // Get the number that follows the command
           Serial.println(value);
           if (value == NO_DATA) {
              Serial.println("No legitimate number read");
             return(flag); // Not getting a legit number
           }
           // Execute the command and return any flags from it
           return(NO_CHANGE);
       }
    }
  }
   
  return(NO_CHANGE);  // no system update required
}

//
// HearNum
//
// Converts xBee characters into a number
// '$' character terminates the number string
//
// If no number, returns NO_DATA signifier

long HearNum() {
  #define BUFSIZE 20
  #define TIMEOUT_TRIES 100
  
  int waitTime = 0;  // Start counter
  int charplace = 0; // Start of number
  char buf[BUFSIZE];
  
  // Clear character buffer
  for (int i = 0; i <BUFSIZE; i++) buf[i] = 0;
  
  while (waitTime < TIMEOUT_TRIES) {
   
    if (Serial.available()) {
      char tmp = Serial.read();
      //Serial.print(tmp);
      if (tmp != '$') { // Hopefully 0-9 and '-'
        waitTime = 0;  // Reset wait counter
        buf[charplace] = tmp;
        if (charplace++ >= BUFSIZE) { return(NO_DATA); } // Number too long
       } else {      // Got a '$' to end the word
         if (charplace>0) return atoi(buf); else {  // successfully reached the end of a number 
           //Serial.println("i=0 number read");
           return(NO_DATA);  // Returning an empty word
         }
       }
      } else { 
        delay(1);
        waitTime++;
      }
    }
  //Serial.println("Out_of_Time");
  return(NO_DATA);  // ran out of time for next character: bail and return NO_DATA
}


