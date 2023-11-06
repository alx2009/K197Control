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

         To check if there is enough space in output buffer for one byte, use canSend(8). 
         Alternatively, wait until noOutputPending() returns true beforre sending new data.
     
         @param data a single byte of data
         @return true if all the 8 boits in data have been sent. False if one or more bits could not be sent (output buffer full).
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

   /*!
     @brief  send one bit of data to the peer
     @details this function pushes a single bit of data to the tail of the output buffer, for subsequent transmission.
     Data in the output FIFO is sent by update() as soon as possible:
     - if the object can be an initiator and no communication is in progress, the next time update() is called initates the transmission
     - if the object cannot be an initiator but a communication is already in progress (no frame end detected), data will be sent after any other data already in the FIFO
     - Otherwise if a communication is already in progress, all data in the FIFO are sent sequentially

     To check if there is enough space in output buffer for one bit, use canSend(). 
     Alternatively, wait until noOutputPending() returns true beforre sending new data.
     
     @param data a single byte of data
     @return true if the bit has been pushed. False if one or more bits could not be pushed (output buffer full).
    */
    bool send(bool bit) {
        if (outputBuffer.full()) {
            return false; // Buffer is full
        }
        outputBuffer.push(bit);
        return true;
    }

   /*!
     @brief  check if there is unread data in the input buffer
     @return true if there is at least one bit in the input buffer. false otherwise
    */
    bool hasData() const {
        return !inputBuffer.empty();
    }
   /*!
     @brief  check if there is unread data in the input buffer
     @param n the number of data to check
     @return true if there is at least n bits of data in the input buffer. false otherwise
    */
    bool hasData(uint8_t n) const {
        return inputBuffer.size() >= n;
    }

   /*!
     @brief  receives one bit of data
     @detail note that when this function returns false, there is no way to know if the input buffer was empty or the received bit was zero. 
     It is recommended to use hasData() before calling this function
     @return true if the received bit has a value of 1 (HIGH). False otherwise.
    */
    bool receive() {
        return inputBuffer.pull();  
    }
    
   /*!
     @brief  receive one byte of data
     @detail note that when block= false, there is no way to know if the input buffer was empty or all the bits werre set to 0 (LOW). 
     It is recommended to use hasData(8) before calling this function in non blocking mode
     In blocking mode, update() is called in a loop while waiting
     @param block if true (default) the function blocks until at least 8 bits are available in the input buffer. 
     @return returns 8 bits of data.
    */
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

   /*!
     @brief  check free space in the output buffer
     @param nbits the number of bits to check
     @return true if there is room in the output buffer to send nbits of data. False otherwise
    */
    bool canSend(size_t nbits=1) { return nbits >= (OUTPUT_FIFO_SIZE-outputBuffer.size()) ? true : false;};
   /*!
     @brief  check if output pending
     @details output pending means that the output buffer still contains data 
     @return true if output is pending, false otherwise
    */
    bool isOutputPending() {return !outputBuffer.empty();};
   /*!
     @brief  check if no output pending
     @details output pending means that the output buffer still contains data 
     @return true if no output is pending, false otherwise
    */
    bool noOutputPending() {return outputBuffer.empty();};

   /*!
     @brief  generate an edge or pulse on the output pin
     @details the function set the output pin to HIGH, waits a number of microseconds (using delayMicroseconds)
     and then set the pin to the requested final state. 
     It is used internally, but it is also available for special uses (for example during startup)
     @param microseconds the minimum duration of the edge or the duration of the pulse
     @param finalState if false a pulse is generated. If true a positive edge is generated
    */
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

private:
   /*!
     @brief  read the input pin using AVR registers directly
     @details the Arduino functions are too slow so using direct I/O is required
     @return true if the input pin is high, false otherwise
    */
    inline bool fast_read() { return (*inputRegister & inputBitmask) != 0 ? true : false; };
   /*!
     @brief  write tpo the output pin using AVR registers directly
     @details the Arduino functions are too slow so using direct I/O is required
     @param value if true the output pin is set to HIGH, otherwise LOW
    */
    inline void fast_write(bool value) {
        if (value) {                
            *outputRegister |= outputBitmask;      
        } else {                    
            *outputRegister &= ~outputBitmask;     
        }
    };
        
private:
    uint8_t inputPin;  ///< input pin
    uint8_t outputPin; ///< output pin
    uint8_t inputBitmask=0x00; ///< bitmask used by fast_read()
    uint8_t outputBitmask=0x00; ///< bitmask used by fast_write()
    volatile uint8_t *inputRegister=NULL; ///< AVR register address used by fast_read()
    volatile uint8_t *outputRegister=NULL; ///< AVR register address used by fast_write()

    unsigned long writePulseMicros; ///< minimum duration of the write pulse
    unsigned long handshakeTimeoutMicros; ///< Timeout for handshakes. If no handshake received the transmission is aborted
    unsigned long readDelayMicros; ///< readDelayMicros delay from the time an edge is detected on the input pin, to the time the bit value is read
    unsigned long writeDelayMicros; ///< writeDelayMicros minimum time when writing

    bool canBeInitiator = true; ///< when true this object can initiate a data transfer. Otherwise it has to wait for the peer to initiate the transfer before sending any data
    bool isInitiator = false; ///< set to true when we initiate a transfer, goes back to false when the last bit of data in the outputBuffer has been sent 

   /*!
     @brief  keep track of the internal state of the transfer 
    */
    enum class State {
        IDLE=0,                ///< idle state (input and output LOW, no data being transferred in either direction)
        BIT_READ_START=1,      ///< waiting for readDelayMicros before reading data
        BIT_WRITE_WAIT_ACK=2,  ///< waiting for a positive edge on the input pin
        BIT_WRITE_END=3,       ///< waiting for writeDelayMicros after detecting a positive egde on the input pin
    } state;

    boolFifo inputBuffer; ///< the input buffer
    boolFifo outputBuffer; ///< the output buffer

protected:
    unsigned long lastBitReadTime; ///< keep track of the time the last bit was read from the input pin
    unsigned long frameTimeout=50000L; ///< the frame timeout value
    
   /*!
     @brief  set the frame timeout value (default at begin() is 5000 us
     @details this is a protected function, to be used in the sub-class implementing the frame layer
     @param newValue the new timeout value to set
    */
    void setFrameTimeout(unsigned long newValue) {frameTimeout=newValue;};
   /*!
     @brief  get the frame timeout value
     @details this is a protected function, to be used in the sub-class implementing the frame layer
     @return the current frame timeout value
    */
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
