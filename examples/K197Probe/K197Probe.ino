/**************************************************************************/
/*!
  @file     K197DataAcquisition.ino

  Arduino K197Control library example sketch

  Copyright (C) 2023 by ALX2009

  License: MIT (see LICENSE)

  This file is part of the Arduino K197control library, please see
  https://github.com/alx2009/K197Control for more information

  This is an example application to probe the interface at low level.
  We use the class GeminiFrame to interface with the K197.
  
  The application is controlled by Serial and allows to send one of 5 pre-configured control frames to the K197.
  The raw data from the K197 are printed back in binary form (as a sequence of ones and zeros as they are received)
  A frame boundary is detected when no data is received for a frame timeout period.

  TODO: make sure if it is ok to run GeminiFrame without an inputBuffer set...

*/
#include <boolFifo.h>
#include <gemini.h>
#include <geminiFrame.h>

#define INPUT_PIN 2      
#define OUTPUT_PIN 3    

////////////////////////////////////////////////////////////////////////////////////
// Predefined command sequences
////////////////////////////////////////////////////////////////////////////////////
uint8_t cmd_0[] = {0x00, 0x00, 0x00, 0x00, 0x00};  // empty command
uint8_t cmd_1[] = {0x00, 0x02, 0x80, 0x00, 0x00};  // B0 command
uint8_t cmd_2[] = {0x00, 0x00, 0xa0, 0x00, 0x00};  // B1 command
uint8_t cmd_3[] = {0x00, 0x00, 0x80, 0x00, 0x00};  // unknown command to try no. 1
uint8_t cmd_4[] = {0x00, 0x40, 0x00, 0x00, 0x00};  // unknown command to try no. 2
uint8_t cmd_5[] = {0x00, 0x50, 0x00, 0x00, 0x00};  // unknown command to try no. 3
uint8_t *cmd[] = {cmd_0, cmd_1, cmd_2, cmd_3, cmd_4, cmd_5};

const size_t cmd_size = sizeof(cmd_0) / sizeof(cmd_0[0]);


GeminiFrame gemini(INPUT_PIN, OUTPUT_PIN, 15, 10, 80, 170, 90);  // in, out, read pulse, write pulse, (not used), read delay, write delay
bool logOnce=false;
bool logAlways=true;

////////////////////////////////////////////////////////////////////////////////////
// Management of the serial user interface
////////////////////////////////////////////////////////////////////////////////////

/*!
      @brief print the prompt to Serial
*/
void printPrompt() { 
  Serial.println();
  Serial.println(F("> "));
}

/*!
      @brief print help text with supported commands to Serial
*/
void printHelp() { // ++ commands similar to Prologix GPIB-USB controller, device dependent command similar to K197 GpIB interface card
  Serial.println();
  Serial.println(F("   ?    > help"));
  Serial.println(F("   R > log next measurement (once)"));
  Serial.println(F("   L > toggle data logging"));
  Serial.println(F("   1 > send cmd no. 0")); 
  Serial.println(F("   1 > send cmd no. 1")); 
  Serial.println(F("   2 > send cmd no. 2"));
  Serial.println(F("   3 > send cmd no. 3"));
  Serial.println(F("   4 > send cmd no. 4"));
  Serial.println(F("   4 > send cmd no. 5"));
  printPrompt();
}

#define INPUT_BUFFER_SIZE                                                      \
  15 ///< size of the input buffer when reading from Serial

/*!
      @brief error message for invalid command to Serial
      @param buf null terminated char array with the invalid command
*/
void printError(const char *buf) { 
  Serial.print(F("Invalid: "));
  Serial.println(buf);
  printHelp();
}

void executeCommand(uint8_t cmd_index) {
    gemini.sendFrame(cmd[cmd_index], cmd_size);
    Serial.print(F("CMD no. ")); Serial.print(cmd_index); Serial.println(F(" queued")); 
}

