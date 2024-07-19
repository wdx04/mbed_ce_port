/**
 *  BMP280 Combined humidity and pressure sensor library
 *
 *  @author  Toyomasa Watarai
 *  @version 1.0
 *  @date    06-April-2015
 *
 * bugfixing by charly
 *
 *  Library for "BMP280 temperature, humidity and pressure sensor module" from Switch Science
 *    https://www.switch-science.com/catalog/2236/
 *
 *  For more information about the BMP280:
 *    http://ae-bst.resource.bosch.com/media/products/dokumente/BMP280/BST-BMP280_DS001-10.pdf
 */

#include "mbed.h"
#include "BMP280.h"

BMP280::BMP280(I2C *i2c_ptr, char slave_adr)
    : i2c_p(i2c_ptr),  address(slave_adr<<1), t_fine(0)
{
}

bool BMP280::init()
{
    char cmd[18];
 
    cmd[0] = 0xf4; // ctrl_meas
    cmd[1] = 0b01010111; // Temparature oversampling x2 010, Pressure oversampling x16 101, Normal mode 11
    if(i2c_p->write(address, cmd, 2) != 0) return false;
 
    cmd[0] = 0xf5; // config
    cmd[1] = 0b10111100; // Standby 1000ms, Filter x16
    if(i2c_p->write(address, cmd, 2) != 0) return false;
 
    cmd[0] = 0x88; // read dig_T regs
    if(i2c_p->write(address, cmd, 1) != 0) return false;
    if(i2c_p->read(address, cmd, 6) != 0) return false;
 
    dig_T1 = (cmd[1] << 8) | cmd[0];
    dig_T2 = (cmd[3] << 8) | cmd[2];
    dig_T3 = (cmd[5] << 8) | cmd[4];
 
    DEBUG_PRINT("dig_T = 0x%x, 0x%x, 0x%x\n\r", dig_T1, dig_T2, dig_T3);
    DEBUG_PRINT("dig_T = %d, %d, %d\n\r", dig_T1, dig_T2, dig_T3);
 
    cmd[0] = 0x8E; // read dig_P regs
    if(i2c_p->write(address, cmd, 1) != 0) return false;
    if(i2c_p->read(address, cmd, 18) != 0) return false;
 
    dig_P1 = (cmd[ 1] << 8) | cmd[ 0];
    dig_P2 = (cmd[ 3] << 8) | cmd[ 2];
    dig_P3 = (cmd[ 5] << 8) | cmd[ 4];
    dig_P4 = (cmd[ 7] << 8) | cmd[ 6];
    dig_P5 = (cmd[ 9] << 8) | cmd[ 8];
    dig_P6 = (cmd[11] << 8) | cmd[10];
    dig_P7 = (cmd[13] << 8) | cmd[12];
    dig_P8 = (cmd[15] << 8) | cmd[14];
    dig_P9 = (cmd[17] << 8) | cmd[16];
 
    DEBUG_PRINT("dig_P = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", dig_P1, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9);

    return true;
}
 
float BMP280::getTemperature()
{
    int32_t temp_raw;
    float tempf;
    char cmd[4];
 
    cmd[0] = 0xfa; // temp_msb
    i2c_p->write(address, cmd, 1);
    i2c_p->read(address, &cmd[1], 3);
 
    temp_raw = (cmd[1] << 12) | (cmd[2] << 4) | (cmd[3] >> 4);
    DEBUG_PRINT("\r\ntemp_raw:%d",temp_raw);
 
    int32_t temp1, temp2,temp;
 
    temp1 =((((temp_raw >> 3) - (dig_T1 << 1))) * dig_T2) >> 11;
    temp2 =(((((temp_raw >> 4) - dig_T1) * ((temp_raw >> 4) - dig_T1)) >> 12) * dig_T3) >> 14;
    DEBUG_PRINT("   temp1:%d   temp2:%d",temp1, temp2);
    t_fine = temp1+temp2;
    DEBUG_PRINT("   t_fine:%d",t_fine);
    temp = (t_fine * 5 + 128) >> 8;
    tempf = (float)temp;
    DEBUG_PRINT("   tempf:%f",tempf);
  
    return (tempf/100.0f);
}
 
float BMP280::getPressure()
{
    uint32_t press_raw;
    float pressf;
    char cmd[4];
 
    cmd[0] = 0xf7; // press_msb
    i2c_p->write(address, cmd, 1);
    i2c_p->read(address, &cmd[1], 3);
 
    press_raw = (cmd[1] << 12) | (cmd[2] << 4) | (cmd[3] >> 4);
 
    int32_t var1, var2;
    uint32_t press;
 
    var1 = (t_fine >> 1) - 64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * dig_P6;
    var2 = var2 + ((var1 * dig_P5) << 1);
    var2 = (var2 >> 2) + (dig_P4 << 16);
    var1 = (((dig_P3 * (((var1 >> 2)*(var1 >> 2)) >> 13)) >> 3) + ((dig_P2 * var1) >> 1)) >> 18;
    var1 = ((32768 + var1) * dig_P1) >> 15;
    if (var1 == 0) {
        return 0;
    }
    press = (((1048576 - press_raw) - (var2 >> 12))) * 3125;
    if(press < 0x80000000) {
        press = (press << 1) / var1;
    } else {
        press = (press / var1) * 2;
    }
    var1 = ((int32_t)dig_P9 * ((int32_t)(((press >> 3) * (press >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(press >> 2)) * (int32_t)dig_P8) >> 13;
    press = (press + ((var1 + var2 + dig_P7) >> 4));
 
    pressf = (float)press;
    return (pressf/100.0f);
}
