#include "adc.h"
#include "board_conf.h"
#include <Arduino.h>
#include <Wire.h>
#include "adc_NAU7803.h"

#ifdef USE_ADC_NAU7802


#define NO_ERR 0x00

/* Implementation of NAU7802 A/D Converter */
char i2c_err;

// Basic communication

static char wellADCWriteRegValue(uint8_t reg_address, uint8_t b) {
  Wire.beginTransmission(NAU7802_DEVICE_ADDRESS); // transmit to device #0x2A
  Wire.write(reg_address);// sends 1 byte
  Wire.write(b);// sends 1 byte
  Wire.endTransmission();// stop transmitting
  return NO_ERR;
}
static char wellADCWriteRegValues (uint8_t reg_address, char *data, uint8_t dataSize) {
  int i;
  Wire.write(reg_address);
  for (i = 0; i < dataSize; i++) {
    Wire.write(data[i]);
  }
  // delay (1); //100 usec
  return NO_ERR;
}

// Read a single register value
static uint8_t wellADCReadRegValue(char reg_address) {
  Wire.beginTransmission(NAU7802_DEVICE_ADDRESS); // transmit to device #0x2A
  Wire.write((uint8_t)reg_address);// sends 1 byte
  Wire.endTransmission(false);// stop transmitting
  Wire.beginTransmission(NAU7802_DEVICE_ADDRESS);// transmit to device #0x2A
  Wire.requestFrom(NAU7802_DEVICE_ADDRESS, 1);// request 6 bytes from slave device #0x2A
  return Wire.read();
}
// Read continuous registers
static uint8_t wellADCReadRegValues(char reg_address, char *out, uint8_t dataSize) {
  Wire.beginTransmission(NAU7802_DEVICE_ADDRESS); // transmit to device #0x2A
  Wire.write((uint8_t)reg_address);// sends 1 byte
  Wire.endTransmission(false);// stop transmitting
  Wire.beginTransmission(NAU7802_DEVICE_ADDRESS);// transmit to device #0x2A
  Wire.requestFrom(NAU7802_DEVICE_ADDRESS, dataSize);// request 6 bytes from slave device #0x2A

  uint8_t reg_val = 0;
  int index = 0;
  while (Wire.available() && index < dataSize) {
    char c = Wire.read(); // receive a byte as character
    out[index++] = c; //Serial.print(c);
  }
}

void clearRegisterBit (uint8_t reg_addr, int index) {
  uint8_t value = wellADCReadRegValue(reg_addr) & ~(1 << index);
  wellADCWriteRegValue(reg_addr, value);
}

void setRegisterBit (uint8_t reg_addr, int index) {
  uint8_t value = wellADCReadRegValue(reg_addr) | (1 << index);
  wellADCWriteRegValue(reg_addr, value);

}
static bool isAdcInitialized = false;
//static uint8_t adc_default_conf = 0b0111100; //320sps (111)(OK)
static uint8_t adc_default_conf = 0b0010100; //40sps (010)
//static uint8_t adc_default_conf = 0b0110100;

void waitForFlag (uint8_t regAddress, int flagIndex, bool flagValue) {
  bool flagResult;
  char read_out[1] = {0xFF};
  int count = 0;
  do {
    wellADCReadRegValues(regAddress, &read_out[0], 1);
    flagResult = read_out[0] & (0x01 << flagIndex);
    count++;
  } while (flagResult != flagValue);

  /*
    Serial.print("took ");
    Serial.println(count);
  */

}

