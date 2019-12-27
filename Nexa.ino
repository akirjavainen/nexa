/*
******************************************************************************************************************************************************************
*
* Nexa 433.92MHz appliances
* 
* Code by Antti Kirjavainen (antti.kirjavainen [_at_] gmail.com)
* 
* This code is command-compatible with the HomeEasy library, so you can capture your remote either with that or use an
* oscilloscope. You can capture your remotes and other Nexa devices with this code (convert decimal to binary as needed):
* 
* https://playground.arduino.cc/Code/HomeEasy?action=sourceblock&num=3
* 
* 
* HOW TO USE
* 
* Capture the binary commands from your Nexa devices with HomeEasy and copy paste binary bits to the sendNexaCommand()
* function as follows:
* 
* sender = first 26 bits: device ID
* group = 1 bit: command by recipient ID (single button on a remote, for example) or ALL ON/OFF
* on_off = 1 bit
* recipient = last 4 bits: button ID on a remote (for example)
* 
* = a total of 32 bits (64 wire bits)
* 
* Note that HomeEasy strips leading zeroes off from sender, so add them as needed to make sender 26 bits long.
* 
* 
* PROTOCOL DESCRIPTION
* 
* Manchester encoding is used.
* A single command is: 2 AGC bits + 32 command bits (64 wire bits) + radio silence
*
* All sample counts below listed with a sample rate of 44100 Hz (sample count / 44100 = microseconds).
* 
* AGC:
* Start with radio silence, LOW of approx. 8844 us (390 samples)
* HIGH of approx. 318 us (14 samples, same as HIGH space between wire bits)
* LOW of approx. 2449 us (108 samples)
* 
* Data bits (each data bit consisting of two wire bits, separated by HIGH space):
* Data 0 = Wire 01
* Data 1 = Wire 10
* 
* Wire bits:
* Wire 0 = HIGH space + short LOW of approx. 204 us (9 samples)
* Wire 1 = HIGH space + long LOW of approx. 1202 us (53 samples)
*  
* "Close" the last wire bit with a HIGH space and end with LOW radio silence of (minimum) 8844 us (390 samples)
* 
* 
* NOTE ABOUT TIMINGS OF NEXA DEVICES
* 
* Timings actually vary considerably between different Nexa devices. I used an oscilloscope to capture about a
* dozen different remotes, key fobs and wall switches. I tried to pick average timings to achieve maximum
* compatibility with this code.
* 
* Nexa receivers generally aren't terribly picky about the timings (probably for the aforementioned reason), but 
* other devices implementing the Nexa protocol can be more so. This is something I noticed with Cozify Hub: some
* tweaking was necessary to make it recognize my commands.
* 
******************************************************************************************************************************************************************
*/


// Copy paste your device ID here from HomeEasy, adding leading zeroes as necessary to make it 26 bits long:
#define NEXA_DEVICE_1                   "00000000000000000000000000"

#define NEXA_GROUP_FALSE                "0"    // Only command recipient ID, example: button 1/A from a remote with recipient 0
#define NEXA_GROUP_TRUE                 "1"    // Example: button "ALL ON/OFF" on a remote
#define NEXA_ON                         "1"
#define NEXA_OFF                        "0"
#define NEXA_RECIPIENT_0                "0000" // Example: button 1/A on a remote
#define NEXA_RECIPIENT_1                "0001" // Example: button 2/B on a remote, but WMR-1000 power relays use this as well
#define NEXA_RECIPIENT_2                "0010" // Example: button 3/C on a remote
#define NEXA_RECIPIENT_3                "0011" // Example: button 4/D on a remote
#define NEXA_RECIPIENT_WALL             "1010" // Example: LWST-615 wall switches

// NOTE: You'll want to use group bit 0/FALSE in almost every case, but some
// devices like the LMLT-711 doorbell may require 1/TRUE for commands to work.


// Timings in microseconds (us). Get sample count by zooming all the way in to the waveform with Audacity.
// Calculate microseconds with: (samples / sample rate, usually 44100 or 48000) - ~15-20 to compensate for delayMicroseconds overhead.
// Sample counts listed below with a sample rate of 44100 Hz:
#define NEXA_PULSE_HIGH_SPACE           310                      // 14 samples, between every LOW wire bit
#define NEXA_PULSE_WIRE_0               200                      // 9 samples, wire bit 0 (LOW)
#define NEXA_PULSE_WIRE_1               1195                     // 53 samples, wire bit 1 (LOW)