/*!
      @brief check if a comand causes data to be sent via gemini object
*/
bool isTXCommand(char c) {
   return (c >= '0') && (c <= '5') ? true : false;
}

/*!
      @brief handle all Serial commands
      @details Reads from Serial and execute any command. Should be invoked when
   there is input availble from Serial. Note: For this application commands are single characters
   Only one command (character) is processed for each function call 
*/
void handleSerial() { 
  char c = Serial.read();
  if (isTXCommand(c)) executeCommand(c - '0');
  else switch (c) {
      case '?': printHelp(); break;
      case 'R': case 'r': logOnce=true; break;
      case 'L': case 'l': logAlways=!logAlways; break;
      case '\n': case '\r': case ' ': /* ignore */; break;
      default: Serial.print(F("Invalid: ")); Serial.println(c); printHelp(); break;
  };
}

////////////////////////////////////////////////////////////////////////////////////
// Arduino setup() & loop()
////////////////////////////////////////////////////////////////////////////////////

/*!
      @brief Arduino setup function
*/
void setup() {
    Serial.begin(115200); //Need a high baud rate or blocks for too long when printing to Serial 
    delay(1000);
    Serial.println(F("K197DataAcquisition"));
    if (!gemini.begin()) {
        Serial.println(F("begin failed!"));
        while(true);
    }
    // uncomment next line to experiment with different frame timeout
    //setFrameTimeout(30000L);
    
    gemini.setInitiatorMode(false);
}

bool startBitDetected=false;  ///< true if a start bit has been identified
unsigned int partial_bit_counter  = 0; ///< counts the number of bit in a sub-frame
unsigned int total_bit_counter  = 0;   ///< counts the total number of bits in a frame

/*!
      @brief Arduino loop function
*/
void loop() {
    gemini.update();
    if ( gemini.frameComplete() ) { // TODO: check if this is needed since we do not set a inputFrmaBuffer
        gemini.resetFrame(); // Must call resetFrame() or getFrame as soon as possible after frameComplete returns true...
    }
    while (gemini.hasData()) {
        if (total_bit_counter == 0) {
            if (logOnce || logAlways) Serial.print('<'); // Indicates the start of a frame
        }
        total_bit_counter++;
        partial_bit_counter++;
        bool bit = gemini.receive();
        if (startBitDetected) { // inside a subframe
            if (logOnce || logAlways) Serial.print(bit ? '1' : '0');
            if (partial_bit_counter>=8) { //last bit of a subframe
                if (logOnce || logAlways) Serial.print(' ');
                startBitDetected = false;
                partial_bit_counter=0;
            }
        } else if (bit) { // start of a subframe
            startBitDetected = true;
            if (logOnce || logAlways) Serial.print(F(" 1 "));
            partial_bit_counter=0;
        } else { // inside a synch sequence
            if (logOnce || logAlways) Serial.print('0');
            if (partial_bit_counter>=8) { //group synch bits in groups of 8
                if (logOnce || logAlways) Serial.print(' ');
                partial_bit_counter=0;
            }
        }        
    }
    if ( (total_bit_counter>0) && (gemini.isFrameEndDetected()) ) {
        if (logOnce || logAlways) Serial.println('>');
        logOnce = false;
        startBitDetected=false;  ///< true if a start bit has been identified
        partial_bit_counter  = 0; ///< counts the number of bit in a sub-frame
        total_bit_counter  = 0;   ///< counts the total number of bits in a frame
    }
    if (gemini.frameTimeoutDetected()) {
        Serial.print(gemini.getFrameTimeoutCounter());  Serial.println(F(" frame timeouts!"));  
        gemini.resetFrameTimeoutCounter();
    }
    while (Serial.available()) {
      char c = Serial.peek();
        if (isTXCommand(c)) {
            // We need to avoid back-to-back commands in the same frame
            if (gemini.isFrameEndDetected() && gemini.noOutputPending()) {
                handleSerial();
            }
            break;
        } else {
            handleSerial();
        }
    }
}
