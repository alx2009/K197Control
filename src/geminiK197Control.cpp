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
#include "GeminiK197Control.h"

static GeminiK197Control::K197measurement defaultMeasurementResult;
static GeminiK197Control::K197control defaultControlRequest;

/*!
     @brief  initialize the object.

     @details begin should be called before using the object
     
     When begin is called without any argument, global default measurement (input) and control (output) objects are used. 
     Note that all instances of the object will share the same global default objects

     PREREQUISITES: Serial.begin must be called to see any error message

     @return true if the call was succesful and the object can be used, false otherwise
*/
bool GeminiK197Control::begin() {
    return begin(&defaultMeasurementResult, &defaultControlRequest);
}

/*!
     @brief  initialize the object.

     @details begin should be called before using the object
     
     When begin is called with a single argument, NO control (output) buffer is assigned. 
     This can be useful in case the object is only used for retrieving measurement result. 
     Alternatively, a control buffer can be set with setControlBuffer(), 
     or the application can include a control object directly in sendImmediately.

     @param newInputBuffer pointer to a measurement (input) buffer to receive measurement results
     @return true if the call was succesful and the object can be used, false otherwise
*/
bool GeminiK197Control::begin(K197measurement *newInputBuffer) {
    setControlBuffer(NULL, false);      
    inputBuffer=newInputBuffer; 
    return GeminiFrame::begin( (uint8_t *) inputBuffer, sizeof(K197measurement)/sizeof(uint8_t));  
}

/*!
     @brief  initialize the object.

     @details begin should be called before using the object
     
     When begin is called with two arguments, it is possible to use sendImmediately() without arguments 
     and/or execute() to send a control frame to the K197. 
     The application can also include a different control object directly in sendImmediately.

     @param newInputBuffer pointer to a measurement (input) buffer to receive measurement results
     @param newOutputBuffer pointer to a control (output) buffer that will be used as default for this object
     @return true if the call was succesful and the object can be used, false otherwise
*/
bool GeminiK197Control::begin(K197measurement *newInputBuffer, K197control *newOutputBuffer) {
    setControlBuffer(newOutputBuffer, true);      
    inputBuffer=newInputBuffer; 
    return GeminiFrame::begin( (uint8_t *) inputBuffer, sizeof(K197measurement)/sizeof(uint8_t));  
}

/****************************************************************************
***********          MEASUREMENT RESULT STRUCTURE                *************
*****************************************************************************/

const size_t GeminiK197Control::K197measurement::valueAsStringMinSize = 12;    ///< char[] lenght required by getValueAsString (incl. term. NULL)
const size_t GeminiK197Control::K197measurement::resultAsStringMinSize = 16;   ///< char[] lenght required by getResultAsString (incl. term. NULL)
const size_t GeminiK197Control::K197measurement::valueAsStringMinSizeER = 14;  ///< char[] lenght required by getValueAsStringER (incl. term. NULL)
const size_t GeminiK197Control::K197measurement::resultAsStringMinSizeER = 18; ///< char[] lenght required by getResultAsStringER (incl. term. NULL)

static double range_power[] {1e-10, 1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1.0, 1e1, 1e2, 1e3}; ///< used for internal calculations
static int8_t range_exponent[] {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8}; ///< used for internal calculations
static int8_t range_baseline[] {3, 6, 0, 0}; ///< used for internal calculations

const char*GeminiK197Control::K197measurement::getUnitString() const {
    switch (byte0.unit) {
        case K197unit::Volt : return byte0.ac_dc ? "ACV" : "DCV";      
        case K197unit::Ohm  : return "OHM";     
        case K197unit::Amp  : return byte0.ac_dc ? "ACA" : "DCA";      
        case K197unit::dB   : return byte0.ac_dc ? "ACD" : "DCD";      
    }
    return ""; // remove warning about missing return statement
}

int8_t GeminiK197Control::K197measurement::getValueExponent() const {
    return range_exponent[range_baseline[byte0.unit]+byte0.range];
}

///////////////////////////////////////////////////////////////////////////////////////////
//       Standard resolution functions
///////////////////////////////////////////////////////////////////////////////////////////

unsigned long GeminiK197Control::K197measurement::getAbsValue() const { 
    uint64_t uuvalue = getCount();
    uuvalue = uuvalue * 3125;  // multiply first to avoid losing accuracy. This is why we need a 64 bit integer...
    uuvalue = uuvalue / 16384;
    return uuvalue;
}

long GeminiK197Control::K197measurement::getValue() const {
    return byte1.negative ? -getAbsValue() : getAbsValue();
}

double GeminiK197Control::K197measurement::getValueAsDouble() const {
    return double(getValue()) * range_power[range_baseline[byte0.unit]+ byte0.range];
}