#define NEXA_PULSE_AGC1                 NEXA_PULSE_HIGH_SPACE    // Same as HIGH space
#define NEXA_PULSE_AGC2                 2440                     // 108 samples, LOW
#define NEXA_RADIO_SILENCE              8844                     // 390 samples, LOW, before the first and after each command

#define NEXA_COMMAND_BIT_ARRAY_SIZE     32                       // Command bit count, wire bit count is double (64)


#define TRANSMIT_PIN                    13                       // We'll use digital 13 for transmitting
#define REPEAT_COMMAND                  8                        // How many times to repeat the same command: original remotes and Cozify Hub repeat 5-7 times
#define DEBUG                           false                    // Some extra info on serial


// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600); // Used for error messages even with DEBUG set to false
  
  if (DEBUG) Serial.println("Starting up...");
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void loop() {

  // Try this command to turn your device ON after setting sender ID.
  // Recipient 0 means button 1/A from a remote (for example):
  sendNexaCommand(NEXA_DEVICE_1, NEXA_GROUP_FALSE, NEXA_ON, NEXA_RECIPIENT_0);
  delay(3000);
  sendNexaCommand(NEXA_DEVICE_1, NEXA_GROUP_FALSE, NEXA_OFF, NEXA_RECIPIENT_0);
  delay(3000);

}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void sendNexaCommand(char* sender, char* group, char* on_off, char* recipient) {
  
  // Prepare for transmitting and check for validity
  pinMode(TRANSMIT_PIN, OUTPUT); // Prepare the digital pin for output

  // Let's form and transmit the full command:
  char* full_command = new char[NEXA_COMMAND_BIT_ARRAY_SIZE];

  full_command[0] = '\0';
  strcat(full_command, sender);
  strcat(full_command, group);
  strcat(full_command, on_off);
  strcat(full_command, recipient);

  if (strlen(full_command) < NEXA_COMMAND_BIT_ARRAY_SIZE) {
    // Sender usually starts with a 0, but HomeEasy capture cuts these leading zeroes out:
    errorLog("sendNexaCommand(): Invalid command (too short). Try adding leading 0's to sender to make it 26 bits long.");
    Serial.println(full_command);
    return;
  }
  if (strlen(full_command) > NEXA_COMMAND_BIT_ARRAY_SIZE) {
    errorLog("sendNexaCommand(): Invalid command (too long), cannot continue.");
    return;
  }

  // Transmit radio silence before the first command in sequence:
  transmitLow(NEXA_RADIO_SILENCE);

  // Repeat the command:
  for (int i = 0; i < REPEAT_COMMAND; i++) {
    doNexaSend(full_command);
  }

  // Disable output to transmitter to prevent interference with
  // other devices. Otherwise the transmitter will keep on transmitting,
  // which will disrupt most appliances operating on the 433.92MHz band:
  digitalWrite(TRANSMIT_PIN, LOW);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void doNexaSend(char* command) {

  // Starting (AGC) bits:
  transmitHigh(NEXA_PULSE_AGC1);
  transmitLow(NEXA_PULSE_AGC2);

  // Transmit command bits:
  for (int i = 0; i < NEXA_COMMAND_BIT_ARRAY_SIZE; i++) {

    // If current command bit is 0, transmit wire bits 01:
    if (command[i] == '0') { // Wire 01
      transmitNexaWireBit(0);
      transmitNexaWireBit(1);
    }

    // If current command bit is 1, transmit wire bits 10:
    if (command[i] == '1') { // Wire 10
      transmitNexaWireBit(1);
      transmitNexaWireBit(0);
    }

   }

  // Radio silence at the end.
  // It's better to rather go a bit over than under required length.
  transmitHigh(NEXA_PULSE_HIGH_SPACE); // HIGH space to "close" the last wire bit
  transmitLow(NEXA_RADIO_SILENCE);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitNexaWireBit(int wire_bit) {
  transmitHigh(NEXA_PULSE_HIGH_SPACE); // HIGH space

  if (wire_bit == 0) transmitLow(NEXA_PULSE_WIRE_0); // Wire bit 0
  if (wire_bit == 1) transmitLow(NEXA_PULSE_WIRE_1); // Wire bit 1
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitHigh(int delay_microseconds) {
  digitalWrite(TRANSMIT_PIN, HIGH);
  //PORTB = PORTB D13high; // If you wish to use faster PORTB calls instead
  delayMicroseconds(delay_microseconds);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitLow(int delay_microseconds) {
  digitalWrite(TRANSMIT_PIN, LOW);
  //PORTB = PORTB D13low; // If you wish to use faster PORTB calls instead
  delayMicroseconds(delay_microseconds);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void errorLog(String message) {
  Serial.println(message);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
