#ifndef ___BOARD_CONF_NINJAPCR___
#define ___BOARD_CONF_NINJAPCR___

#define BAUD_RATE 115200
/* Board Config */
#define USE_ESP8266

#define USE_FAN
#define PIN_FAN 13

// Thermistor value is measured by 2 ranges, switched by analog switch
#define THERMISTOR_2_RANGES
#define PIN_THERMISTOR_RANGE_SWITCH 16 /* Analog switch */

#define USE_WIFI /* Use WiFi functionalities */
#define PIN_WIFI_MODE 16 /* Analog pin to  */
#define VALUE_WIFI_MODE_AP HIGH /* Start with AP mode if value of PIN_WIFI_MODE agrees with this.  */
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
#define PIN_WELL_INA 2
#define PIN_WELL_INB 0
#define PIN_WELL_PWM 4

/* SPI */
#define PIN_WELL_DATAOUT 13//MOSI (Not used)
#define PIN_WELL_DATAIN  12//MISO
#define PIN_WELL_SPICLOCK  14//sck
#define PIN_WELL_SLAVESELECT 5//ss


#define PIN_SPI_SCK 14 /* HSPI_CLK */
#define PIN_SPI_SDO 12 /* HSPI_MISO */
#define PIN_SPI_CS 5 /* GPIO */
#endif /* ___BOARD_CONF_NINJAPCR___ */
