#include "mbed.h"
#include <stdint.h>
#include "ILI9341_SPI.h"
#include "dmaops.h"

ILI9341_SPI::ILI9341_SPI(PinName MOSI, PinName MISO, PinName SCK, PinName DC, PinName CS, PinName RST)
    : lcdPort(MOSI, MISO, SCK), _rs(DC), _cs(CS), _rst(RST)
{
#if DEVICE_SPI_ASYNCH
  lcdPort.set_dma_usage(DMA_USAGE_ALWAYS);
#endif
}

void ILI9341_SPI::writecommand(uint8_t c)
{

  _rs.write(0);
  if (_cs.is_connected())
    _cs.write(0);
  lcdPort.write(c);
  if (_cs.is_connected())
    _cs.write(1);
}

void ILI9341_SPI::writedata(uint8_t c)
{
  _rs.write(1);
  if (_cs.is_connected())
    _cs.write(0);
  lcdPort.write(c);
  if (_cs.is_connected())
    _cs.write(1);
}

void ILI9341_SPI::init(void)
{
  if (_rst.is_connected())
  {
    _rst.write(0);
  }

  // toggle RST low to reset
  if (_rst.is_connected())
  {
    _rst.write(1);
    ThisThread::sleep_for(5ms);
    _rst.write(0);
    ThisThread::sleep_for(20ms);
    _rst.write(1);
    ThisThread::sleep_for(150ms);
  }

  resetSPISettings();

  writecommand(0xEF);
  writedata(0x03);
  writedata(0x80);
  writedata(0x02);

  writecommand(0xCF);
  writedata(0x00);
  writedata(0XC1);
  writedata(0X30);

  writecommand(0xED);
  writedata(0x64);
  writedata(0x03);
  writedata(0X12);
  writedata(0X81);

  writecommand(0xE8);
  writedata(0x85);
  writedata(0x00);
  writedata(0x78);

  writecommand(0xCB);
  writedata(0x39);
  writedata(0x2C);
  writedata(0x00);
  writedata(0x34);
  writedata(0x02);

  writecommand(0xF7);
  writedata(0x20);

  writecommand(0xEA);
  writedata(0x00);
  writedata(0x00);

  writecommand(ILI9341_PWCTR1); // Power control
  writedata(0x23);              // VRH[5:0]

  writecommand(ILI9341_PWCTR2); // Power control
  writedata(0x10);              // SAP[2:0];BT[3:0]

  writecommand(ILI9341_VMCTR1); // VCM control
  writedata(0x3e);              //
  writedata(0x28);

  writecommand(ILI9341_VMCTR2); // VCM control2
  writedata(0x86);              // --

  writecommand(ILI9341_MADCTL); // Memory Access Control
  writedata(0x48);

  writecommand(ILI9341_PIXFMT);
  writedata(0x55);

  writecommand(ILI9341_FRMCTR1);
  writedata(0x00);
  writedata(0x18);

  writecommand(ILI9341_DFUNCTR); // Display Function Control
  writedata(0x08);
  writedata(0x82);
  writedata(0x27);

  writecommand(0xF2); // 3Gamma Function Disable
  writedata(0x00);

  writecommand(ILI9341_GAMMASET); // Gamma curve selected
  writedata(0x01);

  writecommand(ILI9341_GMCTRP1); // Set Gamma
  writedata(0x0F);
  writedata(0x31);
  writedata(0x2B);
  writedata(0x0C);
  writedata(0x0E);
  writedata(0x08);
  writedata(0x4E);
  writedata(0xF1);
  writedata(0x37);
  writedata(0x07);
  writedata(0x10);
  writedata(0x03);
  writedata(0x0E);
  writedata(0x09);
  writedata(0x00);

  writecommand(ILI9341_GMCTRN1); // Set Gamma
  writedata(0x00);
  writedata(0x0E);
  writedata(0x14);
  writedata(0x03);
  writedata(0x11);
  writedata(0x07);
  writedata(0x31);
  writedata(0xC1);
  writedata(0x48);
  writedata(0x08);
  writedata(0x0F);
  writedata(0x0C);
  writedata(0x31);
  writedata(0x36);
  writedata(0x0F);

  writecommand(ILI9341_SLPOUT); // Exit Sleep
  ThisThread::sleep_for(120ms);
  writecommand(ILI9341_DISPON); // Display on
}

