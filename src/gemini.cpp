/**************************************************************************/
/*!
  @file     gemini.cpp

  Arduino K197Control library sketch

  Copyright (C) 2023 by ALX2009

  License: MIT (see LICENSE)

  This file is part of the Arduino K197control library, please see
  https://github.com/alx2009/K197Control for more information

  Note: this file implements the class geminiProtocol
*/
#include <Arduino.h>

#include "gemini.h"

volatile bool inputEdgeDetected = false;

bool GeminiProtocol::begin() {
    if ( digitalPinToInterrupt(inputPin) == NOT_AN_INTERRUPT ) {
        Serial.print(F("Error: Pin ")); Serial.print(inputPin); Serial.println(F(" does not support interrupts!"));
        return false;
    }
    digitalWrite(outputPin, LOW);
    pinMode(inputPin, INPUT);
    pinMode(outputPin, OUTPUT);
    digitalWrite(outputPin, LOW);
    state = State::IDLE;
#   ifdef DEBUG_PORT // Make sure the following pins match the definition of DEBUG_PORT above!
        pinMode(A0, OUTPUT); // TODO: use direct port manipulation so the above is always verified...
        pinMode(A1, OUTPUT);
        pinMode(A2, OUTPUT);
        pinMode(A3, OUTPUT);
        pinMode(A4, OUTPUT);
        pinMode(A5, OUTPUT);
        DEBUG_STATE();
#   endif //DEBUG_PORT
    lastBitReadTime = 0L;
    attachInterrupt(digitalPinToInterrupt(inputPin), risingEdgeInterrupt, RISING);
    return true;
}

void risingEdgeInterrupt() {
    inputEdgeDetected = true;
}

void GeminiProtocol::update() {
        unsigned long currentTime = micros();
        
        switch (state) {
            case State::IDLE:
                ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                      if (inputEdgeDetected) {
                          isInitiator = false;
                          frameEndDetected=false;
                          state = State::BIT_READ_START;
                          DEBUG_STATE();
                          lastBitReadTime = currentTime;
                          inputEdgeDetected = false; // Reset the flag atomically
                          break;
                      }
                }
                if (frameEndDetected) {
                    if ( canBeInitiator && (outputBuffer.size()>0) ) {
                        isInitiator = true;
                        fast_write(HIGH);
                        delayMicroseconds(writeDelayMicros);
                        bool bitToSend = outputBuffer.pop();
                        fast_write(bitToSend);
                        frameEndDetected=false;
                        state = State::BIT_WRITE_WAIT_ACK;
                        DEBUG_STATE();
                        lastBitReadTime = currentTime;
                    }
                } else if (currentTime - lastBitReadTime >= frameTimeout) {
                    frameEndDetected = true;
                }
                break;
            case State::BIT_READ_START:
                if (currentTime - lastBitReadTime >= readEndMicros) {
                    bool bitValue = fast_read();
                    inputBuffer.push(bitValue);

                    if (outputBuffer.empty()) {
                        if (isInitiator) { // we need to stop here
                            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                                fast_write(LOW);           // Just to be sure...
                                inputEdgeDetected = false; // just to be sure...
                                isInitiator = false;       // just to be sure...
                                state = State::IDLE;
                            }
                        } else { // must send an acknowledge
                            fast_write(HIGH);           
                            delayMicroseconds(writeDelayMicros);
                            fast_write(false);
                            state = State::IDLE;
                        }
                    } else {  // if we have data to send, we cannot stop until we have sent it all...
                        fast_write(HIGH);
                        delayMicroseconds(writeDelayMicros);
                        bool bitToSend = outputBuffer.pop();
                        fast_write(bitToSend);
                        state = State::BIT_WRITE_WAIT_ACK;  
                    }
                    DEBUG_STATE();
                    lastBitReadTime = currentTime;
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
                        fast_write(false);
                        state = State::BIT_READ_START;
                        lastBitReadTime = currentTime;
                        DEBUG_STATE();                  
                }
                break;
                
            default:
                break;
        }
}

bool GeminiProtocol::waitInputEdge(unsigned long timeout_micros) {
        unsigned long currentTime = micros();
        unsigned long waitStartTime = currentTime;
        volatile bool wait = true;
        while( wait ) {
            currentTime = micros();
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

void GeminiProtocol::waitInputEdge() {
        volatile bool wait = true;
        while( wait ) {
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                if (inputEdgeDetected) {
                    wait = false;
                    inputEdgeDetected = false;
                }
            }
        } 
}

bool GeminiProtocol::waitInputIdle(unsigned long timeout_micros) {
        unsigned long currentTime = micros();
        unsigned long waitStartTime = micros();
        bool volatile inputPin = fast_read();
        while( inputPin == true  ) {
            currentTime = micros();
            delayMicroseconds(4);
            inputPin = fast_read();
            if (currentTime - waitStartTime >= timeout_micros) {
               return false;
            }            
        }
        return true;
}

bool GeminiProtocol::serverStartup(unsigned long timeout_micros) {
    if (timeout_micros!=0) {
        if (!waitInputEdge(timeout_micros)) {
            return false;
        }
    } else {
        waitInputEdge();
    }
    pulse(1684);
    delayMicroseconds(60);
    pulse(20);

    if (!waitInputIdle(50000UL)) {
      return false;
    }
    delay(35);

    uint8_t initial_data = 0x80;
    send(initial_data);
    send(false);

    while(hasData(9) == false) update();
    //delayMicroseconds(100);
    pulse(30);
    setInitiatorMode(false);
    return true;
}
