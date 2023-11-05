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

// comment the following definition if debug is not needed
#define DEBUG_PORT PORTC

#ifdef DEBUG_PORT
#    define DEBUG_STATE()     DEBUG_PORT = (DEBUG_PORT & 0xfc) | ( ((uint8_t)state) & 0x03)
#    define DEBUG_FRAME_END() DEBUG_PORT = frameEndDetected ? (DEBUG_PORT & 0xfb) | 0x04 : DEBUG_PORT & 0xfb
#else
#    define DEBUG_STATE()
#    define DEBUG_FRAME_END()
#endif //DEBUG_PORT

/*!
      @brief gemini protocol lower layer handler

      @details define a class implementing the lower layer of the gemini protocol specification
      The lower layer is responsible for sending and receiving bits.

      After construction, begin() must be called before using any other function. Then update() must be called on a regular basis.
      
      When data is received, it is stored in an input buffer and hasData() will return true. Data can then be read with receive.

      For practical reasons, this class is also tasked to detect the frame end due to timout. however, the relevant members are protected,
      considering that the complete gemini frame specification should be implemented in a superclass.      
*/
class GeminiProtocol {
public:
    /*!
        @brief  constructor for the class. After construction the FIFO is empty.

        @details See protocol specification for more information about the protocol and its timing
        Note that handshakeTimeoutMicros is not implemented yet
        
        @param inputPin input pin. Must be able to detect an edge interrupt (on a UNO, only pin 2 and 3 can be used)
        @param outputPin output pin. any I/O pin can be used
        @param writePulseMicros minimum duration of the write pulse
        @param handshakeTimeoutMicros timeout for handshakes. If no handshake received the transmission is aborted
        @param readDelayMicros delay from the time an edge is detected on the input pin, to the time the bit value is read
        @param writeDelayMicros minimum time when writing. After an edge is detected on the input pin (signaling 
        the other peer has read the value on the output pin), wait at least writeDelayMicros before returning the outpout pin to LOW 
    */
    GeminiProtocol(uint8_t inputPin, uint8_t outputPin, unsigned long writePulseMicros,
                   unsigned long handshakeTimeoutMicros, unsigned long readDelayMicros, unsigned long writeDelayMicros)
        : inputPin(inputPin), outputPin(outputPin), writePulseMicros(writePulseMicros), 
                   handshakeTimeoutMicros(handshakeTimeoutMicros), readDelayMicros(readDelayMicros), writeDelayMicros(writeDelayMicros) {
                    
        inputBitmask=digitalPinToBitMask(inputPin);
        outputBitmask=digitalPinToBitMask(outputPin);
        uint8_t inputPort = digitalPinToPort(inputPin);
        uint8_t outputPort = digitalPinToPort(outputPin);
        inputRegister=portInputRegister(inputPort);
        outputRegister=portOutputRegister(outputPort);
    }

    bool begin();

    void update();

    
/*!
     @brief  send 8 bits of data to the peer
     @details this function pushes 8 bits of data to the tail of the output buffer, for subsequent transmission.
     Data in the output FIFO is sent by update() as soon as possible:
     - if the object can be an initiator and no communication is in progress, the next time update() is called initates the transmission
     - if the object cannot be an initiator but a communication is already in progress (no frame end detected), data will be sent after any other data already in the FIFO
     - Otherwise if a communication is already in progress, all data in the FIFO are sent sequentially

     If the caller want to ansure data starts a new frame, 
   
     @param data the data to be sent

     @return true if all the bits have been queued for transmission
*/
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
        return inputBuffer.pull();  
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
            bool bitValue = inputBuffer.pull();
            receivedByte |= (bitValue << i);
        }

        return receivedByte;
    }

    bool isOutputPending() {return !outputBuffer.empty();};
    bool noOutputPending() {return outputBuffer.empty();};

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

    unsigned long writePulseMicros;
    unsigned long handshakeTimeoutMicros;
    unsigned long readDelayMicros;
    unsigned long writeDelayMicros;

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

/*!
      @brief detect a frame end
      The frame end is detected if no data is received within the configured frame timout;
      The function will continue to return true until a bit is received, marking the start of a new frame.
      this can be used in a superclass 
      to handle a frame timeout (note that depending on the frame specification, 
      a superclass may have alternative means to detect frame start and end) 
      @return true if a frame timout occurred since the last received bit. False otherwise.
*/
    bool isFrameEndDetected() {return frameEndDetected;};
    bool volatile frameEndDetected=true;  ///< flag to keep track of frame timeouts detected at the lower layer of the gemini protocol
};

#endif //K197CTRL_GEMINI_H
