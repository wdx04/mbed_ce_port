#include "mbed.h"
#include "FMCTransport_8Bit.h"

#if defined(FSMC_NORSRAM_DEVICE) || defined(FMC_NORSRAM_DEVICE)

static SRAM_HandleTypeDef TFTSRAM_Handler; 

static uint32_t TFT_LCD_BASE = 0;
#define TFT_LCD ((TFT_LCD_TypeDef *) TFT_LCD_BASE)

static void SRAM_GPIOInit(unsigned int subbank_no, unsigned int address_no)
{
    GPIO_InitTypeDef GPIO_Initure = { 0 };

#if defined(FSMC_NORSRAM_DEVICE) && !defined(STM32F1)
    __HAL_RCC_FSMC_CLK_ENABLE();
    GPIO_Initure.Alternate=GPIO_AF12_FSMC;
#elif defined(FMC_NORSRAM_DEVICE)
    __HAL_RCC_FMC_CLK_ENABLE();
    GPIO_Initure.Alternate=GPIO_AF12_FMC;
#endif

    GPIO_Initure.Mode=GPIO_MODE_AF_PP; 
    GPIO_Initure.Pull=GPIO_NOPULL;
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_VERY_HIGH;

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_Initure.Pin=GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_14|GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);

    GPIO_Initure.Pin=GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
    HAL_GPIO_Init(GPIOE,&GPIO_Initure);

    switch(subbank_no)
    {
    case 1: // PD7=NE1
        GPIO_Initure.Pin=GPIO_PIN_7;
        HAL_GPIO_Init(GPIOD,&GPIO_Initure);    
        break;
    case 2: // PG9=NE2
        __HAL_RCC_GPIOG_CLK_ENABLE();
        GPIO_Initure.Pin=GPIO_PIN_9;
        HAL_GPIO_Init(GPIOG,&GPIO_Initure);    
        break;
    }

    switch(address_no)
    {
    case 0:
        __HAL_RCC_GPIOF_CLK_ENABLE();
        GPIO_Initure.Pin=GPIO_PIN_0;
        HAL_GPIO_Init(GPIOF,&GPIO_Initure);
        break;
    case 19:
        GPIO_Initure.Pin=GPIO_PIN_3;
        HAL_GPIO_Init(GPIOE,&GPIO_Initure);
        break;
    }
}

