#include "VKL144.h"
#include <algorithm>

constexpr size_t num_of_digits = 8;
constexpr size_t char_model_size = 16;
static const char number_char[char_model_size]=
{
    //0    1    2    3    4    5    6    7    8    9    -    L    o    H    i   C
	'0','1','2','3','4','5','6','7','8','9','-','L','o','H','i', 'C'
};
static const uint8_t number_model[char_model_size]=
{
    //0    1    2    3    4    5    6    7    8    9    -    L    o    H    i   C
	0xF5,0x05,0xD3,0x97,0x27,0xB6,0xF6,0x15,0xF7,0xB7,0x02,0xE0,0xC6,0x67,0x05,0xF0
};

VKL144::VKL144(I2C *i2c) : i2c_(i2c) {
}

bool VKL144::init(void)
{
    uint8_t txBuff[Vkl144_SEGNUM / 2 + 2];

    // soft reset
    txBuff[0] = Vkl144_SOFTRST;
    if(i2c_->write((Vkl144_ADDR << 1) & 0xFE, (const char *) txBuff, 1) != 0) return false;
    
    // default mode
    txBuff[0] = Vkl144_FRPM1|Vkl144_SRPM1|Vkl144_FRAMER;
    if(i2c_->write((Vkl144_ADDR << 1) & 0xFE, (const char *) txBuff, 1) != 0) return false;

    // clear display
    txBuff[0] = Vkl144_ADDR5_0;
    txBuff[1] = Vkl144_ADSET;
    memset(&txBuff[2], 0, Vkl144_SEGNUM / 2);
    if(i2c_->write((Vkl144_ADDR << 1) & 0xFE, (const char *) txBuff, Vkl144_SEGNUM / 2 + 2) != 0) return false;

    // set bias and enable display
    txBuff[0] = Vkl144_MODESET_1_3_ON;
    if(i2c_->write((Vkl144_ADDR << 1) & 0xFE, (const char *) txBuff, 1) != 0) return false;

    return true;
}

 bool VKL144::standby()
 {
    uint8_t txCommand = Vkl144_MODESET_1_3_OFF;
    if(i2c_->write((Vkl144_ADDR << 1) & 0xFE, (const char *) &txCommand, 1) != 0) return false;

    return true;
 }

 void VKL144::apon_on()
 {
    uint8_t txCommand = Vkl144_APCTL_APON_ON;
    i2c_->write((Vkl144_ADDR << 1) & 0xFE, (const char *) &txCommand, 1);
}

 void VKL144::apon_off()
 {
    uint8_t txCommand = Vkl144_APCTL_APON_ON;
    i2c_->write((Vkl144_ADDR << 1) & 0xFE, (const char *) &txCommand, 1);
}

bool VKL144::memset_all(uint8_t dat)
{
    uint8_t txBuff[Vkl144_SEGNUM / 2];
    memset(txBuff, dat, Vkl144_SEGNUM / 2);
    return write_data(0, txBuff, Vkl144_SEGNUM / 2);
}

bool VKL144::set_seg_com(uint8_t seg, uint8_t com)
{
    return write_byte(seg, (1 << com));
}

bool VKL144::reset_seg_com(uint8_t seg, uint8_t com)
{
    return write_byte(seg, ~(1 << com));
}

bool VKL144::write_byte(uint8_t addr, uint8_t val)
{
    uint8_t txCommand[3];
    txCommand[0] = addr > 0x1f ? Vkl144_ADDR5_1: Vkl144_ADDR5_0;
    txCommand[1] = addr & 0x1f;
    txCommand[2] = val;
    if(i2c_->write((Vkl144_ADDR << 1) & 0xFE, (const char *) txCommand, 3) != 0) return false;

    return true;
}

bool VKL144::write_data(uint8_t addr, uint8_t* p_data, uint8_t cnt)
{
    uint8_t txCommand[Vkl144_SEGNUM / 2 + 2];
    txCommand[0] = addr > 0x1f ? Vkl144_ADDR5_1: Vkl144_ADDR5_0;
    txCommand[1] = addr & 0x1f;
    if(cnt > Vkl144_SEGNUM / 2)
    {
        cnt = Vkl144_SEGNUM / 2;
    }
    memcpy(&txCommand[2], p_data, cnt);
    if(i2c_->write((Vkl144_ADDR << 1) & 0xFE, (const char *) txCommand, cnt + 2) != 0) return false;

    return true;
}

bool VKL144::draw_char(uint8_t position, char ch, bool with_dot)
{
    if(position >= num_of_digits)
    {
        return false;
    }
    const char *char_ptr = std::find(number_char, number_char + char_model_size, ch);
    uint8_t val = 0;
    if(char_ptr != number_char + char_model_size)
    {
        size_t char_index = char_ptr - number_char;
        val = number_model[char_index];
    }
    if(with_dot)
    {
        val |= 0x08;
    }
    return write_byte((position << 1), val);
}

bool VKL144::draw_string(uint8_t position, const char *str)
{
    while(*str != 0)
    {
        if(*(str+1) == '.')
        {
            if(!draw_char(position, *str, true)) return false;
            str++;
        }
        else
        {
            if(!draw_char(position, *str, false)) return false;
        }
        str++;
        position++;
    }
    return true;
}
