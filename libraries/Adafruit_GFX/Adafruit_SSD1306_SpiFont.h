#include "Adafruit_SSD1306.h"

class Adafruit_SSD1306_SpiFont : public Adafruit_SSD1306_Spi
{
public:
    Adafruit_SSD1306_SpiFont(SPI &spi, PinName DC, PinName RST, PinName CS, PinName FontCS, uint8_t rawHieght = 32, uint8_t rawWidth = 128, bool flipVertical = false);

    void draw_gb2312_string(uint16_t x, uint16_t y, const char *text_);

protected:
    void get_font_data(uint8_t addrHigh, uint8_t addrMid, uint8_t addrLow, uint8_t *pbuff, uint16_t DataLen);
    void draw_vertical_8p(int x, int y, uint8_t b);
    void draw_8x16(int x, int y, uint8_t *b);
    void draw_16x16(int x, int y, uint8_t *b);

    DigitalOut font_cs;
};
