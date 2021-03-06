int datapin  = 8; // DI
int latchpin = 7; // LI
int enablepin = 6; // EI
int clockpin = 5; // CI

unsigned long SB_CommandPacket;
int SB_CommandMode;
int SB_BlueCommand;
int SB_RedCommand;
int SB_GreenCommand;

void setup() {
   pinMode(datapin, OUTPUT);
   pinMode(latchpin, OUTPUT);
   pinMode(enablepin, OUTPUT);
   pinMode(clockpin, OUTPUT);

   digitalWrite(latchpin, LOW);
   digitalWrite(enablepin, LOW);
}

void SB_SendPacket() {
   SB_CommandPacket = SB_CommandMode & B11;
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_BlueCommand & 1023);
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_RedCommand & 1023);
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_GreenCommand & 1023);

   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 24);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 16);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 8);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket);

   delay(1); // adjustment may be necessary depending on chain length
   digitalWrite(latchpin,HIGH); // latch data into registers
   delay(1); // adjustment may be necessary depending on chain length
   digitalWrite(latchpin,LOW);
}

void loop() {

   SB_CommandMode = B01; // Write to current control registers
   SB_RedCommand = 127; // Full current
   SB_GreenCommand = 127; // Full current
   SB_BlueCommand = 127; // Full current
   SB_SendPacket();

   SB_CommandMode = B00; // Write to PWM control registers
   SB_RedCommand = 1023; // Maximum red
   SB_GreenCommand = 0; // Minimum green
   SB_BlueCommand = 0; // Minimum blue
   SB_SendPacket();

   delay(250);

   SB_CommandMode = B00; // Write to PWM control registers
   SB_RedCommand = 0; // Minimum red
   SB_GreenCommand = 1023; // Maximum green
   SB_BlueCommand = 0; // Minimum blue
   SB_SendPacket();

   delay(250);

   SB_CommandMode = B00; // Write to PWM control registers
   SB_RedCommand = 0; // Minimum red
   SB_GreenCommand = 0; // Minimum green
   SB_BlueCommand = 1023; // Maximum blue
   SB_SendPacket();

   delay(250);
}
