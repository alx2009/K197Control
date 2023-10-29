#include <boolFifo.h>
#include <gemini.h>
#include <geminiFrame.h>
#include <geminiK197Control.h>

/**************************************************************************/
/*!
  @file     src.ino

  Arduino K197Control library sketch

  Copyright (C) 2023 by ALX2009

  License: MIT (see LICENSE)

  This file is part of the Arduino K197control library, please see
  https://github.com/alx2009/K197Control for more information

  This is an example data logger, sending any measurement result from the K197 to Serial.
  We use an alternative method of using the K197Control library compared to other examples: 
  Instead of using the class K197control, we interface using a GeminiFrame object, then we cast 
  the received frame to GeminiK197Control::K197measurement

  Note that in this way the methods defined in the class K197control  are not available, however since most of them 
  are used to control the voltmeter they are not needed here, resulting in some RAM and flash size reduction.
*/

#define INPUT_PIN 2      
#define OUTPUT_PIN 3    

GeminiFrame gemini(INPUT_PIN, OUTPUT_PIN, 15, 10, 80, 170, 90);  // in, out, read pulse, write pulse, (not used), read delay, write delay
uint8_t inputFrame[4];

void setup() {
    //if all alternative print statements are enabled, a higher baud rate may be needed
    //Serial.begin(500000); 
    Serial.begin(115200); 
#ifdef DEBUG_PORT // Make sure the following pins match the definition inside gemini.h
    pinMode(A0, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(A2, OUTPUT);
    pinMode(A3, OUTPUT);
#endif //DEBUG_PORT
    delay(1000);
    Serial.println(F("K197ControlDataLogger"));
    if (!gemini.begin(inputFrame, sizeof(inputFrame)/sizeof(inputFrame[0]))) {
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

unsigned long lastTimeMicros=0L;
bool dataPrinted=false;

void loop() {
    gemini.update();
    if ( gemini.frameComplete() ) {
        uint8_t *pframe = gemini.getFrame();

        // Uncomment to print the received frame buffer
        //uint8_t frame_len=gemini.getFrameLenght();
        //for (int i=0; i<frame_len; i++) {
        //   Serial.print(pframe[i], BIN); Serial.print(' ');   
        //}

        GeminiK197Control::K197measurement *pmeasurement = (GeminiK197Control::K197measurement*) pframe;

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

        //For enhanced precision replace the above statemept with the following:
        //Serial.println(pmeasurement->getResultAsStringEP(buffer));

    }
    if (gemini.frameTimeoutDetected()) {
        Serial.print(gemini.getFrameTimeoutCounter());  Serial.println(F(" frame timeouts!"));  
        gemini.resetFrameTimeoutCounter();
    }

}