void ILI9341_SPI::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  writecommand(ILI9341_CASET); // Column addr set
  writedata(x0 >> 8);
  writedata(x0 & 0xFF); // XSTART
  writedata(x1 >> 8);
  writedata(x1 & 0xFF); // XEND

  writecommand(ILI9341_PASET); // Row addr set
  writedata(y0 >> 8);
  writedata(y0); // YSTART
  writedata(y1 >> 8);
  writedata(y1); // YEND

  writecommand(ILI9341_RAMWR); // write to RAM
}

#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH 0x04

void ILI9341_SPI::setRotation(uint8_t m)
{

  writecommand(ILI9341_MADCTL);
  _rotation = m % 4; // can't be higher than 3
  uint8_t color_spec = MADCTL_BGR;
  bool swap_width_height = false;
  if(m & 0x04)
  {
    color_spec = MADCTL_RGB;
  }
  if(m & 0x08)
  {
    swap_width_height = true;
  }
  switch (_rotation)
  {
  case 0:
    writedata(MADCTL_MX | color_spec);
    _width = ILI9341_WIDTH;
    _height = ILI9341_HEIGHT;
    break;
  case 1:
    writedata(MADCTL_MV | color_spec);
    _width = ILI9341_HEIGHT;
    _height = ILI9341_WIDTH;
    break;
  case 2:
    writedata(MADCTL_MY | color_spec);
    _width = ILI9341_WIDTH;
    _height = ILI9341_HEIGHT;
    break;
  case 3:
    writedata(MADCTL_MX | MADCTL_MY | MADCTL_MV | color_spec);
    _width = ILI9341_WIDTH;
    _height = ILI9341_HEIGHT;
    break;
  }
  if(swap_width_height)
  {
    std::swap(_width, _height);
  }
}

void ILI9341_SPI::drawImage(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *image)
{
  if (x < 0 || y < 0 || x + w > ILI9341_WIDTH || y + h > ILI9341_HEIGHT)
  {
    return;
  }
  setAddrWindow(x, y, x + w - 1, y + h - 1);
  _rs = 1;
  const char *image_buffer = reinterpret_cast<const char *>(image);
  int bytes_to_transfer = int(w) * h * 2;
  if (_cs.is_connected())
    _cs = 0;
  constexpr int max_dma_transfer_size = 65280;
  while (bytes_to_transfer > 0)
  {
    int bytes_in_batch = bytes_to_transfer > max_dma_transfer_size ? max_dma_transfer_size : bytes_to_transfer;
#if DEVICE_SPI_ASYNCH
    lcdPort.transfer_and_wait(image_buffer, bytes_in_batch, nullptr, 0);
#else
    lcdPort.write(image_buffer, bytes_in_batch, nullptr, 0);
#endif
    image_buffer += bytes_in_batch;
    bytes_to_transfer -= bytes_in_batch;
  }
  if (_cs.is_connected())
    _cs = 1;
}

