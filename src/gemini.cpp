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

GeminiProtocol gemini(INPUT_PIN, OUTPUT_PIN, 15, 10, 80, 170, 90);  // in, out, read pulse, write pulse, (not used), read delay, write delay

void startup_sequence () {
    gemini.waitInputEdge();
    //if (!gemini.waitInputEdge(9000000UL)) {
    //      Serial.println(F("No K197 found (timeout)"));
    //      return;
    //}
    //delayMicroseconds(38);
    gemini.pulse(1684);
    delayMicroseconds(60);
    gemini.pulse(20);

    if (!gemini.waitInputIdle(50000UL)) {
      Serial.println(F("W01"));
    }
    delay(35);

    uint8_t initial_data = 0x80;
    gemini.send(initial_data);
    gemini.send(false);

    while(gemini.hasData(9) == false) gemini.update();
    //delayMicroseconds(100);
    gemini.pulse(30);
    gemini.setInitiatorMode(false);
}


void setup() {
    Serial.begin(115200);
    gemini.begin();
    pinMode(A0, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(A2, OUTPUT);
    pinMode(A3, OUTPUT);
    delay(1000);
    Serial.println();
    Serial.println(F("Gemini 0.1"));
    Serial.println();
#ifdef USE_INTERRUPT
    if ( digitalPinToInterrupt(INPUT_PIN) == NOT_AN_INTERRUPT ) {
        Serial.print(F("Error: Pin ")); Serial.print(INPUT_PIN); Serial.println(F(" does not support interrupts!"));
        Serial.println(F("Execution stopped"));
        while(1);
    }
    attachInterrupt(digitalPinToInterrupt(INPUT_PIN), risingEdgeInterrupt, RISING);
#endif //USE_INTERRUPT

    startup_sequence ();
}

unsigned long lastTimeMicros=0L;
bool dataPrinted=false;

unsigned long ndata_frame=0;
unsigned long ndata_subframe=0;
bool subframe_start_detected=false;

void loop() {
    if (Serial.available()) {
        while (Serial.available() > 0) {  // empty serial buffer
            pollInputEdge ();
            Serial.read();
        }
        pollInputEdge ();
        uint8_t data[] = { 0x30, 0x00, 0x00, 0x00, 0x00 };
        for (int i=0; i<5; i++) {
           gemini.send(true);
           gemini.send(data[i]);
        } 
        Serial.println(F("Data sent to gemini"));
    }

    pollInputEdge ();
    gemini.update();

    unsigned long micros_now = micros();
    if (gemini.hasData()) {
        bool receivedBit = gemini.receive();
        pollInputEdge ();
        if (subframe_start_detected) {
            Serial.print(receivedBit ? '1' : '0'); // Sending received byte over Serial
            dataPrinted = true;
            ndata_subframe++;
            if (ndata_subframe>=8) {
                Serial.print(' ');
                subframe_start_detected = false;
                ndata_subframe = 0;
            }
        } else {
            if (receivedBit) subframe_start_detected = true;
        }
        
        pollInputEdge ();
        lastTimeMicros = micros_now;
        ndata_frame++;
    }
    if (dataPrinted && (micros_now - lastTimeMicros >= 50000)) {
        Serial.print(F(" - n="));Serial.print(ndata_frame);
        ndata_frame=0;
        ndata_subframe=0;
        subframe_start_detected=false;
        Serial.println();
        dataPrinted=false;
        lastTimeMicros = micros_now;
    }
    
/*
    if (gemini.hasData(8)) {
        uint8_t receivedByte = gemini.receiveByte();
        pollInputEdge ();
        Serial.write(receivedByte); // Sending received byte over Serial
        pollInputEdge ();
    }
*/

    // Your other loop logic here
}
