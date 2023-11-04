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
#    define DEBUG_FRAME_STATE() DEBUG_PORT = (DEBUG_PORT & 0xe7) | ( (((uint8_t)frameState)<<3) & 0x18)
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
    
/*!
      @brief gemini frame layer handler

      @details define a class implementing the frame layer of the gemini protocol specification
      The frame layer is responsible for sending and receiving byte sequences as gemini frames

      After construction, begin() must be called before using any other function. Then update() must be called on a regular basis.
      
      When data is received, it is stored in a frame input buffer. When a complete sequence of data has been received, frameComplete()
      will return true. Data must be read with getFrame() before a new frame can be received (alternatively resetFrame() can be called).

*/
class GeminiFrame : public GeminiProtocol {   
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
        GeminiFrame(uint8_t inputPin, uint8_t outputPin, unsigned long writePulseMicros,
                   unsigned long handshakeTimeoutMicros, unsigned long readDelayMicros, unsigned long writeDelayMicros)
        : GeminiProtocol(inputPin, outputPin, writePulseMicros,
                   handshakeTimeoutMicros, readDelayMicros, writeDelayMicros) {
                   frameState=FrameState::WAIT_FRAME_START;
                    
    }

    /*!
     @brief  initialize the object.

     @details It should be called before using the object
     Note that when begin is called with no argument, no input frame buffer is assigned but any data received will be left in the FIFO buffer.

     This is useful in case an application want to have access the raw frame, including synchronization sequences and stop bits 
     (the base class methods shall be used for that). IT is still possible to send a frame using sendFrame().

     PREREQUISITES: Serial.begin must be called to see any error message

     @return true if the call was succesful and the object can be used, false otherwise
    */

    bool begin() {  // with this form of begin only the output is handled
        if (!GeminiProtocol::begin()) return false;
        pInputData=NULL;
        pInputData_len=0;
        frameState=FrameState::WAIT_FRAME_START;
#   ifdef DEBUG_PORT // Make sure the following pins match the definition of DEBUG_PORT above!
        pinMode(A3, OUTPUT); // TODO: use direct port manipulation so the above is always verified...
        pinMode(A4, OUTPUT);
        DEBUG_FRAME_STATE();
#   endif //DEBUG_PORT
        return true;      
    }
  
/*!
     @brief  initialize the object.

     @details It should be called before using the object
     When a frame is received, pdata[0] will include the first sub-frame sent, 
     pdata[1] the second and so on until the last sub-frame is stored in pdata[nbytes-1].
     
     Note that synchronization sequences and start bits are discarded, only the actual data is stored.

     PREREQUISITES: Serial.begin must be called to see any error message

     @param pdata point to a buffer where to store frame data
     @param nbytes number of data in a frame
     @return true if the call was succesful and the object can be used, false otherwise
*/
    bool begin(uint8_t *pdata, uint8_t nbytes) {
        if ( (nbytes == 0) || (pdata == NULL) ) {
            return false;
        }
        if (!begin()) {
            return false;
        }
        setInputBuffer(pdata, nbytes);
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
                // if pInputData == NULL means ignore input data
                if (hasData() && (pInputData != NULL) ) {
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
                if ( hasData() && (pInputData != NULL) ) {
                    handleFrameData();
                }
                if ( frameEndDetected ) {
                    frameState = FrameState::FRAME_END;    
                    DEBUG_FRAME_STATE();
                    DEBUG_PRINT(']');
                }
                break;

            case FrameState::FRAME_END:
                // if pInputData == NULL means ignore input data
                while( hasData() && (pInputData != NULL) ) receive();
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
                   pInputData[byte_counter] = receiveByte(false);
                   DEBUG_PRINT(' '); DEBUG_PRINT(pInputData[byte_counter], BIN);
                   byte_counter++;
                   start_bit = false;
                   if (frameComplete()) {
                       DEBUG_PRINT('>');
                   }
                } 
                // eventually we will have 8 bits or frameEndDetected in update() 
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

    bool frameComplete() const { return byte_counter >= pInputData_len ? true : false; }
    
    void resetFrame() {
        byte_counter=0;
        start_bit=false;
        DEBUG_PRINT('#');
    }

    uint8_t *getFrame() { DEBUG_PRINT('@'); resetFrame(); return pInputData;  };
    uint8_t getFrameLenght() const { return pInputData_len; };


    bool frameTimeoutDetected() const {return frameTimeoutCounter>0 ? true : false;};
    unsigned long getFrameTimeoutCounter() const {return frameTimeoutCounter;};
    void resetFrameTimeoutCounter() { frameTimeoutCounter = 0L;};

    protected:
        void setInputBuffer(uint8_t *pdata, uint8_t nbytes, bool doResetFrame=false) {
            pInputData=pdata;
            pInputData_len=nbytes;
            if (doResetFrame) resetFrame();
        }
    
    private: 
        bool checkFrameTimeout() {return micros() - lastBitReadTime >= frameTimeout ? true : false;};
        bool frameStarted() const { return (byte_counter >0 || start_bit==true) ? true : false; };
    
        uint8_t *pInputData=NULL;
        uint8_t pInputData_len;
        uint8_t byte_counter=0;
        bool start_bit=false;        

        unsigned long frameTimeoutCounter=0;

        enum class FrameState {
            WAIT_FRAME_START=0,
            WAIT_FRAME_DATA=1,
            FRAME_END=2,
        } frameState;

    protected:
     using GeminiProtocol::begin;

};

#endif //K197CTRL_GEMINI_FRAME_H
