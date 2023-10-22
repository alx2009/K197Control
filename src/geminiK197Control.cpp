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

const size_t GeminiK197Control::K197measurement::valueAsStringMinSize = 12;
const size_t GeminiK197Control::K197measurement::resultAsStringMinSize = 16;
const size_t GeminiK197Control::K197measurement::valueAsStringMinSizeEP = 14;
const size_t GeminiK197Control::K197measurement::resultAsStringMinSizeEP = 18;

static double range_power[] {1e-10, 1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1.0, 1e1, 1e2, 1e3};
static int8_t range_exponent[] {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8};
static int8_t range_baseline[] {3, 6, 0, 0};

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
//       Standard precision functions
///////////////////////////////////////////////////////////////////////////////////////////

unsigned long GeminiK197Control::K197measurement::getAbsValue() const { 
    uint64_t uuvalue = getCount();
    uuvalue = uuvalue * 3125;  // multiply first to avoid losing precision. This is why we need a 64 bit integer...
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
//       Extended precision functions
///////////////////////////////////////////////////////////////////////////////////////////

unsigned long GeminiK197Control::K197measurement::getAbsValueEP() const {
    uint64_t uuvalue = getCount();
    uuvalue = uuvalue * 78125;  // multiply first to avoid losing precision. This is why we need a 64 bit integer...
    uuvalue = uuvalue / 4096;
    return uuvalue;
}

long GeminiK197Control::K197measurement::getValueEP() const {
    return byte1.negative ? -getAbsValueEP() : getAbsValueEP();
}

double GeminiK197Control::K197measurement::getValueAsDoubleEP() const {
    return double(getValueEP()) * range_power[range_baseline[byte0.unit]+ byte0.range] * 0.01;  
}

char *GeminiK197Control::K197measurement::getValueAsStringEP(char *buffer) const {
    char *tmpbuf = buffer;
    uint32_t uvalue = getAbsValueEP();
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
 
char *GeminiK197Control::K197measurement::getResultAsStringEP(char *buffer) const {
    char *tmpbuf = buffer;
    tmpbuf[0] = byte1.ovrange ? 'O' : isZero() ? 'Z' : 'N';
    tmpbuf++;
    strcpy(tmpbuf, getUnitString()); 
    tmpbuf+=3;
    getValueAsStringEP(tmpbuf);
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
