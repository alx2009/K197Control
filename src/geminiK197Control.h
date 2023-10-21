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


    struct K197mr_byte0 {
        /*!
           @brief measurement result frame byte 0
        */
        union {
            uint8_t byte0; ///< allows access to all the flags in the
                       ///< union as one byte as uint8_t
            struct {
                uint8_t range     : 3;   ///< measurement range
                bool    relative  : 1;   ///< relative measurement when true
                bool    undefined : 1;   ///< unknown use, normally set to 1
                bool    ac_dc     : 1;   ///< true if AC
                uint8_t unit      : 2;   ///< measurement unit
            };
        } __attribute__((packed)); ///<
    }; ///< Structure designed to pack a number of flags into one byte


struct K197measurement {
    /*!
       @brief control frame byte 0
    */
    union {
      uint8_t frameBuffer[1]; ///< allows acccess to all the flags in the
                              ///< union as one unsigned char
      struct {
          K197mr_byte0 byte0;     
      };
    } __attribute__((packed)); ///<
  }; ///< Structure designed to pack a number of flags into one byte

};

#endif //K197CTRL_GEMINI_K197_CONTROL_H
