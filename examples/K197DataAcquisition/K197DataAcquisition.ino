/**************************************************************************/
/*!
  @file     K197DataAcquisition.ino

  Arduino K197Control library example sketch

  Copyright (C) 2023 by ALX2009

  License: MIT (see LICENSE)

  This file is part of the Arduino K197control library, please see
  https://github.com/alx2009/K197Control for more information

*/
#include <boolFifo.h>
#include <gemini.h>
#include <geminiFrame.h>
#include <geminiK197Control.h>

#define INPUT_PIN 2      
#define OUTPUT_PIN 3    

GeminiK197Control gemini(INPUT_PIN, OUTPUT_PIN, 15, 10, 80, 170, 90);  // in, out, read pulse, write pulse, (not used), read delay, write delay
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
  Serial.println(F("   ++?    > help"));
  Serial.println(F("   ++read > log next measurement (once)"));
  Serial.println(F("   ++log  > toggle data logging"));
  Serial.println(F("   ++trg  > trigger")); 
  Serial.println(F("   ++loc  > local mode"));
  Serial.println(F("   ++llc  > remote mode"));
  Serial.println(F("   or device dependent command(s):"));
  Serial.println(F("   D0/1 > DB off/on, R0-6 > set range 0-6, Z0/1 > set relative off/on, T0-5 Set trigger mode 0/5"));
  Serial.println(F("   B0/1 > send display/stored reading, X > execute command (ONLY allowed as last character)"));
  
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

bool checkInput(char min, char max, char c) { return (c>=min) && (c<=max) ? true : false; }
uint8_t getInputValue(char c) { return c - '0'; }
void rangeError(char command, char parameter) {
    Serial.print(command); Serial.print(parameter); Serial.println(F(": out of range"));          
}

#define PREVENT_RANGE_ERROR(command, min, max, c) {\
    Serial.print('<');Serial.print(c);Serial.print('>');                \
    if(!checkInput(min, max, c)) {  \
        rangeError(command, c);     \
        return;                     \
    }                               \
}

void setDecibel(char c) { PREVENT_RANGE_ERROR('D', '0', '1', c); Serial.print(F("Db=")); Serial.println(getInputValue(c)); 
       gemini.getControlBuffer()->setDbMode(getInputValue(c));}
void setRange(char c) {PREVENT_RANGE_ERROR('R', '0', '5', c); Serial.print(F("Range=")); Serial.println(getInputValue(c));
       gemini.getControlBuffer()->setRange((GeminiK197Control::K197range) getInputValue(c));}
void setRelative(char c) {PREVENT_RANGE_ERROR('Z', '0', '1', c); Serial.print(F("Relative=")); Serial.println(getInputValue(c)); 
       gemini.getControlBuffer()->setRelative(getInputValue(c)); }
void setTriggerMode(char c) {PREVENT_RANGE_ERROR('T', '0', '5', c); Serial.print(F("Trigger=")); Serial.println(getInputValue(c)); 
       gemini.getControlBuffer()->setTriggerMode((GeminiK197Control::K197triggerMode) getInputValue(c)); }
void setReadings(char c) {PREVENT_RANGE_ERROR('B', '0', '1', c); Serial.print(F("Readings=")); Serial.println(getInputValue(c)); 
       gemini.getControlBuffer()->setSendStoredReadings(getInputValue(c)); }
void executeCommand() {Serial.println(F("execute")); gemini.sendImmediately(); }

/*!
      @brief handle all Serial commands
      @details Reads from Serial and execute any command. Should be invoked when
   there is input availble from Serial. Note: It will block for the default
   Serial timeout if a command is not complete, affecting the display. Terminals
   sending complete lines are therefore preferred (e.g. the Serial Monitor in
   the Arduibno GUI)
*/
void handleSerial() { // Here we want to use Serial, rather than DebugOut
  char buf[INPUT_BUFFER_SIZE + 1];
  size_t buflen = Serial.readBytesUntil('\n', buf, INPUT_BUFFER_SIZE);
  buf[buflen] = 0;
  if (buflen == 0) { // no characters read
    Serial.println(F("buf empty!"));
    return;
  }
  for (uint8_t i=0; i<buflen; i++) {
    if (buf[i] == '\r' || buf[i] == '\n') buf[i] = 0;
  }
  Serial.print(F("buf=")); Serial.println(buf);
  if (strcasecmp_P(buf, PSTR("++?")) == 0) {
    printHelp();
    return;
  }
  if ((strcasecmp_P(buf, PSTR("++read")) == 0)) {
      Serial.println(F("++read"));
      logOnce=true;
  } else if ((strcasecmp_P(buf, PSTR("++log")) == 0)) {
      Serial.println(F("++log"));    
      logAlways=!logAlways;
  } else if ((strcasecmp_P(buf, PSTR("++trg")) == 0)) {
      Serial.println(F("++trg"));
      gemini.getControlBuffer()->setTriggerMode(GeminiK197Control::K197triggerMode::T_TALK);
      executeCommand();
  } else if ((strcasecmp_P(buf, PSTR("++loc")) == 0)) {
      Serial.println(F("++loc"));
      gemini.getControlBuffer()->setLocalMode();
      executeCommand();
  } else if ((strcasecmp_P(buf, PSTR("++llc")) == 0)) {
      Serial.println(F("++llc"));
      gemini.getControlBuffer()->setRemoteMode();
      executeCommand();
  } else if (buf[0]=='+') {
      Serial.print(F("Invalid: ")); Serial.println(buf);
  } else { // consider as a device dependent command string
      char *pbuf = buf;
      while(*pbuf !=0) {
          Serial.print('[');Serial.print(*pbuf);Serial.print(']');
          char cmd = *pbuf;
          if ( cmd == ' ') {
              // ignore    
          } else if (cmd == 'D' || cmd == 'd') {
              setDecibel(*(++pbuf));
          } else if (cmd == 'R' || cmd == 'r') {
              setRange(*(++pbuf));
          } else if (cmd == 'Z' || cmd == 'z') {
              setRelative(*(++pbuf));
          } else if (cmd == 'T' || cmd == 't') {
              setTriggerMode(*(++pbuf));
          } else if (cmd == 'B' || cmd == 'b') {
              setReadings(*(++pbuf));
          } else if (cmd == 'X' || cmd == 'x') {
              if (pbuf[1] == 0) executeCommand();
              else Serial.println(F("Intermediate X ignored"));
          } else {
              Serial.print(F("Invalid command: ")); Serial.println(*pbuf);
          }
          pbuf++;
      }
  }
}

