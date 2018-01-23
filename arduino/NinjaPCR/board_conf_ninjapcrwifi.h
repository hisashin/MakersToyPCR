#ifndef ___BOARD_CONF_NINJAPCR___
#define ___BOARD_CONF_NINJAPCR___

#define BAUD_RATE 115200
/* Board Config */
#define USE_ESP8266

/* ADC (NAU7803 or MCP3554) */
//#define USE_ADC_NAU7803
#define USE_ADC_MCP3554
#define TEHRMISTORS_NINJAPCR

#define USE_FAN
#define PIN_FAN 13

// Thermistor value is measured by 2 ranges, switched by analog switch
#define THERMISTOR_2_RANGES
#define PIN_THERMISTOR_RANGE_SWITCH 16 /* Analog switch */

#define USE_WIFI /* Use WiFi functionalities */
#define PIN_WIFI_MODE 16 /* Digitai input pin to switch AP (pairing) mode and Normal mode  */
#define VALUE_WIFI_MODE_AP HIGH /* Start with AP (pairing) mode if value of PIN_WIFI_MODE agrees with this.  */
// #define USE_LCD /* Use display */
// #define USE_STATUS_PINS /* Use status LEDs */
// #define DEBUG_DISPLAY

/* Use LCD */

/* LCD Pins (Dummy) */
#define PIN_LCD_RS 6
#define PIN_LCD_ENABLE 7
#define PIN_LCD_D4 8
//#define PIN_LCD_D5 A5
#define PIN_LCD_D5 5 // TMP
#define PIN_LCD_D6 16
#define PIN_LCD_D7 17
#define PIN_LCD_CONTRAST 5

/* Lid */
#define PIN_LID_THERMISTOR_AIN 1 // TODO Use TOUT
#define PIN_LID_PWM 15 // PWM is available
#define USE_ESP8266

/* Well */
#ifdef  USE_ADC_NAU7803
#define PIN_WELL_INA 2
#endif
#ifdef USE_ADC_MCP3554
#define PIN_WELL_INA 12
#endif
#define PIN_WELL_INB 0
#define PIN_WELL_PWM 4
#define PIN_WELL_HIGH_TEMP 16 /* TODO implement thermistor switching */

/* SPI */
#define PIN_WELL_MCP3554_DATAOUT 13//MOSI (Not used)
#define PIN_WELL_MCP3554_DATAIN  12//MISO
#define PIN_WELL_MCP3554_SPICLOCK  14//sck
#define PIN_WELL_MCP3554_SLAVESELECT 5//ss


/* SPI */
#ifdef USE_ADC_MCP3554
#define PIN_WELL_MCP3554_DATAOUT 13//MOSI (Not used)
#define PIN_WELL_MCP3554_DATAIN  12//MISO
#define PIN_WELL_MCP3554_SPICLOCK  14//sck
#define PIN_WELL_MCP3554_SLAVESELECT 5//ss
#endif /* USE_ADC_MCP3554 */

#ifdef USE_ADC_NAU7803
#define PIN_WELL_NAU7803_SCL
#define PIN_WELL_NAU7803_SDA
#define PIN_WELL_NAU7803_RDY
#endif /* USE_ADC_NAU7803 */

#endif /* ___BOARD_CONF_NINJAPCR___ */
