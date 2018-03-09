#ifndef ___ADC_H___
#define ___ADC_H___

#include <stdint.h>

uint8_t initADC ();
// Return (ADC value)/(ADC resolution)
float getADCValue ();

#endif /* ___ADC_H___ */
