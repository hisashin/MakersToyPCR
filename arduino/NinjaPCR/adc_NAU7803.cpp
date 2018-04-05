#include "adc.h"
#include "board_conf.h"
#include <Arduino.h>
#include <Wire.h>

#ifdef USE_ADC_NAU7803

#define NAU7802_DEVICE_ADDRESS  /*(0x2A<<1)*/0x2A
#define NAU7802_REG_PU_CTRL_ADDRESS 0x00 // Initialization
#define NAU7802_REG_ADCO_B2_ADDRESS 0x12 // ADC value (Followed by next 2 registers)
#define NAU7802_REG_CTRL2_ADDRESS   0x02 // Calibration
#define NAU7802_REG_REVISION  0xFF

#define NAU7802_BIT_RR 0
#define NAU7802_BIT_PUD 1
#define NAU7802_BIT_PUR 3
#define NAU7802_BIT_CRS 4

#define NAU7802_RATE_320SPS 0b0111
#define NAU7802_RATE_80SPS 0b0011
#define NAU7802_RATE_40SPS 0b0010
#define NAU7802_RATE_20SPS 0b0001
#define NAU7802_RATE_10SPS 0b0000 //default

#define NO_ERR 0x00
/*
 *
 #define PIN_WELL_NAU7803_SCL
 #define PIN_WELL_NAU7803_SDA
 #define PIN_WELL_NAU7803_RDY
 */
/* Implementation of NAU7803 A/D Converter */
char i2c_err;
static char wellADCWriteRegVal (uint8_t reg_address, char *data,
        uint8_t dataSize)
{
    //char *write_block = (char*)malloc(sizeof(char)*16);
    int i;
    //write_block[0] = reg_address;
    Wire.write(reg_address);
    for (i = 0; i < dataSize; i++) {
        //write_block[i + 1] = data[i];
        Wire.write(data[i]);
    }
    //free(write_block);
    delay (1); //100 usec
// Error check?
//TODO I2C_ERR_CHECK
    return NO_ERR;
}

static char wellADCWriteRegByte(uint8_t reg_address, uint8_t b)
{

    Wire.beginTransmission(NAU7802_DEVICE_ADDRESS); // transmit to device #0x2A
    Wire.write(reg_address);// sends 1 byte
    Wire.write(b);// sends 1 byte
    Wire.endTransmission();// stop transmitting

    delay (1);//100 usec
    return NO_ERR;
}

static uint8_t wellADCReadRegVal(char reg_address, char *out,
        uint8_t dataSize)
{

    Wire.beginTransmission(NAU7802_DEVICE_ADDRESS); // transmit to device #0x2A
    Wire.write(reg_address);// sends 1 byte
    Wire.endTransmission(false);// stop transmitting
    Wire.beginTransmission(NAU7802_DEVICE_ADDRESS);// transmit to device #0x2A
    Wire.requestFrom(NAU7802_DEVICE_ADDRESS, dataSize);// request 6 bytes from slave device #0x2A

    uint8_t reg_val = 0;
    int index = 0;
    while(Wire.available() && index<dataSize)// slave may send less than requested
    { 
        char c = Wire.read(); // receive a byte as character
        out[index++] = c;
        Serial.print(c);
    }
    return NO_ERR;
}

static bool isAdcInitialized = false;


