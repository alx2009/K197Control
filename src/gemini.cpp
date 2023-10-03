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
                          state = State::BIT_READ_START;
                          DEBUG_STATE();
                          lastBitReadTime = currentTime;
                          inputEdgeDetected = false; // Reset the flag atomically
                          break;
                      }
                }
                if ( canBeInitiator && (outputBuffer.size()>0) ) {
                    isInitiator = true;
                    fast_write(HIGH);
                    state = State::BIT_WRITE_START;
                    DEBUG_STATE();
                    lastBitReadTime = currentTime;
                }
                break;
            case State::BIT_READ_START:
                if (currentTime - lastBitReadTime >= readEndMicros) {
                    bool bitValue = fast_read();
                    inputBuffer.push(bitValue);

                    if (outputBuffer.empty()) {
                        if (isInitiator) { // we need to stop here
                            fast_write(LOW);           // Just to be sure...
                            inputEdgeDetected = false; // just to be sure...
                            isInitiator = false;       // just to be sure...
                            state = State::IDLE;
                        } else { // must send an acknowledge
                            fast_write(HIGH);           
                            state = State::BIT_HANDSHAKE_START;
                        }
                    } else {  // if we have data to send, we cannot stop until we have sent it all...
                        fast_write(HIGH);
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
                        fast_write(bitToSend);
                        state = State::BIT_WRITE_WAIT_ACK;
                    } else {
                        Serial.println(F("Invalid state in BIT_WRITE_START"));
                        fast_write(false);
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
                        fast_write(false);
                        state = State::BIT_READ_START;
                        lastBitReadTime = currentTime;
                        DEBUG_STATE();                  
                }
                break;
                
            case State::BIT_HANDSHAKE_START:
                if (currentTime - lastBitReadTime >= writeDelayMicros) {
                        fast_write(false);
                        state = State::IDLE;
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
