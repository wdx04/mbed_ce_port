#pragma once

#if defined(FSMC_NORSRAM_DEVICE) || defined(FMC_NORSRAM_DEVICE)

#include "mbed.h"
#include "FMCTransport_8Bit.h"
#include "cvimgproc.h"

#ifndef ST7789_WIDTH
#define ST7789_WIDTH 320
#endif

#ifndef ST7789_HEIGHT
#define ST7789_HEIGHT 240
#endif

#if (ST7789_WIDTH == 240) && (ST7789_HEIGHT == 135)
#define ST7789_OFFSETX 40
#define ST7789_OFFSETY 53
#else
#define ST7789_OFFSETX 0
#define ST7789_OFFSETY 0
#endif

#define ST_CMD_DELAY 0x80 // special signifier for command lists

#define ST7789_NOP 0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID 0x04
#define ST7789_RDDST 0x09

#define ST7789_SLPIN 0x10
#define ST7789_SLPOUT 0x11
#define ST7789_PTLON 0x12
#define ST7789_NORON 0x13

#define ST7789_INVOFF 0x20
#define ST7789_INVON 0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON 0x29
#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C
#define ST7789_RAMRD 0x2E

#define ST7789_PTLAR 0x30
#define ST7789_COLMOD 0x3A
#define ST7789_MADCTL 0x36
#define ST7789_RAMCTRL 0xB0

#define ST7789_MADCTL_MY 0x80
#define ST7789_MADCTL_MX 0x40
#define ST7789_MADCTL_MV 0x20
#define ST7789_MADCTL_ML 0x10
#define ST7789_MADCTL_BGR 0x08
#define ST7789_MADCTL_RGB 0x00

#define ST7789_RDID1 0xDA
#define ST7789_RDID2 0xDB
#define ST7789_RDID3 0xDC
#define ST7789_RDID4 0xDD

#define ST7789_BLACK 0x0000
#define ST7789_BLUE 0x001F
#define ST7789_RED 0xF800
#define ST7789_GREEN 0x07E0
#define ST7789_CYAN 0x07FF
#define ST7789_MAGENTA 0xF81F
#define ST7789_YELLOW 0xFFE0
#define ST7789_WHITE 0xFFFF

class ST7789_FMC
{

public:
  ST7789_FMC(int subBankNo = 1, int addressNo = 0, PinName RST = NC);

  void init(void);
  void setAddrWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
  void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *image);
  void synchronize(cv::Painter &painter, int offset_x = 0, int offset_y = 0);
  void invertDisplay(bool i);

  void setRotation(uint8_t r);

private:
  FMCTransport_8Bit lcdPort;
  DigitalOut _rst;
  void writecommand(uint8_t c);
  void writedata(uint8_t d);
  int16_t _width = ST7789_WIDTH, _height = ST7789_HEIGHT;
  uint8_t _rotation = 0;  
};

#endif
