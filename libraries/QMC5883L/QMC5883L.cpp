/*   QMC5883L Digital Compass Library
*
*    @author: Baser Kandehir 
*    @date: August 5, 2015
*    @license: MIT license
*     
*   Copyright (c) 2015, Baser Kandehir, baser.kandehir@ieee.metu.edu.tr
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in
*   all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*   THE SOFTWARE.
*
*/

// Some part of the code is adapted from Adafruit HMC5883 library

#include "QMC5883L.h"

QMC5883L::QMC5883L(PinName sda, PinName scl): QMC5883L_i2c(sda, scl)
{
}

float QMC5883L::setMagRange(MagScale Mscale)
{
    float mRes; // Varies with gain
    
    switch(Mscale)
    {
        case MagScale_2G:
            mRes = 1.0/12000;  //LSB/G
            break;
        case MagScale_8G:
            mRes = 1.0/3000;
            break;
    } 
    return mRes;
}

void QMC5883L::QMC5883L_WriteByte(uint8_t QMC5883L_reg, uint8_t QMC5883L_data)
{
    char data_out[2];
    data_out[0]=QMC5883L_reg;
    data_out[1]=QMC5883L_data;
    this->QMC5883L_i2c.write(QMC5883L_ADDRESS, data_out, 2, 0);
}

uint8_t QMC5883L::QMC5883L_ReadByte(uint8_t QMC5883L_reg)
{
    char data_out[1], data_in[1];
    data_out[0] = QMC5883L_reg;
    this->QMC5883L_i2c.write(QMC5883L_ADDRESS, data_out, 1, 1);
    this->QMC5883L_i2c.read(QMC5883L_ADDRESS, data_in, 1, 0);
    return (data_in[0]);
}

void QMC5883L::ChipID()
{
    uint8_t ChipID = QMC5883L_ReadByte(CHIP_ID);   // Should return 0x68  
}

void QMC5883L::init()
{   
    setMagRange(MagScale_8G);
    QMC5883L_WriteByte(CONTROL_A, 0x0D | MagScale_8G);  // Range: 8G, ODR: 200 Hz, mode:Continuous-Measurement
    QMC5883L_WriteByte(SET_RESET, 0x01);
    ThisThread::sleep_for(10ms);
}

int16_t QMC5883L::getMagXvalue()
{
    uint8_t LoByte, HiByte;
    LoByte = QMC5883L_ReadByte(OUT_X_LSB); // read Accelerometer X_Low  value
    HiByte = QMC5883L_ReadByte(OUT_X_MSB); // read Accelerometer X_High value
    return((HiByte<<8) | LoByte);
}

int16_t QMC5883L::getMagYvalue()
{
    uint8_t LoByte, HiByte;
    LoByte = QMC5883L_ReadByte(OUT_Y_LSB); // read Accelerometer X_Low  value
    HiByte = QMC5883L_ReadByte(OUT_Y_MSB); // read Accelerometer X_High value
    return ((HiByte<<8) | LoByte);
}

int16_t QMC5883L::getMagZvalue()
{
    uint8_t LoByte, HiByte;
    LoByte = QMC5883L_ReadByte(OUT_Z_LSB); // read Accelerometer X_Low  value
    HiByte = QMC5883L_ReadByte(OUT_Z_MSB); // read Accelerometer X_High value
    return ((HiByte<<8) | LoByte);
}

int16_t QMC5883L::getMagTemp()
{
    uint8_t LoByte, HiByte;
    LoByte = QMC5883L_ReadByte(TEMP_LSB); // read Accelerometer X_Low  value
    HiByte = QMC5883L_ReadByte(TEMP_MSB); // read Accelerometer X_High value
    return ((HiByte<<8) | LoByte);
}