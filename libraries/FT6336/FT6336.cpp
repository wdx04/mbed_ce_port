#include "FT6336.h"

#define I2C_ADDR_FT6336 0x38
//
#define FT6336_ADDR_READ  0x71
#define FT6336_ADDR_WRITE 0x70

// Touch Parameter
#define FT6336_PRES_DOWN 0x2
#define FT6336_COORD_UD 0x1

// Registers
#define FT6336_ADDR_DEVICE_MODE 0x00

typedef enum
{
    working_mode = 0b000,
    factory_mode = 0b100,
} DEVICE_MODE_Enum;

#define FT6336_ADDR_GESTURE_ID 0x01
#define FT6336_ADDR_TD_STATUS 0x02

#define FT6336_ADDR_TOUCH1_EVENT 0x03
#define FT6336_ADDR_TOUCH1_ID 0x05
#define FT6336_ADDR_TOUCH1_X 0x03
#define FT6336_ADDR_TOUCH1_Y 0x05
#define FT6336_ADDR_TOUCH1_WEIGHT 0x07
#define FT6336_ADDR_TOUCH1_MISC 0x08

#define FT6336_ADDR_TOUCH2_EVENT 0x09
#define FT6336_ADDR_TOUCH2_ID 0x0B
#define FT6336_ADDR_TOUCH2_X 0x09
#define FT6336_ADDR_TOUCH2_Y 0x0B
#define FT6336_ADDR_TOUCH2_WEIGHT 0x0D
#define FT6336_ADDR_TOUCH2_MISC 0x0E

#define FT6336_ADDR_THRESHOLD 0x80
#define FT6336_ADDR_FILTER_COE 0x85
#define FT6336_ADDR_CTRL 0x86

typedef enum
{
    keep_active_mode = 0,
    switch_to_monitor_mode = 1,
} CTRL_MODE_Enum;

#define FT6336_ADDR_TIME_ENTER_MONITOR 0x87
#define FT6336_ADDR_ACTIVE_MODE_RATE 0x88
#define FT6336_ADDR_MONITOR_MODE_RATE 0x89

#define FT6336_ADDR_RADIAN_VALUE 0x91
#define FT6336_ADDR_OFFSET_LEFT_RIGHT 0x92
#define FT6336_ADDR_OFFSET_UP_DOWN 0x93
#define FT6336_ADDR_DISTANCE_LEFT_RIGHT 0x94
#define FT6336_ADDR_DISTANCE_UP_DOWN 0x95
#define FT6336_ADDR_DISTANCE_ZOOM 0x96

#define FT6336_ADDR_LIBRARY_VERSION_H 0xA1
#define FT6336_ADDR_LIBRARY_VERSION_L 0xA2
#define FT6336_ADDR_CHIP_ID 0xA3
#define FT6336_ADDR_G_MODE 0xA4

typedef enum
{
    pollingMode = 0,
    triggerMode = 1,
} G_MODE_Enum;

#define FT6336_ADDR_POWER_MODE 0xA5
#define FT6336_ADDR_FIRMARE_ID 0xA6
#define FT6336_ADDR_FOCALTECH_ID 0xA8
#define FT6336_ADDR_RELEASE_CODE_ID 0xAF
#define FT6336_ADDR_STATE 0xBC


bool FT6336::init()
{
    // reset
	if(rstPin.is_connected())
    {
        rstPin = 0;
        ThisThread::sleep_for(10ms);
        rstPin = 1;
        ThisThread::sleep_for(100ms);
    }
    // enable polling mode
    char command[2];
    command[0] = FT6336_ADDR_G_MODE;
    command[1] = pollingMode;
    i2c.write(FT6336_ADDR_WRITE, command, 2);
	return true;
}

bool FT6336::get_touch_state(int16_t& x, int16_t& y)
{
    // read touch status
    char command[2];
    command[0] = FT6336_ADDR_TD_STATUS;
    i2c.write(FT6336_ADDR_WRITE, command, 1);
    i2c.read(FT6336_ADDR_READ, command, 1);
    uint8_t touch_num = uint8_t(command[0] & 0x0F);
    if(touch_num == 1 || touch_num == 2)
    {
        command[0] = FT6336_ADDR_TOUCH1_X + 1;
        i2c.write(FT6336_ADDR_WRITE, command, 1);
        i2c.read(FT6336_ADDR_READ, command, 1);
        command[1] = FT6336_ADDR_TOUCH1_X;
        i2c.write(FT6336_ADDR_WRITE, command + 1, 1);
        i2c.read(FT6336_ADDR_READ, command + 1, 1);
        command[1] &= 0x0F;
        x = *reinterpret_cast<int16_t*>(command);
        command[0] = FT6336_ADDR_TOUCH1_Y + 1;
        i2c.write(FT6336_ADDR_WRITE, command, 1);
        i2c.read(FT6336_ADDR_READ, command, 1);
        command[1] = FT6336_ADDR_TOUCH1_Y;
        i2c.write(FT6336_ADDR_WRITE, command + 1, 1);
        i2c.read(FT6336_ADDR_READ, command + 1, 1);
        command[1] &= 0x0F;
        y = *reinterpret_cast<int16_t*>(command);
        return true;
    }
    return false;
}
