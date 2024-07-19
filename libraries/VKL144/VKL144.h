#ifndef VKL144_H
#define VKL144_H

/**
 * Includes
 */
#include "mbed.h"

/**
 * Defines
 */
#define Vkl144_ADDR                 0x3e
#define Vkl144_SOFTRST       		0xea    //执行软件复位   
#define Vkl144_ADDR5_0       		0xe8    //映射地址bit5 
#define Vkl144_ADDR5_1       		0xec    //映射地址bit5 
#define Vkl144_ADSET       			0x00    //映射地址bit4-0 
#define Vkl144_MODESET_1_3_ON  	    0xc8    // 打开显示 1/3bias
#define Vkl144_MODESET_1_3_OFF 	    0xc0    // 关闭显示 1/3bias
#define Vkl144_MODESET_1_2_ON  	    0xcc    // 打开显示 1/2bias
#define Vkl144_MODESET_1_2_OFF 	    0xc4    // 关闭显示 1/2bias
//显示控制
#define Vkl144_DISCTL			    0xA0  	//bit7~bit5=101 
//bit4-bit3省电模式FR bit2LINE,FRAME驱动波形 bit1-bit0省电模式SR
//电流Inor=1  Im1=0.5Inor  Im2=0.67Inor  Ihp=1.8Inor
//省电模式FR
#define Vkl144_FRNOR                0x00  //bit4-bit3=00   FR NORMAL 上电默认 
#define Vkl144_FRPM1                0x08  //bit4-bit3=01   FR POWER SAVE MODE1
#define Vkl144_FRPM2                0x10  //bit4-bit3=10   FR POWER SAVE MODE2
#define Vkl144_FRPM3                0x18  //bit4-bit3=11   FR POWER SAVE MODE3 最省电
//省电模式SR
#define Vkl144_SRHP                 0x03  //bit1-bit0=11   SR NORMAL 
#define Vkl144_SRNOR                0x02  //bit1-bit0=10   SR POWER SAVE MODE1 上电默认 
#define Vkl144_SRPM2                0x01  //bit1-bit0=01   SR POWER SAVE MODE2
#define Vkl144_SRPM1                0x00  //bit1-bit0=00   SR POWER SAVE MODE1 最省电
//LINE FRAME波形驱动
#define Vkl144_LINER                0x00  //bit2=0   LINE翻转	上电默认
#define Vkl144_FRAMER               0x04  //bit2=1   FRAME翻转 省电
//常用显示控制
//Vkl144_FRPM1|Vkl144_SRPM1|Vkl144_FRAMER    //典型
//Vkl144_FRPM2|Vkl144_SRPM1|Vkl144_FRAMER    //
//Vkl144_FRPM3|Vkl144_SRPM1|Vkl144_FRAMER    //电流最省
//Vkl144_FRNOR|Vkl144_SRHP|Vkl144_LINER      //电流最大
//闪烁控制
#define Vkl144_BLKCTL_OFF           0xF0  // 关闭闪烁
#define Vkl144_BLKCTL_05HZ          0xF1  // 闪烁频率为0.5HZ
#define Vkl144_BLKCTL_1HZ           0xF2  // 闪烁频率为1HZ
#define Vkl144_BLKCTL_2HZ           0xF3  // 闪烁频率为2HZ
//全屏强行开或关,与显示内存内容无关,两个功能中全屏关为优先执行
#define Vkl144_APCTL_APON_ON        0xFE // 全屏强行全显示_开
#define Vkl144_APCTL_APON_OFF       0xFC // 全屏强行全显示_关
#define Vkl144_APCTL_APOFF_ON       0xFD // 全屏强行关显示_开
#define Vkl144_APCTL_APOFF_OFF      0xFC // 全屏强行关显示_关
//驱动seg数
#define 	Vkl144_SEGNUM			36

/**
 * VKL144  8-digit LCD Display
 */
class VKL144 {

public:

    /**
     * Constructor.
     *
     * @param i2c pointer to the mbed I2C object
     */
    VKL144(I2C* i2c);

    // initialization command
    bool init();

    // enter standby mode(re-init to exit standby mode)
    bool standby();

    // show/hide all pixels
    void apon_on();
    void apon_off();

    // set all data to the same value
    bool memset_all(uint8_t dat);

    // set or reset single seg/com
    bool set_seg_com(uint8_t seg, uint8_t com);
    bool reset_seg_com(uint8_t seg, uint8_t com);

    // write to VKL144 memory
    bool write_byte(uint8_t addr, uint8_t val);
    bool write_data(uint8_t addr, uint8_t* p_data, uint8_t cnt);

    // draw chracter & string
    bool draw_char(uint8_t position, char ch, bool with_dot = false);
    bool draw_string(uint8_t position, const char *str);

private:
    I2C* i2c_;
};

#endif /* VKL144_H */
