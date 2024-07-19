#include "mbed_error.h"
#include "stm32h7xx_hal.h"

#define SDRAM_BANK_ADDR     		((uint32_t)0xC0000000) 		// FMC SDRAM 数据基地址
#define FMC_SDRAM_BANK				FMC_SDRAM_BANK1
#define FMC_COMMAND_TARGET_BANK		FMC_SDRAM_CMD_TARGET_BANK1	//	SDRAM 的bank选择
#define MPU_SDRAM_SIZE				MPU_REGION_SIZE_32MB
#define SDRAM_TIMEOUT     			((uint32_t)0x1000) 			// 超时判断时间

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000) 
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200) 

#if MBED_CONF_TARGET_MAP_SDRAM
static void MPU_Config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct;

	HAL_MPU_Disable();

	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = SDRAM_BANK_ADDR;
	MPU_InitStruct.Size             = MPU_SDRAM_SIZE;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER2;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

SDRAM_HandleTypeDef hsdram1;		// SDRAM_HandleTypeDef 结构体变量
FMC_SDRAM_CommandTypeDef command;	// 控制指令

static void HAL_FMC_MspInit(void)
{
  GPIO_InitTypeDef GPIO_InitStruct ={0};
  /* Peripheral clock enable */
  __HAL_RCC_FMC_CLK_ENABLE();

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();  
  __HAL_RCC_GPIOF_CLK_ENABLE();  
  __HAL_RCC_GPIOG_CLK_ENABLE();  
  __HAL_RCC_GPIOH_CLK_ENABLE();  
  
  /** FMC GPIO Configuration
  PF0   ------> FMC_A0		PD14   ------> FMC_D0		PC0/PH5 -----> FMC_SDNWE
  PF1   ------> FMC_A1      PD15   ------> FMC_D1       PG15   ------> FMC_SDNCAS 
  PF2   ------> FMC_A2      PD0    ------> FMC_D2       PF11   ------> FMC_SDNRAS
  PF3   ------> FMC_A3      PD1    ------> FMC_D3       PH3    ------> FMC_SDNE0  
  PF4   ------> FMC_A4      PE7    ------> FMC_D4       PG4    ------> FMC_BA0
  PF5   ------> FMC_A5      PE8    ------> FMC_D5       PG5    ------> FMC_BA1
  PF12  ------> FMC_A6     	PE9    ------> FMC_D6       PH2    ------> FMC_SDCKE0 
  PF13  ------> FMC_A7     	PE10   ------> FMC_D7       PG8    ------> FMC_SDCLK
  PF14  ------> FMC_A8     	PE11   ------> FMC_D8       PE1    ------> FMC_NBL1
  PF15  ------> FMC_A9     	PE12   ------> FMC_D9       PE0    ------> FMC_NBL0
  PG0   ------> FMC_A10     PE13   ------> FMC_D10		PH6    ------> FMC_SDNE1
  PG1   ------> FMC_A11     PE14   ------> FMC_D11      PH7    ------> FMC_SDCKE1
  PG2   ------> FMC_A12     PE15   ------> FMC_D12
                            PD8    ------> FMC_D13
                            PD9    ------> FMC_D14
                            PD10   ------> FMC_D15

  */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_11|GPIO_PIN_12
                          |GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  GPIO_InitStruct.Pin =  GPIO_PIN_0;                    
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef* hsdram)
{
	HAL_FMC_MspInit();
}

