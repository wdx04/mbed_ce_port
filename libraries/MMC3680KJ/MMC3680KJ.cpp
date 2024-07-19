#include "mbed.h"
#include "MMC3680KJ.h"

#define MMC3680KJ_REG_DATA                                0x00
#define MMC3680KJ_REG_XL                                  0x00
#define MMC3680KJ_REG_XH                                  0x01
#define MMC3680KJ_REG_YL                                  0x02
#define MMC3680KJ_REG_YH                                  0x03
#define MMC3680KJ_REG_ZL                                  0x04
#define MMC3680KJ_REG_ZH                                  0x05
#define MMC3680KJ_REG_TEMP                                0x06
#define MMC3680KJ_REG_STATUS                              0x07
#define MMC3680KJ_REG_CTRL0                               0x08
#define MMC3680KJ_REG_CTRL1                               0x09
#define MMC3680KJ_REG_CTRL2                               0x0a
#define MMC3680KJ_REG_X_THD                               0x0b
#define MMC3680KJ_REG_Y_THD                               0x0c
#define MMC3680KJ_REG_Z_THD                               0x0d
#define MMC3680KJ_REG_SELFTEST                            0x0e
#define MMC3680KJ_REG_PASSWORD                            0x0f
#define MMC3680KJ_REG_OTPMODE                             0x12
#define MMC3680KJ_REG_TESTMODE                            0x13
#define MMC3680KJ_REG_SR_PWIDTH                           0x20
#define MMC3680KJ_REG_OTP                                 0x2a
#define MMC3680KJ_REG_PRODUCTID                           0x2f

#define MMC3680KJ_CMD_REFILL                              0x20
#define MMC3680KJ_CMD_RESET                               0x10
#define MMC3680KJ_CMD_SET                                 0x08
#define MMC3680KJ_CMD_TM_M                                0x01
#define MMC3680KJ_CMD_TM_T                                0x02
#define MMC3680KJ_CMD_START_MDT                           0x04
#define MMC3680KJ_CMD_100HZ                               0x00
#define MMC3680KJ_CMD_200HZ                               0x01
#define MMC3680KJ_CMD_400HZ                               0x02
#define MMC3680KJ_CMD_600HZ                               0x03
#define MMC3680KJ_CMD_CM_14HZ                             0x01
#define MMC3680KJ_CMD_CM_5HZ                              0x02
#define MMC3680KJ_CMD_CM_1HZ                              0x04
#define MMC3680KJ_CMD_SW_RST                              0x80
#define MMC3680KJ_CMD_PASSWORD                            0xe1
#define MMC3680KJ_CMD_OTP_OPER                            0x11
#define MMC3680KJ_CMD_OTP_MR                              0x80
#define MMC3680KJ_CMD_OTP_ACT                             0x80
#define MMC3680KJ_CMD_OTP_NACT                            0x00
#define MMC3680KJ_CMD_STSET_OPEN                          0x02
#define MMC3680KJ_CMD_STRST_OPEN                          0x04
#define MMC3680KJ_CMD_ST_CLOSE                            0x00
#define MMC3680KJ_CMD_INT_MD_EN                           0x40
#define MMC3680KJ_CMD_INT_MDT_EN                          0x20

#define MMC3680KJ_PRODUCT_ID                              0x0a
#define MMC3680KJ_OTP_READ_DONE_BIT                       0x10
#define MMC3680KJ_PUMP_ON_BIT                             0x08
#define MMC3680KJ_MDT_BIT                                 0x04
#define MMC3680KJ_MEAS_T_DONE_BIT                         0x02
#define MMC3680KJ_MEAS_M_DONE_BIT                         0x01

#define MMC3680KJ_I2C_SLAVE_ADDR                          0x30
#define MMC3680KJ_ADDR_TRANS(n)                           ((n)<<1)
#define MMC3680KJ_I2C_ADDR                                MMC3680KJ_ADDR_TRANS(MMC3680KJ_I2C_SLAVE_ADDR)

