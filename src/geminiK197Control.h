/**************************************************************************/
/*!
  @file     geminiK197Control.h

  Arduino K197Control library

  Copyright (C) 2023 by ALX2009

  License: MIT (see LICENSE)

  This file is part of the Arduino K197Display sketch, please see
  https://github.com/alx2009/K197Control for more information

  This file defines the GeminiK197Control class
  The GeminiK197Control class implements the application layer of the K197 control protocol. 
  Control frames are sent to/measurement results received from the K197 using the gemini frame layer (via base class geminiFrame)

*/
/**************************************************************************/
#ifndef K197CTRL_GEMINI_K197_CONTROL_H
#define K197CTRL_GEMINI_K197_CONTROL_H
#include "geminiFrame.h"

class GeminiK197Control : public GeminiFrame {   
public:
    /*!
        @brief  constructor for the class.

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
        GeminiK197Control(uint8_t inputPin, uint8_t outputPin, unsigned long writePulseMicros,
                   unsigned long handshakeTimeoutMicros, unsigned long readDelayMicros, unsigned long writeDelayMicros)
        : GeminiFrame(inputPin, outputPin, writePulseMicros,
                   handshakeTimeoutMicros, readDelayMicros, writeDelayMicros) {
                    
    }

    /**************************************************************************/
    /*!
        @brief  Define the measurement unit 
    */
    /**************************************************************************/
    enum K197unit {
        Volt     = 0b00,
        Amp      = 0b10,
        Ohm      = 0b01,
        dB       = 0b11,

    };

    struct K197mr_byte0 {
        /*!
           @brief measurement result frame byte 0
        */
        union {
            uint8_t byte0; ///< allows access to all the flags in the
                           ///< union as one byte as uint8_t
            struct {
                uint8_t range     : 3;   ///< measurement range
                bool    relative  : 1;   ///< relative measurement when true
                bool    undefined : 1;   ///< unknown use, normally set to 1
                bool    ac_dc     : 1;   ///< true if AC
                K197unit unit     : 2;   ///< measurement unit
            };
        } __attribute__((packed)); ///<
    }; ///< Structure designed to pack a number of flags into one byte

    struct K197mr_byte1 {
        /*!
           @brief measurement result frame byte 1
        */
        union {
            uint8_t byte1; ///< allows access to all the flags in the
                           ///< union as one byte as uint8_t
            struct {
                uint8_t msb       : 5;   ///< measurement Most Significant Bits
                bool    ovrange   : 1;   ///<  when true indicates overrange 
                bool    undefined : 1;   ///< unknown use, normally set to 0
                bool negative     : 1;   ///< measurement unit
            };

        } __attribute__((packed)); ///<
    }; ///< Structure designed to pack a number of flags into one byte

    enum K197range {
        R0_Auto       = 0b000,
        R1_200mV_Ohm  = 0b001,
        R2_2V_KOhm    = 0b010,
        R3_20V_KOhm   = 0b011,
        R4_200V_KOhm  = 0b100,
        R5_1KV_2MOhm  = 0b101,
        R6_20MOhm     = 0b110,
        R7_200MOhm    = 0b111,

        // the following are just shorter aliases of the long names above
        R0  = 0b000,
        R1  = 0b001,
        R2  = 0b010,
        R3  = 0b011,
        R4  = 0b100,
        R5  = 0b101,
        R6  = 0b110,
        R7  = 0b111,
    };

    struct K197measurement {
        static const size_t valueAsStringMinSize;
        static const size_t resultAsStringMinSize;
        static const size_t valueAsStringMinSizeER;
        static const size_t resultAsStringMinSizeER;
    
    /*!
       @brief measurement frame
    */
    union {
      uint8_t frameBuffer[1]; ///< allows access to all the data as uint8_t array

      struct {
          K197mr_byte0 byte0;   
          K197mr_byte1 byte1;   
          //uint16_t lsb;
          struct {
              uint8_t hi;
              uint8_t lo;
          } lsb;
      };

      int32_t value;    ///< allows access to all bytes as a 32 bit signed integer value
      uint32_t uvalue;  ///< allows access to all bytes as a 32 bit unsigned integer value
    
    } __attribute__((packed)); ///<

