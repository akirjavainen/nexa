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
* sender (string) = first 26 bits
* group (int) = 1 bit
* on_off (int) = 1 bit
* recipient (string) = last 4 bits
* 
* = a total of 32 bits (64 wire bits)
* 
* Note that HomeEasy strips leading zeroes off from sender, so add them as needed to make sender 26 bits long.
* 
* 
* 
* PROTOCOL DESCRIPTION
* 
* A single command is: 2 AGC bits + 32 command bits (64 wire bits) + radio silence
*
* All sample counts below listed with a sample rate of 44100 Hz (sample count / 44100 = microseconds).
* 
* AGC:
* HIGH of approx. 340 us (15 samples)
* LOW of approx 2812 us (124 samples)
* 
* Pulse length:
* SHORT: approx. 249 us (11 samples), used for HIGH space and wire bit 0 (LOW)
* LONG: approx. 1383 us (61 samples), used for wire bit 1 (LOW)
* 
* Data bits:
* Data 0 = Wire 01
* Data 1 = Wire 10
* 
* Wire bits:
* Wire 0 = HIGH space (short), short LOW
* Wire 1 = HIGH space (short), long LOW
*  
* "Close" the last bit with a (short) HIGH space and end with LOW radio silence of (minimum) 471 samples = 10680 us
* 
******************************************************************************************************************************************************************
*/





#define NEXA_KEYFOB_ANTTI               "00100100000110110011011010" // 9465050, recipient 0, LKCT-614 key fob
#define NEXA_KEYFOB_SABRINA             "01010010010001111110010010" // 21569426, recipient 2, LKCT-614 key fob
#define NEXA_PLUG_LIVING_ROOM_SHADE     "01110010110011100111101110" // Nexa Remote 30095854, recipient 1, MYCR-1000 power switch (switch marked "2")
#define NEXA_PLUG_SAUNA_60              "00100110110100000000001010" // Cozify 10174474, recipient 1 or 2?, MYCR-1000 power switch (switch marked "3")
#define NEXA_PLUG_SHADES                "00100110110100000000001011" // Cozify 10174475, Cozify virtual power switch
#define NEXA_PLUG_OUTDOOR               "00100110110100000000000101" // Cozify 10174469, LGDR-3500
#define NEXA_REMOTE                     "01110010110011100111101110" // Nexa Remote 30095854, recipient 0-2 = buttons 1-3, thin remote
#define NEXA_REMOTE_LCD                 "00100011101001011100101010" // 9344810, recipient 0-15 = channels 1-16, TMT-918
#define NEXA_RELAY_FRONT                "00100110110100000000001001" // Cozify 10174473, recipient 1, WMR-1000 power relay
#define NEXA_RELAY_TERRACE              "00100110110100000000000111" // Cozify 10174471, recipient 1, WMR-1000 power relay
#define NEXA_WALLSWITCH_SAUNA           "01100101000011100101000010" // 26491202, recipient 10, LWST-615 wall switch
#define DOORBELL_FRONT                  "01100000010000111010111110" // LMLT-711, recipient 0, ON, group 1
#define DOORBELL_TERRACE                "01100100100000011100100010" // LMLT-711, recipient 0, ON, group 1

#define NEXA_RECIPIENT_SAUNA            "1110" // LCD remote: Sauna 60 scene, channel 15, recipient 14
#define NEXA_RECIPIENT_GUEST            "1111" // LCD remote: Guest room power switch, channel 16, recipient 15
// OTHER LCD RECIPIENTS: channel 2: living room lights, 3: hallway lights, 4: kitchen lights, 5: kitchen lightstrip, 6: shower & bathroom lights




#define NEXA_DEVICE_1                   "00000000000000000000000000" // Copy paste your device ID here from HomeEasy

#define NEXA_GROUP_DEFAULT              0
#define NEXA_GROUP_1                    1
#define NEXA_ON                         1
#define NEXA_OFF                        0
#define NEXA_RECIPIENT_0                "0000"
#define NEXA_RECIPIENT_1                "0001"
#define NEXA_RECIPIENT_2                "0010"
#define NEXA_RECIPIENT_3                "0011"
#define NEXA_RECIPIENT_WALL             "1010" // Wall switches


// Timings in microseconds (us). Get sample count by zooming all the way in to the waveform with Audacity.
// Calculate microseconds with: (samples / sample rate, usually 44100 or 48000) - ~15-20 to compensate for delayMicroseconds overhead.
// Sample counts listed below with a sample rate of 44100 Hz:
#define NEXA_AGC1_PULSE                 330    // 15 samples, HIGH
#define NEXA_AGC2_PULSE                 2800   // 124 samples, LOW
#define NEXA_RADIO_SILENCE              10680  // 471 samples, LOW

#define NEXA_PULSE_SHORT                240    // 11 samples, used for HIGH space and wire bit 0
#define NEXA_PULSE_LONG                 1370   // 61 samples, used for wire bit 1

#define NEXA_COMMAND_BIT_ARRAY_SIZE     32     // Command bit count, wire bit count is double (64)


#define TRANSMIT_PIN                    13     // We'll use digital 13 for transmitting
#define REPEAT_COMMAND                  8      // How many times to repeat the same command: original remotes and Cozify Hub repeat 5-7 times
#define DEBUG                           false  // Some extra info on serial


// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600); // Used for error messages even with DEBUG set to false
  
  if (DEBUG) Serial.println("Starting up...");
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void loop() {
  //sendNexaCommand(NEXA_DEVICE_1, NEXA_GROUP_1, NEXA_ON, NEXA_RECIPIENT_0);
  sendNexaCommand(NEXA_PLUG_LIVING_ROOM_SHADE, NEXA_GROUP_DEFAULT, NEXA_OFF, NEXA_RECIPIENT_2);
  delay(5000);
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
    return;
  }
  if (strlen(full_command) > NEXA_COMMAND_BIT_ARRAY_SIZE) {
    errorLog("sendNexaCommand(): Invalid command (too long), cannot continue.");
    return;
  }
  
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
  transmitHigh(NEXA_AGC1_PULSE);
  transmitLow(NEXA_AGC2_PULSE);

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
  transmitHigh(NEXA_PULSE_SHORT); // HIGH space to close the last wire bit
  transmitLow(NEXA_RADIO_SILENCE);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitNexaWireBit(int wire_bit) {
  transmitHigh(NEXA_PULSE_SHORT); // HIGH space

  if (wire_bit == 0) transmitLow(NEXA_PULSE_SHORT); // Wire bit 0
  if (wire_bit == 1) transmitLow(NEXA_PULSE_LONG); // Wire bit 1
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
