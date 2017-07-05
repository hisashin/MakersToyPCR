#ifndef ___BOARD_CONF_NINJAPCR___
#define ___BOARD_CONF_NINJAPCR___
/* Board Config */
#define USE_ESP8266
#define USE_WIFI /* Use WiFi functionalities */
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
#define PIN_LID_PWM 2 // TODO Switch PWM/OnOff
#define USE_ESP8266

/* Well */
#define PIN_WELL_INA 0
#define PIN_WELL_INB 4
#define PIN_WELL_PWM 15

/* SPI */
#define PIN_WELL_DATAOUT 13//MOSI
#define PIN_WELL_DATAIN  12//MISO
#define PIN_WELL_SPICLOCK  14//sck
#define PIN_WELL_SLAVESELECT 5//ss

#endif /* ___BOARD_CONF_NINJAPCR___ */
