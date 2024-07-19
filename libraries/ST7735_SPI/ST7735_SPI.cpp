#include "mbed.h"
#include "ST7735_SPI.h"
#include "dmaops.h"

// Constructor
ST7735_SPI::ST7735_SPI(PinName mosi, PinName miso, PinName sck, PinName cs, PinName rs, PinName rst)
    : lcdPort(mosi, miso, sck), _cs(cs), _rs(rs), _rst(rst)
{
#if DEVICE_SPI_ASYNCH
  lcdPort.set_dma_usage(DMA_USAGE_ALWAYS);
#endif
}

void ST7735_SPI::writecommand(uint8_t c)
{
  _rs = 0;
  _cs = 0;
  lcdPort.write(c);
  _cs = 1;
}

void ST7735_SPI::writedata(uint8_t c)
{
  _rs = 1;
  _cs = 0;
  lcdPort.write(c);
  _cs = 1;
}

// Rather than a bazillion writecommand() and writedata() calls, screen
// initialization commands and arguments are organized in these tables
// stored in PROGMEM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundreds of bytes more compact
// than the equivalent code.  Companion function follows.
#define DELAY 0x80
// based on Adafruit ST7735 library for Arduino
static const uint8_t
    init_cmds1[] = {           // Init for 7735R, part 1 (red or green tab)
        15,                    // 15 commands in list:
        ST7735_SWRESET, DELAY, //  1: Software reset, 0 args, w/delay
        150,                   //     150 ms delay
        ST7735_SLPOUT, DELAY,  //  2: Out of sleep mode, 0 args, w/delay
        255,                   //     500 ms delay
        ST7735_FRMCTR1, 3,     //  3: Frame rate ctrl - normal mode, 3 args:
        0x01, 0x2C, 0x2D,      //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
        ST7735_FRMCTR2, 3,     //  4: Frame rate control - idle mode, 3 args:
        0x01, 0x2C, 0x2D,      //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
        ST7735_FRMCTR3, 6,     //  5: Frame rate ctrl - partial mode, 6 args:
        0x01, 0x2C, 0x2D,      //     Dot inversion mode
        0x01, 0x2C, 0x2D,      //     Line inversion mode
        ST7735_INVCTR, 1,      //  6: Display inversion ctrl, 1 arg, no delay:
        0x07,                  //     No inversion
        ST7735_PWCTR1, 3,      //  7: Power control, 3 args, no delay:
        0xA2,
        0x02,             //     -4.6V
        0x84,             //     AUTO mode
        ST7735_PWCTR2, 1, //  8: Power control, 1 arg, no delay:
        0xC5,             //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
        ST7735_PWCTR3, 2, //  9: Power control, 2 args, no delay:
        0x0A,             //     Opamp current small
        0x00,             //     Boost frequency
        ST7735_PWCTR4, 2, // 10: Power control, 2 args, no delay:
        0x8A,             //     BCLK/2, Opamp current small & Medium low
        0x2A,
        ST7735_PWCTR5, 2, // 11: Power control, 2 args, no delay:
        0x8A, 0xEE,
        ST7735_VMCTR1, 1, // 12: Power control, 1 arg, no delay:
        0x0E,
        ST7735_INVOFF, 0, // 13: Don't invert display, no args, no delay
        ST7735_MADCTL, 1, // 14: Memory access control (directions), 1 arg:
        ST7735_ROTATION,  //     row addr/col addr, bottom to top refresh
        ST7735_COLMOD, 1, // 15: set color mode, 1 arg, no delay:
        0x05},            //     16-bit color

#if (defined(ST7735_IS_128X128) || defined(ST7735_IS_160X128))
    init_cmds2[] = {     // Init for 7735R, part 2 (1.44" display)
        2,               //  2 commands in list:
        ST7735_CASET, 4, //  1: Column addr set, 4 args, no delay:
        0x00, 0x00,      //     XSTART = 0
        0x00, 0x7F,      //     XEND = 127
        ST7735_RASET, 4, //  2: Row addr set, 4 args, no delay:
        0x00, 0x00,      //     XSTART = 0
        0x00, 0x7F},     //     XEND = 127
#endif                   // ST7735_IS_128X128

#ifdef ST7735_IS_160X80
    init_cmds2[] = {      // Init for 7735S, part 2 (160x80 display)
        3,                //  3 commands in list:
        ST7735_CASET, 4,  //  1: Column addr set, 4 args, no delay:
        0x00, 0x00,       //     XSTART = 0
        0x00, 0x4F,       //     XEND = 79
        ST7735_RASET, 4,  //  2: Row addr set, 4 args, no delay:
        0x00, 0x00,       //     XSTART = 0
        0x00, 0x9F,       //     XEND = 159
        ST7735_INVON, 0}, //  3: Invert colors
