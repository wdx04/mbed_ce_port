#include "mbed.h"
#include "ST7789_FMC_8Bit.h"

#if defined(FSMC_NORSRAM_DEVICE) || defined(FMC_NORSRAM_DEVICE)

#include "dmaops.h"

// Constructor
ST7789_FMC::ST7789_FMC(int subBankNo, int addressNo, PinName RST)
  : lcdPort(subBankNo, addressNo), _rst(RST)
{
}

void ST7789_FMC::writecommand(uint8_t c)
{
    lcdPort.write_register(c);
}

void ST7789_FMC::writedata(uint8_t d)
{
    lcdPort.write_data(d);
}

// Initialization for ST7789 screens
void ST7789_FMC::init(void)
{
  if (_rst.is_connected())
  {
    _rst = 1;
    ThisThread::sleep_for(120ms);
    _rst = 0;
    ThisThread::sleep_for(120ms);
    _rst = 1;
    ThisThread::sleep_for(120ms);
  }

  writecommand(ST7789_SWRESET);
  ThisThread::sleep_for(120ms);
  writecommand(ST7789_SLPOUT);
  ThisThread::sleep_for(120ms);

  writecommand(ST7789_RAMCTRL);
  writedata(0x00);
  writedata(0xF8); // little endian
  writecommand(ST7789_MADCTL);
  writedata(0x70);
  writecommand(ST7789_COLMOD);
  writedata(0x55);

  writecommand(ST7789_INVON);
  ThisThread::sleep_for(10ms);
  writecommand(ST7789_NORON);
  ThisThread::sleep_for(10ms);
  writecommand(ST7789_DISPOFF);
  ThisThread::sleep_for(10ms);
  writecommand(ST7789_DISPON);
  ThisThread::sleep_for(10ms);
}

void ST7789_FMC::setAddrWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  x0 += ST7789_OFFSETX;
  x1 += ST7789_OFFSETX;
  y0 += ST7789_OFFSETY;
  y1 += ST7789_OFFSETY;

  writecommand(ST7789_CASET); // Column addr set
  writedata(x0 >> 8);
  writedata(x0 & 0xFF);
  writedata(x1 >> 8);
  writedata(x1 & 0xFF);

  writecommand(ST7789_RASET); // Row addr set
  writedata(y0 >> 8);
  writedata(y0 & 0xFF);
  writedata(y1 >> 8);
  writedata(y1 & 0xFF);

  writecommand(ST7789_RAMWR); // write to RAM
}

void ST7789_FMC::setRotation(uint8_t m)
{
  writecommand(ST7789_MADCTL);
  rotation = m % 4; // can't be higher than 3
  uint8_t color_spec = ST7789_MADCTL_RGB;
  bool swap_width_height = false;
  if(m & 0x04)
  {
    color_spec = ST7789_MADCTL_BGR;
  }
  if(m & 0x08)
  {
    swap_width_height = true;
  }
  switch (rotation)
  {
  case 0:
    writedata(color_spec);
    _width = ST7789_WIDTH;
    _height = ST7789_HEIGHT;
    break;
  case 1:
    writedata(ST7789_MADCTL_MX | ST7789_MADCTL_MV | color_spec);
    _width = ST7789_HEIGHT;
    _height = ST7789_WIDTH;
    break;
  }
  if(swap_width_height)
  {
    std::swap(_width, _height);
  }
}

void ST7789_FMC::drawImage(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *image)
{
  if (x < 0 || y < 0 || x + w > ST7789_WIDTH || y + h > ST7789_HEIGHT)
  {
    return;
  }
  setAddrWindow(x, y, x + w - 1, y + h - 1);
  const uint8_t *image_buffer = reinterpret_cast<const uint8_t *>(image);
  int bytes_to_transfer = int(w) * h * 2;
  lcdPort.write_data(image_buffer, bytes_to_transfer);
}

void ST7789_FMC::synchronize(cv::Painter &painter, int offset_x, int offset_y)
{
  auto dirty_rects = painter.get_dirty_rects();
  if (dirty_rects.empty())
  {
    return;
  }
  for (auto dirty_rect: dirty_rects)
  {
    dirty_rect.x += offset_x;
    dirty_rect.y += offset_y;
    dirty_rect &= cv::Rect(0, 0, _width, _height);
    dirty_rect.x -= offset_x;
    dirty_rect.y -= offset_y;
    cv::Mat mat = painter.get_mat();
    if (mat.type == cv::RGB565)
    {
#if defined(DMA2D) && USE_DMA2D
      setAddrWindow(dirty_rect.x + offset_x, dirty_rect.y + offset_y, dirty_rect.x + offset_x + dirty_rect.width - 1, dirty_rect.y + offset_y + dirty_rect.height - 1);
      dma2d_flat_copy(mat, dirty_rect, lcdPort.get_data_pointer());
#else
      if (mat.isContinuous() && dirty_rect.width == mat.cols)
      {
        // continous
        drawImage(dirty_rect.x + offset_x, dirty_rect.y + offset_y, dirty_rect.width, dirty_rect.height, mat.ptr<uint16_t>(dirty_rect.y));
      }
      else
      {
        // non-continous
        int16_t x = dirty_rect.x;
        for (int16_t y = dirty_rect.y; y < dirty_rect.y + dirty_rect.height; y++)
        {
            drawImage(x + offset_x, y + offset_y, dirty_rect.width, 1, mat.ptr<uint16_t>(y, x));
        }
      }
#endif
    }
    else // mat.type == cv::RGB332
    {
#if defined(DMA2D) && USE_DMA2D
      setAddrWindow(dirty_rect.x + offset_x, dirty_rect.y + offset_y, dirty_rect.x + offset_x + dirty_rect.width - 1, dirty_rect.y + offset_y + dirty_rect.height - 1);
      dma2d_flat_rgb332_to_rgb565(mat, dirty_rect, lcdPort.get_data_pointer());
#else
        int16_t x = dirty_rect.x;
        int16_t y = dirty_rect.y;
        setAddrWindow(x + offset_x, y + offset_y, x + offset_x + dirty_rect.width - 1, y + offset_y + dirty_rect.height - 1);
        for (int16_t y = dirty_rect.y; y < dirty_rect.y + dirty_rect.height; y++)
        {
          uint8_t *src_ptr = mat.ptr<uint8_t>(y, x);
          for(int16_t x = 0; x < dirty_rect.width; x++)
          {
            uint16_t color = cv::rgb332_to_rgb565(src_ptr[x]);
            lcdPort.write_data(color & 0xFF);
            lcdPort.write_data(color >> 8);
          }
        }
#endif
    }
  }
  painter.reset_dirty_rects();
}

void ST7789_FMC::invertDisplay(bool i)
{
  writecommand(i ? ST7789_INVON : ST7789_INVOFF);
}

#endif
