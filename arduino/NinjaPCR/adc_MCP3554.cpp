#include <Arduino.h>
#include <SPI.h>
#include "adc.h"
#include "board_conf.h"

#ifdef USE_ADC_MCP3554

/* Implementation of USE_ADC_MCP3554 A/D Converter */
static bool isAdcInitialized = false;1
uint8_t initADC () {
    if (isAdcInitialized) {
        return 0;
    }
    isAdcInitialized = true;
    //spi setup
    pinMode(PIN_WELL_MCP3554_DATAOUT, OUTPUT);
    pinMode(PIN_WELL_MCP3554_DATAIN, INPUT);
    pinMode(PIN_WELL_MCP3554_SPICLOCK, OUTPUT);
    pinMode(PIN_WELL_MCP3554_SLAVESELECT, OUTPUT);
    digitalWrite(PIN_WELL_MCP3554_SLAVESELECT, HIGH); //disable device
    return 0;
}
// Return (ADC value)/(ADC resolution)
float getADCValue () {
    Serial.println("2_1???");
    /* ADC Start */
    digitalWrite(PIN_WELL_MCP3554_SLAVESELECT, LOW);

    //read data
    while(digitalRead(PIN_WELL_MCP3554_DATAIN)) {}

    uint8_t spiBuf[4];
    memset(spiBuf, 0, sizeof(spiBuf));

    digitalWrite(PIN_WELL_MCP3554_SLAVESELECT, LOW);
    for(int i = 0; i < 4; i++) {
      spiBuf[i] = SPI.transfer(0xFF);
    }

    unsigned long conv = (((unsigned long)spiBuf[3] >> 7) & 0x01) + ((unsigned long)spiBuf[2] << 1) + ((unsigned long)spiBuf[1] << 9) + (((unsigned long)spiBuf[0] & 0x1F) << 17); //((spiBuf[0] & 0x1F) << 16) + (spiBuf[1] << 8) + spiBuf[2];

    unsigned long adcDivisor = 0x1FFFFF;
    float voltage = (float)conv /* * 5.0 */ / adcDivisor;
    digitalWrite(PIN_WELL_MCP3554_SLAVESELECT, HIGH);
    /* ADC End */
    return voltage;

}
#endif /* USE_ADC_MCP3554 */
