/*!
 *  @file Adafruit_SHT4x.cpp
 *
 *  @mainpage Adafruit SHT4x Digital Humidity & Temp Sensor
 *
 *  @section intro_sec Introduction
 *
 *  This is a library for the SHT4x Digital Humidity & Temp Sensor
 *
 *  Designed specifically to work with the SHT4x Digital sensor from Adafruit
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
 * SHT40 i2c Humidity and Temperature sensor.
 *
 */

#ifndef SHT40_H
#define SHT40_H

/**
 * Includes
 */
#include "mbed.h"

/**
 * Defines
 */
// Acquired from adafruit arduino SHT4x.h

#define SHT40_I2C_ADDRESS           0x44
#define SHT4x_NOHEAT_HIGHPRECISION  0xFD 
#define SHT4x_NOHEAT_MEDPRECISION   0xF6 
#define SHT4x_NOHEAT_LOWPRECISION   0xE0 
#define SHT4x_HIGHHEAT_1S           0x39 
#define SHT4x_HIGHHEAT_100MS        0x32 
#define SHT4x_MEDHEAT_1S            0x2F 
#define SHT4x_MEDHEAT_100MS         0x24 
#define SHT4x_LOWHEAT_1S            0x1E 
#define SHT4x_LOWHEAT_100MS         0x15 
#define SHT4x_READSERIAL            0x89 
#define SHT4x_SOFTRESET             0x94  
#define SHT4x_CRC8_POLYNOMIAL 0x31
#define SHT4x_CRC8_INIT 0xFF
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
 * Adafruit SHT40 i2c digital humidity and temperature sensor.
 */
class SHT40 {

public:

    /**
     * Constructor.
     *
     * @param i2c pointer to the mbed I2C object
     */
    SHT40(I2C* i2c);

    // Read the serial number
    bool init(void);

     // Reads the temperature, input void, outputs a float in celcius.
    float tempCF(void);
      
    // Reads the relative humidity, input void, outputs a float.
    float relHumidF(void);

    // Reads the temperature and relative humidity
    bool readBoth(float *temp, float *humid);

    // Serial number
    uint32_t serial_number;

private:
    uint8_t generateCRC(uint8_t *data, uint8_t datalen);

    I2C* i2c_;
};

#endif /* SHT40_H */
