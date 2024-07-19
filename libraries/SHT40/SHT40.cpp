 /**
 * @author Andrew Reed 
 * 
 * Freeware --
 * simple driver hacked together by hardware engineer, definite scope for 
 * improvement by someone who knows waht they are doing.
 *
 * @section DESCRIPTION
 *
 * SHT40 i2c Humidity and Temperature sensor.
 *
 * Datasheet, specs, and information:
 *
 * https://www.adafruit.com/product/4885
 */

/**
 * Includes
 */
#include "SHT40.h"

SHT40::SHT40(I2C *i2c) : i2c_(i2c) {
}

bool SHT40::init(void)
{
    uint8_t txBuff[1];
    uint8_t rxBuff[6];

    txBuff[0] = SHT4x_READSERIAL;
    if(i2c_->write((SHT40_I2C_ADDRESS << 1) & 0xFE, (const char *) txBuff, 1) != 0) return false;
    thread_sleep_for(1);
    
    if(i2c_->read((SHT40_I2C_ADDRESS << 1) | 0x01, (char *) rxBuff, 6) != 0) return false;

    uint8_t crc1 = generateCRC(rxBuff, 2);
    uint8_t crc2 = generateCRC(rxBuff + 3, 2);
    if(crc1 != rxBuff[2] || crc2 != rxBuff[5]) return false;

    serial_number = (uint32_t(rxBuff[0]) << 24)  + (uint32_t(rxBuff[1]) << 16) + (uint32_t(rxBuff[3]) << 8) + uint32_t(rxBuff[4]);

    return true;
}

float SHT40::tempCF(void) {

    uint8_t txBuff[1];
    uint8_t rxBuff[6];

    txBuff[0] = SHT4x_NOHEAT_HIGHPRECISION; // Triggers a temperature measure by feeding correct opcode.
    i2c_->write((SHT40_I2C_ADDRESS << 1) & 0xFE, (const char *) txBuff, 1);
    thread_sleep_for(10); // Per datasheet, wait long enough for device to sample temperature
    
    // Reads triggered measure
    i2c_->read((SHT40_I2C_ADDRESS << 1) | 0x01, (char *) rxBuff, 6);
    thread_sleep_for(1);
    
    // Algorithm from arduino sht4x.h to compute temperature.
    unsigned int rawTemperature = ((unsigned int) rxBuff[0] << 8) | (unsigned int) rxBuff[1];
    rawTemperature &= 0xFFFC;

    float tempTemperature = float(rawTemperature) / 65535.0f;
    float realTemperature = -45.0f + (175.0f * tempTemperature);
 
    return realTemperature;
}

float SHT40::relHumidF(void) {

    uint8_t txBuff[1];
    uint8_t rxBuff[6];

    txBuff[0] = SHT4x_NOHEAT_HIGHPRECISION; // send command.
    i2c_->write((SHT40_I2C_ADDRESS << 1) & 0xFE, (const char *) txBuff, 1);
    thread_sleep_for(16); // delay to sample humidity
    
    // Reads measurement
    i2c_->read((SHT40_I2C_ADDRESS << 1) | 0x01, (char *) rxBuff, 6);
    thread_sleep_for(1);
    
    //Algorithm from sht4x.h.
    unsigned int rawHumidity = ((unsigned int) rxBuff[3] << 8) | (unsigned int) rxBuff[4];

    rawHumidity &= 0xFFFC; //Zero out the status bits but keep them in place
    
    //Convert to relative humidity
    float RH = float(rawHumidity) / 65535.0f;
    float rh = -6.0f + (125.0f * RH);
    rh = min(max(rh, 0.0f), 100.0f);

    return rh;
}

bool SHT40::readBoth(float *temp, float *humid)
{
    uint8_t txBuff[1];
    uint8_t rxBuff[6];

    txBuff[0] = SHT4x_NOHEAT_HIGHPRECISION; // Triggers a temperature measure by feeding correct opcode.
    if(i2c_->write((SHT40_I2C_ADDRESS << 1) & 0xFE, (const char *) txBuff, 1) != 0)
    {
        return false;
    }
    thread_sleep_for(10); // Per datasheet, wait long enough for device to sample temperature
    
    // Reads triggered measure
    if(i2c_->read((SHT40_I2C_ADDRESS << 1) | 0x01, (char *) rxBuff, 6) != 0)
    {
        return false;
    }
    
    uint8_t crc1 = generateCRC(rxBuff, 2);
    uint8_t crc2 = generateCRC(rxBuff + 3, 2);
    if(crc1 != rxBuff[2] || crc2 != rxBuff[5])
    {
        return false;
    }
    
    // Algorithm from arduino sht4x.h to compute temperature.
    unsigned int rawTemperature = ((unsigned int) rxBuff[0] << 8) | (unsigned int) rxBuff[1];
    rawTemperature &= 0xFFFC;

    float tempTemperature = float(rawTemperature) / 65535.0f;
    *temp = -45.0f + (175.0f * tempTemperature);
 
    //Algorithm from sht4x.h.
    unsigned int rawHumidity = ((unsigned int) rxBuff[3] << 8) | (unsigned int) rxBuff[4];

    rawHumidity &= 0xFFFC; //Zero out the status bits but keep them in place
    
    //Convert to relative humidity
    float RH = float(rawHumidity) / 65535.0f;
    float rh = -6.0f + (125.0f * RH);

    *humid = min(max(rh, 0.0f), 100.0f);

    return true;
}

uint8_t SHT40::generateCRC(uint8_t *data, uint8_t datalen) {
  // calculates 8-Bit checksum with given polynomial
  uint8_t crc = SHT4x_CRC8_INIT;

  for (uint8_t i = 0; i < datalen; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ SHT4x_CRC8_POLYNOMIAL;
      else
        crc <<= 1;
    }
  }
  return crc;
}
