/**************************************************************************/
/*!
  @file     geminiFrame.h

  Arduino K197Control library

  Copyright (C) 2023 by ALX2009

  License: MIT (see LICENSE)

  This file is part of the Arduino K197Display sketch, please see
  https://github.com/alx2009/K197Control for more information

  This file defines the GeminiFrame class
  The Gemini class implements the gemini protocol, sending and receiving frames
  of data between two peers

*/
/**************************************************************************/
#ifndef K197CTRL_GEMINI_FRAME_H
#define K197CTRL_GEMINI_FRAME_H
#include "gemini.h"

#ifdef DEBUG_PORT
/*!
 * @brief macro used to output frameState to DEBUG_PORT
 */
#define DEBUG_FRAME_STATE()                                                    \
  DEBUG_PORT = (DEBUG_PORT & 0xe7) | ((((uint8_t)frameState) << 3) & 0x18)
#else
#define DEBUG_FRAME_STATE()
#endif // DEBUG_PORT

//#define DEBUG_GEMINI_FRAME  // comment/uncomment to activate debug prints
#ifdef DEBUG_GEMINI_FRAME
/*!
 * @brief debug output macro
 */
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
/*!
 * @brief debug output macro
 */
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
/*!
 * @brief debug output macro
 */
#define DEBUG_PRINT(...) 
/*!
 * @brief debug output macro
 */
#define DEBUG_PRINTLN(...)
#endif // DEBUG_GEMINI_FRAME

/*!
      @brief gemini frame layer handler

      @details define a class implementing the frame layer of the gemini
   protocol specification The frame layer is responsible for sending and
   receiving byte sequences as gemini frames

      After construction, begin() must be called before using any other
   function. Then update() must be called on a regular basis.

      When data is received, it is stored in uint8_t array passed at begin().
   When a complete sequence of data has been received, frameComplete() will
   return true. Data must be read with getFrame() before a new frame can be
   received (alternatively resetFrame() can be called).

      When no array is passed at begin(), the function can only be used to
   receive data.

      The method sendFrame()is used to send a uint8_t array in a gemini frame.
*/
class GeminiFrame : public GeminiProtocol {
public:
  /*!
      @brief  constructor for the class. After construction the FIFO is empty.

      @details See protocol specification for more information about the
     protocol and its timing Note that handshakeTimeoutMicros is not implemented
     yet

      @param inputPin input pin. Must be able to detect an edge interrupt (on a
     UNO, only pin 2 and 3 can be used)
      @param outputPin output pin. any I/O pin can be used
      @param writePulseMicros minimum duration of the write pulse
      @param handshakeTimeoutMicros timeout for handshakes. If no handshake
     received the transmission is aborted
      @param readDelayMicros delay from the time an edge is detected on the
     input pin, to the time the bit value is read
      @param writeDelayMicros minimum time when writing. After an edge is
     detected on the input pin (signaling the other peer has read the value on
     the output pin), wait at least writeDelayMicros before returning the
     outpout pin to LOW
  */
  GeminiFrame(uint8_t inputPin, uint8_t outputPin,
              unsigned long writePulseMicros,
              unsigned long handshakeTimeoutMicros,
              unsigned long readDelayMicros, unsigned long writeDelayMicros)
      : GeminiProtocol(inputPin, outputPin, writePulseMicros,
                       handshakeTimeoutMicros, readDelayMicros,
                       writeDelayMicros) {
    frameState = FrameState::WAIT_FRAME_START;
  }

  /*!
   @brief  initialize the object.

   @details It should be called before using the object
   Note that when begin is called with no argument, no input frame buffer is
   assigned but any data received will be left in the FIFO buffer.

   This is useful in case an application want to have access the raw frame,
   including synchronization sequences and stop bits (the base class methods
   shall be used for that). It is still possible to send a frame using
   sendFrame().

   PREREQUISITES: Serial.begin must be called to see any error message

   @return true if the call was succesful and the object can be used, false
   otherwise
  */
  bool begin() { // with this form of begin only the output is handled
    if (!GeminiProtocol::begin())
      return false;
    pInputData = NULL;
    pInputData_len = 0;
    frameState = FrameState::WAIT_FRAME_START;
#ifdef DEBUG_PORT        // Make sure the following pins match the definition of
                         // DEBUG_PORT above!
    pinMode(A3, OUTPUT); // TODO: use direct port manipulation so the above is
                         // always verified...
    pinMode(A4, OUTPUT);
    DEBUG_FRAME_STATE();
#endif // DEBUG_PORT
    return true;
  }

