/**************************************************************************/
/*!
  @file     src.ino

  Arduino K197Control library sketch

  Copyright (C) 2023 by ALX2009

  License: MIT (see LICENSE)

  This file is part of the Arduino K197control library, please see
  https://github.com/alx2009/K197Control for more information

  This is an example data logger, sending any measurement result from the K197
  to Serial. We use an alternative method of using the K197Control library
  compared to other examples: Instead of using the class K197control, we
  interface using a GeminiFrame object, then we cast the received frame to
  GeminiK197Control::K197measurement

  Note that in this way the methods defined in the class K197control  are not
  available, however since most of them are used to control the voltmeter they
  are not needed here, resulting in some RAM and flash size reduction.
*/
#include <boolFifo.h>
#include <gemini.h>
#include <geminiFrame.h>
#include <geminiK197Control.h>

#define INPUT_PIN 2
#define OUTPUT_PIN 3

GeminiFrame 
    gemini(INPUT_PIN, OUTPUT_PIN, 10, 80, 170,
           90); ///< handle the interface to the K197 using the Gemini Protocol
                // in, out, write pulse, (not used), read delay, write delay

uint8_t inputFrame[4]; ///< buffer for a complete K197 measurement frame (4 bytes) 

/*!
      @brief Arduino setup function
      @details setup the Serial port and the gemini object
*/
void setup() {
  // if all alternative print statements are enabled, 
  // a higher baud rate may be needed 
  // Serial.begin(500000);
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("K197ControlDataLogger"));
  if (!gemini.begin(inputFrame, sizeof(inputFrame) / sizeof(inputFrame[0]))) {
    Serial.println(F("begin failed!"));
    while (true)
      ;
  }

  // the following line is normally not needed
  // gemini.serverStartup(2000000);
  
  // the following is only required if we need to control the K197, but doesn't hurt to have it
  gemini.setInitiatorMode(false);  
}

/*!
      @brief Arduino setup function
      @details call update() to handle the gemini framing protocol and communicate with the K197. 
      When a new frame has been received, cast it to a GeminiK197Control::K197measurementand in order to print the measurement result
*/
void loop() {
  gemini.update();  // call update() as frequently as possible
  if (gemini.frameComplete()) { // if true we have got a new frame
    uint8_t *pframe = gemini.getFrame(); // get a pointer to the frame buffer and allow a new frame to be received

    // We now have the data in pframe. No data will be over-written until we call update() again
    // How much time do we have to process the data?
    // The gemini protocol is fairly resilient to delays, however it has a limit
    // if the processing takes too much time before calling update(), the k197 may timout
    // and may skip a measurement as a result  

    // Uncomment to print the received frame buffer
    // uint8_t frame_len=gemini.getFrameLenght();
    // for (int i=0; i<frame_len; i++) {
    //   Serial.print(pframe[i], BIN); Serial.print(' ');
    //}

    // we now cast the generic frame pointer to a K197measurement object,
    // allowing easy access to the information
    GeminiK197Control::K197measurement *pmeasurement =
        (GeminiK197Control::K197measurement *)pframe;

    // Uncomment one or more of the following statements for alternative ways to
    // print the data or for troubleshooting
    // Serial.print(F(" B0=")); Serial.print(pmeasurement->byte0.byte0, BIN);
    // Serial.print(F(": unit=")); Serial.print(pmeasurement->byte0.unit);
    // Serial.print(F(": AC/DC=")); Serial.print(pmeasurement->byte0.ac_dc);
    // Serial.print(F(", undef0=")); Serial.print(pmeasurement->byte0.undefined);
    // Serial.print(F(", rel=")); Serial.print(pmeasurement->byte0.relative); 
    // Serial.print(F(", range=")); Serial.print(pmeasurement->byte0.range); 
    // Serial.print(F(", OV=")); Serial.print(pmeasurement->byte1.ovrange); 
    // Serial.print(F(", undef1=")); Serial.print(pmeasurement->byte1.undefined); 
    // Serial.print(F(", neg=")); Serial.print(pmeasurement->byte1.negative); 
    // Serial.print(F(" - getCount()=")); Serial.println(pmeasurement->getCount());
    // Serial.print(F("getAbsValue()=")); Serial.print(pmeasurement->getAbsValue());
    // Serial.print(F(", getValue()=")); Serial.print(pmeasurement->getValue()); 
    // Serial.print(F(", getValueAsDouble()=")); Serial.print(pmeasurement->getValueAsDouble(), 9);

    char buffer[GeminiK197Control::K197measurement::resultAsStringMinSize];
    Serial.println(pmeasurement->getResultAsString(buffer));

    // For enhanced resolution replace the above statements with the following:
    // char buffer[GeminiK197Control::K197measurement::resultAsStringMinSizeER];
    // Serial.println(pmeasurement->getResultAsStringER(buffer));
  }

  // This is how we can check for timeouts. 
  if (gemini.frameTimeoutDetected()) {
    Serial.print(gemini.getFrameTimeoutCounter());
    Serial.println(F(" frame timeouts!"));
    gemini.resetFrameTimeoutCounter();
  }
}