void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command)
{
	__IO uint32_t tmpmrd = 0;

	/* Configure a clock configuration enable command */
	Command->CommandMode 				= FMC_SDRAM_CMD_CLK_ENABLE;	// 开启SDRAM时钟 
	Command->CommandTarget 				= FMC_COMMAND_TARGET_BANK; 	// 选择要控制的区域
	Command->AutoRefreshNumber 		= 1;
	Command->ModeRegisterDefinition 	= 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);	// 发送控制指令
	HAL_Delay(1);		// 延时等待

	/* Configure a PALL (precharge all) command */ 
	Command->CommandMode 				= FMC_SDRAM_CMD_PALL;		// 预充电命令
	Command->CommandTarget 				= FMC_COMMAND_TARGET_BANK;	// 选择要控制的区域
	Command->AutoRefreshNumber 		= 1;
	Command->ModeRegisterDefinition 	= 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);  // 发送控制指令

	/* Configure a Auto-Refresh command */ 
	Command->CommandMode 				= FMC_SDRAM_CMD_AUTOREFRESH_MODE;	// 使用自动刷新
	Command->CommandTarget 				= FMC_COMMAND_TARGET_BANK;          // 选择要控制的区域
	Command->AutoRefreshNumber			= 8;                                // 自动刷新次数
	Command->ModeRegisterDefinition 	= 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);	// 发送控制指令

	/* Program the external memory mode register */
	tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2         |
							SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
							SDRAM_MODEREG_CAS_LATENCY_3           |
							SDRAM_MODEREG_OPERATING_MODE_STANDARD |
							SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

	Command->CommandMode				= FMC_SDRAM_CMD_LOAD_MODE;	// 加载模式寄存器命令
	Command->CommandTarget 				= FMC_COMMAND_TARGET_BANK;	// 选择要控制的区域
	Command->AutoRefreshNumber 			= 1;
	Command->ModeRegisterDefinition 	= tmpmrd;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);	// 发送控制指令
	
	HAL_SDRAM_ProgramRefreshRate(hsdram, 918);  // 配置刷新率918@120MHz
}

void MX_FMC_Init(void)
{
	FMC_SDRAM_TimingTypeDef SdramTiming = {0};

	hsdram1.Instance = FMC_SDRAM_DEVICE;
	/* hsdram1.Init */
	hsdram1.Init.SDBank				 	= FMC_SDRAM_BANK;					// 选择BANK区
	hsdram1.Init.ColumnBitsNumber 		= FMC_SDRAM_COLUMN_BITS_NUM_9;         // 行地址宽度
	hsdram1.Init.RowBitsNumber 			= FMC_SDRAM_ROW_BITS_NUM_13;           // 列地址线宽度
	hsdram1.Init.MemoryDataWidth 		= FMC_SDRAM_MEM_BUS_WIDTH_16;          // 数据宽度  
	hsdram1.Init.InternalBankNumber 	= FMC_SDRAM_INTERN_BANKS_NUM_4;        // bank数量
	hsdram1.Init.CASLatency 			= FMC_SDRAM_CAS_LATENCY_3;             // CAS 
	hsdram1.Init.WriteProtection 		= FMC_SDRAM_WRITE_PROTECTION_DISABLE;  // 禁止写保护
	hsdram1.Init.SDClockPeriod 			= FMC_SDRAM_CLOCK_PERIOD_2;            // 分频
	hsdram1.Init.ReadBurst 				= FMC_SDRAM_RBURST_ENABLE;             // 突发模式
	hsdram1.Init.ReadPipeDelay 			= FMC_SDRAM_RPIPE_DELAY_1;             // 读延迟
	
	/* SdramTiming */
	SdramTiming.LoadToActiveDelay 		= 2;
	SdramTiming.ExitSelfRefreshDelay 	= 7;
	SdramTiming.SelfRefreshTime 		= 4;
	SdramTiming.RowCycleDelay 			= 7;
	SdramTiming.WriteRecoveryTime 		= 3;
	SdramTiming.RPDelay 				= 2;
	SdramTiming.RCDDelay 				= 2;

	HAL_SDRAM_Init(&hsdram1, &SdramTiming);	// 初始化FMC接口
									
	SDRAM_Initialization_Sequence(&hsdram1,&command); //配置SDRAM
}

void TargetBSP_Init(void)
{
    MX_FMC_Init();
    MPU_Config();
}
#endif
