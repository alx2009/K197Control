/**************************************************************************/
/*!
  @file     geminiK197Control.h

  Arduino K197Control library

  Copyright (C) 2023 by ALX2009

  License: MIT (see LICENSE)

  This file is part of the Arduino K197Display sketch, please see
  https://github.com/alx2009/K197Control for more information

  This file defines the GeminiK197Control class
  The GeminiK197Control class implements the application layer of the K197 control protocol. 
  Control frames are sent to/measurement results received from the K197 using the gemini frame layer (via base class geminiFrame)

*/
/**************************************************************************/
#ifndef K197CTRL_GEMINI_K197_CONTROL_H
#define K197CTRL_GEMINI_K197_CONTROL_H
#include "geminiFrame.h"

class GeminiK197Control : public GeminiFrame {   
public:
        GeminiK197Control(uint8_t inputPin, uint8_t outputPin, unsigned long readDelayMicros, unsigned long writeDelayMicros,
                   unsigned long handshakeTimeoutMicros, unsigned long readEndMicros, unsigned long writeEndMicros)
        : GeminiFrame(inputPin, outputPin, readDelayMicros, writeDelayMicros,
                   handshakeTimeoutMicros, readEndMicros, writeEndMicros) {
                    
    }


};

#endif //K197CTRL_GEMINI_K197_CONTROL_H
