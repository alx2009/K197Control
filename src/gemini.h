/**************************************************************************/
/*!
  @file     gemini.h

  Arduino K197Control library

  Copyright (C) 2023 by ALX2009

  License: MIT (see LICENSE)

  This file is part of the Arduino K197Display sketch, please see
  https://github.com/alx2009/K197Control for more information

  This file defines the Gemini class
  The Gemini class implements the gemini protocol, sending and receiving frames of data between two peers

*/
/**************************************************************************/
#ifndef K197CTRL_GEMINI_H
#define K197CTRL_GEMINI_H

#include <Arduino.h>
#include <util/atomic.h> // Include the atomic library

#include "boolFifo.h"

// Note that using interrupts is required to catch the leading edge on the input pin. On a UNO, only pin 2 or 3 will work as input pin!
// + ===> MISO = pin 12, * ==> SCK = PIN 13
// ICSP Connector layout:
// - * +
// - - -
//

// comment the following definition if debug is not needed
#define DEBUG_PORT PORTC

#ifdef DEBUG_PORT
#    define DEBUG_STATE()     DEBUG_PORT = (DEBUG_PORT & 0xfc) | ( ((uint8_t)state) & 0x03)
//#    define DEBUG_STATE()     DEBUG_PORT = (DEBUG_PORT & 0xe7)   | ( (((uint8_t)state)<<3) & 0x18)
#    define DEBUG_FRAME_END() DEBUG_PORT = frameEndDetected ? (DEBUG_PORT & 0xfb) | 0x04 : DEBUG_PORT & 0xfb
#else
#    define DEBUG_STATE()
#    define DEBUG_FRAME_END()
#endif //DEBUG_PORT

void risingEdgeInterrupt();

class GeminiProtocol {
public:
    GeminiProtocol(uint8_t inputPin, uint8_t outputPin, unsigned long readDelayMicros, unsigned long writeDelayMicros,
                   unsigned long handshakeTimeoutMicros, unsigned long readEndMicros, unsigned long writeEndMicros)
        : inputPin(inputPin), outputPin(outputPin), readDelayMicros(readDelayMicros), writeDelayMicros(writeDelayMicros), 
                   handshakeTimeoutMicros(handshakeTimeoutMicros), readEndMicros(readEndMicros), writeEndMicros(writeEndMicros) {
                    
        inputBitmask=digitalPinToBitMask(inputPin);
        outputBitmask=digitalPinToBitMask(outputPin);
        uint8_t inputPort = digitalPinToPort(inputPin);
        uint8_t outputPort = digitalPinToPort(outputPin);
        inputRegister=portInputRegister(inputPort);
        outputRegister=portOutputRegister(outputPort);
    }

    bool begin();

    void update();

    bool send(uint8_t data) {
        for (int i = 7; i >= 0; i--) {
            bool bitToSend = (data >> i) & 1;
            if (!send(bitToSend)) {
                return false; // Sending failed
            }
        }
        return true; // All bits sent successfully
    }

    bool send(bool bit) {
        if (outputBuffer.full()) {
            return false; // Buffer is full
        }
        outputBuffer.push(bit);
        return true;
    }

    bool hasData() const {
        return !inputBuffer.empty();
    }
    bool hasData(uint8_t n) const {
        return inputBuffer.size() >= n;
    }

    bool receive() {
        return inputBuffer.pop();  
    }
    uint8_t receiveByte(bool block = true) {
        if (!hasData(8)) {
            if (block) {
                while (!hasData(8)) {
                    update();
                }
            } else {
                return 0; // No data available and not blocking
            }
        }

        uint8_t receivedByte = 0;
        for (int i = 7; i >= 0; i--) {
            bool bitValue = inputBuffer.pop();
            receivedByte |= (bitValue << i);
        }

        return receivedByte;
    }

    void pulse(unsigned long microseconds, bool finalState=false) {
        fast_write(true);
        delayMicroseconds(microseconds);
        fast_write(finalState);
    }

    bool waitInputEdge(unsigned long timeout_micros);
    void waitInputEdge();
    bool waitInputIdle(unsigned long timeout_micros);
    
    bool getInitiatorMode() {return canBeInitiator;};
    void setInitiatorMode(bool newMode) {canBeInitiator=newMode;};

    bool serverStartup(unsigned long timeout_micros=0L);

private:
    inline bool fast_read() { return (*inputRegister & inputBitmask) != 0 ? true : false; };
    inline void fast_write(bool value) {
        if (value) {                
            *outputRegister |= outputBitmask;      
        } else {                    
            *outputRegister &= ~outputBitmask;     
        }
    };
        
private:
    uint8_t inputPin;
    uint8_t outputPin;
    uint8_t inputBitmask=0x00;
    uint8_t outputBitmask=0x00;
    volatile uint8_t *inputRegister=NULL;
    volatile uint8_t *outputRegister=NULL;

    unsigned long readDelayMicros;
    unsigned long writeDelayMicros;
    unsigned long handshakeTimeoutMicros;
    unsigned long readEndMicros;
    unsigned long writeEndMicros;

    bool canBeInitiator = true;
    bool isInitiator = false;

    enum class State {
        IDLE=0,
        BIT_READ_START=1,
        BIT_WRITE_WAIT_ACK=2,
        BIT_WRITE_END=3,
    } state;

    boolFifo inputBuffer;
    boolFifo outputBuffer;

protected:
    unsigned long lastBitReadTime;
    unsigned long frameTimeout=50000L;
    void setFrameTimeout(unsigned long newValue) {frameTimeout=newValue;};
    unsigned long getFrameTimeout() const {return frameTimeout;};
    bool volatile frameEndDetected=true;
};

#endif //K197CTRL_GEMINI_H