  /*!
       @brief  initialize the object.

       @details It should be called before using the object
       When a frame is received, pdata[0] will include the first sub-frame sent,
       pdata[1] the second and so on until the last sub-frame is stored in
     pdata[nbytes-1].

       Note that synchronization sequences and start bits are discarded, only
     the actual data is stored.

       PREREQUISITES: Serial.begin must be called to see any error message

       @param pdata point to a buffer where to store frame data
       @param nbytes number of data in a frame
       @return true if the call was succesful and the object can be used, false
     otherwise
  */
  bool begin(uint8_t *pdata, uint8_t nbytes) {
    if ((nbytes == 0) || (pdata == NULL)) {
      return false;
    }
    if (!begin()) {
      return false;
    }
    setInputBuffer(pdata, nbytes);
    return true;
  }

  /*!
       @brief  send a sequence of bytes as a frame to the lower layer
       @details this function send a sequence of bytes as a frame
       each byte is prepended with a start bit, as required by the Gemini Frame
     specification

       This function does not send any synchronization sequence at the beginning
     or end of the frame. If required, the methods of the base class can be used
     to send any number of 0 bits before and after calling sendFrame()

       @param pdata pointer to an array of uint8_t with the bytes to send
     (without any start/stop bits)
       @param nbytes number of bytes to send
  */
  void sendFrame(uint8_t *pdata, uint8_t nbytes) {
    for (uint8_t i = 0; i < nbytes; i++) {
      send(true);
      send(pdata[i]);
    }
  }

