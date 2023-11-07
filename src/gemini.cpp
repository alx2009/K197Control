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

volatile bool inputEdgeDetected = false; ///< flag, set in the interrupt handler
/*!
     @brief  interrupt handler.

     @details this is the interrupt detecting the rising edge on the input pin
     when begin() is called, this handler is attached to the input pin with
   attachInterrupt()

*/
void risingEdgeInterrupt() { inputEdgeDetected = true; }

/*!
     @brief  initialize the object.

     @details It should be called before using the object

     PREREQUISITES: Serial.begin must be called to see any error message
     @return true if the call was succesful and the object can be used, false
   otherwise
*/
bool GeminiProtocol::begin() {
  if (digitalPinToInterrupt(inputPin) == NOT_AN_INTERRUPT) {
    Serial.print(F("Error: Pin "));
    Serial.print(inputPin);
    Serial.println(F(" does not support interrupts!"));
    return false;
  }
  digitalWrite(outputPin, LOW);
  pinMode(inputPin, INPUT);
  pinMode(outputPin, OUTPUT);
  digitalWrite(outputPin, LOW);
  state = State::IDLE;
#ifdef DEBUG_PORT      // Make sure the following pins match the definition of
                       // DEBUG_PORT above!
  pinMode(A0, OUTPUT); // TODO: use direct port manipulation so the above is
                       // always verified...
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  DEBUG_STATE();
  DEBUG_FRAME_END();
#endif // DEBUG_PORT
  lastBitReadTime = 0L;
  attachInterrupt(digitalPinToInterrupt(inputPin), risingEdgeInterrupt, RISING);
  return true;
}

/*!
     @brief  main input/output handler

     @details in this function we update the internal state machine, reading and
   writing data as required by the first layer of the gemini protocol
   specification

     The gemini protocol does not require a strict timing at eitehr side,
   however update() should be called as often as possible (at least once every
   loop() iteraction) to achieve maximum possible throughput and lowest possible
   latency

     If this function is not called for a significant amount of time, the
   protocol may time-out and abort the current frame transmission
*/
void GeminiProtocol::update() {
  unsigned long currentTime = micros();

  switch (state) {
  case State::IDLE:
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (inputEdgeDetected) {
        isInitiator = false;
        frameEndDetected = false;
        state = State::BIT_READ_START;
        DEBUG_STATE();
        DEBUG_FRAME_END();
        lastBitReadTime = currentTime;
        inputEdgeDetected = false; // Reset the flag atomically
        break;
      }
    }
    if (frameEndDetected) {
      if (canBeInitiator && (outputBuffer.size() > 0)) {
        isInitiator = true;
        fast_write(HIGH);
        delayMicroseconds(writePulseMicros);
        bool bitToSend = outputBuffer.pull();
        fast_write(bitToSend);
        frameEndDetected = false;
        state = State::BIT_WRITE_WAIT_ACK;
        DEBUG_STATE();
        DEBUG_FRAME_END();
        lastBitReadTime = currentTime;
      }
    } else if (currentTime - lastBitReadTime >= frameTimeout) {
      frameEndDetected = true;
      DEBUG_FRAME_END();
    }
    break;
  case State::BIT_READ_START:
    if (currentTime - lastBitReadTime >= readDelayMicros) {
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
          delayMicroseconds(writePulseMicros);
          fast_write(false);
          state = State::IDLE;
        }
      } else { // if we have data to send, we cannot stop until we have sent it
               // all...
        fast_write(HIGH);
        delayMicroseconds(writePulseMicros);
        bool bitToSend = outputBuffer.pull();
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
    if (currentTime - lastBitReadTime >= writeDelayMicros) {
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

/*!
     @brief  wait for a positive edge on the input pin

     @details this is a helper function that waits for a transition on the input
   pin This function will block until a positive edge is detected on the input
   pin or a timeout occurs. Only interrupt handlers are running while this
   function is waiting.

     Note that this function will reset the edge detection, so the next update()
   will not detect the same input edge. This is by design.

     This function is not required at all to imnplement the gemini protocol, but
   it can be useful in special circumstances. For example, at startup an
   application may want to display an error message if the peer is not running.

     @param timeout_micros timeout in microseconds
     @return true if an edge was detected, false if the function returns due to
   timeout
*/
bool GeminiProtocol::waitInputEdge(unsigned long timeout_micros) {
  unsigned long currentTime = micros();
  unsigned long waitStartTime = currentTime;
  volatile bool wait = true;
  while (wait) {
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

/*!
     @brief  wait for a positive edge on the input pin

     @details this is a helper function that waits for a transition on the input
   pin This function will block until a positive edge is detected on the input
   pin. Only interrupt handlers are running while this function is waiting.

     Note that this function will reset the edge detection, so the next update()
   will not detect the same input edge. This is by design.

     This function is not required at all to imnplement the gemini protocol, but
   it can be useful in special circumstances. For example, at startup an
   application may want to display an error message if the peer is not running.
*/
void GeminiProtocol::waitInputEdge() {
  volatile bool wait = true;
  while (wait) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (inputEdgeDetected) {
        wait = false;
        inputEdgeDetected = false;
      }
    }
  }
}

/*!
     @brief  wait for the input pin to be LOW

     @details this is a helper function that waits for the input pin to be LOW
     This function will block until the input pin is low or a timeout occurs.
     Only interrupt handlers are running while this function is waiting.

     This function is not required at all to imnplement the gemini protocol, but
   it can be useful in special circumstances. For example, at startup an
   application may want to detect a pulse lasting 100 microseconds or less on
   the input pin. This can be achieved calling waitInputEdge() and
   waitInputIdle(100) one after the other.
   @param timeout_micros timeout in microseconds 
   @return true if the input edge was detected, false in case of timeout
*/
bool GeminiProtocol::waitInputIdle(unsigned long timeout_micros) {
  unsigned long currentTime = micros();
  unsigned long waitStartTime = micros();
  bool volatile inputPin = fast_read();
  while (inputPin == true) {
    currentTime = micros();
    delayMicroseconds(4);
    inputPin = fast_read();
    if (currentTime - waitStartTime >= timeout_micros) {
      return false;
    }
  }
  return true;
}
