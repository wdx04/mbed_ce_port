#include "Adafruit_SSD1306_SpiFont.h"

Adafruit_SSD1306_SpiFont::Adafruit_SSD1306_SpiFont(SPI &spi, PinName DC, PinName RST, PinName CS, PinName FontCS, uint8_t rawHieght, uint8_t rawWidth, bool flipVertical)
    : Adafruit_SSD1306_Spi(spi, DC, RST, CS, rawHieght, rawWidth, flipVertical), font_cs(FontCS, true)
{
}

void Adafruit_SSD1306_SpiFont::get_font_data(uint8_t addrHigh, uint8_t addrMid, uint8_t addrLow, uint8_t *pbuff, uint16_t DataLen)
{
    font_cs = 0; // 拉低片�?
    mspi.write(0x03);
    mspi.write(addrHigh);
    mspi.write(addrMid);
    mspi.write(addrLow);
    mspi.write(nullptr, 0, reinterpret_cast<char*>(pbuff), DataLen);
    mspi.write(0x00);
    font_cs = 1; // 拉高片�?
}

void Adafruit_SSD1306_SpiFont::draw_vertical_8p(int x, int y, uint8_t b)
{
  if ((b & 0x80) == 0x80)
    drawPixel(x, y + 7, WHITE);
  if ((b & 0x40) == 0x40)
    drawPixel(x, y + 6, WHITE);
  if ((b & 0x20) == 0x20)
    drawPixel(x, y + 5, WHITE);
  if ((b & 0x10) == 0x10)
    drawPixel(x, y + 4, WHITE);
  if ((b & 0x08) == 0x08)
    drawPixel(x, y + 3, WHITE);
  if ((b & 0x04) == 0x04)
    drawPixel(x, y + 2, WHITE);
  if ((b & 0x02) == 0x02)
    drawPixel(x, y + 1, WHITE);
  if ((b & 0x01) == 0x01)
    drawPixel(x, y + 0, WHITE);
}

void Adafruit_SSD1306_SpiFont::draw_8x16(int x, int y, uint8_t *b)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        draw_vertical_8p(x + i, y, b[i]);
    }
    for (i = 0; i < 8; i++)
    {
        draw_vertical_8p(x + i, y + 8, b[i + 8]);
    }
}

void Adafruit_SSD1306_SpiFont::draw_16x16(int x, int y, uint8_t *b)
{
    int i;
    for (i = 0; i < 16; i++)
    {
        draw_vertical_8p(x + i, y, b[i]);
    }
    for (i = 0; i < 16; i++)
    {
        draw_vertical_8p(x + i, y + 8, b[i + 16]);
    }
}

void Adafruit_SSD1306_SpiFont::draw_gb2312_string(uint16_t x, uint16_t y, const char *text_)
{
    const uint8_t *text = reinterpret_cast<const uint8_t*>(text_);
    unsigned int fontaddr = 0;
    uint16_t i=0;
    uint8_t addrHigh,addrMid,addrLow;
    uint8_t fontbuf[32];
    while(text[i]>0x00)
    {
        if((text[i]>=0xb0)&&(text[i]<=0xf7)&&(text[i+1]>=0xa1))
        {
            //国标简体（gb2312）汉字在晶联讯字库IC中的地址由以下公式来计算�?
            //Address = ((MSB - 0xB0) * 94 + (LSB - 0xA1)+ 846)*32+ BaseAdd;BaseAdd=0
            fontaddr=((text[i]-0xb0)*94 + (text[i+1]-0xa1)+846) * 32;
            addrHigh=(fontaddr&0xff0000)>>16;   //地址的高8�?,�?24�?
            addrMid=(fontaddr&0xff00)>>8;       //地址的中8�?,�?24�?
            addrLow=(fontaddr&0xff);            //地址的低8�?,�?24�?
            
            get_font_data(addrHigh, addrMid, addrLow, fontbuf, 32);
            //�?32个字节的数据，存�?"fontbuf[32]"
            draw_16x16(x, y, fontbuf);
            //显示汉字到LCD上，y为页地址，x为列地址，fontbuf[]为数�?
            x+=16;
            i+=2;
        }
        else if((text[i]>=0xa1)&&(text[i]<=0xa3)&&(text[i+1]>=0xa1))
        {
            fontaddr=((text[i]-0xa1)*94 + (text[i+1]-0xa1)) * 32;
            
            addrHigh=(fontaddr&0xff0000)>>16;
            addrMid=(fontaddr&0xff00)>>8;
            addrLow=(fontaddr&0xff);
            
            get_font_data(addrHigh,addrMid,addrLow,fontbuf,32);
            draw_16x16(x, y, fontbuf);
            x+=16;
            i+=2;
        }
        else if((text[i]>=0x20)&&(text[i]<=0x7e))
        {
            fontaddr=(text[i]-0x20) * 16 + 0x03b7c0;
            addrHigh=(fontaddr&0xff0000)>>16;
            addrMid=(fontaddr&0xff00)>>8;
            addrLow=fontaddr&0xff;
            
            get_font_data(addrHigh,addrMid,addrLow,fontbuf,16);
            draw_8x16(x, y, fontbuf);
            x+=8;
            i+=1;
        }
        else 
            i++;
    }
}