////////////////////////////////////////////////////////////////////////////////////
// Arduino setup() & loop()
////////////////////////////////////////////////////////////////////////////////////

/*!
      @brief Arduino setup function
*/
void setup() {
    Serial.begin(115200); //Need a high baud rate or blocks for too long when printing to Serial 
#ifdef DEBUG_PORT // Make sure the following pins match the definition inside gemini.h
    pinMode(A0, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(A2, OUTPUT);
    pinMode(A3, OUTPUT);
#endif //DEBUG_PORT
    delay(1000);
    Serial.println(F("K197DataAcquisition"));
    if (!gemini.begin()) {
        Serial.println(F("begin failed!"));
        while(true);
    }
    if ( digitalPinToInterrupt(INPUT_PIN) == NOT_AN_INTERRUPT ) {
        Serial.print(F("Error: Pin ")); Serial.print(INPUT_PIN); Serial.println(F(" does not support interrupts!"));
        Serial.println(F("Execution stopped"));
        while(1);
    }

    // the following line is normally not needed 
    //gemini.serverStartup(2000000);
    gemini.setInitiatorMode(false);
}

/*!
      @brief Arduino loop function
*/
void loop() {
    if (Serial.available()) {
      handleSerial();
    }
    gemini.update();
    if ( gemini.frameComplete() ) {
        gemini.resetFrame(); // Must call resetFrame() or getFrame as soon as possible after frameComplete returns true...
        if (logOnce || logAlways) {
            logOnce = false;
            GeminiK197Control::K197measurement *pmeasurement = gemini.getMeasurementBuffer();

            // Uncomment one or more of the following statements for alternative ways to print the data or for troubleshooting 
            //Serial.print(F(" B0=")); Serial.print(pmeasurement->byte0.byte0, BIN);
            //Serial.print(F(": unit=")); Serial.print(pmeasurement->byte0.unit);
            //Serial.print(F(": AC/DC=")); Serial.print(pmeasurement->byte0.ac_dc);
            //Serial.print(F(", undef0=")); Serial.print(pmeasurement->byte0.undefined);
            //Serial.print(F(", rel=")); Serial.print(pmeasurement->byte0.relative);
            //Serial.print(F(", range=")); Serial.print(pmeasurement->byte0.range);
            //Serial.print(F(", OV=")); Serial.print(pmeasurement->byte1.ovrange);
            //Serial.print(F(", undef1=")); Serial.print(pmeasurement->byte1.undefined);
            //Serial.print(F(", neg=")); Serial.print(pmeasurement->byte1.negative); */
            //Serial.print(F(" - getCount()=")); Serial.println(pmeasurement->getCount());
            //Serial.print(F("getAbsValue()=")); Serial.print(pmeasurement->getAbsValue());
            //Serial.print(F(", getValue()=")); Serial.print(pmeasurement->getValue());
            //Serial.print(F(", getValueAsDouble()=")); Serial.print(pmeasurement->getValueAsDouble(), 9);

            char buffer[GeminiK197Control::K197measurement::resultAsStringMinSizeEP];
            Serial.println(pmeasurement->getResultAsString(buffer));

            //For enhanced resolution replace the above statement with the following:
            //Serial.println(pmeasurement->getResultAsStringER(buffer));
        }

    }
    if (gemini.frameTimeoutDetected()) {
        Serial.print(gemini.getFrameTimeoutCounter());  Serial.println(F(" frame timeouts!"));  
        gemini.resetFrameTimeoutCounter();
    }

}