#define MMC3680KJ_OFFSET                                  32768
#define MMC3680KJ_SENSITIVITY                             1024
#define MMC3680KJ_T_ZERO                                  -75
#define MMC3680KJ_T_SENSITIVITY                           80

#define MMC3680KJ_MAG_DATA_SIZE                           6
#define OTP_CONVERT(REG)                                  (((REG) >=32 ? (32 - (REG)) : (REG)) * 6)

MMC3680KJ::MMC3680KJ(I2C &i2c_obj, char slave_adr)
    : i2c(i2c_obj), address(slave_adr<<1)
{
}

MMC3680KJ::~MMC3680KJ()
{
}
    
bool MMC3680KJ::initialize()
{
    char cmd[18];
    uint8_t reg_data[2] = {0};

    cmd[0] = MMC3680KJ_REG_STATUS;
    if(i2c.write(address, cmd, 1, true) != 0) {
        return false;
    }
    if(i2c.read(address, reinterpret_cast<char*>(reg_data), 1) != 0) {
        return false;
    }
    if ((reg_data[0] & MMC3680KJ_OTP_READ_DONE_BIT) != MMC3680KJ_OTP_READ_DONE_BIT) {
        return false;
    }

    cmd[0] = MMC3680KJ_REG_CTRL1;
    cmd[1] = MMC3680KJ_CMD_200HZ;
    if(i2c.write(address, cmd, 2) != 0) {
        return false;
    }

    return true;
} 

uint8_t MMC3680KJ::getId()
{
    uint8_t id = 0;
    char cmd = MMC3680KJ_REG_PRODUCTID;
    if(i2c.write(address, &cmd, 1, true) != 0) {
        return false;
    }
    i2c.read(address, reinterpret_cast<char*>(&id), 1);
    return id;
}

bool MMC3680KJ::enable()
{
    char cmd[2];

    cmd[0] = MMC3680KJ_REG_CTRL0;
    cmd[1] = MMC3680KJ_CMD_SET;
    if(i2c.write(address, cmd, 2) != 0) {
        return false;
    }

    cmd[1] = MMC3680KJ_CMD_TM_M|MMC3680KJ_CMD_TM_T;
    if(i2c.write(address, cmd, 2) != 0) {
        return false;
    }

    do {
        ThisThread::sleep_for(5ms);

        cmd[0] = MMC3680KJ_REG_STATUS;
        if(i2c.write(address, cmd, 1, true) != 0) {
            return false;
        }
        if(i2c.read(address, reinterpret_cast<char*>(cmd), 1) != 0) {
            return false;
        }
    } while ((cmd[0] & 0x03) != 0x03);

    return true;
}
 
bool MMC3680KJ::getMagnets(Magnets *result)
{

    uint8_t reg_raw[MMC3680KJ_MAG_DATA_SIZE] = {0};
    int32_t data_raw[3] = {0};

    if(!enable())
    {
        return false;
    }

    char cmd = MMC3680KJ_REG_DATA;
    if(i2c.write(address, &cmd, 1, true) != 0) {
        return false;
    }
    if(i2c.read(address, reinterpret_cast<char*>(reg_raw), MMC3680KJ_MAG_DATA_SIZE) != 0) {
        return false;
    }

    data_raw[0] = (uint16_t)(reg_raw[1] << 8 | reg_raw[0]);
    data_raw[1] = (uint16_t)(reg_raw[3] << 8 | reg_raw[2]);
    data_raw[2] = (uint16_t)(reg_raw[5] << 8 | reg_raw[4]);

    result->x= float(data_raw[0] - MMC3680KJ_OFFSET) / float(MMC3680KJ_SENSITIVITY);
    result->y = float(data_raw[1] - MMC3680KJ_OFFSET) / float(MMC3680KJ_SENSITIVITY);
    result->z = float(data_raw[2] - MMC3680KJ_OFFSET) / float(MMC3680KJ_SENSITIVITY);

    return true;
}
