/**************************************************************************/
/*!
  @file     gemini_fifo.h

  Arduino K197Control library

  Copyright (C) 2023 by ALX2009

  License: MIT (see LICENSE)

  This file is part of the Arduino K197Display sketch, please see
  https://github.com/alx2009/K197Control for more information

  This file defines the FIFO used by the Gemini class

*/
/**************************************************************************/
#ifndef K197CTRL_BOOL_FIFO_H
#define K197CTRL_BOOL_FIFO_H

// Define the maximum size of the FIFO queues
// Tested only with a size greater than the maximum frame lenght expected 
// including synchronization sequence(s) and stop bits. Should work
// with smaller buffer if data read frequently enough, but not tested
#define FIFO_SIZE 64               ///< default size of the FIFO
#define INPUT_FIFO_SIZE FIFO_SIZE  ///< size of the input FIFO
#define OUTPUT_FIFO_SIZE FIFO_SIZE ///< size of the output FIFO

/*!
      @brief define a class implementing a FIFO buffer

      @details Records can be puished to the tail and pulled from the head of the buffer, first in first out(FIFO)
      A bool is used to represent a single bit value (0= false, 1= true). 
      TODO: could be improved to pack 8 bools in one bit, right now we waste a lot of RAM 
*/
class boolFifo {
public:

    /*!
        @brief  constructor for the class. After construction the FIFO is empty.
    */
    boolFifo() : head(0), tail(0), count(0) {};

    /*!
        @brief  store a new value in the FIFO if there is space left 
        @param value the value that should be pushed at the tail of the FIFO
        @return true if the value was stored correctly, false otherwise (FIFO full)
    */
    bool push(bool value) {
        if (count >= FIFO_SIZE) {
            return false; // FIFO is full
        }
        buffer[tail] = value;
        tail = (tail + 1) % FIFO_SIZE;
        count++;
        return true;
    };

    /*!
        @brief  removes the value at the head of the FIFO and returns it to the caller
        @return the oldest element still stored
    */
    bool pull() {
        if (count <= 0) {
            return false; // FIFO is empty
        }
        bool value = buffer[head];
        head = (head + 1) % FIFO_SIZE;
        count--;
        return value;
    };

    /*!
        @brief  check if the buffer is empty 
        @return true if empty, false otherwise
    */
    bool empty() const {
        return count == 0;
    };

    /*!
        @brief  check if the buffer is full 
        @return true if full, false otherwise
    */
    bool full() const {
        return count == FIFO_SIZE;
    }
 
    /*!
        @brief  get the number of records stored in the FIFO 
        @return the number of records currently in the FIFO queue
    */
    size_t size() const {
        return count;
    };

private:
    bool buffer[FIFO_SIZE]; ///< the FIFO buffer itself
    size_t head = 0;        ///< index to the head of the FIFO buffer
    size_t tail = 0;        ///< index to the tail of the FIFO buffer
    size_t count = 0;       ///< count how many elements are stored in the FIFO
};

#endif //K197CTRL_BOOL_FIFO_H
