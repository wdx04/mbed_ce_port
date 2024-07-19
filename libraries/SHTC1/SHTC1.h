/*!
 *  @file Adafruit_SHTC1.cpp
 *
 *  @mainpage Adafruit SHTC1 Digital Humidity & Temp Sensor
 *
 *  @section intro_sec Introduction
 *
 *  This is a library for the SHTC1 Digital Humidity & Temp Sensor
 *
 *  Designed specifically to work with the SHTC1 Digital sensor from Adafruit
 *
 *  Pick one up today in the adafruit shop!
 *  ------> https://www.adafruit.com/product/4885
 *
 *  These sensors use I2C to communicate, 2 pins are required to interface
 *
 *  Adafruit invests time and resources providing this open source code,
 *  please support Adafruit andopen-source hardware by purchasing products
 *  from Adafruit!
 *
 *  @section author Author
 *
 *  Limor Fried/Ladyada (Adafruit Industries).
 *
 *  @section license License
 *
 *  BSD license, all text above must be included in any redistribution
 * 
 * @reworked by Andrew Reed areed@cityplym.ac.uk
 * 
 * simple driver hacked together by hardware engineer, definite scope for 
 * improvement by someone who knows what they are doing.
 *
 * @section DESCRIPTION
 *
 * SHTC1 i2c Humidity and Temperature sensor.
 *
 */

#ifndef SHTC1_H
#define SHTC1_H

/**
 * Includes
 */
#include "mbed.h"
#include <utility>

/**
 * Defines
 */
// Acquired from adafruit arduino SHTC1.h

#define SHTC1_I2C_ADDRESS           0x70
#define SHTC1_READ_TFIRST         0x6678
#define SHTC1_READ_HFIRST         0xE058
#define SHTC1_SOFTRESET           0x5D80  
#define I2C_SPEED_STANDARD_MODE   100000
#define I2C_SPEED_FAST_MODE       400000

/**
 * 2 byte packet is sent to device as instruction
 * 6 byte packet is received from device
 * received data includes checksum in byte 3 for bytes 1 and 2
 * and byte 6 for bytes 4 and 5.
 */
static uint8_t crc8(const uint8_t *data, int len); 
 
/**
 * Adafruit SHTC1 i2c digital humidity and temperature sensor.
 */
class SHTC1 {

public:

    /**
     * Constructor.
     *
     * @param sda mbed pin to use for SDA line of I2C interface.
     * @param scl mbed pin to use for SCL line of I2C interface.
     */
    SHTC1(PinName sda, PinName scl);

    SHTC1(I2C* i2c);

     // Reads the temperature, input void, outputs a float in celcius.
    float tempCF(void);
      
    // Reads the relative humidity, input void, outputs a float.
    float relHumidF(void);

    // Reads the temperature and humidity
    std::pair<float, float> tempCF_relHumidF();

private:
    I2C* i2c_;

};

#endif /* SHTC1_H */
