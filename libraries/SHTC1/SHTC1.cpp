 /**
 * @author Andrew Reed 
 * 
 * Freeware --
 * simple driver hacked together by hardware engineer, definite scope for 
 * improvement by someone who knows waht they are doing.
 *
 * @section DESCRIPTION
 *
 * SHTC1 i2c Humidity and Temperature sensor.
 *
 * Datasheet, specs, and information:
 *
 * https://www.adafruit.com/product/4885
 */

/**
 * Includes
 */
#include "SHTC1.h"

SHTC1::SHTC1(PinName sda, PinName scl) {

    i2c_ = new I2C(sda, scl);
    //400KHz, as specified by the datasheet.
    i2c_->frequency(I2C_SPEED_FAST_MODE);
}

SHTC1::SHTC1(I2C *i2c) : i2c_(i2c) {

    //400KHz, as specified by the datasheet.
    i2c_->frequency(I2C_SPEED_FAST_MODE);
}

float SHTC1::tempCF(void) {

    char txBuff[2];
    char rxBuff[6];

    txBuff[0] = SHTC1_READ_TFIRST; // Triggers a temperature measure by feeding correct opcode.
    txBuff[1] = SHTC1_READ_TFIRST >> 8;
    i2c_->write((SHTC1_I2C_ADDRESS << 1) & 0xFE, txBuff, 2);
    thread_sleep_for(15); // Per datasheet, wait long enough for device to sample temperature
    
    // Reads triggered measure
    i2c_->read((SHTC1_I2C_ADDRESS << 1) | 0x01, rxBuff, 6);
    thread_sleep_for(1);
    
    // Algorithm from arduino SHTC1.h to compute temperature.
    unsigned int rawTemperature = ((unsigned int) rxBuff[0] << 8) | (unsigned int) rxBuff[1];
    rawTemperature &= 0xFFFC;

    float tempTemperature = float(rawTemperature) / 65535.0f;
    float realTemperature = -45.0f + (175.0f * tempTemperature);
 
    return realTemperature;
}

float SHTC1::relHumidF(void) {

    char txBuff[2];
    char rxBuff[6];

    txBuff[0] = SHTC1_READ_TFIRST; // send command.
    txBuff[1] = SHTC1_READ_TFIRST >> 8;
    i2c_->write((SHTC1_I2C_ADDRESS << 1) & 0xFE, txBuff, 2);
    thread_sleep_for(15); // delay to sample humidity
    
    // Reads measurement
    i2c_->read((SHTC1_I2C_ADDRESS << 1) | 0x01, rxBuff, 6);
    thread_sleep_for(1);
    
    //Algorithm from SHTC1.h.
    unsigned int rawHumidity = ((unsigned int) rxBuff[3] << 8) | (unsigned int) rxBuff[4];

    rawHumidity &= 0xFFFC; //Zero out the status bits but keep them in place
    
    //Convert to relative humidity
    float RH = float(rawHumidity) / 65535.0f;
    float rh = 100.0f * RH;
    rh = min(max(rh, 0.0f), 100.0f);

    return rh;
}

std::pair<float, float> SHTC1::tempCF_relHumidF()
{

    char txBuff[2];
    char rxBuff[6];

    txBuff[0] = SHTC1_READ_TFIRST; // send command.
    txBuff[1] = SHTC1_READ_TFIRST >> 8;
    i2c_->write((SHTC1_I2C_ADDRESS << 1) & 0xFE, txBuff, 2);
    thread_sleep_for(15); // delay to sample humidity
    
    // Reads measurement
    i2c_->read((SHTC1_I2C_ADDRESS << 1) | 0x01, rxBuff, 6);
    thread_sleep_for(1);
    
    //Algorithm from SHTC1.h.
    unsigned int rawTemperature = ((unsigned int) rxBuff[0] << 8) | (unsigned int) rxBuff[1];
    rawTemperature &= 0xFFFC;

    float tempTemperature = float(rawTemperature) / 65535.0f;
    float realTemperature = -45.0f + (175.0f * tempTemperature);

    unsigned int rawHumidity = ((unsigned int) rxBuff[3] << 8) | (unsigned int) rxBuff[4];

    rawHumidity &= 0xFFFC; //Zero out the status bits but keep them in place
    
    //Convert to relative humidity
    float RH = float(rawHumidity) / 65535.0f;
    float rh = 100.0f * RH;
    rh = min(max(rh, 0.0f), 100.0f);

    return std::make_pair(realTemperature, rh);
}
