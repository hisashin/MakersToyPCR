#ifndef ___ADC_H___
#define ___ADC_H___

#include <stdint.h>

uint8_t initADC ();
// Return (ADC value)/(ADC resolution)
typedef uint8_t adc_result;
adc_result getWellADCValue (float *val);
adc_result getLidADCValue (float *val);

#define ADC_NO_ERROR 0
#define ADC_TIMEOUT 1

#endif /* ___ADC_H___ */
