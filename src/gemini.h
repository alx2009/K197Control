/**************************************************************************/
/*!
  @file     gemini_fifo.h

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

// Note that if USE_INTERRUPT is defined, only pin 2 or 3 will work as input pin!
#define USE_INTERRUPT 

// Define the maximum size of the FIFO queues
#define FIFO_SIZE 64
#define INPUT_FIFO_SIZE FIFO_SIZE
#define OUTPUT_FIFO_SIZE FIFO_SIZE
// + ===> MISO = pin 12, * ==> SCK = PIN 13
// ICSP Connector layout:
// - * +
// - - -
//

// This is a quick hack, bypassing the pins set in the class. 
// If this was production SW we would calculate at run time inside the class.

// Arduino PIN12 corresponds to AVR PB4
#define INPUT_PIN 2      
#define INPUT_PORT PIND 
#define INPUT_PORT_PIN 2  

// Arduino PIN13 corresponds to AVR PB5
#define OUTPUT_PIN 3    
#define OUTPUT_PORT PORTD
#define OUTPUT_PORT_PIN 3  

#define DEBUG_PORT PORTC

#define DEBUG_STATE() DEBUG_PORT = (DEBUG_PORT & 0xf0) | ( (uint8_t)state & 0x0f)

#define FAST_READ()  ( (INPUT_PORT & _BV(INPUT_PORT_PIN)) != 0 ? true : false )

#define FAST_WRITE(value) \
    if (value) {                \
        OUTPUT_PORT |= _BV(OUTPUT_PORT_PIN);      \
    } else {                    \
        OUTPUT_PORT &= ~_BV(OUTPUT_PORT_PIN);     \
    }

volatile bool inputEdgeDetected = false;


#ifdef USE_INTERRUPT
#   include <util/atomic.h> // Include the atomic library
    void risingEdgeInterrupt() {
        inputEdgeDetected = true;
    }
#   define pollInputEdge()    
#else
#   define ATOMIC_BLOCK(arg)
#   define ATOMIC_RESTORESTATE
    volatile bool inputPinState = false;
    void pollInputEdge () {
        bool inputPinNow = FAST_READ();
        if (inputPinNow != inputPinState) {
            if (inputPinNow )  inputEdgeDetected = true; 
            inputPinState = inputPinNow;
        }
    }
#endif //USE_INTERRUPT

/*
*/

class boolFifo {
public:
    boolFifo() : head(0), tail(0), count(0) {}

    bool push(bool value) {
        if (count >= FIFO_SIZE) {
            return false; // FIFO is full
        }
        buffer[tail] = value;
        tail = (tail + 1) % FIFO_SIZE;
        count++;
        return true;
    }

    bool pop() {
        if (count <= 0) {
            return false; // FIFO is empty
        }
        bool value = buffer[head];
        head = (head + 1) % FIFO_SIZE;
        count--;
        return value;
    }

    bool empty() const {
        return count == 0;
    }

    bool full() const {
        return count == FIFO_SIZE;
    }

    size_t size() const {
        return count;
    }

private:
    bool buffer[FIFO_SIZE];
    size_t head = 0;
    size_t tail = 0;
    size_t count = 0;
};

class GeminiProtocol {
public:
    GeminiProtocol(uint8_t inputPin, uint8_t outputPin, unsigned long readDelayMicros, unsigned long writeDelayMicros,
                   unsigned long handshakeTimeoutMicros, unsigned long readEndMicros, unsigned long writeEndMicros)
        : inputPin(inputPin), outputPin(outputPin), readDelayMicros(readDelayMicros), writeDelayMicros(writeDelayMicros), 
                   handshakeTimeoutMicros(handshakeTimeoutMicros), readEndMicros(readEndMicros), writeEndMicros(writeEndMicros) {
        digitalWrite(outputPin, LOW);
        pinMode(inputPin, INPUT);
        pinMode(outputPin, OUTPUT);
        digitalWrite(outputPin, LOW);
    }

    void begin() {
        pinMode(outputPin, OUTPUT);
        state = State::IDLE;
        lastBitReadTime = 0;
    }

