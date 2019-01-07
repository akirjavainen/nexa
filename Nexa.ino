/*
******************************************************************************************************************************************************************
*
* Nexa 433.92MHz appliances
* 
* Code by Antti Kirjavainen (antti.kirjavainen [_at_] gmail.com)
* 
* This code is command-compatible with the HomeEasy library, so you can capture your remote either with that or use an
* oscillator. You can capture your remotes and other Nexa devices with this code (convert decimal to binary as needed):
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
* 
* 
* PROTOCOL DESCRIPTION
* 
* A single command is: 2 AGC bits + 32 command bits + radio silence
*
* All sample counts below listed with a sample rate of 44100 Hz (sample count / 44100 = microseconds).
* 
* Pulse length:
* SHORT: approx. 11 samples = 249 us
* LONG: approx. 61 samples = 1383 us
* 
* Data bits:
* Data 0 = short LOW, short HIGH, short LOW, long HIGH (wire 01011)
* Data 1 = short LOW, long HIGH, short LOW, short HIGH (wire 01101)
*  
* End with LOW radio silence of (minimum) 400 samples = 9070 us
* 
* 
* HOW THIS WAS STARTED
* 
* Commands were captured by a "poor man's oscillator": 433.92MHz receiver unit (data pin) -> 10K Ohm resistor -> USB sound card line-in.
* Try that at your own risk. Power to the 433.92MHz receiver unit was provided by Arduino (connected to 5V and GND).
*
* To view the waveform Arduino is transmitting (and debugging timings etc.), I found it easiest to directly connect the digital pin (13)
* from Arduino -> 10K Ohm resistor -> USB sound card line-in. This way the waveform was very clear.
* 
******************************************************************************************************************************************************************
*/



#define NEXA_DEVICE_1         "00000000000000000000000000" // Copy paste your device ID here from HomeEasy

#define NEXA_GROUP_DEFAULT    0
#define NEXA_GROUP_1          1
#define NEXA_ON               1
#define NEXA_OFF              0
#define NEXA_RECIPIENT_0      "0000"
#define NEXA_RECIPIENT_1      "0001"
#define NEXA_RECIPIENT_2      "0010"
#define NEXA_RECIPIENT_3      "0011"


// Timings in microseconds (us). Get sample count by zooming all the way in to the waveform with Audacity.
// Calculate microseconds with: (samples / sample rate, usually 44100 or 48000) - ~15-20 to compensate for delayMicroseconds overhead.
// Sample counts listed below with a sample rate of 44100 Hz:
#define NEXA_AGC1_PULSE               240    // 11 samples
#define NEXA_AGC2_PULSE               2800   // 124 samples
#define NEXA_RADIO_SILENCE            9070   // 400 samples

#define NEXA_PULSE_SHORT              240    // 11 samples
#define NEXA_PULSE_LONG               1370   // 61 samples

#define NEXA_COMMAND_BIT_ARRAY_SIZE   32     // Command bit count


#define TRANSMIT_PIN                  13     // We'll use digital 13 for transmitting
#define REPEAT_COMMAND                8      // How many times to repeat the same command: original remotes and Cozify Hub repeat 5-7 times
#define DEBUG                         false  // Some extra info on serial


// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600); // Used for error messages even with DEBUG set to false    
  if (DEBUG) Serial.println("Starting up...");
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void loop() {
  sendNexaCommand(NEXA_DEVICE_1, NEXA_GROUP_1, NEXA_ON, NEXA_RECIPIENT_0);
  delay(5000);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void sendNexaCommand(String sender, int group, int on_off, String recipient) {
  // Prepare for transmitting and check for validity
  pinMode(TRANSMIT_PIN, OUTPUT); // Prepare the digital pin for output

  String command = sender + String(group) + String(on_off) + recipient;

  if (command.length() < NEXA_COMMAND_BIT_ARRAY_SIZE) {
    errorLog("sendNexaCommand(): Invalid command (too short), cannot continue.");
    return;
  }
  if (command.length() > NEXA_COMMAND_BIT_ARRAY_SIZE) {
    errorLog("sendNexaCommand(): Invalid command (too long), cannot continue.");
    return;
  }

  // Declare the array (int) of command bits
  int command_array[NEXA_COMMAND_BIT_ARRAY_SIZE];

  // Processing a string during transmit is just too slow,
  // let's convert it to an array of int first:
  convertStringToArrayOfInt(command, command_array, NEXA_COMMAND_BIT_ARRAY_SIZE);
  
  // Repeat the command:
  for (int i = 0; i < REPEAT_COMMAND; i++) {
    doNexaSend(command_array);
  }

  // Disable output to transmitter to prevent interference with
  // other devices. Otherwise the transmitter will keep on transmitting,
  // which will disrupt most appliances operating on the 433.92MHz band:
  digitalWrite(TRANSMIT_PIN, LOW);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void doNexaSend(int *command_array) {
  if (command_array == NULL) {
    errorLog("doNexaSend(): Array pointer was NULL, cannot continue.");
    return;
  }

  // Starting (AGC) bits:
  transmitWaveformLow(NEXA_AGC1_PULSE);
  transmitWaveformHigh(NEXA_AGC2_PULSE);

  // Transmit command:
  for (int i = 0; i < NEXA_COMMAND_BIT_ARRAY_SIZE; i++) {

    // If current command bit is 0, transmit wire 01011:
    if (command_array[i] == 0) {
      transmitWaveformLow(NEXA_PULSE_SHORT);
      transmitWaveformHigh(NEXA_PULSE_SHORT);
      transmitWaveformLow(NEXA_PULSE_SHORT);
      transmitWaveformHigh(NEXA_PULSE_LONG);
    }

    // If current command bit is 1, transmit wire 01101:
    if (command_array[i] == 1) {
      transmitWaveformLow(NEXA_PULSE_SHORT);
      transmitWaveformHigh(NEXA_PULSE_LONG);
      transmitWaveformLow(NEXA_PULSE_SHORT);
      transmitWaveformHigh(NEXA_PULSE_SHORT);
    }

   }

  // Radio silence at the end.
  // It's better to rather go a bit over than under required length.
  transmitWaveformLow(NEXA_PULSE_SHORT);
  transmitWaveformHigh(NEXA_RADIO_SILENCE);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitWaveformHigh(int delay_microseconds) {
  digitalWrite(TRANSMIT_PIN, LOW); // Digital pin low transmits a high waveform
  //PORTB = PORTB D13low; // If you wish to use faster PORTB commands instead
  delayMicroseconds(delay_microseconds);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitWaveformLow(int delay_microseconds) {
  digitalWrite(TRANSMIT_PIN, HIGH); // Digital pin high transmits a low waveform
  //PORTB = PORTB D13high; // If you wish to use faster PORTB commands instead
  delayMicroseconds(delay_microseconds);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
int convertStringToInt(String s) {
  char carray[2];
  int i = 0;
  
  s.toCharArray(carray, sizeof(carray));
  i = atoi(carray);

  return i;
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void convertStringToArrayOfInt(String command, int *int_array, int command_array_size) {
  String c = "";

  if (int_array == NULL) {
    errorLog("convertStringToArrayOfInt(): Array pointer was NULL, cannot continue.");
    return;
  }
 
  for (int i = 0; i < command_array_size; i++) {
      c = command.substring(i, i + 1);

      if (c == "0" || c == "1") {
        int_array[i] = convertStringToInt(c);
      } else {
        errorLog("convertStringToArrayOfInt(): Invalid character " + c + " in command.");
        return;
      }
  }
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void errorLog(String message) {
  Serial.println(message);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