void ILI9341_SPI::synchronize(cv::Painter &painter, int offset_x, int offset_y)
{
  auto dirty_rects = painter.get_dirty_rects();
  if(dirty_rects.empty())
  {
    return;
  }
  resetSPISettings();
#if DEVICE_SPI_ASYNCH
  event_callback_t transferCallback([&](int event) {
    transferFlags.set(event);
  });
#endif
  for(auto dirty_rect: dirty_rects)
  {
    dirty_rect.x += offset_x;
    dirty_rect.y += offset_y;
    dirty_rect &= cv::Rect(0, 0, _width, _height);
    dirty_rect.x -= offset_x;
    dirty_rect.y -= offset_y;
    cv::Mat mat = painter.get_mat();
    if (mat.type == cv::RGB565)
    {
      constexpr size_t block_size = MBED_CONF_ILI9341_SPI_FRAMEBUFFER_SIZE / 4;
      // double buffering to hide byte swapping time const
      uint16_t *block_buffer_1 = reinterpret_cast<uint16_t*>(gfx_framebuffer);
      uint16_t *block_buffer_2 = reinterpret_cast<uint16_t*>(gfx_framebuffer + MBED_CONF_ILI9341_SPI_FRAMEBUFFER_SIZE / 2);
      int16_t rows_in_block = int16_t(block_size / size_t(dirty_rect.width));
      if (rows_in_block >= 2)
      {
        size_t block_count = (dirty_rect.height + rows_in_block - 1) / rows_in_block;
        int16_t remaining_rows = dirty_rect.height;
        for (size_t block_id = 0; block_id < block_count; block_id++)
        {
          uint16_t *current_block_buffer = (block_id % 2) == 0 ? block_buffer_1 : block_buffer_2;
          int16_t x = dirty_rect.x;
          int16_t y = dirty_rect.y + block_id * rows_in_block;
          int16_t width = dirty_rect.width;
          int16_t height = remaining_rows > rows_in_block ? rows_in_block : remaining_rows;
          // TODO implement byte swapping using DMA2D
          for (int16_t row = 0; row < height; row++)
          {
            uint16_t *src_ptr = mat.ptr<uint16_t>(y + row, x);
            std::transform(src_ptr, src_ptr + width, &current_block_buffer[width * row], &cv::swap_bytes);
          }
          if (block_id == 0)
          {
            // Initialize address window at first block
            setAddrWindow(x + offset_x, y + offset_y, x + offset_x + dirty_rect.width - 1, y + offset_y + dirty_rect.height - 1);
            _rs = 1;
            if (_cs.is_connected())
              _cs = 0;
#if DEVICE_SPI_ASYNCH
            lcdPort.transfer(reinterpret_cast<const char *>(current_block_buffer), width * height * 2, nullptr, 0, transferCallback, SPI_EVENT_ALL);
#else
            lcdPort.write(reinterpret_cast<const char *>(current_block_buffer), width * height * 2, nullptr, 0);
#endif
          }
          else
          {
#if DEVICE_SPI_ASYNCH
            transferFlags.wait_any(SPI_EVENT_ALL);
            lcdPort.transfer(reinterpret_cast<const char *>(current_block_buffer), width * height * 2, nullptr, 0, transferCallback, SPI_EVENT_ALL);
#else
            lcdPort.write(reinterpret_cast<const char *>(current_block_buffer), width * height * 2, nullptr, 0);
#endif
          }
          remaining_rows -= height;
        }
#if DEVICE_SPI_ASYNCH
        transferFlags.wait_any(SPI_EVENT_ALL);
#endif
        if (_cs.is_connected())
          _cs = 1;
      }
      else
      {
        int16_t x = dirty_rect.x;
        int16_t width = dirty_rect.width;
        for (int16_t y = dirty_rect.y; y < dirty_rect.y + dirty_rect.height; y++)
        {
          uint16_t *current_block_buffer = (y % 2) == 0 ? block_buffer_1 : block_buffer_2;
          uint16_t *src_ptr = mat.ptr<uint16_t>(y, x);
          std::transform(src_ptr, src_ptr + width, current_block_buffer, &cv::swap_bytes);
          if (y == dirty_rect.y)
          {
            // Initialize address window at first block
            setAddrWindow(x + offset_x, y + offset_y, x + offset_x + dirty_rect.width - 1, y + offset_y + dirty_rect.height - 1);
            _rs = 1;
            if (_cs.is_connected())
              _cs = 0;
#if DEVICE_SPI_ASYNCH
            lcdPort.transfer(reinterpret_cast<const char *>(current_block_buffer), width * 2, nullptr, 0, transferCallback, SPI_EVENT_ALL);
#else
            lcdPort.write(reinterpret_cast<const char *>(current_block_buffer), width * 2, nullptr, 0);
#endif
          }
          else
          {
#if DEVICE_SPI_ASYNCH
            transferFlags.wait_any(SPI_EVENT_ALL);
            lcdPort.transfer(reinterpret_cast<const char *>(current_block_buffer), width * 2, nullptr, 0, transferCallback, SPI_EVENT_ALL);
#else
            lcdPort.write(reinterpret_cast<const char *>(current_block_buffer), width * 2, nullptr, 0);
#endif
          }
        }
#if DEVICE_SPI_ASYNCH
        transferFlags.wait_any(SPI_EVENT_ALL);
#endif
        if (_cs.is_connected())
          _cs = 1;
      }
    }
    else // mat.type == cv::RGB332
    {
      constexpr size_t block_size = MBED_CONF_ILI9341_SPI_FRAMEBUFFER_SIZE / 4;
      // double buffering to hide rgb332 to rgb565 time const
      uint16_t *block_buffer_1 = reinterpret_cast<uint16_t*>(gfx_framebuffer);
      uint16_t *block_buffer_2 = reinterpret_cast<uint16_t*>(gfx_framebuffer + MBED_CONF_ILI9341_SPI_FRAMEBUFFER_SIZE / 2);
      int16_t rows_in_block = int16_t(block_size / size_t(dirty_rect.width));
      if (rows_in_block >= 2)
      {
        size_t block_count = (dirty_rect.height + rows_in_block - 1) / rows_in_block;
        int16_t remaining_rows = dirty_rect.height;
        for (size_t block_id = 0; block_id < block_count; block_id++)
        {
          uint16_t *current_block_buffer = (block_id % 2) == 0 ? block_buffer_1 : block_buffer_2;
          int16_t x = dirty_rect.x;
          int16_t y = dirty_rect.y + block_id * rows_in_block;
          int16_t width = dirty_rect.width;
          int16_t height = remaining_rows > rows_in_block ? rows_in_block : remaining_rows;
          // TODO implement LUT using DMA2D
          for (int16_t row = 0; row < height; row++)
          {
            uint8_t *src_ptr = mat.ptr<uint8_t>(y + row, x);
            std::transform(src_ptr, src_ptr + width, &current_block_buffer[width * row], &cv::rgb332_to_rgb565_swapped);
          }
          if (block_id == 0)
          {
            // Initialize address window at first block
            setAddrWindow(x + offset_x, y + offset_y, x + offset_x + dirty_rect.width - 1, y + offset_y + dirty_rect.height - 1);
            _rs = 1;
            if (_cs.is_connected())
              _cs = 0;
#if DEVICE_SPI_ASYNCH
            lcdPort.transfer(reinterpret_cast<const char *>(current_block_buffer), width * height * 2, nullptr, 0, transferCallback, SPI_EVENT_ALL);
#else
            lcdPort.write(reinterpret_cast<const char *>(current_block_buffer), width * height * 2, nullptr, 0);
#endif
          }
          else
          {
#if DEVICE_SPI_ASYNCH
            transferFlags.wait_any(SPI_EVENT_ALL);
            lcdPort.transfer(reinterpret_cast<const char *>(current_block_buffer), width * height * 2, nullptr, 0, transferCallback, SPI_EVENT_ALL);
#else
            lcdPort.write(reinterpret_cast<const char *>(current_block_buffer), width * height * 2, nullptr, 0);
#endif
          }
          remaining_rows -= height;
        }
#if DEVICE_SPI_ASYNCH
        transferFlags.wait_any(SPI_EVENT_ALL);
#endif
        if (_cs.is_connected())
          _cs = 1;
      }
      else
      {
        int16_t x = dirty_rect.x;
        int16_t width = dirty_rect.width;
        for (int16_t y = dirty_rect.y; y < dirty_rect.y + dirty_rect.height; y++)
        {
          uint16_t *current_block_buffer = (y % 2) == 0 ? block_buffer_1 : block_buffer_2;
          uint8_t *src_ptr = mat.ptr<uint8_t>(y, x);
          std::transform(src_ptr, src_ptr + width, current_block_buffer, &cv::rgb332_to_rgb565_swapped);
          if (y == dirty_rect.y)
          {
            // Initialize address window at first block
            setAddrWindow(x + offset_x, y + offset_y, x + offset_x + dirty_rect.width - 1, y + offset_y + dirty_rect.height - 1);
            _rs = 1;
            if (_cs.is_connected())
              _cs = 0;
#if DEVICE_SPI_ASYNCH
            lcdPort.transfer(reinterpret_cast<const char *>(current_block_buffer), width * 2, nullptr, 0, transferCallback, SPI_EVENT_ALL);
#else
            lcdPort.write(reinterpret_cast<const char *>(current_block_buffer), width * 2, nullptr, 0);
#endif
          }
          else
          {
#if DEVICE_SPI_ASYNCH
            transferFlags.wait_any(SPI_EVENT_ALL);
            lcdPort.transfer(reinterpret_cast<const char *>(current_block_buffer), width * 2, nullptr, 0, transferCallback, SPI_EVENT_ALL);
#else
            lcdPort.write(reinterpret_cast<const char *>(current_block_buffer), width * 2, nullptr, 0);
#endif
          }
        }
#if DEVICE_SPI_ASYNCH
        transferFlags.wait_any(SPI_EVENT_ALL);
#endif
        if (_cs.is_connected())
          _cs = 1;
      }
    }
  }
  painter.reset_dirty_rects();
}

void ILI9341_SPI::invertDisplay(bool i)
{
  writecommand(i ? ILI9341_INVON : ILI9341_INVOFF);
}

void ILI9341_SPI::resetSPISettings()
{
  lcdPort.format(ILI9341_SPI_BITS, ILI9341_SPI_MODE);
  lcdPort.frequency(ILI9341_SPI_FREQ);
}

SPI& ILI9341_SPI::getSPI()
{
    return lcdPort;
}
