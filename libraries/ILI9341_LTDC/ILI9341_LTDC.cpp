#include "mbed.h"
#include <stdint.h>
#include "ILI9341_LTDC.h"
#include "dmaops.h"

LTDC_HandleTypeDef LtdcHandler;

ILI9341_LTDC::ILI9341_LTDC(PinName MOSI, PinName MISO, PinName SCK, PinName DC, PinName CS, PinName RST)
    : lcdPort(MOSI, MISO, SCK), _rs(DC), _cs(CS), _rst(RST)
{
}

void ILI9341_LTDC::writecommand(uint8_t c)
{
  _rs.write(0);
  if (_cs.is_connected())
    _cs.write(0);
  lcdPort.write(c);
  if (_cs.is_connected())
    _cs.write(1);
}

void ILI9341_LTDC::writedata(uint8_t c)
{
  _rs.write(1);
  if (_cs.is_connected())
    _cs.write(0);
  lcdPort.write(c);
  if (_cs.is_connected())
    _cs.write(1);
}

void ILI9341_LTDC::init(void)
{
  LtdcHandler.Instance = LTDC;

  /* Timing configuration  (Typical configuration from ILI9341 datasheet)
        HSYNC=10 (9+1)
        HBP=20 (29-10+1)
        ActiveW=240 (269-20-10+1)
        HFP=10 (279-240-20-10+1)

        VSYNC=2 (1+1)
        VBP=2 (3-2+1)
        ActiveH=320 (323-2-2+1)
        VFP=4 (327-320-2-2+1)
    */

  /* Configure horizontal synchronization width */
  LtdcHandler.Init.HorizontalSync = ILI9341_HSYNC;
  /* Configure vertical synchronization height */
  LtdcHandler.Init.VerticalSync = ILI9341_VSYNC;
  /* Configure accumulated horizontal back porch */
  LtdcHandler.Init.AccumulatedHBP = ILI9341_HBP;
  /* Configure accumulated vertical back porch */
  LtdcHandler.Init.AccumulatedVBP = ILI9341_VBP;
  /* Configure accumulated active width */
  LtdcHandler.Init.AccumulatedActiveW = 269;
  /* Configure accumulated active height */
  LtdcHandler.Init.AccumulatedActiveH = 323;
  /* Configure total width */
  LtdcHandler.Init.TotalWidth = 279;
  /* Configure total height */
  LtdcHandler.Init.TotalHeigh = 327;

  /* Configure R,G,B component values for LCD background color */
  LtdcHandler.Init.Backcolor.Red = 0;
  LtdcHandler.Init.Backcolor.Blue = 0;
  LtdcHandler.Init.Backcolor.Green = 0;

  setLTDCClock();

  /* Polarity */
  LtdcHandler.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  LtdcHandler.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  LtdcHandler.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  LtdcHandler.Init.PCPolarity = LTDC_PCPOLARITY_IPC;

  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable the LTDC and DMA2D Clock */
  __HAL_RCC_LTDC_CLK_ENABLE();

  /* Enable GPIOs clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /* GPIOs Configuration */
  /*
   +------------------------+-----------------------+----------------------------+
   +                       LCD pins assignment                                   +
   +------------------------+-----------------------+----------------------------+
   |  LCD_TFT R2 <-> PC.10  |  LCD_TFT G2 <-> PA.06 |  LCD_TFT B2 <-> PD.06      |
   |  LCD_TFT R3 <-> PB.00  |  LCD_TFT G3 <-> PG.10 |  LCD_TFT B3 <-> PG.11      |
   |  LCD_TFT R4 <-> PA.11  |  LCD_TFT G4 <-> PB.10 |  LCD_TFT B4 <-> PG.12      |
   |  LCD_TFT R5 <-> PA.12  |  LCD_TFT G5 <-> PB.11 |  LCD_TFT B5 <-> PA.03      |
   |  LCD_TFT R6 <-> PB.01  |  LCD_TFT G6 <-> PC.07 |  LCD_TFT B6 <-> PB.08      |
   |  LCD_TFT R7 <-> PG.06  |  LCD_TFT G7 <-> PD.03 |  LCD_TFT B7 <-> PB.09      |
   -------------------------------------------------------------------------------
            |  LCD_TFT HSYNC <-> PC.06  | LCDTFT VSYNC <->  PA.04 |
            |  LCD_TFT CLK   <-> PG.07  | LCD_TFT DE   <->  PF.10 |
             -----------------------------------------------------
  */

  /* GPIOA configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 |
                           GPIO_PIN_11 | GPIO_PIN_12;
  GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
  GPIO_InitStructure.Alternate = GPIO_AF14_LTDC;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* GPIOB configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_8 | \
                           GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* GPIOC configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* GPIOD configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_3 | GPIO_PIN_6;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);

  /* GPIOF configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_10;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStructure);

  /* GPIOG configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_6 | GPIO_PIN_7 | \
                           GPIO_PIN_11;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

  /* GPIOB configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStructure.Alternate = GPIO_AF9_LTDC;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* GPIOG configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_10 | GPIO_PIN_12;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

  HAL_LTDC_Init(&LtdcHandler);

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

  writecommand(0xCA);
  writedata(0xC3);
  writedata(0x08);
  writedata(0x50);
  writecommand(ILI9341_POWERB);
  writedata(0x00);
  writedata(0xC1);
  writedata(0x30);
  writecommand(ILI9341_POWER_SEQ);
  writedata(0x64);
  writedata(0x03);
  writedata(0x12);
  writedata(0x81);
  writecommand(ILI9341_DTCA);
  writedata(0x85);
  writedata(0x00);
  writedata(0x78);
  writecommand(ILI9341_POWERA);
  writedata(0x39);
  writedata(0x2C);
  writedata(0x00);
  writedata(0x34);
  writedata(0x02);
  writecommand(ILI9341_PRC);
  writedata(0x20);
  writecommand(ILI9341_DTCB);
  writedata(0x00);
  writedata(0x00);
  writecommand(ILI9341_FRMCTR1);
  writedata(0x00);
  writedata(0x1B);
  writecommand(ILI9341_DFC);
  writedata(0x0A);
  writedata(0xA2);
  writecommand(ILI9341_POWER1);
  writedata(0x10);
  writecommand(ILI9341_POWER2);
  writedata(0x10);
  writecommand(ILI9341_VCOM1);
  writedata(0x45);
  writedata(0x15);
  writecommand(ILI9341_VCOM2);
  writedata(0x90);
  writecommand(ILI9341_MAC);
  writedata(0xC8);
  writecommand(ILI9341_3GAMMA_EN);
  writedata(0x00);
  writecommand(ILI9341_RGB_INTERFACE);
  writedata(0xC2);
  writecommand(ILI9341_DFC);
  writedata(0x0A);
  writedata(0xA7);
  writedata(0x27);
  writedata(0x04);
  
  /* Colomn address set */
  writecommand(ILI9341_COLUMN_ADDR);
  writedata(0x00);
  writedata(0x00);
  writedata(0x00);
  writedata(0xEF);
  /* Page address set */
  writecommand(ILI9341_PAGE_ADDR);
  writedata(0x00);
  writedata(0x00);
  writedata(0x01);
  writedata(0x3F);
  writecommand(ILI9341_INTERFACE);
  writedata(0x01);
  writedata(0x00);
  writedata(0x06);
  
  writecommand(ILI9341_GRAM);
  ThisThread::sleep_for(200ms);
  
  writecommand(ILI9341_GAMMA);
  writedata(0x01);
  
  writecommand(ILI9341_PGAMMA);
  writedata(0x0F);
  writedata(0x29);
  writedata(0x24);
  writedata(0x0C);
  writedata(0x0E);
  writedata(0x09);
  writedata(0x4E);
  writedata(0x78);
  writedata(0x3C);
  writedata(0x09);
  writedata(0x13);
  writedata(0x05);
  writedata(0x17);
  writedata(0x11);
  writedata(0x00);
  writecommand(ILI9341_NGAMMA);
  writedata(0x00);
  writedata(0x16);
  writedata(0x1B);
  writedata(0x04);
  writedata(0x11);
  writedata(0x07);
  writedata(0x31);
  writedata(0x33);
  writedata(0x42);
  writedata(0x05);
  writedata(0x0C);
  writedata(0x0A);
  writedata(0x28);
  writedata(0x2F);
  writedata(0x0F);
  
  writecommand(ILI9341_SLEEP_OUT);
  ThisThread::sleep_for(120ms);
  writecommand(ILI9341_DISPLAY_ON);
}

