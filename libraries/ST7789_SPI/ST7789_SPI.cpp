#include "mbed.h"
#include "ST7789_SPI.h"
#include "glcdfont.h"
#include "dmaops.h"

// Constructor
ST7789_SPI::ST7789_SPI(PinName mosi, PinName miso, PinName sck, PinName cs, PinName rs, PinName rst)
  : lcdPort(mosi, miso, sck), _cs(cs), _rs(rs), _rst(rst)
{
#if DEVICE_SPI_ASYNCH
  lcdPort.set_dma_usage(DMA_USAGE_ALWAYS);
#endif
}

void ST7789_SPI::writecommand(uint8_t c)
{
  _rs = 0;
  if (_cs.is_connected())
    _cs = 0;
  lcdPort.write(c);
  if (_cs.is_connected())
    _cs = 1;
}

void ST7789_SPI::writedata(uint8_t c)
{
  _rs = 1;
  if (_cs.is_connected())
    _cs = 0;
  lcdPort.write(c);
  if (_cs.is_connected())
    _cs = 1;
}

// Initialization for ST7789 screens
void ST7789_SPI::init(void)
{
  resetSPISettings();

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

void ST7789_SPI::setAddrWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
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

void ST7789_SPI::setRotation(uint8_t m)
{
  writecommand(ST7789_MADCTL);
  uint8_t rotation = m % 4; // can't be higher than 3
  switch(rotation)
  {
   case 0:
        writedata(0x00);
        break;
   case 1:
        writedata(0x70);
        break;
  }
}

void ST7789_SPI::drawImage(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *image)
{
  if (x < 0 || y < 0 || x + w > ST7789_WIDTH || y + h > ST7789_HEIGHT)
  {
    return;
  }
  setAddrWindow(x, y, x + w - 1, y + h - 1);
  _rs = 1;
  const char *image_buffer = reinterpret_cast<const char *>(image);
  int bytes_to_transfer = int(w) * h * 2;
  if (_cs.is_connected())
    _cs = 0;
  // transfer max 65280 pixels at one time
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

void ST7789_SPI::synchronize(cv::Painter &painter, int offset_x, int offset_y)
{
  auto dirty_rects = painter.get_dirty_rects();
  if (dirty_rects.empty())
  {
    return;
  }
  resetSPISettings();
#if DEVICE_SPI_ASYNCH
  event_callback_t transferCallback([&](int event)
                                    { transferFlags.set(event); });
#endif
  for (auto dirty_rect : dirty_rects)
  {
    dirty_rect.x += offset_x;
    dirty_rect.y += offset_y;
    dirty_rect &= cv::Rect(0, 0, _width, _height);
    dirty_rect.x -= offset_x;
    dirty_rect.y -= offset_y;
    cv::Mat mat = painter.get_mat();
    if (mat.type == cv::RGB565)
    {
      if (mat.isContinuous() && dirty_rect.width == mat.cols)
      {
        // continous
        drawImage(dirty_rect.x + offset_x, dirty_rect.y + offset_y, dirty_rect.width, dirty_rect.height, mat.ptr<uint16_t>(dirty_rect.y));
      }
      else
      {
        // non-continous
        constexpr size_t block_size = MBED_CONF_ST7789_SPI_FRAMEBUFFER_SIZE / 2;
        uint16_t *block_buffer = reinterpret_cast<uint16_t*>(gfx_framebuffer);
        size_t rows_in_block = block_size / size_t(dirty_rect.width);
        if (rows_in_block >= 2)
        {
          size_t block_count = (dirty_rect.height + rows_in_block - 1) / rows_in_block;
          int16_t remaining_rows = dirty_rect.height;
          for (size_t block_id = 0; block_id < block_count; block_id++)
          {
            int16_t x = dirty_rect.x;
            int16_t y = dirty_rect.y + block_id * rows_in_block;
            int16_t width = dirty_rect.width;
            int16_t height = remaining_rows > rows_in_block ? rows_in_block : remaining_rows;
#if defined(DMA2D) && USE_DMA2D
            dma2d_flat_copy(mat, cv::Rect(x, y, width, height), block_buffer);
#else
            for (int16_t row = 0; row < height; row++)
            {
              std::copy_n(mat.ptr<uint16_t>(y + row, x), width, &block_buffer[width * row]);
            }
#endif
            drawImage(x + offset_x, y + offset_y, width, height, block_buffer);
            remaining_rows -= height;
          }
        }
        else
        {
          int16_t x = dirty_rect.x;
          int16_t width = dirty_rect.width;
          constexpr int16_t height = 1;
          for (int16_t y = dirty_rect.y; y < dirty_rect.y + dirty_rect.height; y++)
          {
            drawImage(x + offset_x, y + offset_y, width, height, mat.ptr<uint16_t>(y, x));
          }
        }
      }
    }
    else // mat.type == cv::RGB332
    {
      constexpr size_t block_size = MBED_CONF_ST7789_SPI_FRAMEBUFFER_SIZE / 4;
      // double buffering to hide rgb332 to rgb565 time const
      uint16_t *block_buffer_1 = reinterpret_cast<uint16_t*>(gfx_framebuffer);
      uint16_t *block_buffer_2 = reinterpret_cast<uint16_t*>(gfx_framebuffer + MBED_CONF_ST7789_SPI_FRAMEBUFFER_SIZE / 2);
      size_t rows_in_block = block_size / size_t(dirty_rect.width);
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
            std::transform(src_ptr, src_ptr + width, &current_block_buffer[width * row], &cv::rgb332_to_rgb565);
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
          std::transform(src_ptr, src_ptr + width, current_block_buffer, &cv::rgb332_to_rgb565);
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

void ST7789_SPI::invertDisplay(bool i)
{
  writecommand(i ? ST7789_INVON : ST7789_INVOFF);
}

void ST7789_SPI::resetSPISettings()
{
  lcdPort.format(ST7789_SPI_BITS, ST7789_SPI_MODE);
  lcdPort.frequency(ST7789_SPI_FREQ);
}

SPI& ST7789_SPI::getSPI()
{
    return lcdPort;
}