  /*!
     @brief  main input/output handler

     @details in this function we update the internal state machine, reading and
     writing data as required by the K197

     The gemini protocol does not require a strict timing at either side,
     however update() should be called as often as possible (at least once every
     loop() iteraction) to achieve maximum possible throughput and lowest
     possible latency

     If this function is not called for a significant amount of time, the
     protocol may time-out and abort the current frame transmission
  */
  void update() {
    GeminiProtocol::update();
    switch (frameState) {
    case FrameState::WAIT_FRAME_START:
      // pInputData == NULL means ignore input data
      if (hasData() && (pInputData != NULL)) {
        while (hasData() && frameEndDetected)
          receive();
      }
      if (!frameEndDetected) {
        resetFrame();
        frameState = FrameState::WAIT_FRAME_DATA;
        DEBUG_FRAME_STATE();
        DEBUG_PRINTLN();
        DEBUG_PRINT('[');
      }
      break;

    case FrameState::WAIT_FRAME_DATA:
      if (hasData() && (pInputData != NULL)) {
        handleFrameData();
      }
      if (frameEndDetected) {
        frameState = FrameState::FRAME_END;
        DEBUG_FRAME_STATE();
        DEBUG_PRINT(']');
      }
      break;

    case FrameState::FRAME_END:
      // if pInputData == NULL means ignore input data
      while (hasData() && (pInputData != NULL))
        receive();
      if (frameEndDetected) {
        frameState = FrameState::WAIT_FRAME_START;
        DEBUG_FRAME_STATE();
        DEBUG_PRINT('*');
        DEBUG_PRINTLN();
      }
      break;
    }
  }

private:
  /*!
     @brief  private function, handles data while a frame is being received
     @details this function is called by GeminiFrame(), it is not intended for
     any other use
  */
  void handleFrameData() {
    if (frameComplete()) {
      while (hasData()) {
#ifdef DEBUG_GEMINI_FRAME
        bool receivedBit = receive();
        DEBUG_PRINT('-');
        DEBUG_PRINT(receivedBit ? '1' : '0');
#else
        receive();
#endif // DEBUG_GEMINI_FRAME
      }
      return;
    }
    if (hasData()) { // fill the frame
      if (start_bit) {
        if (hasData(8)) {
          pInputData[byte_counter] = receiveByte(false);
          DEBUG_PRINT(' ');
          DEBUG_PRINT(pInputData[byte_counter], BIN);
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
      if (checkFrameTimeout()) {
        frameTimeoutCounter++;
        DEBUG_PRINT('T');
        resetFrame();
      }
    }
  }

public:
  /*!
     @brief check if a complete frame is available
     @details when this function returns true, a complete frame has been
     received and is available to the caller, for example using getFrame(). Once
     a new frame is available, no more frames will be received until either
     getFrame() or resetFrame() is called
     @return true if a complete new frame has been received and is available in
     the current input buffer
  */
  bool frameComplete() const {
    return byte_counter >= pInputData_len ? true : false;
  }

  /*!
     @brief reset the current input buffer
     @details after this function is called, the input buffer can receive a new
     measurement Note that the data in the buffer is not actually changed until
     new data is actually received from the K197. See also getFrame()
  */
  void resetFrame() {
    byte_counter = 0;
    start_bit = false;
    DEBUG_PRINT('#');
  }

  /*!
     @brief get the current input buffer
     @return pointer to the current input buffer
  */
  uint8_t *getInputBuffer() { return pInputData; };
  /*!
     @brief get the current input buffer size
     @return pointer to the size of the current input buffer
  */
  uint8_t getInputBufferSize() { return pInputData_len; };

  /*!
     @brief get the current input buffer and reset the frame
     @details tipically this function is called in the main loop, right after
     frameComplete() returns true, to process the data received in a new frame.
     After this function is called, the input buffer can receive a new
     measurement (same as calling resetFrame()) The data in the buffer is not
     actually changed until new data is actually received from the K197 update()
     should be called while processing the frame if the processing takes a
     significant time (even after a complete frame is received, the object may
     need to handle further handshakes and/or synchronization sequences).

     When using this function, the caller must process the new frame quick
     enough to avoid any data overrun

     Alternatively, the caller can use getInputBuffer() and then call
     resetFrame() once is done processing. In such a case, make sure that the
     FIFO in the lower layer is large enough to buffer multiple frames otherwise
     one or more frames will be lost when the FIFO buffer is full
     @return pointer to the current input buffer
  */
  uint8_t *getFrame() {
    DEBUG_PRINT('@');
    resetFrame();
    return pInputData;
  };
  /*!
     @brief get the current input frame buffer size
     @return the size (number of bytes) of the current input frame buffer
  */
  uint8_t getFrameLenght() const { return pInputData_len; };

  /*!
     @brief check if a frame timeout has been detected
     @details the function checks if a frame timoeut has been detected before
     receiving a complete frame this flag is reset when the function
     resetFrameTimeoutCounter() is called
     @return true if a frame timout was detected, false otherwise
  */
  bool frameTimeoutDetected() const {
    return frameTimeoutCounter > 0 ? true : false;
  };
  /*!
     @brief get the frame timeout counter
     @details the frame timeout counter is incremented any time a frame timout
     is detected before receiving a complete frame. the counter is reset
     when the function resetFrameTimeoutCounter() is called
     @return the value of the frame timeout counter
  */
  unsigned long getFrameTimeoutCounter() const { return frameTimeoutCounter; };
  /*!
     @brief reset the frame timeout counter
     @details For more information see frameTimeoutDetected() and
     getFrameTimeoutCounter()
  */
  void resetFrameTimeoutCounter() { frameTimeoutCounter = 0L; };

protected:
  /*!
      @brief set the input frame buffer
      @details this function is used to set the current input frame buffer
      It is intended for use in a sub-class that may need to use a more
     specialized data structure rather than a generic byte array
     @param pdata pointer to an input frame buffer
     @param nbytes lenght of the array pointed to by pdata
     @param doResetFrame if true any data in the new frame buffer will be reset (default to false)
  */
  void setInputBuffer(uint8_t *pdata, uint8_t nbytes,
                      bool doResetFrame = false) {
    pInputData = pdata;
    pInputData_len = nbytes;
    if (doResetFrame)
      resetFrame();
  }

private:
  /*!
      @brief detect a frame timeout
      @details this function is used internally to check the frame timout timer
      A user of this object as well as a sub-class should use
     frameTimeoutDetected()
      @return true if a frame timeout is detected, false otherwise
  */
  bool checkFrameTimeout() {
    return micros() - lastBitReadTime >= frameTimeout ? true : false;
  };
  /*!
      @brief check if a frame reception has started
      @details this function is used internally to check if a frame has started
      @return true if a frame has started, false otherwise
  */
  bool frameStarted() const {
    return (byte_counter > 0 || start_bit == true) ? true : false;
  };

  uint8_t *pInputData = NULL; ///< pointer to the input frame buffer
  uint8_t pInputData_len;     ///< array lenght of the input frame buffer
  uint8_t byte_counter =
      0; ///< while a frame is received, keeps track of the current byte
  bool start_bit = false; ///< set when a start bit is received, reset after the
                          ///< last bit of a byte has been received

  unsigned long frameTimeoutCounter = 0; ///< frame timeout counter

  /*!
      @brief state machine for the gemini frame layer
  */
  enum class FrameState {
    WAIT_FRAME_START = 0, ///< waiting for a frame transmission to start
    WAIT_FRAME_DATA = 1,  ///< set after the first bit of a frame is received
    FRAME_END = 2,        ///< set after the last bit of a frame is received
  } frameState; ///< keep track of the state for the gemini frame protocol

protected:
  using GeminiProtocol::begin;

public:
  // in the current implementation, we re-use the protected base class function making it public
  using GeminiProtocol::isFrameEndDetected; 
};

#endif // K197CTRL_GEMINI_FRAME_H
