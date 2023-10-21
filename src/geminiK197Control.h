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

    /**************************************************************************/
    /*!
        @brief  Define the measurement unit 
    */
    /**************************************************************************/
    enum K197unit {
        Volt     = 0b00,
        Amp      = 0b10,
        Ohm      = 0b01,
        reserved = 0b11,
    };

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
                K197unit unit     : 2;   ///< measurement unit
            };
        } __attribute__((packed)); ///<
    }; ///< Structure designed to pack a number of flags into one byte

    struct K197mr_byte1 {
        /*!
           @brief measurement result frame byte 1
        */
        union {
            uint8_t byte1; ///< allows access to all the flags in the
                           ///< union as one byte as uint8_t
            struct {
                uint8_t msb       : 5;   ///< measurement Most Significant Bits
                bool    ovrange   : 1;   ///<  when true indicates overrange 
                bool    undefined : 1;   ///< unknown use, normally set to 0
                bool negative     : 1;   ///< measurement unit
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
          K197mr_byte1 byte1;   
          //uint16_t lsb;
          struct {
              uint8_t hi;
              uint8_t lo;
          } lsb;
      };

      int32_t value;    ///< allows access to all bytes as a 32 bit signed integer value
      uint32_t uvalue;  ///< allows access to all bytes as a 32 bit unsigned integer value
    
    } __attribute__((packed)); ///<
  }; ///< Structure designed to pack a number of flags into one byte

};

#endif //K197CTRL_GEMINI_K197_CONTROL_H