char * GeminiK197Control::K197measurement::getValueAsString(char *buffer) const {
    char *tmpbuf = buffer;
    uint32_t uvalue = getAbsValue();
    tmpbuf[0] = isNegative() ? '-' : '+';
    tmpbuf++;
    uint32_t digit6 = uvalue / 100000UL;
    tmpbuf[0]= '0' + digit6;  
    tmpbuf++;
    uvalue-= digit6*100000UL;
    tmpbuf[0]= '.';
    tmpbuf++;
    // print leading zeros
    for( uint32_t zlim = 10000; zlim > 10; zlim/=10) {
        if (uvalue < zlim) {
            tmpbuf[0] = ('0');
            tmpbuf++;  
        }
    } 
    ultoa(uvalue, tmpbuf, 10);
    return buffer;
}

char * GeminiK197Control::K197measurement::getResultAsString(char *buffer) const {
    char *tmpbuf = buffer;
    tmpbuf[0] = byte1.ovrange ? 'O' : isZero() ? 'Z' : 'N';
    tmpbuf++;
    strcpy(tmpbuf, getUnitString()); 
    tmpbuf+=3;
    getValueAsString(tmpbuf);
    tmpbuf+=8;
    tmpbuf[0] = 'E';
    tmpbuf++;
    int8_t e = getValueExponent();
    tmpbuf[0]= e>= 0 ? '+' : '-';
    tmpbuf++;
    tmpbuf[0]= '0' + abs(e);
    tmpbuf++;
    tmpbuf[0]=0;
    
    return buffer;
}

///////////////////////////////////////////////////////////////////////////////////////////
//       Extended resolution functions
///////////////////////////////////////////////////////////////////////////////////////////

unsigned long GeminiK197Control::K197measurement::getAbsValueER() const {
    uint64_t uuvalue = getCount();
    uuvalue = uuvalue * 78125;  // multiply first to avoid losing accuracy. This is why we need a 64 bit integer...
    uuvalue = uuvalue / 4096;
    return uuvalue;
}

long GeminiK197Control::K197measurement::getValueER() const {
    return byte1.negative ? -getAbsValueER() : getAbsValueER();
}

double GeminiK197Control::K197measurement::getValueAsDoubleER() const {
    return double(getValueER()) * range_power[range_baseline[byte0.unit]+ byte0.range] * 0.01;  
}

char *GeminiK197Control::K197measurement::getValueAsStringER(char *buffer) const {
    char *tmpbuf = buffer;
    uint32_t uvalue = getAbsValueER();
    tmpbuf[0] = isNegative() ? '-' : '+';
    tmpbuf++;
    uint32_t digit8 = uvalue / 10000000UL;
    tmpbuf[0]= '0' + digit8;  
    tmpbuf++;
    uvalue-= digit8*10000000UL;
    tmpbuf[0]= '.';
    tmpbuf++;
    // print leading zeros
    for( uint32_t zlim = 1000000; zlim > 10; zlim/=10) {
        if (uvalue < zlim) {
            tmpbuf[0] = ('0');
            tmpbuf++;  
        }
    } 
    ultoa(uvalue, tmpbuf, 10);
    return buffer;  
}
 
char *GeminiK197Control::K197measurement::getResultAsStringER(char *buffer) const {
    char *tmpbuf = buffer;
    tmpbuf[0] = byte1.ovrange ? 'O' : isZero() ? 'Z' : 'N';
    tmpbuf++;
    strcpy(tmpbuf, getUnitString()); 
    tmpbuf+=3;
    getValueAsStringER(tmpbuf);
    tmpbuf+=10;
    tmpbuf[0] = 'E';
    tmpbuf++;
    int8_t e = getValueExponent();
    tmpbuf[0]= e>= 0 ? '+' : '-';
    tmpbuf++;
    tmpbuf[0]= '0' + abs(e);
    tmpbuf++;
    tmpbuf[0]=0;
    
    return buffer;  
}

/*****************************************************************************
******************             CONTROL STRUCTURE                  ************
*****************************************************************************/

void GeminiK197Control::K197control::setRange(K197range range) {
    byte0.range = range;
    byte0.set_range = true;
}

void GeminiK197Control::K197control::setRelative(bool isRelative) {
    byte0.relative = isRelative;
    byte0.set_rel = true;
}

void GeminiK197Control::K197control::setDbMode(bool is_dB) {
    byte0.dB = is_dB;
    byte0.set_db = true;
}

void GeminiK197Control::K197control::setTriggerMode(K197triggerMode triggerMode) {
    byte1.trigger = triggerMode;
    byte1.set_trigger = true;
}

void GeminiK197Control::K197control::setRemoteMode(bool isRemote) {
  byte1.ctrl_mode = isRemote;
  byte1.set_ctrl_mode = true;  
}

void GeminiK197Control::K197control::setSendStoredReadings(bool sendStored) {
  byte2.sent_readings = sendStored;
  byte2.set_sent_readings = true;  
}

bool GeminiK197Control::serverStartup(unsigned long timeout_micros) {
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