    void update() {
        unsigned long currentTime = micros();
        pollInputEdge();
        
        switch (state) {
            case State::IDLE:
                ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                      if (inputEdgeDetected) {
                          isInitiator = false;
                          state = State::BIT_READ_START;
                          DEBUG_STATE();
                          lastBitReadTime = currentTime;
                          inputEdgeDetected = false; // Reset the flag atomically
                          break;
                      }
                }
                if ( canBeInitiator && (outputBuffer.size()>0) ) {
                    isInitiator = true;
                    FAST_WRITE(HIGH);
                    state = State::BIT_WRITE_START;
                    DEBUG_STATE();
                    lastBitReadTime = currentTime;
                }
                break;
            case State::BIT_READ_START:
                if (currentTime - lastBitReadTime >= readEndMicros) {
                    bool bitValue = FAST_READ();
                    inputBuffer.push(bitValue);

                    if (outputBuffer.empty()) {
                        if (isInitiator) { // we need to stop here
                            FAST_WRITE(LOW);           // Just to be sure...
                            inputEdgeDetected = false; // just to be sure...
                            isInitiator = false;       // just to be sure...
                            state = State::IDLE;
                        } else { // must send an acknowledge
                            FAST_WRITE(HIGH);           
                            state = State::BIT_HANDSHAKE_START;
                        }
                    } else {  // if we have data to send, we cannot stop until we have sent it all...
                        FAST_WRITE(HIGH);
                        state = State::BIT_WRITE_START;
                    }
                    DEBUG_STATE();
                    lastBitReadTime = currentTime;
                }
                break;

            case State::BIT_WRITE_START:
                if (currentTime - lastBitReadTime >= writeDelayMicros) {
                    if (!outputBuffer.empty()) {
                        bool bitToSend = outputBuffer.pop();
                        FAST_WRITE(bitToSend);
                        state = State::BIT_WRITE_WAIT_ACK;
                    } else {
                        Serial.println(F("Invalid state in BIT_WRITE_START"));
                        FAST_WRITE(false);
                        state = State::IDLE;
                    }      
                    DEBUG_STATE();
                        break;
                }
                break;
            case State::BIT_WRITE_WAIT_ACK:
                ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                    if (inputEdgeDetected) {
                        state = State::BIT_WRITE_END;
                        lastBitReadTime = currentTime;
                        DEBUG_STATE();
                        inputEdgeDetected = false; // Reset the flag atomically
                    } 
                }
                break;

            case State::BIT_WRITE_END: 
                if (currentTime - lastBitReadTime >= writeEndMicros) {
                        FAST_WRITE(false);
                        state = State::BIT_READ_START;
                        lastBitReadTime = currentTime;
                        DEBUG_STATE();                  
                }
                break;
                
            case State::BIT_HANDSHAKE_START:
                if (currentTime - lastBitReadTime >= writeDelayMicros) {
                        FAST_WRITE(false);
                        //state = State::BIT_HANDSHAKE_END;
                        state = State::IDLE;
                        DEBUG_STATE();
                }
                break;

/*
            case State::BIT_HANDSHAKE_END:
                ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                    if (inputEdgeDetected) {
                        state = State::IDLE;
                        DEBUG_STATE();
                        inputEdgeDetected = false; // Reset the flag atomically
                        break;
                    } 
                }
                if (micros() - lastBitReadTime >= handshakeTimeoutMicros) {
                    FAST_WRITE(false);         // just to be sure...
                    inputEdgeDetected = false; // just to be sure...
                    state = State::IDLE;
                    DEBUG_STATE();
                }
                break;
*/
            // Add more states and transitions as needed

            default:
                break;
        }
    }

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

    void pulse(unsigned long microseconds) {
        FAST_WRITE(true);
        delayMicroseconds(microseconds);
        FAST_WRITE(false);
    }

    bool waitInputEdge(unsigned long timeout_micros) {
        unsigned long currentTime = micros();
        unsigned long waitStartTime = currentTime;
        volatile bool wait = true;
        while( wait ) {
            currentTime = micros();
            pollInputEdge(); // we wait, hence the member function name ;-)
            delayMicroseconds(4);
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                if (inputEdgeDetected) {
                    wait = false;
                    inputEdgeDetected = false;
                }
            }
            if (currentTime - waitStartTime >= timeout_micros) {
               return false;
            }
        } 
        
        return true;
    }

    void waitInputEdge() {
        volatile bool wait = true;
        while( wait ) {
            pollInputEdge(); // we wait, hence the member function name ;-)
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                if (inputEdgeDetected) {
                    wait = false;
                    inputEdgeDetected = false;
                }
            }
        } 
    }


    bool waitInputIdle(unsigned long timeout_micros) {
        unsigned long currentTime = micros();
        unsigned long waitStartTime = micros();
        bool volatile inputPin = FAST_READ();
        while( inputPin == true  ) {
            currentTime = micros();
            pollInputEdge(); // we wait, hence the member function name ;-)
            delayMicroseconds(4);
            inputPin = FAST_READ();
            if (currentTime - waitStartTime >= timeout_micros) {
               return false;
            }            
        }
        return true;
    }

    bool getInitiatorMode() {return canBeInitiator;};
    void setInitiatorMode(bool newMode) {canBeInitiator=newMode;};

private:
    uint8_t inputPin;
    uint8_t outputPin;
    unsigned long readDelayMicros;
    unsigned long writeDelayMicros;
    unsigned long handshakeTimeoutMicros;
    unsigned long lastBitReadTime;
    unsigned long writeEndMicros;
    unsigned long readEndMicros;

    bool canBeInitiator = true;
    bool isInitiator = false;

    enum class State {
        IDLE=0,
        BIT_READ_START=1,
        BIT_WRITE_START=2,
        BIT_WRITE_WAIT_ACK=3,
        BIT_WRITE_END=4,
        BIT_HANDSHAKE_START=5,
        // Add more states as needed
    } state;

    boolFifo inputBuffer;
    boolFifo outputBuffer;
};

#endif //K197CTRL_GEMINI_H
