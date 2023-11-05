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

/*!
      @brief handles communications with a K197 voltmeter using the interface used by the IEEE-488 option card

      @details the K197 can be equipped with an optional IEEE-488 card. This card communicates with the K197 main board 
      with a 2 wire interface. This class implements the application layer of this 2 wire interface.

      The class uses the gemini protocol inherithed by the base classes to receive measurement results and send control commands to the K197.

      After construction, begin() must be called before using any other function. Then update() must be called on a regular basis.
      
      When a measurement is received, it is stored in a K197measurement buffer. FrameComplete() in the base class can be used to know 
      when a new frame has been received. One of the methods resetFrame() or getFrame() in the base class must be called 
      before a new frame can be received.

      To send control commands two methods can be used:
      - execute() queues the commands currently stored in the current control structure to be sent as soon as possible as a new frame. 
      This is the recommended (and safer) way to control the k197. The current control buffer is the one specified at begin() or using setControlBuffer
      - sendImmediately() will queue the specified control buffer structure immediately. 
      With this method the caller must make sure that there isn't a transmission already queued or ongoing, otherwise the effect can be unpredictable.
      It is recommended that the caller make sure both isFrameEndDetected() and noOutputPending() return true before calling sendImmediately().
*/
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

    /*!
        @brief  Define the measurement unit 
    */
    enum K197unit {
        Volt     = 0b00, ///< Volt
        Amp      = 0b10, ///< Ampere
        Ohm      = 0b01, ///< Ohm
        dB       = 0b11, ///< decibel

    };

    /*!
        @brief  Define byte 0 of the measurement structure
    */
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

    /*!
        @brief  Define byte 1 of the measurement structure
    */
    struct K197mr_byte1 {
        /*!
           @brief measurement result frame byte 1
        */
        union {
            uint8_t byte1; ///< allows access to all the flags in the
                           ///< union as one byte as uint8_t
            struct {
                uint8_t msb       : 5;   ///< binary count, bits 16-20
                bool    ovrange   : 1;   ///<  when true indicates overrange 
                bool    undefined : 1;   ///< unknown use, normally set to 0
                bool negative     : 1;   ///< measurement unit
            };

        } __attribute__((packed)); ///<
    }; ///< Structure designed to pack a number of flags into one byte

    /*!
        @brief  Define the range. 
        @details Valid for both measurements and control frame.
        When controlling the K197, only Volt and Ampere can be changed programmatically
        When reading the measurement results, the range reflects the actual measurement range
        (auto range is never returned)
        See the instruction manual for the K197 IEEE-488 for more information
    */
    enum K197range {
        R0_Auto       = 0b000,  ///< send: auto range
        R1_200mV_Ohm  = 0b001,  ///< send: 200 mV/Ohm receive: 200 mV/Ohm/uA
        R2_2V_KOhm    = 0b010,  ///< send: 2 V/KOhm receive: 2 V/KOhm/mA
        R3_20V_KOhm   = 0b011,  ///< send: 20 V/KOhm receive: 20 V/KOhm/mA
        R4_200V_KOhm  = 0b100,  ///< send: 200 V/KOhm receive: 200 V/KOhm/mA
        R5_1KV_2MOhm  = 0b101,  ///< send: 2000V, >2MOhm. receive: 2000V, 2MOhm, 10A
        R6_20MOhm     = 0b110,  ///< receive: 20MOhm, 10A 
        R7_200MOhm    = 0b111,  ///< receive: 200MOhm

        // the following are just shorter aliases of the long names above
        R0  = 0b000, ///< alias of R0_Auto
        R1  = 0b001, ///< alias of R1_200mV_Ohm
        R2  = 0b010, ///< alias of R2_2V_KOhm
        R3  = 0b011, ///< alias of R3_20V_KOhm
        R4  = 0b100, ///< alias of R4_200V_KOhm
        R5  = 0b101, ///< alias of R5_1KV_2MOhm
        R6  = 0b110, ///< alias of R6_20MOhm
        R7  = 0b111, ///< alias of R7_200MOhm
    };

    /*!
        @brief  the measurement data sent by the K197  
        @details this is a four byte data structure to access the measurement results from the K197
        @data all data can be accessed via the data members, but using the provided methods is probably more convenient 
    */
    struct K197measurement {
        static const size_t valueAsStringMinSize; 
        static const size_t resultAsStringMinSize;
        static const size_t valueAsStringMinSizeER;
        static const size_t resultAsStringMinSizeER;
    
    /*!
       @brief the measurement frame
    */
    union {
      uint8_t frameBuffer[1]; ///< allows access to all the data as uint8_t array

      struct {
          K197mr_byte0 byte0;   ///< byte 0
          K197mr_byte1 byte1;   ///< byte 1
          //uint16_t lsb;
          struct {
              uint8_t hi;       ///< binary count, bits 8-15
              uint8_t lo;       ///< binary count, bits 0-7 
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

    /*!
       @brief get the binary count
       @return binary count (no sign), ranges from 0x000000 to x1FFFFF, corresponding to the K197 display visualizing 0 to 400000 
    */
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

  }; // Structure designed to pack a number of flags into one byte


    /*!
        @brief  Define byte 0 of the control structure
    */
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
                bool    set_rel   : 1;   ///< when true ask to set the relative mode
                bool    dB        : 1;   ///< true = dB mode (false = Volt mode)
                bool    set_db    : 1;   ///< when true ask to set dB or Volt
            };
        } __attribute__((packed)); ///<
    }; // Structure designed to pack a number of flags into one byte

    /*!
        @brief  Define the trigger mode. 
        @details we define a named value for each of the trigger mode mentioned in the 
        nstruction manual for the K197 IEEE-488
        However, there is no difference between TALK and GET on the internal interface. 
        See the instruction manual for the K197 IEEE-488 for more information
        In addition, we define other constants that are used 
    */
    enum K197triggerMode {
        invalid_000    = 0b000, ///< not used
        invalid_001    = 0b001, ///< not used
        T0_Cont_onTALK = 0b010, ///< when set, send T_TALK to trigger
        T1_Once_onTALK = 0b011, ///< when set, send T_TALK to trigger
        T2_Cont_onGET  = 0b010, // Alias of T0
        T3_Once_onGET  = 0b011, // Alias of T1
        invalid_101    = 0b101, ///< not used
        T4_Cont_onX    = 0b110, ///< when set, send any control frame to trigger
        T5_Once_onX    = 0b111, ///< when set, send any control frame to trigger

        // the following is used to trigger the measurement in modes T0-T3 
        // note that in mode T4 and T5 any control frame will act as a trigger
        T_TALK         = 0b100,
        T_GET          = 0b100, // alias of T_TALK

        // the following are just shorter aliases of the Tx_ long names above
        T0 = 0b010, // Alias of T0
        T1 = 0b011, // Alias of T1
        T2 = 0b010, // Alias of T0
        T3 = 0b011, // Alias of T1
        T4 = 0b110, // Alias of T4
        T5 = 0b111, // Alias of T5

        // the following bitmaps can be or-ed together to define the required mode combination
        T_Continuos_bm  = 0b000,  // use for continuos modes
        T_Once_bm       = 0b001,  // use for one-shot modes
        T_TALK_bm    = 0b010,     // use for modes triggered on TALK
        T_GET_bm     = 0b010,     // alias of T_on_TALK_bm
        T_X_bm       = 0b100,     // use for modes triggered on GET
    };

    /*!
        @brief  Define byte 1 of the control structure
    */
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

    /*!
        @brief  Define byte 2 of the control structure
    */
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

    /*!
        @brief  the control data sent to the K197  
        @details this is a five byte data structure used to control the K197
        @data all data can be accessed via the data members, but using the provided methods is probably more convenient 
    */
    struct K197control {
        /*!
            @brief the control frame
        */
        union {
            uint8_t frameBuffer[1]; ///< allows access to all the data as uint8_t array

            struct {
                K197ct_byte0 byte0;   ///< byte 0 of the control frame
                K197ct_byte1 byte1;   ///< byte 1 of the control frame
                K197ct_byte2 byte2;   ///< byte 2 of the control frame
                uint8_t byte3;        ///< byte 3 of the control frame
                uint8_t byte4;        ///< byte 4 of the control frame
            };    
        } __attribute__((packed)); // packs everything together

        /*!
            @brief reset a frame to an empty request
            @sending the empty request to the K197 will not do anything 
            except trigger when the voltmeter is in trigger mode T4 or T5
        */
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

   /*!
       @brief  main input/output handler

       @details in this function we update the internal state machine, reading and writing data as required 
       by the K197

       The gemini protocol does not require a strict timing at either side, however update() should be called as often 
       as possible (at least once every loop() iteraction) to achieve maximum possible throughput and lowest possible latency

       If this function is not called for a significant amount of time, the protocol may time-out and abort the current frame transmission
   */
    void update() {
        if (outputQueued && isFrameEndDetected() && noOutputPending()) {
            sendImmediately(); 
            outputQueued = false;
        }
        GeminiFrame::update();
    }

    /*!
        @brief get the current measurement buffer
        @return a pointer to the current meaasurement buffer
    */
    K197measurement *getMeasurementBuffer() const {
        return inputBuffer;
    };
    /*!
        @brief set the measurement buffer
        @details the new buffer will receive the result of the measurements from the K197
        @param newInputBuffer a pointer to a meaasurement buffer
        @param resetBuffer if true the buffer is reset (same as calling newInputBuffer.clear()). Default is false
    */
    void setMeasurementBuffer(K197measurement *newInputBuffer, bool resetBuffer=false) {
        inputBuffer=newInputBuffer;
        setInputBuffer((uint8_t *) newInputBuffer, sizeof(K197measurement)/sizeof(uint8_t), resetBuffer);
    };
    
    /*!
        @brief get the current control buffer
        @return a pointer to the current control buffer
    */
    K197control *getControlBuffer() const {
        return outputBuffer;
    };
    /*!
        @brief set the control buffer
        @details the new buffer will be used by execute() and the variant of sendImmediately() 
        without a pointer to a K197control structure
        @param newOutputBuffer a pointer to a meaasurement buffer
        @param resetBuffer if true the buffer is reset (same as calling newInputBuffer.clear()). Default is false
    */
    void setControlBuffer(K197control *newOutputBuffer, bool resetBuffer=false) {
        outputBuffer=newOutputBuffer;
        if (resetBuffer) outputBuffer->clear();
        outputQueued = false;
    };
    
    /*!
        @brief send a control buffer immediately
        @details send a control buffer immediately using GeminiFrame::sendFrame(). 
        When using this method, the caller must make sure that there isn't a transmission already queued or ongoing, 
        otherwise the effect can be unpredictable. It is recommended that the caller make sure 
        both isFrameEndDetected() and noOutputPending() return true before calling sendImmediately().

        @param bufferToSend a pointer to the meaasurement buffer that should be sent
        @param resetAfterSending if true the buffer is reset after sending (same as calling bufferToSend.clear()). Default is true
    */
    bool sendImmediately(K197control *bufferToSend, bool resetAfterSending=true) {
        if (bufferToSend == NULL) return false;
        sendFrame((uint8_t *) bufferToSend, sizeof(K197control)/sizeof(uint8_t));      
        if (resetAfterSending)  bufferToSend->clear();
        return true;
    };

    /*!
        @brief send the current control buffer immediately
        @details When using this method, the caller must make sure that there isn't a transmission already queued or ongoing, 
        otherwise the effect can be unpredictable. It is recommended that the caller make sure 
        both isFrameEndDetected() and noOutputPending() return true before calling sendImmediately().

        @param resetAfterSending if true the buffer is reset after sending (same as calling getControlBuffer()->clear()). Default is true
   */
   bool sendImmediately(bool resetAfterSending=true) { 
        return sendImmediately(outputBuffer, resetAfterSending); 
        outputQueued=false;
    };
    
    /*!
        @brief transmit the current control buffer as soon as possible
        @details This is the safest and recommended method to send a control frame to the K197.
        When using this method, an internal flag is set to request transmission of the control frame.
        When this flag is set, update() will make sure the control buffer is sent to the lower layer (see GeminiFrame::sendFrame()), 
        making sure only one control buffer at a time can be transmitted.

        The current input buffer can be modified at any time until update() send the buffer with GeminiFrame::sendFrame(). 
        The information sent will reflect all the modifications until that point.

        If the sender want to know when update() has sent the buffer, it can checked executeComplete(). 
        When executeComplete() returns true, any change to the control buffer will be sent as as eparate frame. 

        Note that executeComplete() does not esnure that the information has been actually sent to the K197. this is up to the lower layers.
        In order to know that a sert of command has been sent to the K197, the following steps are recommended (see also note below):
        - make sure a control buffer is set in begin or with setControlBuffer()
        - make sure that the buffer includs all required commands and settings
        - call execute()
        - wait until executeComplete() returns true
        - wait until both isFrameEndDetected() and noOutputPending() return true

        Note: the application should continue to call update() between the steps above, otherwise the protocol will hang. 
    */
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