#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
static void DisableCachingForSubBand(uint32_t subband_address)
{
    HAL_MPU_Disable();
    MPU_Region_InitTypeDef MPU_InitStruct;
    MPU_InitStruct.Enable=MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = subband_address;
    MPU_InitStruct.Size = MPU_REGION_SIZE_64MB; 
    MPU_InitStruct.AccessPermission=MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.TypeExtField=MPU_TEX_LEVEL0;
    MPU_InitStruct.IsCacheable=MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable=MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsShareable=MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number=MPU_REGION_NUMBER4; // max region number used in mbed_mpu_init is 3
    MPU_InitStruct.SubRegionDisable=0x00;
    MPU_InitStruct.DisableExec=MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
#endif

FMCTransport_8Bit::FMCTransport_8Bit(int subbank_no, int address_no)
{
  // Sub-Bank 1(PD7), Sub-Bank 2(PG9), Address A0(PF0), Address A19(PE3)
  if(subbank_no >= 1 && subbank_no <= 2 && (address_no == 0 || address_no == 19))
  {
      TFT_LCD_BASE = (uint32_t)0x60000000;
      TFT_LCD_BASE += (uint32_t)((subbank_no - 1) * 0x4000000);
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
      DisableCachingForSubBand(TFT_LCD_BASE);
#endif
      TFT_LCD_BASE += (uint32_t)((1U << address_no) - 1);
      init(subbank_no, address_no);
  }
}

bool FMCTransport_8Bit::is_valid()
{
    return TFT_LCD != nullptr;
}

void FMCTransport_8Bit::init(unsigned int subbank_no, unsigned int address_no)
{
    SRAM_GPIOInit(subbank_no, address_no);
#if defined(FSMC_NORSRAM_DEVICE) && !defined(STM32F1)
    FSMC_NORSRAM_TimingTypeDef FSMC_ReadWriteTim = {};
    FSMC_NORSRAM_TimingTypeDef FSMC_WriteTim = {};
    
    TFTSRAM_Handler.Instance=FSMC_NORSRAM_DEVICE;                
    TFTSRAM_Handler.Extended=FSMC_NORSRAM_EXTENDED_DEVICE;    
    
    TFTSRAM_Handler.Init.NSBank=(subbank_no - 1) * 2;
    TFTSRAM_Handler.Init.DataAddressMux=FSMC_DATA_ADDRESS_MUX_DISABLE; 
    TFTSRAM_Handler.Init.MemoryType=FSMC_MEMORY_TYPE_SRAM; 
    TFTSRAM_Handler.Init.MemoryDataWidth=FSMC_NORSRAM_MEM_BUS_WIDTH_8;
    TFTSRAM_Handler.Init.BurstAccessMode=FSMC_BURST_ACCESS_MODE_DISABLE;
    TFTSRAM_Handler.Init.WaitSignalPolarity=FSMC_WAIT_SIGNAL_POLARITY_LOW;
    TFTSRAM_Handler.Init.WaitSignalActive=FSMC_WAIT_TIMING_BEFORE_WS;
    TFTSRAM_Handler.Init.WriteOperation=FSMC_WRITE_OPERATION_ENABLE;
    TFTSRAM_Handler.Init.WaitSignal=FSMC_WAIT_SIGNAL_DISABLE;
    TFTSRAM_Handler.Init.ExtendedMode=FSMC_EXTENDED_MODE_ENABLE;
    TFTSRAM_Handler.Init.AsynchronousWait=FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    TFTSRAM_Handler.Init.WriteBurst=FSMC_WRITE_BURST_DISABLE;
    TFTSRAM_Handler.Init.ContinuousClock=FSMC_CONTINUOUS_CLOCK_SYNC_ASYNC;
    
    FSMC_ReadWriteTim.AddressSetupTime=0x0F;
    FSMC_ReadWriteTim.AddressHoldTime=1;
    FSMC_ReadWriteTim.DataSetupTime=60;
    FSMC_ReadWriteTim.AccessMode=FSMC_ACCESS_MODE_A;
    
    uint32_t HCLK_MHz = HAL_RCC_GetHCLKFreq() / 1000000UL;
    uint32_t Cycles = HCLK_MHz * 45 / 1000; // Write Cycle = 45us
    FSMC_WriteTim.AddressSetupTime = Cycles / 2;
    FSMC_WriteTim.AddressHoldTime = 0;
    FSMC_WriteTim.DataSetupTime = Cycles - Cycles / 2; // Minimal value for ST7789 = 6@HCLK=280MHz
    FSMC_WriteTim.BusTurnAroundDuration = 0;
    FSMC_WriteTim.AccessMode = FSMC_ACCESS_MODE_A;

    HAL_SRAM_Init(&TFTSRAM_Handler, &FSMC_ReadWriteTim,&FSMC_WriteTim);
#elif defined(FMC_NORSRAM_DEVICE)
    FMC_NORSRAM_TimingTypeDef FMC_ReadWriteTim = {};
    FMC_NORSRAM_TimingTypeDef FMC_WriteTim = {};
    
    TFTSRAM_Handler.Instance=FMC_NORSRAM_DEVICE;                
    TFTSRAM_Handler.Extended=FMC_NORSRAM_EXTENDED_DEVICE;    
    
    TFTSRAM_Handler.Init.NSBank=(subbank_no - 1) * 2;
    TFTSRAM_Handler.Init.DataAddressMux=FMC_DATA_ADDRESS_MUX_DISABLE; 
    TFTSRAM_Handler.Init.MemoryType=FMC_MEMORY_TYPE_SRAM; 
    TFTSRAM_Handler.Init.MemoryDataWidth=FMC_NORSRAM_MEM_BUS_WIDTH_8;
    TFTSRAM_Handler.Init.BurstAccessMode=FMC_BURST_ACCESS_MODE_DISABLE;
    TFTSRAM_Handler.Init.WaitSignalPolarity=FMC_WAIT_SIGNAL_POLARITY_LOW;
    TFTSRAM_Handler.Init.WaitSignalActive=FMC_WAIT_TIMING_BEFORE_WS;
    TFTSRAM_Handler.Init.WriteOperation=FMC_WRITE_OPERATION_ENABLE;
    TFTSRAM_Handler.Init.WaitSignal=FMC_WAIT_SIGNAL_DISABLE;
    TFTSRAM_Handler.Init.ExtendedMode=FMC_EXTENDED_MODE_ENABLE;
    TFTSRAM_Handler.Init.AsynchronousWait=FMC_ASYNCHRONOUS_WAIT_DISABLE;
    TFTSRAM_Handler.Init.WriteBurst=FMC_WRITE_BURST_DISABLE;
    TFTSRAM_Handler.Init.ContinuousClock=FMC_CONTINUOUS_CLOCK_SYNC_ONLY;
    TFTSRAM_Handler.Init.WriteFifo = FMC_WRITE_FIFO_ENABLE;
    TFTSRAM_Handler.Init.PageSize = FMC_PAGE_SIZE_NONE;

    FMC_ReadWriteTim.AddressSetupTime = 15;
    FMC_ReadWriteTim.AddressHoldTime = 0;
    FMC_ReadWriteTim.DataSetupTime = 60;
    FMC_ReadWriteTim.BusTurnAroundDuration = 0;
    FMC_ReadWriteTim.AccessMode = FMC_ACCESS_MODE_A;

    uint32_t HCLK_MHz = HAL_RCC_GetHCLKFreq() / 1000000UL;
    uint32_t Cycles = HCLK_MHz * 45 / 1000; // Write Cycle = 45us
    FMC_WriteTim.AddressSetupTime = Cycles / 2;
    FMC_WriteTim.AddressHoldTime = 0;
    FMC_WriteTim.DataSetupTime = Cycles - Cycles / 2; // Minimal value for ST7789 = 6@HCLK=280MHz
    FMC_WriteTim.BusTurnAroundDuration = 0;
    FMC_WriteTim.AccessMode = FMC_ACCESS_MODE_A;

    HAL_SRAM_Init(&TFTSRAM_Handler, &FMC_ReadWriteTim,&FMC_WriteTim);
#endif
}

void FMCTransport_8Bit::write_register(uint8_t regval)
{
    TFT_LCD->LCD_REG=regval;
}

void FMCTransport_8Bit::write_data(uint8_t data)
{
    TFT_LCD->LCD_RAM=data;
}

void FMCTransport_8Bit::write_data(const uint8_t *data, size_t count)
{
    for(size_t i = 0; i < count; i++)
    {
        TFT_LCD->LCD_RAM = data[i];
    }
}

uint8_t FMCTransport_8Bit::read_data(void)
{
    volatile uint8_t ram=TFT_LCD->LCD_RAM;
    return ram;
}

volatile uint8_t* FMCTransport_8Bit::get_register_pointer()
{
    return &TFT_LCD->LCD_REG;
}

volatile uint8_t* FMCTransport_8Bit::get_data_pointer()
{
    return &TFT_LCD->LCD_RAM;
}

#endif