    inline bool isZero() const { return lsb.lo == 0 && lsb.hi == 0 && byte1.msb == 0 ? true : false; };
    inline bool isAC() const {return byte0.ac_dc     ;};
    inline bool isDC() const {return !byte0.ac_dc     ;};
    inline bool isRelative() const {return byte0.relative;};
    inline bool isAbsolute() const {return !byte0.relative;};

    inline bool isVolt() const {return byte0.unit == K197unit::Volt ? true : false;};
    inline bool isOhm() const {return byte0.unit == K197unit::Ohm ? true : false;};
    inline bool isAmp() const {return byte0.unit == K197unit::Amp ? true : false;};
    inline bool is_dB() const {return byte0.unit == K197unit::dB ? true : false;};
    
    inline bool isNegative() const {return byte1.negative;};
    inline bool isOvrange() const {return byte1.ovrange;};

    unsigned long getCount() const {return (((int32_t) byte1.msb)<<16) + (((int32_t) lsb.hi)<<8) + (((int32_t) lsb.lo));};

    const char*getUnitString() const;

    unsigned long getAbsValue() const;
    long getValue() const;
    int8_t getValueExponent() const;
    double getValueAsDouble() const;
    char *getValueAsString(char *buffer) const; 
    char *getResultAsString(char *buffer) const;

    unsigned long getAbsValueER() const;
    long getValueER() const;
    double getValueAsDoubleER() const;
    char *getValueAsStringER(char *buffer) const; 
    char *getResultAsStringER(char *buffer) const;

    
  }; ///< Structure designed to pack a number of flags into one byte


    struct K197ct_byte0 {
        /*!
           @brief control frame byte 0
        */
        union {
            uint8_t byte0; ///< allows access to all the flags in the
                           ///< union as one byte as uint8_t
            struct {
                K197range   range : 3;   ///< measurement range 
                bool    set_range : 1;   ///< when true ask to set the range
                bool    relative  : 1;   ///< true = relative mode (false = absolute) 
                bool    set_rel   : 1;   ///< when true ask to set the range
                bool    dB        : 1;   ///< true = dB mode (false = Volt mode)
                bool    set_db    : 1;   ///< when true ask to set dB or Volt
            };
        } __attribute__((packed)); ///<
    }; ///< Structure designed to pack a number of flags into one byte

    enum K197triggerMode {
        invalid_000    = 0b000,
        invalid_001    = 0b001,
        T0_Cont_onTALK = 0b010,
        T1_Once_onTALK = 0b011,
        T2_Cont_onGET  = 0b010, // Alias of T0
        T3_Once_onGET  = 0b011, // Alias of T1
        invalid_101    = 0b101,
        T4_Cont_onX    = 0b110,
        T5_Once_onX    = 0b111,

        // the following is used to trigger the measurement in modes T0-T3 
        // note that in mode T4 and T5 any control frame will act as a trigger
        T_TALK         = 0b100,
        T_GET          = 0b100, // alias of T_TALK

        // the following are just shorter aliases of the Tx_ long names above
        T0 = 0b010,
        T1 = 0b011,
        T2 = 0b010, // Alias of T0
        T3 = 0b011, // Alias of T1
        T4 = 0b110,
        T5 = 0b111,

        // the following bitmaps can be or-ed together to define the required mode combination
        T_Continuos_bm  = 0b000,
        T_Once_bm       = 0b001,
        T_TALK_bm    = 0b010,
        T_GET_bm     = 0b010, // alias of T_on_TALK_bm
        T_X_bm       = 0b100,
    };

    struct K197ct_byte1 {
        /*!
           @brief control frame byte 1
        */
        union {
            uint8_t byte1; ///< allows access to all the flags in the
                           ///< union as one byte as uint8_t
            struct {
                K197triggerMode   trigger : 3;   ///< trigger mode/status
                bool    set_trigger: 1; ///< when true change trigger mode/status
                bool    undefined4 : 1; ///< unknown use, normally set to 1
                bool    ctrl_mode  : 1; ///< true = remote (false = local)
                bool    undefined6 : 1; ///< unknown function
                bool    set_ctrl_mode : 1; ///< when true ask to set ctrl_mode
            };
        } __attribute__((packed)); ///<
    }; ///< Structure designed to pack a number of flags into one byte

