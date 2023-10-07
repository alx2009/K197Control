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
        GeminiFrame(uint8_t inputPin, uint8_t outputPin, unsigned long readDelayMicros, unsigned long writeDelayMicros,
                   unsigned long handshakeTimeoutMicros, unsigned long readEndMicros, unsigned long writeEndMicros)
        : GeminiProtocol(inputPin, outputPin, readDelayMicros, writeDelayMicros,
                   handshakeTimeoutMicros, readEndMicros, writeEndMicros) {
                   frameState=FrameState::WAIT_FRAME_START;
                    
    }
  
    bool begin(uint8_t *pdata, uint8_t nbytes) {
        if ( (nbytes == 0) || (pdata == NULL) ) {
            return false;
        }
        this->pdata=pdata;
        pdata_len=nbytes;
        resetFrame();
        GeminiProtocol::begin();
        frameState=FrameState::WAIT_FRAME_START;
        return true;
    }

    void sendFrame(uint8_t *pdata, uint8_t nbytes) {
        while(nbytes>0) {
            nbytes--;
            send(true);
            send(pdata[nbytes]);    
        }
    }
    
    void update() {
        GeminiProtocol::update();
        switch(frameState) {
            case FrameState::WAIT_FRAME_START:
                if (hasData() ) {
                    while(hasData() && frameEndDetected) receive();
                }
                if (!frameEndDetected) {
                    frameState = FrameState::WAIT_FRAME_DATA;
                    Serial.println(); Serial.print('[');
                }
                break;

            case FrameState::WAIT_FRAME_DATA:
                if (hasData()) {
                    handleFrameData();
                }
                if ( frameEndDetected ) {
                    frameState = FrameState::FRAME_END;    
                    Serial.print(']');
                }
                break;

            case FrameState::FRAME_END:
                while(hasData()) receive();
                if ( frameEndDetected) {
                    frameState = FrameState::WAIT_FRAME_START;
                    Serial.print('*'); Serial.println(); 
                }
                break;
        }
    }    

    void handleFrameData() {
        if (frameComplete()) {
            while(hasData()) {
                bool receivedBit = receive();
                Serial.print('-'); 
                Serial.print(receivedBit ? '1' : '0');
            }
            return;          
        }
        if (hasData()) { // fill the frame
            if (start_bit) {
                if (hasData(8)) {
                   pdata[byte_counter] = receiveByte(false);
                   Serial.print(' '); Serial.print(pdata[byte_counter], BIN);
                   byte_counter++;
                   if (frameComplete()) {
                       Serial.print('>');
                   }
                }
            } else {
                start_bit = receive();
                Serial.print(start_bit ? '1' : '0');
            }
        } else if (frameStarted()) {
            if ( checkFrameTimeout() ) {
                frameTimeoutCounter++;
                Serial.print('T');
                resetFrame();
            }
        }
    }

    bool frameComplete() const { return byte_counter >= pdata_len ? true : false; }
    
    void resetFrame() {
        byte_counter=0;
        start_bit=false;
        Serial.print('<');
    }

    uint8_t *getFrame() { resetFrame(); return pdata;  };
    uint8_t getFrameLenght() const { return pdata_len; };


    bool frameTimeoutDetected() const {return frameTimeoutCounter>0 ? true : false;};
    unsigned long getFrameTimeoutCounter() const {return frameTimeoutCounter;};
    void resetFrameTimeoutCounter() { frameTimeoutCounter = 0L;};
    
    private: 
        bool checkFrameTimeout() {return micros() - lastBitReadTime >= frameTimeout ? true : false;};
        bool frameStarted() const { return (byte_counter >0 || start_bit==true) ? true : false; };
    
        uint8_t *pdata=NULL;
        uint8_t pdata_len;
        uint8_t byte_counter=0;
        bool start_bit=false;        

        unsigned long frameTimeoutCounter=0;

        enum class FrameState {
            WAIT_FRAME_START=0,
            WAIT_FRAME_DATA=1,
            FRAME_END=2,
        } frameState;
};

#endif //K197CTRL_GEMINI_FRAME_H