static uint8_t adc_default_conf = 0b0010000;
uint8_t initADC () {
    if (isAdcInitialized) {
        return 0;
    }
    isAdcInitialized = true;
    /*
    Wire.begin(PIN_WELL_NAU7803_SDA, PIN_WELL_NAU7803_SCL); //sda, scl
    char read_out[1] = {0xFF};
    char write_init_block[1] = {0x16};
    char write_calib_block[1] = {0x04};
    char write_rate_block[1] = {adc_default_conf}; // 111=320SPS

    // Select slave device (0x2A)
    delay(10);

    // PU init
    i2c_err = wellADCWriteRegVal(NAU7802_REG_PU_CTRL_ADDRESS,
            write_init_block, 1);
    delay(10);

    // Wait for "Cycle ready" flag
    i2c_err = wellADCWriteRegVal(0x02, write_init_block, 1);
    delay(10);

    // Calibration
    i2c_err = wellADCWriteRegVal(NAU7802_REG_CTRL2_ADDRESS, write_rate_block, 1);

    // Wait for "Calibration Done" flag
    delay(10);
    i2c_err++;

    do {
        i2c_err = wellADCReadRegVal(NAU7802_REG_CTRL2_ADDRESS, &read_out[0], 1);
        // TODO I2C_ERR_CHECK
        Serial.print("ADC=");
        Serial.println((int)read_out[0]);
        delay(1000);
    } while (read_out[0] & 0x04); 1 
    return NO_ERR;
    */
    
  Wire.begin(PIN_WELL_NAU7803_SDA, PIN_WELL_NAU7803_SCL);
  
  delay(50);
  Wire.beginTransmission(NAU7802_DEVICE_ADDRESS); // transmit to device #0x2A
  Wire.write(0x00);        // sends 1 byte
  Wire.write(0x16);        // sends 1 byte
  Wire.endTransmission();    // stop transmitting

  return NO_ERR;
}

uint8_t readRegister (uint8_t reg_addr) {
  Serial.print("REG=0x");
  Serial.print(reg_addr, HEX);         // print the character
    
  Wire.beginTransmission(NAU7802_DEVICE_ADDRESS); // transmit to device #0x2A
  Wire.write(reg_addr);        // sends 1 byte
  Wire.endTransmission(false);    // stop transmitting
  Wire.beginTransmission(NAU7802_DEVICE_ADDRESS); // transmit to device #0x2A
  Wire.requestFrom(NAU7802_DEVICE_ADDRESS, 1);    // request 6 bytes from slave device #0x2A
  
  uint8_t reg_val = 0;
  while(Wire.available())    // slave may send less than requested
  { 
    char c = Wire.read(); // receive a byte as character
    reg_val = c;

  }

    Serial.print(", val=0x");
    Serial.println(reg_val, (0xFF&HEX));
    return reg_val;
}

/*
 * ADC Test
 * Read register 0x1F (revision code)
 * Expected response vaue is 0x0F
 */
void printRevisionCode () {
  delay(100);
  char read_out[1] = {0xFF};
  wellADCReadRegVal(0x1F, &read_out[0], 1);
  Serial.print("Reg="); // 0x0F is expected
  Serial.println(0+read_out[0]);

}

uint8_t prev_channel = 0;
float getADCValueAt (uint8_t channel) {
  if (channel != prev_channel) {
    // Switch channel
    // clear bit
    char calib;
    if (channel==1) {
        // 2ch (set bit 7)
        calib = adc_default_conf | (1<<7); //これまちがってる
    } else {
        // 1ch (clear bit 7)
        calib = adc_default_conf & ~(1<<7);
    }
    wellADCWriteRegByte(NAU7802_REG_CTRL2_ADDRESS, calib);  //0x02
    Serial.print("Switch ");
    Serial.println(calib, (0xFF&HEX));
    prev_channel = channel;
    delay(100);
  }

  if (channel==0) {
    Serial.print("1ch ");
  } else {
    Serial.print("2ch ");
  }
    //printRevisionCode(); // Test
    uint32_t adc_val = 0xFFFFFF;
    char read_out[3] = {0x00, 0x00, 0x00};
    i2c_err = wellADCReadRegVal(NAU7802_REG_ADCO_B2_ADDRESS,
            &read_out[0], 3);
    Serial.print("Raw=");
    Serial.print(read_out[0], (0xFF&HEX));Serial.print(",");
    Serial.print(read_out[1], (0xFF&HEX));Serial.print(",");
    Serial.print(read_out[2], (0xFF&HEX));
    Serial.println("");
    read_out[0] -= 0x80; // signed->unsigned
    adc_val = (read_out[0] << 16) | (read_out[1] << 8) | read_out[2];
    return (float) adc_val/(1.0 * 0x1000000);
 
}
// Read ADC value of channel 0
float getWellADCValue () {
  return getADCValueAt(0);
}

// Read ADC value of channel 1
float getLidADCValue () {
  return getADCValueAt(1);
}

#endif /* USE_ADC_NAU7803 */