    struct K197ct_byte2 {
        /*!
           @brief control frame byte 2
        */
        union {
            uint8_t byte2; ///< allows access to all the flags in the
                           ///< union as one byte as uint8_t
            struct {
                bool    undefined0_4  : 5;   ///< unknown function
                bool    sent_readings : 1; ///< true=send stored (false=send display)
                bool    undefined6  : 1;   ///< unknown function
                bool    set_sent_readings : 1; ///< when true ask to set sent_readings  
            };
        } __attribute__((packed)); ///<
    }; ///< Structure designed to pack a number of flags into one byte

    struct K197control {
        static const size_t valueAsStringMinSize;
        static const size_t resultAsStringMinSize;
        static const size_t valueAsStringMinSizeER;
        static const size_t resultAsStringMinSizeER;
    
        /*!
            @brief control frame
        */
        union {
            uint8_t frameBuffer[1]; ///< allows access to all the data as uint8_t array

            struct {
                K197ct_byte0 byte0;   
                K197ct_byte1 byte1;
                K197ct_byte2 byte2;
                uint8_t byte3;
                uint8_t byte4;   
            };    
        } __attribute__((packed)); ///<

        void clear() {
            byte0.byte0 = 0x00;
            byte1.byte1 = 0x00;
            byte2.byte2 = 0x00;
            byte3 = 0x00;
            byte4 = 0x00;
        };

        void setRange(K197range range);
        void setRelative(bool isRelative=true);
        void setAbsolute(bool isAbsolute=false) {setRelative(!isAbsolute);};
        void setDbMode(bool is_dB=true);
        void setTriggerMode(K197triggerMode triggerMode);
        void setRemoteMode(bool isRemote=true) ;
        void setLocalMode(bool isLocal=true) {setRemoteMode(!isLocal);};
        void setSendStoredReadings(bool sendStored=true);
        void setSendDisplayReadings(bool sendDisplay=true){setSendStoredReadings(!sendDisplay);};
    };

  public:
    bool begin();
    bool begin(K197measurement *newInputBuffer);
    bool begin(K197measurement *newInputBuffer, K197control *newOutputBuffer);

    void update() {
        if (outputQueued && isFrameEndDetected() && noOutputPending()) {
            sendImmediately(); 
            outputQueued = false;
        }
        GeminiFrame::update();
    }


    K197measurement *getMeasurementBuffer() const {
        return inputBuffer;
    };
    void setMeasurementBuffer(K197measurement *newInputBuffer, bool resetBuffer=false) {
        inputBuffer=newInputBuffer;
        setInputBuffer((uint8_t *) newInputBuffer, sizeof(K197measurement)/sizeof(uint8_t), resetBuffer);
    };
    
    K197control *getControlBuffer() const {
        return outputBuffer;
    };
    void setControlBuffer(K197control *newOutputBuffer, bool resetBuffer=false) {
        outputBuffer=newOutputBuffer;
        if (resetBuffer) outputBuffer->clear();
        outputQueued = false;
    };
    
    bool sendImmediately(K197control *bufferToSend, bool resetAfterSending=true) {
        if (bufferToSend == NULL) return false;
        sendFrame((uint8_t *) bufferToSend, sizeof(K197control)/sizeof(uint8_t));      
        if (resetAfterSending)  bufferToSend->clear();
        return true;
    };
    bool sendImmediately(bool resetAfterSending=true) { 
        return sendImmediately(outputBuffer, resetAfterSending); 
        outputQueued=false;
    };
    
    void execute() {
        if (outputBuffer == NULL) return;
        outputQueued=true;
    };

    bool serverStartup();

  private: 
    K197measurement *inputBuffer=NULL;
    K197control *outputBuffer=NULL;

  protected:
     using GeminiFrame::sendFrame;
     using GeminiFrame::begin;
     bool outputQueued=false;
    
};

#endif //K197CTRL_GEMINI_K197_CONTROL_H