uint8_t initADC () {
  if (isAdcInitialized) {
    return 0;
  }
  isAdcInitialized = true;

  Wire.begin(PIN_WELL_NAU7802_SDA, PIN_WELL_NAU7802_SCL); //sda, scl
  char read_out[1] = {0xFF};
  char write_init_block[1] = {0x16};
  char write_calib_block[1] = {0x04};
  char write_rate_block[1] = {adc_default_conf}; // 111=320SPS
  // Reset registers
  setRegisterBit(NAU7802_REG_ADDR_PU_CTRL, NAU7802_BIT_RR);
  clearRegisterBit(NAU7802_REG_ADDR_PU_CTRL, NAU7802_BIT_RR);
  // Power up digital
  setRegisterBit(NAU7802_REG_ADDR_PU_CTRL, NAU7802_BIT_PUD);
  // Wait for power up ready flag
  waitForFlag(NAU7802_REG_ADDR_PU_CTRL, NAU7802_BIT_PUR, true);
  // Power up analog
  setRegisterBit(NAU7802_REG_ADDR_PU_CTRL, NAU7802_BIT_PUA);
  // Cycle start
  setRegisterBit(NAU7802_REG_ADDR_PU_CTRL, NAU7802_BIT_PUA);

  wellADCWriteRegValue(NAU7802_REG_ADDR_CTRL2, adc_default_conf);

  // Set sampling rate
  setRegisterBit(NAU7802_REG_ADDR_CTRL2, 4);
  setRegisterBit(NAU7802_REG_ADDR_CTRL2, 5);
  setRegisterBit(NAU7802_REG_ADDR_CTRL2, 6);

  // Start conversion
  setRegisterBit(NAU7802_REG_ADDR_CTRL2, NAU7802_BIT_CS);
  
  Serial.print("Initial NAU7802_REG_ADDR_CTRL2=");
  Serial.println(wellADCReadRegValue(NAU7802_REG_ADDR_CTRL2));
  return NO_ERR;
}

void printRevisionCode () {
  Serial.print("Reg="); // 0x0F is expected
  Serial.println(0 + wellADCReadRegValue(NAU7802_REG_ADDR_REVISION));
}

uint8_t prev_channel = 0;
float getADCValueAt (uint8_t channel) {

  uint32_t adc_val = 0xFFFFFF;
  char read_out[3] = {0xFF, 0xFF, 0xFF};
  /*
  if (channel == 0) {
    Serial.print("1ch ");
  } else {
    Serial.print("2ch ");
  }
  */
  
  if (channel != prev_channel) {
    // Switch channel
    if (channel == 1) {
      // 2ch (set bit 7)
      setRegisterBit(NAU7802_REG_ADDR_CTRL2, 7);
    } else {
      // 1ch (clear bit 7)
      clearRegisterBit(NAU7802_REG_ADDR_CTRL2, 7);
    }
    setRegisterBit(NAU7802_REG_ADDR_CTRL2, 2); //Calib
    // Calib OK
    waitForFlag(NAU7802_REG_ADDR_CTRL2, 2, false);
    prev_channel = channel;
  }
  //printRevisionCode(); // Test

  setRegisterBit(NAU7802_REG_ADDR_PU_CTRL, NAU7802_BIT_CS);
  waitForFlag(NAU7802_REG_ADDR_PU_CTRL, NAU7802_BIT_CR, true); // Cycle ready

  delay(50);
  i2c_err = wellADCReadRegValues(NAU7802_REG_ADDR_ADCO_B2,
                                 &read_out[0], 3);
                                 /*
  Serial.print("Raw=");
  Serial.print(read_out[0], (0xFF & HEX)); Serial.print(",");
  Serial.print(read_out[1], (0xFF & HEX)); Serial.print(",");
  Serial.print(read_out[2], (0xFF & HEX));
  Serial.print(" / ");
  */
  read_out[0] -= 0x80; // signed->unsigned
  adc_val = (read_out[0] << 16) | (read_out[1] << 8) | read_out[2];
  float ratio =  (float) adc_val / (1.0 * 0x1000000);
  return ratio;

}
// Read ADC value of channel 0
float getWellADCValue () {
  return getADCValueAt(0);
}

// Read ADC value of channel 1
float getLidADCValue () {
  return getADCValueAt(1);
}

#endif /* USE_ADC_NAU7802 */
