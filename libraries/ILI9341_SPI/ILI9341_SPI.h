#pragma once
#include "mbed.h"
#include "cvimgproc.h"
#include <stdint.h>
#include <stdbool.h>

// #defines

#define ILI9341_SPI_MODE 0x00
#define ILI9341_SPI_BITS 0x08
#define ILI9341_SPI_FREQ 45000000

#ifndef ILI9341_WIDTH
#define ILI9341_WIDTH 240
#endif
#ifndef ILI9341_HEIGHT
#define ILI9341_HEIGHT 320
#endif

#define ILI9341_NOP 0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID 0x04
#define ILI9341_RDDST 0x09

#define ILI9341_SLPIN 0x10
#define ILI9341_SLPOUT 0x11
#define ILI9341_PTLON 0x12
#define ILI9341_NORON 0x13

#define ILI9341_RDMODE 0x0A
#define ILI9341_RDMADCTL 0x0B
#define ILI9341_RDPIXFMT 0x0C
#define ILI9341_RDIMGFMT 0x0A
#define ILI9341_RDSELFDIAG 0x0F

#define ILI9341_INVOFF 0x20
#define ILI9341_INVON 0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON 0x29

#define ILI9341_CASET 0x2A
#define ILI9341_PASET 0x2B
#define ILI9341_RAMWR 0x2C
#define ILI9341_RAMRD 0x2E

#define ILI9341_PTLAR 0x30
#define ILI9341_MADCTL 0x36
#define ILI9341_PIXFMT 0x3A

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR 0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1 0xC0
#define ILI9341_PWCTR2 0xC1
#define ILI9341_PWCTR3 0xC2
#define ILI9341_PWCTR4 0xC3
#define ILI9341_PWCTR5 0xC4
#define ILI9341_VMCTR1 0xC5
#define ILI9341_VMCTR2 0xC7

#define ILI9341_RDID1 0xDA
#define ILI9341_RDID2 0xDB
#define ILI9341_RDID3 0xDC
#define ILI9341_RDID4 0xDD

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1

// Color definitions
#define ILI9341_BLACK 0x0000
#define ILI9341_BLUE 0x001F
#define ILI9341_RED 0xF800
#define ILI9341_GREEN 0x07E0
#define ILI9341_CYAN 0x07FF
#define ILI9341_MAGENTA 0xF81F
#define ILI9341_YELLOW 0xFFE0
#define ILI9341_WHITE 0xFFFF

class ILI9341_SPI
{
public:
  ILI9341_SPI(PinName MOSI, PinName MISO, PinName SCK, PinName DC, PinName CS, PinName RST);

  void init(void);
  void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
  void setRotation(uint8_t r);
  void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *image);
  void synchronize(cv::Painter &painter, int offset_x = 0, int offset_y = 0);
  void invertDisplay(bool i);

  void resetSPISettings();
  SPI& getSPI();
  void writecommand(uint8_t c);
  void writedata(uint8_t d);

private:
  SPI lcdPort;
#if DEVICE_SPI_ASYNCH
  rtos::EventFlags transferFlags;
#endif
  DigitalOut _rs;
  DigitalOut _cs;
  DigitalOut _rst;
  int16_t _width = ILI9341_WIDTH, _height = ILI9341_HEIGHT;
  uint8_t _rotation = 0;
  alignas(16) uint8_t gfx_framebuffer[MBED_CONF_ILI9341_SPI_FRAMEBUFFER_SIZE];
};
