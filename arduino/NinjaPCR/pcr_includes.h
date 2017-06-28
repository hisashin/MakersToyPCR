/*
 *  pcr_includes.h - OpenPCR control software.
 *  Copyright (C) 2010-2012 Josh Perfetto. All Rights Reserved.
 *
 *  OpenPCR control software is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenPCR control software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  the OpenPCR control software.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PCR_INCLUDES_H_
#define _PCR_INCLUDES_H_

#define USE_ESP8266
/* Use WiFi functionalities */
#define USE_WIFI

/* Use LCD */
#define USE_LCD

#ifdef USE_LCD
#define LCD_PIN_RS 6
#define LCD_PIN_ENABLE 7
#define LCD_PIN_D4 8
//#define LCD_PIN_D5 A5
#define LCD_PIN_D5 5 // TMP
#define LCD_PIN_D6 16
#define LCD_PIN_D7 17
#endif

//#define DEBUG_DISPLAY

#define SERIAL_PREFIX "pcr"
#define OPENPCR_FIRMWARE_VERSION_STRING "1.0.5"


#include "Arduino.h"

#ifndef USE_WIFI
#include <avr/pgmspace.h>
#endif

#define PLATE_FAST_RAMP_THRESHOLD_MS 1000

class Thermocycler;
extern Thermocycler* gpThermocycler;
inline Thermocycler& GetThermocycler() { return *gpThermocycler; }

//fixes for incomplete C++ implementation, defined in util.cpp
void* operator new(size_t size);
void operator delete(void * ptr);
extern "C" void __cxa_pure_virtual(void);

//defines
#define STEP_NAME_LENGTH       14
#define MAX_CYCLE_ITEMS        16
#define MAX_COMMAND_SIZE      256

enum PcrStatus {
  ESuccess = 0,
  ETooManySteps = 32,
  ENoProgram,
  ENoPower
};

#define SUCCEEDED(status) (status == ESuccess)

void sprintFloat(char* str, float val, int decimalDigits, boolean pad);
unsigned short htons(unsigned short val);
double absf(double val);
char* rps(const char* progString);

#endif

