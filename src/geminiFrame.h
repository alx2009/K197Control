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

#ifdef DEBUG_PORT
#    define DEBUG_FRAME_STATE() DEBUG_PORT = (DEBUG_PORT & 0xe7) | ( ((uint8_t)frameState<<3) & 0x18)
#else
#    define DEBUG_FRAME_STATE()
#endif //DEBUG_PORT

//#define DEBUG_GEMINI_FRAME  // comment/uncomment to activate debug prints
#ifdef DEBUG_GEMINI_FRAME
# define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
# define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
# define DEBUG_PRINT(...) 
# define DEBUG_PRINTLN(...) 
#endif //DEBUG_GEMINI_FRAME
    
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
        if (!GeminiProtocol::begin()) return false;
        setInputBuffer(pdata, nbytes);
        frameState=FrameState::WAIT_FRAME_START;
        DEBUG_FRAME_STATE();
        return true;
    }

    void sendFrame(uint8_t *pdata, uint8_t nbytes) {
        for(uint8_t i=0; i<nbytes; i++) {
            send(true);
            send(pdata[i]);    
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
                    resetFrame();
                    frameState = FrameState::WAIT_FRAME_DATA;
                    DEBUG_FRAME_STATE();
                    DEBUG_PRINTLN(); DEBUG_PRINT('[');
                }
                break;

            case FrameState::WAIT_FRAME_DATA:
                if (hasData()) {
                    handleFrameData();
                }
                if ( frameEndDetected ) {
                    frameState = FrameState::FRAME_END;    
                    DEBUG_FRAME_STATE();
                    DEBUG_PRINT(']');
                }
                break;

            case FrameState::FRAME_END:
                while(hasData()) receive();
                if ( frameEndDetected) {
                    frameState = FrameState::WAIT_FRAME_START;
                    DEBUG_FRAME_STATE();
                    DEBUG_PRINT('*'); DEBUG_PRINTLN(); 
                }
                break;
        }
    }    

  private:
    void handleFrameData() {
        if (frameComplete()) {
            while(hasData()) {
#               ifdef DEBUG_GEMINI_FRAME
                    bool receivedBit = receive();
                    DEBUG_PRINT('-'); 
                    DEBUG_PRINT(receivedBit ? '1' : '0');
#               else 
                    receive();
#               endif //DEBUG_GEMINI_FRAME
            }
            return;          
        }
        if (hasData()) { // fill the frame
            if (start_bit) {
                if (hasData(8)) {
                   pdata[byte_counter] = receiveByte(false);
                   DEBUG_PRINT(' '); DEBUG_PRINT(pdata[byte_counter], BIN);
                   byte_counter++;
                   start_bit = false;
                   if (frameComplete()) {
                       DEBUG_PRINT('>');
                   }
                }
            } else {
                start_bit = receive();
                if (start_bit) {
                    DEBUG_PRINT(F(" 1<"));
                } else {
                    DEBUG_PRINT(0);
                }
            }
        } else if (frameStarted()) {
            if ( checkFrameTimeout() ) {
                frameTimeoutCounter++;
                DEBUG_PRINT('T');
                resetFrame();
            }
        }
    }

  public:

    bool frameComplete() const { return byte_counter >= pdata_len ? true : false; }
    
    void resetFrame() {
        byte_counter=0;
        start_bit=false;
        DEBUG_PRINT('#');
    }

    uint8_t *getFrame() { DEBUG_PRINT('@'); resetFrame(); return pdata;  };
    uint8_t getFrameLenght() const { return pdata_len; };


    bool frameTimeoutDetected() const {return frameTimeoutCounter>0 ? true : false;};
    unsigned long getFrameTimeoutCounter() const {return frameTimeoutCounter;};
    void resetFrameTimeoutCounter() { frameTimeoutCounter = 0L;};

    protected:
        void setInputBuffer(uint8_t *pdata, uint8_t nbytes, bool doResetFrame=false) {
            this->pdata=pdata;
            pdata_len=nbytes;
            if (doResetFrame) resetFrame();
        }
    
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
