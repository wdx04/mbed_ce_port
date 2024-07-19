#include "XPT2046.h"

#define XPT2046_SPI_FREQ 2000000
#define XPT2046_SPI_FORMAT 0
#define XPT2046_SPI_BITS 8

bool XPT2046::init()
{
    csPin = 1;
    if(tirqPin.is_connected())
    {
        tirqPin.mode(PinMode::PullUp);
    }
    return true;
}

void XPT2046::set_conversion(float _x_a, float _x_offset, float _y_b, float _y_offset)
{
    x_a = _x_a;
    x_b = 0.0f;
    x_offset = _x_offset;
    y_a = 0.0f;
    y_b = _y_b;
    y_offset = _y_offset;
}

void XPT2046::set_conversion(float _x_a, float _x_b, float _x_offset, float _y_a, float _y_b, float _y_offset)
{
    x_a = _x_a;
    x_b = _x_b;
    x_offset = _x_offset;
    y_a = _y_a;
    y_b = _y_b;
    y_offset = _y_offset;
}

bool XPT2046::get_touch_state(int16_t& x, int16_t& y)
{
    if(tirqPin.is_connected() && tirqPin.read() != 0)
    {
        return false;
    }
    // read 6 times, ignoring the first result
    char x_cmds[] = { 0xD0, 0x00, 0x00, 0xD0, 0x00, 0x00, 0xD0, 0x00, 0x00, 0xD0, 0x00, 0x00, 0xD0, 0x00, 0x00, 0xD0, 0x00, 0x00 };
    char y_cmds[] = { 0x90, 0x00, 0x00, 0x90, 0x00, 0x00, 0x90, 0x00, 0x00, 0x90, 0x00, 0x00, 0x90, 0x00, 0x00, 0x90, 0x00, 0x00 };
    char x_results[18], y_results[18];
    spi.frequency(XPT2046_SPI_FREQ);
    spi.format(XPT2046_SPI_BITS, XPT2046_SPI_FORMAT);
    csPin = 0;
    spi.write(x_cmds, 18, x_results, 18);
    spi.write(y_cmds, 18, y_results, 18);
    csPin = 1;
    // get the median value
    uint16_t x_values[5], y_values[5];
    for(size_t i = 0; i < 5; i++)
    {
        size_t address = (i + 1) * 3 + 1;
        x_values[i] = uint8_t(x_results[address]) * uint16_t(32) + (uint8_t(x_results[address + 1]) >> 3);
        y_values[i] = uint8_t(y_results[address]) * uint16_t(32) + (uint8_t(y_results[address + 1]) >> 3);
    }
    std::sort(x_values, x_values + 5);
    std::sort(y_values, y_values + 5);
    uint16_t raw_x = x_values[2];
    uint16_t raw_y = y_values[2];
    bool valid = false;
    if(raw_x != 0 && raw_x != 4095 && raw_y != 0 && raw_y != 4095)
    {
        x = int16_t(float(raw_x) * x_a + float(raw_y) * x_b + x_offset);
        y = int16_t(float(raw_x) * y_a + float(raw_y) * y_b + y_offset);
        valid = true;
    }
    return valid;
}