void ILI9341_LTDC::setLTDCClock()
{
#if defined(STM32F429xx)
  /* LCD clock configuration for STM32F429I-DISCO */
  /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
  /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
  /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/4 = 48 Mhz */
  /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_8 = 48/4 = 6Mhz */
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 4;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
#else
  error("Please inherit ILI9341_LTDC class and override setLTDCClock().");
#endif
}

void ILI9341_LTDC::synchronize(cv::Painter &painter)
{
  cv::Mat mat = painter.get_mat();
  if(!_layer_configured)
  {
    LTDC_LayerCfgTypeDef Layercfg;

    /* Layer Init */
    Layercfg.WindowX0 = 0;
    Layercfg.WindowX1 = mat.cols;
    Layercfg.WindowY0 = 0;
    Layercfg.WindowY1 = mat.rows;
    Layercfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
    Layercfg.FBStartAdress = reinterpret_cast<uint32_t>(mat.ptr<uint16_t>());
    Layercfg.Alpha = 255;
    Layercfg.Alpha0 = 0;
    Layercfg.Backcolor.Blue = 0;
    Layercfg.Backcolor.Green = 0;
    Layercfg.Backcolor.Red = 0;
    Layercfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    Layercfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    Layercfg.ImageWidth = mat.cols;
    Layercfg.ImageHeight = mat.rows;
    HAL_LTDC_ConfigLayer(&LtdcHandler, &Layercfg, LTDC_LAYER_1);
    HAL_LTDC_SetPitch(&LtdcHandler, mat.step[0] / mat.elemSize(), LTDC_LAYER_1);

    _layer_configured = true;
  }
  else
  {
    HAL_LTDC_SetAddress_NoReload(&LtdcHandler, reinterpret_cast<uint32_t>(mat.ptr<uint16_t>()), LTDC_LAYER_1);
    HAL_LTDC_SetPitch_NoReload(&LtdcHandler, mat.step[0] / mat.elemSize(), LTDC_LAYER_1);
    HAL_LTDC_Reload(&LtdcHandler, LTDC_RELOAD_VERTICAL_BLANKING);
  }
}

void ILI9341_LTDC::invertDisplay(bool i)
{
  writecommand(i ? ILI9341_DINVON : ILI9341_DINVOFF);
}

void ILI9341_LTDC::resetSPISettings()
{
  lcdPort.format(ILI9341_SPI_BITS, ILI9341_SPI_MODE);
  lcdPort.frequency(ILI9341_SPI_FREQ);
}

SPI& ILI9341_LTDC::getSPI()
{
    return lcdPort;
}
