#pragma once

#include "mbed.h"

class XPT2046
{
public:
    XPT2046(SPI& _spi, PinName _cspin, PinName _tirq= NC)
        : spi(_spi), csPin(_cspin), tirqPin(_tirq) { }

    bool init();

    void set_conversion(float _x_a, float _x_offset, float _y_b, float _y_offset);

    void set_conversion(float _x_a, float _x_b, float _x_offset, float _y_a, float _y_b, float _y_offset);

    bool get_touch_state(int16_t& x, int16_t& y);

protected:
    SPI& spi;
    DigitalOut csPin;
    DigitalIn tirqPin;
    float x_a = 1.0f;
    float x_b = 0.0f;
    float x_offset = 0.0f;
    float y_a = 0.0f;
    float y_b = 1.0f;
    float y_offset = 0.0f;
};