#endif

    init_cmds3[] = {                                                                                                         // Init for 7735R, part 3 (red or green tab)
        4,                                                                                                                   //  4 commands in list:
        ST7735_GMCTRP1, 16,                                                                                                  //  1: Gamma Adjustments (pos. polarity), 16 args, no delay:
        0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10, ST7735_GMCTRN1, 16,  //  2: Gamma Adjustments (neg. polarity), 16 args, no delay:
        0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10, ST7735_NORON, DELAY, //  3: Normal display on, no args, w/delay
        10,                                                                                                                  //     10 ms delay
        ST7735_DISPON, DELAY,                                                                                                //  4: Main screen turn on, no args w/delay
        100};                                                                                                                //     100 ms delay

// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in byte array.
void ST7735_SPI::commandList(const uint8_t *addr)
{

  uint8_t numCommands, numArgs;
  uint16_t ms;

  numCommands = *addr++; // Number of commands to follow
  while (numCommands--)
  {                        // For each command...
    writecommand(*addr++); //   Read, issue command
    numArgs = *addr++;     //   Number of args to follow
    ms = numArgs & DELAY;  //   If hibit set, delay follows args
    numArgs &= ~DELAY;     //   Mask out delay bit
    while (numArgs--)
    {                     //   For each argument...
      writedata(*addr++); //     Read, issue argument
    }

    if (ms)
    {
      ms = *addr++; // Read post-command delay time (ms)
      if (ms == 255)
        ms = 500; // If 255, delay for 500 ms
      ThisThread::sleep_for(chrono::milliseconds(ms));
    }
  }
}

// Initialization code common to both 'B' and 'R' type displays
void ST7735_SPI::commonInit(const uint8_t *cmdList)
{

  _rs = 1;
  _cs = 1;

  resetSPISettings();

  // toggle RST low to reset; CS low so it'll listen to us
  _cs = 0;
  if (_rst.is_connected())
  {
    _rst = 1;
    ThisThread::sleep_for(500ms);
    _rst = 0;
    ThisThread::sleep_for(500ms);
    _rst = 1;
    ThisThread::sleep_for(500ms);
  }

  if (cmdList)
    commandList(cmdList);
}

// Initialization for ST7735R screens (green or red tabs)
void ST7735_SPI::init()
{
  commonInit(init_cmds1);
  commandList(init_cmds2);
  commandList(init_cmds3);
}

void ST7735_SPI::setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1,
                                    uint8_t y1)
{

  writecommand(ST7735_CASET); // Column addr set
  writedata(0x00);
  writedata(x0 + ST7735_XSTART); // XSTART
  writedata(0x00);
  writedata(x1 + ST7735_XSTART); // XEND

  writecommand(ST7735_RASET); // Row addr set
  writedata(0x00);
  writedata(y0 + ST7735_YSTART); // YSTART
  writedata(0x00);
  writedata(y1 + ST7735_YSTART); // YEND

  writecommand(ST7735_RAMWR); // write to RAM
}

void ST7735_SPI::drawImage(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *image)
{
  if (x < 0 || y < 0 || x + w > ST7735_WIDTH || y + h > ST7735_HEIGHT)
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

void ST7735_SPI::synchronize(cv::Painter &painter, int offset_x, int offset_y)
{
  auto dirty_rects = painter.get_dirty_rects();
  if (dirty_rects.empty())
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
      constexpr size_t block_size = MBED_CONF_ST7735_SPI_FRAMEBUFFER_SIZE / 4;
      // double buffering to hide rgb332 to rgb565 time const
      uint16_t *block_buffer_1 = reinterpret_cast<uint16_t*>(gfx_framebuffer);
      uint16_t *block_buffer_2 = reinterpret_cast<uint16_t*>(gfx_framebuffer + MBED_CONF_ST7735_SPI_FRAMEBUFFER_SIZE / 2);
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
      constexpr size_t block_size = MBED_CONF_ST7735_SPI_FRAMEBUFFER_SIZE / 4;
      // double buffering to hide rgb332 to rgb565 time const
      uint16_t *block_buffer_1 = reinterpret_cast<uint16_t*>(gfx_framebuffer);
      uint16_t *block_buffer_2 = reinterpret_cast<uint16_t*>(gfx_framebuffer + MBED_CONF_ST7735_SPI_FRAMEBUFFER_SIZE / 2);
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

void ST7735_SPI::invertDisplay(bool i)
{
  writecommand(i ? ST7735_INVON : ST7735_INVOFF);
}

void ST7735_SPI::resetSPISettings()
{
  lcdPort.format(ST7735_SPI_BITS, ST7735_SPI_MODE);
  lcdPort.frequency(ST7735_SPI_FREQ);
}

SPI& ST7735_SPI::getSPI()
{
    return lcdPort;
}
