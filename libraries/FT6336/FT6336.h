#pragma once

#include "mbed.h"

class FT6336
{
public:
	FT6336(I2C& _i2c, PinName _rstpin = NC)
        : i2c(_i2c), rstPin(_rstpin) { }

	bool init();

    bool get_touch_state(int16_t& x, int16_t& y);

protected:
	I2C& i2c;
	DigitalOut rstPin;
};
