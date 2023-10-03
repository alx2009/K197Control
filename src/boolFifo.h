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
#define FIFO_SIZE 64
#define INPUT_FIFO_SIZE FIFO_SIZE
#define OUTPUT_FIFO_SIZE FIFO_SIZE

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

#endif //K197CTRL_BOOL_FIFO_H
