/**************************************************************************/
/*!
  @file     geminiFrame.h

  Arduino K197Control library

  Copyright (C) 2023 by ALX2009

  License: MIT (see LICENSE)

  This file is part of the Arduino K197Display sketch, please see
  https://github.com/alx2009/K197Control for more information

  This file defines the GeminiFrame class
  The Gemini class implements the gemini protocol, sending and receiving frames of data between two peers

*/
/**************************************************************************/
#ifndef K197CTRL_GEMINI_FRAME_H
#define K197CTRL_GEMINI_FRAME_H
#include "gemini.h"
    
class GeminiFrame : public GeminiProtocol {   
public:

        GeminiProtocol(uint8_t inputPin, uint8_t outputPin, unsigned long readDelayMicros, unsigned long writeDelayMicros,
                   unsigned long handshakeTimeoutMicros, unsigned long readEndMicros, unsigned long writeEndMicros)
        : GeminiProtocol(inputPin, outputPin, readDelayMicros, writeDelayMicros,
                   handshakeTimeoutMicros, readEndMicros, writeEndMicros) {
                    
    }
  
    bool sendFrame(uint8_t *pdata, uint8_t nbytes) {
        while(nbytes>0) {
            nbytes--;
            send(true);
            send(pdata[nbytes]);    
        }
    }

    bool begin(uint8_t *pdata, uint8_t_t nbytes) {
        this->data=pdata;
        pdata_len=nbytes;
    }

    void update() {
        GeminiProtocol::update();
        // fill the frame
    }

    private: 
        uint8_t *pdata=NULL;
        uint8_t pdata_len;
        uint8_t bit_counter=0;
        uint8_t byte_counter=0;
        bool startBit=false;
        
}
