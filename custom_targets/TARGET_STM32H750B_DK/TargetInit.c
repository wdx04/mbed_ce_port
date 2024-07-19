#include "mbed_error.h"
#include "stm32h7xx_hal.h"

#define SDRAM_BANK_ADDR ((uint32_t)0xD0000000)	 // FMC SDRAM 数据基地址
#define QSPI_BASE_ADDRESS ((uint32_t)0x90000000) // QSPI 内存映射模式的地址
#define FMC_SDRAM_BANK FMC_SDRAM_BANK2
#define FMC_COMMAND_TARGET_BANK FMC_SDRAM_CMD_TARGET_BANK2 //	SDRAM 的bank选择
#define MPU_SDRAM_SIZE MPU_REGION_SIZE_16MB

#define SDRAM_TIMEOUT ((uint32_t)0x1000) // 超时判断时间

#define SDRAM_MODEREG_BURST_LENGTH_1 ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2 ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4 ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8 ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2 ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3 ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE ((uint16_t)0x0200)

static void MPU_Config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct;

	HAL_MPU_Disable();

	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress = SDRAM_BANK_ADDR;
	MPU_InitStruct.Size = MPU_SDRAM_SIZE;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER2;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress = QSPI_BASE_ADDRESS;
	MPU_InitStruct.Size = MPU_REGION_SIZE_128MB;
	MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO_URO;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER3;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

SDRAM_HandleTypeDef hsdram1;	  // SDRAM_HandleTypeDef 结构体变量
FMC_SDRAM_CommandTypeDef command; // 控制指令

static void HAL_FMC_MspInit(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
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
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_0 | GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_0 | GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
}

void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef *hsdram)
{
	HAL_FMC_MspInit();
}

void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command)
{
	__IO uint32_t tmpmrd = 0;

	/* Configure a clock configuration enable command */
	Command->CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;  // 开启SDRAM时钟
	Command->CommandTarget = FMC_COMMAND_TARGET_BANK; // 选择要控制的区域
	Command->AutoRefreshNumber = 1;
	Command->ModeRegisterDefinition = 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT); // 发送控制指令
	HAL_Delay(1);										   // 延时等待

	/* Configure a PALL (precharge all) command */
	Command->CommandMode = FMC_SDRAM_CMD_PALL;		  // 预充电命令
	Command->CommandTarget = FMC_COMMAND_TARGET_BANK; // 选择要控制的区域
	Command->AutoRefreshNumber = 1;
	Command->ModeRegisterDefinition = 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT); // 发送控制指令

	/* Configure a Auto-Refresh command */
	Command->CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE; // 使用自动刷新
	Command->CommandTarget = FMC_COMMAND_TARGET_BANK;	   // 选择要控制的区域
	Command->AutoRefreshNumber = 8;						   // 自动刷新次数
	Command->ModeRegisterDefinition = 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT); // 发送控制指令

	/* Program the external memory mode register */
	tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2 |
			 SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |
			 SDRAM_MODEREG_CAS_LATENCY_3 |
			 SDRAM_MODEREG_OPERATING_MODE_STANDARD |
			 SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

	Command->CommandMode = FMC_SDRAM_CMD_LOAD_MODE;	  // 加载模式寄存器命令
	Command->CommandTarget = FMC_COMMAND_TARGET_BANK; // 选择要控制的区域
	Command->AutoRefreshNumber = 1;
	Command->ModeRegisterDefinition = tmpmrd;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT); // 发送控制指令

#if !MBED_CONF_TARGET_ENABLE_OVERDRIVE_MODE
	HAL_SDRAM_ProgramRefreshRate(hsdram, 1539); // 配置刷新率@100MHz
#else
	HAL_SDRAM_ProgramRefreshRate(hsdram, 1846); // 配置刷新率@120MHz
#endif
}

void MX_FMC_Init(void)
{
	FMC_SDRAM_TimingTypeDef SdramTiming = {0};

	hsdram1.Instance = FMC_SDRAM_DEVICE;
	/* hsdram1.Init */
	hsdram1.Init.SDBank = FMC_SDRAM_BANK;							   // 选择BANK区
	hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;	   // 行地址宽度
	hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;			   // 列地址线宽度
	hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;		   // 数据宽度
	hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;	   // bank数量
	hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;				   // CAS
	hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE; // 禁止写保护
	hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;			   // 分频
	hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;				   // 突发模式
#if !MBED_CONF_TARGET_ENABLE_OVERDRIVE_MODE
	hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0; // 读延迟
#else
	hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1; // 读延迟
#endif

	/* SdramTiming */
	SdramTiming.LoadToActiveDelay = 2;
	SdramTiming.ExitSelfRefreshDelay = 7;
	SdramTiming.SelfRefreshTime = 4;
	SdramTiming.RowCycleDelay = 7;
#if !MBED_CONF_TARGET_ENABLE_OVERDRIVE_MODE
	SdramTiming.WriteRecoveryTime = 2;
#else
	SdramTiming.WriteRecoveryTime = 3;
#endif
	SdramTiming.RPDelay = 2;
	SdramTiming.RCDDelay = 2;

	HAL_SDRAM_Init(&hsdram1, &SdramTiming); // 初始化FMC接口

	SDRAM_Initialization_Sequence(&hsdram1, &command); // 配置SDRAM
}

QSPI_HandleTypeDef hqspi;

/* Common Error codes */
#define BSP_ERROR_NONE 0
#define BSP_ERROR_NO_INIT -1
#define BSP_ERROR_WRONG_PARAM -2
#define BSP_ERROR_BUSY -3
#define BSP_ERROR_PERIPH_FAILURE -4
#define BSP_ERROR_COMPONENT_FAILURE -5
#define BSP_ERROR_UNKNOWN_FAILURE -6
#define BSP_ERROR_UNKNOWN_COMPONENT -7
#define BSP_ERROR_BUS_FAILURE -8
#define BSP_ERROR_CLOCK_FAILURE -9
#define BSP_ERROR_MSP_FAILURE -10
#define BSP_ERROR_FEATURE_NOT_SUPPORTED -11

/* BSP OSPI error codes */
#define BSP_ERROR_QSPI_ASSIGN_FAILURE -24
#define BSP_ERROR_QSPI_SETUP_FAILURE -25
#define BSP_ERROR_QSPI_MMP_LOCK_FAILURE -26
#define BSP_ERROR_QSPI_MMP_UNLOCK_FAILURE -27

/* MT25TL01G Component Error codes */
#define MT25TL01G_OK 0
#define MT25TL01G_ERROR_INIT -1
#define MT25TL01G_ERROR_COMMAND -2
#define MT25TL01G_ERROR_TRANSMIT -3
#define MT25TL01G_ERROR_RECEIVE -4
#define MT25TL01G_ERROR_AUTOPOLLING -5
#define MT25TL01G_ERROR_MEMORYMAPPED -6

/******************MT25TL01G_Transfer_t**********************/
typedef enum
{
	MT25TL01G_SPI_MODE = 0, /*!< 1-1-1 commands, Power on H/W default setting */
	MT25TL01G_SPI_2IO_MODE, /*!< 1-1-2, 1-2-2 read commands                   */
	MT25TL01G_SPI_4IO_MODE, /*!< 1-1-4, 1-4-4 read commands                   */
	MT25TL01G_QPI_MODE		/*!< 4-4-4 commands                               */
} MT25TL01G_Interface_t;

/******************MT25TL01G_Transfer_t**********************/

typedef enum
{
	MT25TL01G_STR_TRANSFER = 0, /* Single Transfer Rate */
	MT25TL01G_DTR_TRANSFER		/* Double Transfer Rate */
} MT25TL01G_Transfer_t;

/******************MT25TL01G_DualFlash_t**********************/

typedef enum
{
	MT25TL01G_DUALFLASH_DISABLE = QSPI_DUALFLASH_DISABLE, /*!<  Single flash mode              */
	MT25TL01G_DUALFLASH_ENABLE = QSPI_DUALFLASH_ENABLE
} MT25TL01G_DualFlash_t;

/******************MT25TL01G_Erase_t**********************/

typedef enum
{
	MT25TL01G_ERASE_4K = 0, /*!< 4K size Sector erase */
	MT25TL01G_ERASE_32K,	/*!< 32K size Block erase */
	MT25TL01G_ERASE_64K,	/*!< 64K size Block erase */
	MT25TL01G_ERASE_CHIP	/*!< Whole chip erase     */
} MT25TL01G_Erase_t;
/**
 * @}
 */

/** @defgroup MT25TL01G_Exported_Constants
 * @{
 */

/**
 * @brief  MT25TL01G Configuration
 */
#define MT25TL01G_FLASH_SIZE 0x8000000	/* 2 * 512 MBits => 2 * 64MBytes => 128MBytes*/
#define MT25TL01G_SECTOR_SIZE 0x10000	/* 2 * 1024 sectors of 64KBytes */
#define MT25TL01G_SUBSECTOR_SIZE 0x1000 /* 2 * 16384 subsectors of 4kBytes */
#define MT25TL01G_PAGE_SIZE 0x100		/* 2 * 262144 pages of 256 bytes */

#define MT25TL01G_DIE_ERASE_MAX_TIME 460000
#define MT25TL01G_SECTOR_ERASE_MAX_TIME 1000
#define MT25TL01G_SUBSECTOR_ERASE_MAX_TIME 400

/**
 * @brief  MT25TL01G Commands
 */
/* Reset Operations */
#define MT25TL01G_RESET_ENABLE_CMD 0x66
#define MT25TL01G_RESET_MEMORY_CMD 0x99

/* Identification Operations */
#define MT25TL01G_READ_ID_CMD 0x9E
#define MT25TL01G_READ_ID_CMD2 0x9F
#define MT25TL01G_MULTIPLE_IO_READ_ID_CMD 0xAF
#define MT25TL01G_READ_SERIAL_FLASH_DISCO_PARAM_CMD 0x5A

/* Read Operations */
#define MT25TL01G_READ_CMD 0x03
#define MT25TL01G_READ_4_BYTE_ADDR_CMD 0x13

#define MT25TL01G_FAST_READ_CMD 0x0B
#define MT25TL01G_FAST_READ_DTR_CMD 0x0D
#define MT25TL01G_FAST_READ_4_BYTE_ADDR_CMD 0x0C
#define MT25TL01G_FAST_READ_4_BYTE_DTR_CMD 0x0E

#define MT25TL01G_DUAL_OUT_FAST_READ_CMD 0x3B
#define MT25TL01G_DUAL_OUT_FAST_READ_DTR_CMD 0x3D
#define MT25TL01G_DUAL_OUT_FAST_READ_4_BYTE_ADDR_CMD 0x3C

#define MT25TL01G_DUAL_INOUT_FAST_READ_CMD 0xBB
#define MT25TL01G_DUAL_INOUT_FAST_READ_DTR_CMD 0xBD
#define MT25TL01G_DUAL_INOUT_FAST_READ_4_BYTE_ADDR_CMD 0xBC

#define MT25TL01G_QUAD_OUT_FAST_READ_CMD 0x6B
#define MT25TL01G_QUAD_OUT_FAST_READ_DTR_CMD 0x6D
#define MT25TL01G_QUAD_OUT_FAST_READ_4_BYTE_ADDR_CMD 0x6C

#define MT25TL01G_QUAD_INOUT_FAST_READ_CMD 0xEB
#define MT25TL01G_QUAD_INOUT_FAST_READ_DTR_CMD 0xED
#define MT25TL01G_QUAD_INOUT_FAST_READ_4_BYTE_ADDR_CMD 0xEC
#define MT25TL01G_QUAD_INOUT_FAST_READ_4_BYTE_DTR_CMD 0xEE
/* Write Operations */
#define MT25TL01G_WRITE_ENABLE_CMD 0x06
#define MT25TL01G_WRITE_DISABLE_CMD 0x04

/* Register Operations */
#define MT25TL01G_READ_STATUS_REG_CMD 0x05
#define MT25TL01G_WRITE_STATUS_REG_CMD 0x01

#define MT25TL01G_READ_LOCK_REG_CMD 0xE8
#define MT25TL01G_WRITE_LOCK_REG_CMD 0xE5

#define MT25TL01G_READ_FLAG_STATUS_REG_CMD 0x70
#define MT25TL01G_CLEAR_FLAG_STATUS_REG_CMD 0x50

#define MT25TL01G_READ_NONVOL_CFG_REG_CMD 0xB5
#define MT25TL01G_WRITE_NONVOL_CFG_REG_CMD 0xB1

#define MT25TL01G_READ_VOL_CFG_REG_CMD 0x85
#define MT25TL01G_WRITE_VOL_CFG_REG_CMD 0x81

#define MT25TL01G_READ_ENHANCED_VOL_CFG_REG_CMD 0x65
#define MT25TL01G_WRITE_ENHANCED_VOL_CFG_REG_CMD 0x61

#define MT25TL01G_READ_EXT_ADDR_REG_CMD 0xC8
#define MT25TL01G_WRITE_EXT_ADDR_REG_CMD 0xC5

/* Program Operations */
#define MT25TL01G_PAGE_PROG_CMD 0x02
#define MT25TL01G_PAGE_PROG_4_BYTE_ADDR_CMD 0x12

#define MT25TL01G_DUAL_IN_FAST_PROG_CMD 0xA2
#define MT25TL01G_EXT_DUAL_IN_FAST_PROG_CMD 0xD2

#define MT25TL01G_QUAD_IN_FAST_PROG_CMD 0x32
#define MT25TL01G_EXT_QUAD_IN_FAST_PROG_CMD 0x38
#define MT25TL01G_QUAD_IN_FAST_PROG_4_BYTE_ADDR_CMD 0x34

/* Erase Operations */
#define MT25TL01G_SUBSECTOR_ERASE_CMD_4K 0x20
#define MT25TL01G_SUBSECTOR_ERASE_4_BYTE_ADDR_CMD_4K 0x21

#define MT25TL01G_SUBSECTOR_ERASE_CMD_32K 0x52

#define MT25TL01G_SECTOR_ERASE_CMD 0xD8
#define MT25TL01G_SECTOR_ERASE_4_BYTE_ADDR_CMD 0xDC

#define MT25TL01G_DIE_ERASE_CMD 0xC7

#define MT25TL01G_PROG_ERASE_RESUME_CMD 0x7A
#define MT25TL01G_PROG_ERASE_SUSPEND_CMD 0x75

/* One-Time Programmable Operations */
#define MT25TL01G_READ_OTP_ARRAY_CMD 0x4B
#define MT25TL01G_PROG_OTP_ARRAY_CMD 0x42

/* 4-byte Address Mode Operations */
#define MT25TL01G_ENTER_4_BYTE_ADDR_MODE_CMD 0xB7
#define MT25TL01G_EXIT_4_BYTE_ADDR_MODE_CMD 0xE9

/* Quad Operations */
#define MT25TL01G_ENTER_QUAD_CMD 0x35
#define MT25TL01G_EXIT_QUAD_CMD 0xF5
#define MT25TL01G_ENTER_DEEP_POWER_DOWN 0xB9
#define MT25TL01G_RELEASE_FROM_DEEP_POWER_DOWN 0xAB

/*ADVANCED SECTOR PROTECTION Operations*/
#define MT25TL01G_READ_SECTOR_PROTECTION_CMD 0x2D
#define MT25TL01G_PROGRAM_SECTOR_PROTECTION 0x2C
#define MT25TL01G_READ_PASSWORD_CMD 0x27
#define MT25TL01G_WRITE_PASSWORD_CMD 0x28
#define MT25TL01G_UNLOCK_PASSWORD_CMD 0x29
#define MT25TL01G_READ_GLOBAL_FREEZE_BIT 0xA7
#define MT25TL01G_READ_VOLATILE_LOCK_BITS 0xE8
#define MT25TL01G_WRITE_VOLATILE_LOCK_BITS 0xE5
/*ADVANCED SECTOR PROTECTION Operations with 4-Byte Address*/
#define MT25TL01G_WRITE_4_BYTE_VOLATILE_LOCK_BITS 0xE1
#define MT25TL01G_READ_4_BYTE_VOLATILE_LOCK_BITS 0xE0
/*One Time Programmable Operations */
#define MT25TL01G_READ_OTP_ARRAY 0x4B
#define MT25TL01G_PROGRAM_OTP_ARRAY 0x42
/* MMP performance enhance reade enable/disable */
#define CONF_MT25TL01G_READ_ENHANCE 1

#define CONF_QSPI_ODS MT25TL01G_CR_ODS_15

#define CONF_QSPI_DUMMY_CLOCK 8U

/* Dummy cycles for STR read mode */
#define MT25TL01G_DUMMY_CYCLES_READ_QUAD 8U
#define MT25TL01G_DUMMY_CYCLES_READ 8U
/* Dummy cycles for DTR read mode */
#define MT25TL01G_DUMMY_CYCLES_READ_DTR 6U
#define MT25TL01G_DUMMY_CYCLES_READ_QUAD_DTR 8U

/**
 * @brief  MT25TL01G Registers
 */
/* Status Register */
#define MT25TL01G_SR_WIP ((uint8_t)0x01)	  /*!< Write in progress */
#define MT25TL01G_SR_WREN ((uint8_t)0x02)	  /*!< Write enable latch */
#define MT25TL01G_SR_BLOCKPR ((uint8_t)0x5C)  /*!< Block protected against program and erase operations */
#define MT25TL01G_SR_PRBOTTOM ((uint8_t)0x20) /*!< Protected memory area defined by BLOCKPR starts from top or bottom */
#define MT25TL01G_SR_SRWREN ((uint8_t)0x80)	  /*!< Status register write enable/disable */

/* Non volatile Configuration Register */
#define MT25TL01G_NVCR_NBADDR ((uint16_t)0x0001)   /*!< 3-bytes or 4-bytes addressing */
#define MT25TL01G_NVCR_SEGMENT ((uint16_t)0x0002)  /*!< Upper or lower 128Mb segment selected by default */
#define MT25TL01G_NVCR_DUAL ((uint16_t)0x0004)	   /*!< Dual I/O protocol */
#define MT25TL01G_NVCR_QUAB ((uint16_t)0x0008)	   /*!< Quad I/O protocol */
#define MT25TL01G_NVCR_RH ((uint16_t)0x0010)	   /*!< Reset/hold */
#define MT25TL01G_NVCR_DTRP ((uint16_t)0x0020)	   /*!< Double transfer rate protocol */
#define MT25TL01G_NVCR_ODS ((uint16_t)0x01C0)	   /*!< Output driver strength */
#define MT25TL01G_NVCR_XIP ((uint16_t)0x0E00)	   /*!< XIP mode at power-on reset */
#define MT25TL01G_NVCR_NB_DUMMY ((uint16_t)0xF000) /*!< Number of dummy clock cycles */

/* Volatile Configuration Register */
#define MT25TL01G_VCR_WRAP ((uint8_t)0x03)	   /*!< Wrap */
#define MT25TL01G_VCR_XIP ((uint8_t)0x08)	   /*!< XIP */
#define MT25TL01G_VCR_NB_DUMMY ((uint8_t)0xF0) /*!< Number of dummy clock cycles */

/* Extended Address Register */
#define MT25TL01G_EAR_HIGHEST_SE ((uint8_t)0x03) /*!< Select the Highest 128Mb segment */
#define MT25TL01G_EAR_THIRD_SEG ((uint8_t)0x02)	 /*!< Select the Third 128Mb segment */
#define MT25TL01G_EAR_SECOND_SEG ((uint8_t)0x01) /*!< Select the Second 128Mb segment */
#define MT25TL01G_EAR_LOWEST_SEG ((uint8_t)0x00) /*!< Select the Lowest 128Mb segment (default) */

/* Enhanced Volatile Configuration Register */
#define MT25TL01G_EVCR_ODS ((uint8_t)0x07)	/*!< Output driver strength */
#define MT25TL01G_EVCR_RH ((uint8_t)0x10)	/*!< Reset/hold */
#define MT25TL01G_EVCR_DTRP ((uint8_t)0x20) /*!< Double transfer rate protocol */
#define MT25TL01G_EVCR_DUAL ((uint8_t)0x40) /*!< Dual I/O protocol */
#define MT25TL01G_EVCR_QUAD ((uint8_t)0x80) /*!< Quad I/O protocol */

/* Flag Status Register */
#define MT25TL01G_FSR_NBADDR ((uint8_t)0x01) /*!< 3-bytes or 4-bytes addressing */
#define MT25TL01G_FSR_PRERR ((uint8_t)0x02)	 /*!< Protection error */
#define MT25TL01G_FSR_PGSUS ((uint8_t)0x04)	 /*!< Program operation suspended */
#define MT25TL01G_FSR_PGERR ((uint8_t)0x10)	 /*!< Program error */
#define MT25TL01G_FSR_ERERR ((uint8_t)0x20)	 /*!< Erase error */
#define MT25TL01G_FSR_ERSUS ((uint8_t)0x40)	 /*!< Erase operation suspended */
#define MT25TL01G_FSR_READY ((uint8_t)0x80)	 /*!< Ready or command in progress */

/* Definition for QSPI Flash ID */
#define BSP_QSPI_FLASH_ID QSPI_FLASH_ID_1

/* QSPI block sizes for dual flash */
#define BSP_QSPI_BLOCK_8K MT25TL01G_SECTOR_4K
#define BSP_QSPI_BLOCK_64K MT25TL01G_BLOCK_32K
#define BSP_QSPI_BLOCK_128K MT25TL01G_BLOCK_64K

/* Definition for QSPI clock resources */
#define QSPI_CLK_ENABLE() __HAL_RCC_QSPI_CLK_ENABLE()
#define QSPI_CLK_DISABLE() __HAL_RCC_QSPI_CLK_DISABLE()
#define QSPI_CLK_GPIO_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_BK1_CS_GPIO_CLK_ENABLE() __HAL_RCC_GPIOG_CLK_ENABLE()
#define QSPI_BK1_D0_GPIO_CLK_ENABLE() __HAL_RCC_GPIOD_CLK_ENABLE()
#define QSPI_BK1_D1_GPIO_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_BK1_D2_GPIO_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_BK1_D3_GPIO_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_BK2_CS_GPIO_CLK_ENABLE() __HAL_RCC_GPIOG_CLK_ENABLE()
#define QSPI_BK2_D0_GPIO_CLK_ENABLE() __HAL_RCC_GPIOH_CLK_ENABLE()
#define QSPI_BK2_D1_GPIO_CLK_ENABLE() __HAL_RCC_GPIOH_CLK_ENABLE()
#define QSPI_BK2_D2_GPIO_CLK_ENABLE() __HAL_RCC_GPIOG_CLK_ENABLE()
#define QSPI_BK2_D3_GPIO_CLK_ENABLE() __HAL_RCC_GPIOG_CLK_ENABLE()

#define QSPI_FORCE_RESET() __HAL_RCC_QSPI_FORCE_RESET()
#define QSPI_RELEASE_RESET() __HAL_RCC_QSPI_RELEASE_RESET()

/* Definition for QSPI Pins */
#define QSPI_CLK_PIN GPIO_PIN_10
#define QSPI_CLK_GPIO_PORT GPIOF
/* Bank 1 */
#define QSPI_BK1_CS_PIN GPIO_PIN_6
#define QSPI_BK1_CS_GPIO_PORT GPIOG
#define QSPI_BK1_D0_PIN GPIO_PIN_11
#define QSPI_BK1_D0_GPIO_PORT GPIOD
#define QSPI_BK1_D1_PIN GPIO_PIN_9
#define QSPI_BK1_D1_GPIO_PORT GPIOF
#define QSPI_BK1_D2_PIN GPIO_PIN_7
#define QSPI_BK1_D2_GPIO_PORT GPIOF
#define QSPI_BK1_D3_PIN GPIO_PIN_6
#define QSPI_BK1_D3_GPIO_PORT GPIOF

/* Bank 2 */
#define QSPI_BK2_CS_PIN GPIO_PIN_6
#define QSPI_BK2_CS_GPIO_PORT GPIOG
#define QSPI_BK2_D0_PIN GPIO_PIN_2
#define QSPI_BK2_D0_GPIO_PORT GPIOH
#define QSPI_BK2_D1_PIN GPIO_PIN_3
#define QSPI_BK2_D1_GPIO_PORT GPIOH
#define QSPI_BK2_D2_PIN GPIO_PIN_9
#define QSPI_BK2_D2_GPIO_PORT GPIOG
#define QSPI_BK2_D3_PIN GPIO_PIN_14
#define QSPI_BK2_D3_GPIO_PORT GPIOG

/* MT25TL01G Micron memory */
/* Size of the flash */
#define QSPI_FLASH_SIZE 26 /* Address bus width to access whole memory space */
#define QSPI_PAGE_SIZE 256

typedef struct
{
	uint32_t FlashSize;			 /*!< Size of the flash */
	uint32_t EraseSectorSize;	 /*!< Size of sectors for the erase operation */
	uint32_t EraseSectorsNumber; /*!< Number of sectors for the erase operation */
	uint32_t ProgPageSize;		 /*!< Size of pages for the program operation */
	uint32_t ProgPagesNumber;	 /*!< Number of pages for the program operation */
} MT25TL01G_Info_t;

typedef struct
{
	uint32_t FlashSize;
	uint32_t ClockPrescaler;
	uint32_t SampleShifting;
	uint32_t DualFlashMode;
} MX_QSPI_Init_t;

static void QSPI_MspInit(QSPI_HandleTypeDef *hQspi)
{
	GPIO_InitTypeDef gpio_init_structure;

	/* Prevent unused argument(s) compilation warning */
	UNUSED(hQspi);

	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* Enable the QuadSPI memory interface clock */
	QSPI_CLK_ENABLE();
	/* Reset the QuadSPI memory interface */
	QSPI_FORCE_RESET();
	QSPI_RELEASE_RESET();
	/* Enable GPIO clocks */
	QSPI_CLK_GPIO_CLK_ENABLE();
	QSPI_BK1_CS_GPIO_CLK_ENABLE();
	QSPI_BK1_D0_GPIO_CLK_ENABLE();
	QSPI_BK1_D1_GPIO_CLK_ENABLE();
	QSPI_BK1_D2_GPIO_CLK_ENABLE();
	QSPI_BK1_D3_GPIO_CLK_ENABLE();

	QSPI_BK2_CS_GPIO_CLK_ENABLE();
	QSPI_BK2_D0_GPIO_CLK_ENABLE();
	QSPI_BK2_D1_GPIO_CLK_ENABLE();
	QSPI_BK2_D2_GPIO_CLK_ENABLE();
	QSPI_BK2_D3_GPIO_CLK_ENABLE();

	/*##-2- Configure peripheral GPIO ##########################################*/
	/* QSPI CLK GPIO pin configuration  */
	gpio_init_structure.Pin = QSPI_CLK_PIN;
	gpio_init_structure.Mode = GPIO_MODE_AF_PP;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	gpio_init_structure.Pull = GPIO_NOPULL;
	gpio_init_structure.Alternate = GPIO_AF9_QUADSPI;
	HAL_GPIO_Init(QSPI_CLK_GPIO_PORT, &gpio_init_structure);

	/* QSPI CS GPIO pin configuration  */
	gpio_init_structure.Pin = QSPI_BK1_CS_PIN;
	gpio_init_structure.Pull = GPIO_PULLUP;
	gpio_init_structure.Alternate = GPIO_AF10_QUADSPI;
	HAL_GPIO_Init(QSPI_BK1_CS_GPIO_PORT, &gpio_init_structure);

	/* QSPI D0 GPIO pin configuration  */
	gpio_init_structure.Pin = QSPI_BK1_D0_PIN;
	gpio_init_structure.Pull = GPIO_NOPULL;
	gpio_init_structure.Alternate = GPIO_AF9_QUADSPI;
	HAL_GPIO_Init(QSPI_BK1_D0_GPIO_PORT, &gpio_init_structure);

	gpio_init_structure.Pin = QSPI_BK2_D0_PIN;
	gpio_init_structure.Alternate = GPIO_AF9_QUADSPI;
	HAL_GPIO_Init(QSPI_BK2_D0_GPIO_PORT, &gpio_init_structure);

	/* QSPI D1 GPIO pin configuration  */
	gpio_init_structure.Pin = QSPI_BK1_D1_PIN;
	gpio_init_structure.Alternate = GPIO_AF10_QUADSPI;
	HAL_GPIO_Init(QSPI_BK1_D1_GPIO_PORT, &gpio_init_structure);

	gpio_init_structure.Pin = QSPI_BK2_D1_PIN;
	gpio_init_structure.Alternate = GPIO_AF9_QUADSPI;
	HAL_GPIO_Init(QSPI_BK2_D1_GPIO_PORT, &gpio_init_structure);

	/* QSPI D2 GPIO pin configuration  */
	gpio_init_structure.Pin = QSPI_BK1_D2_PIN;
	gpio_init_structure.Alternate = GPIO_AF9_QUADSPI;
	HAL_GPIO_Init(QSPI_BK1_D2_GPIO_PORT, &gpio_init_structure);

	gpio_init_structure.Pin = QSPI_BK2_D2_PIN;
	HAL_GPIO_Init(QSPI_BK2_D2_GPIO_PORT, &gpio_init_structure);

	/* QSPI D3 GPIO pin configuration  */
	gpio_init_structure.Pin = QSPI_BK1_D3_PIN;
	HAL_GPIO_Init(QSPI_BK1_D3_GPIO_PORT, &gpio_init_structure);

	gpio_init_structure.Pin = QSPI_BK2_D3_PIN;
	HAL_GPIO_Init(QSPI_BK2_D3_GPIO_PORT, &gpio_init_structure);

	/*##-3- Configure the NVIC for QSPI #########################################*/
	/* NVIC configuration for QSPI interrupt */
	HAL_NVIC_SetPriority(QUADSPI_IRQn, 0x0F, 0);
	HAL_NVIC_EnableIRQ(QUADSPI_IRQn);
}

int32_t MT25TL01G_GetFlashInfo(MT25TL01G_Info_t *pInfo)
{
	pInfo->FlashSize = MT25TL01G_FLASH_SIZE;
	pInfo->EraseSectorSize = (2 * MT25TL01G_SUBSECTOR_SIZE);
	pInfo->ProgPageSize = MT25TL01G_PAGE_SIZE;
	pInfo->EraseSectorsNumber = (MT25TL01G_FLASH_SIZE / pInfo->EraseSectorSize);
	pInfo->ProgPagesNumber = (MT25TL01G_FLASH_SIZE / pInfo->ProgPageSize);
	return MT25TL01G_OK;
}

int32_t MT25TL01G_ResetEnable(QSPI_HandleTypeDef *Ctx, MT25TL01G_Interface_t Mode)
{
  QSPI_CommandTypeDef s_command;

  /* Initialize the reset enable command */
  s_command.InstructionMode   = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction       = MT25TL01G_RESET_ENABLE_CMD;
  s_command.AddressMode       = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_NONE;
  s_command.DummyCycles       = 0;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MT25TL01G_ERROR_COMMAND;
  }

  return MT25TL01G_OK;
}

int32_t MT25TL01G_AutoPollingMemReady(QSPI_HandleTypeDef *Ctx, MT25TL01G_Interface_t Mode)
{

	QSPI_CommandTypeDef s_command;
	QSPI_AutoPollingTypeDef s_config;

	/* Configure automatic polling mode to wait for memory ready */
	s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
	s_command.Instruction = MT25TL01G_READ_STATUS_REG_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_4_LINES;
	s_command.DummyCycles = 2;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	s_config.Match = 0;
	s_config.MatchMode = QSPI_MATCH_MODE_AND;
	s_config.Interval = 0x10;
	s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
	s_config.Mask = MT25TL01G_SR_WIP | (MT25TL01G_SR_WIP << 8);
	s_config.StatusBytesSize = 2;

	if (HAL_QSPI_AutoPolling(Ctx, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return MT25TL01G_ERROR_AUTOPOLLING;
	}

	return MT25TL01G_OK;
}

int32_t MT25TL01G_WriteEnable(QSPI_HandleTypeDef *Ctx, MT25TL01G_Interface_t Mode)
{
  QSPI_CommandTypeDef     s_command;
  QSPI_AutoPollingTypeDef s_config;

  /* Enable write operations */
  s_command.InstructionMode   = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;

  s_command.Instruction       = MT25TL01G_WRITE_ENABLE_CMD;
  s_command.AddressMode       = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_NONE;
  s_command.DummyCycles       = 0;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MT25TL01G_ERROR_COMMAND;
  }

  /* Configure automatic polling mode to wait for write enabling */
  s_config.Match           = MT25TL01G_SR_WREN | (MT25TL01G_SR_WREN << 8);
  s_config.Mask            = MT25TL01G_SR_WREN | (MT25TL01G_SR_WREN << 8);
  s_config.MatchMode       = QSPI_MATCH_MODE_AND;
  s_config.StatusBytesSize = 2;
  s_config.Interval        = 0x10;
  s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  s_command.Instruction    = MT25TL01G_READ_STATUS_REG_CMD;
  s_command.DataMode       = (Mode == MT25TL01G_QPI_MODE) ? QSPI_DATA_4_LINES : QSPI_DATA_1_LINE;


  if (HAL_QSPI_AutoPolling(Ctx, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MT25TL01G_ERROR_AUTOPOLLING;
  }

  return MT25TL01G_OK;
}

int32_t MT25TL01G_ResetMemory(QSPI_HandleTypeDef *Ctx, MT25TL01G_Interface_t Mode)
{
  QSPI_CommandTypeDef s_command;

  /* Initialize the reset enable command */
  s_command.InstructionMode   = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction       = MT25TL01G_RESET_MEMORY_CMD ;
  s_command.AddressMode       = QSPI_ADDRESS_NONE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_NONE;
  s_command.DummyCycles       = 0;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MT25TL01G_ERROR_COMMAND;
  }

  return MT25TL01G_OK;
}

int32_t MT25TL01G_Enter4BytesAddressMode(QSPI_HandleTypeDef *Ctx, MT25TL01G_Interface_t Mode)
{
	QSPI_CommandTypeDef s_command;

	/* Initialize the command */
	s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = MT25TL01G_ENTER_4_BYTE_ADDR_MODE_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/*write enable */
	if (MT25TL01G_WriteEnable(Ctx, Mode) != MT25TL01G_OK)
	{
		return MT25TL01G_ERROR_COMMAND;
	}
	/* Send the command */
	if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return MT25TL01G_ERROR_COMMAND;
	}

	/* Configure automatic polling mode to wait the memory is ready */
	else if (MT25TL01G_AutoPollingMemReady(Ctx, Mode) != MT25TL01G_OK)
	{
		return MT25TL01G_ERROR_COMMAND;
	}

	return MT25TL01G_OK;
}

__weak HAL_StatusTypeDef MX_QSPI_Init(QSPI_HandleTypeDef *hQspi, MX_QSPI_Init_t *Config)
{
	/* QSPI initialization */
	/* QSPI freq = SYSCLK /(1 + ClockPrescaler) Mhz */
	hQspi->Instance = QUADSPI;
	hQspi->Init.ClockPrescaler = Config->ClockPrescaler;
	hQspi->Init.FifoThreshold = 1;
	hQspi->Init.SampleShifting = Config->SampleShifting;
	hQspi->Init.FlashSize = Config->FlashSize;
	hQspi->Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_4_CYCLE; /* Min 50ns for nonRead */
	hQspi->Init.ClockMode = QSPI_CLOCK_MODE_0;
	hQspi->Init.FlashID = QSPI_FLASH_ID_1;
	hQspi->Init.DualFlash = Config->DualFlashMode;

	return HAL_QSPI_Init(hQspi);
}

static int32_t QSPI_ResetMemory(MT25TL01G_Interface_t InterfaceMode)
{
	int32_t ret = BSP_ERROR_NONE;

	/* Send RESET ENABLE command in QPI mode (QUAD I/Os, 4-4-4) */
	if (MT25TL01G_ResetEnable(&hqspi, MT25TL01G_QPI_MODE) != MT25TL01G_OK)
	{
		ret = BSP_ERROR_COMPONENT_FAILURE;
	} /* Send RESET memory command in QPI mode (QUAD I/Os, 4-4-4) */
	else if (MT25TL01G_ResetMemory(&hqspi, MT25TL01G_QPI_MODE) != MT25TL01G_OK)
	{
		ret = BSP_ERROR_COMPONENT_FAILURE;
	} /* Wait Flash ready */
	else if (MT25TL01G_AutoPollingMemReady(&hqspi, InterfaceMode) != MT25TL01G_OK)
	{
		ret = BSP_ERROR_COMPONENT_FAILURE;
	} /* Send RESET ENABLE command in SPI mode (1-1-1) */
	else if (MT25TL01G_ResetEnable(&hqspi, MT25TL01G_SPI_MODE) != MT25TL01G_OK)
	{
		ret = BSP_ERROR_COMPONENT_FAILURE;
	} /* Send RESET memory command in SPI mode (1-1-1) */
	else if (MT25TL01G_ResetMemory(&hqspi, MT25TL01G_SPI_MODE) != MT25TL01G_OK)
	{
		ret = BSP_ERROR_COMPONENT_FAILURE;
	}

	/* Return BSP status */
	return ret;
}

static int32_t QSPI_DummyCyclesCfg(MT25TL01G_Interface_t InterfaceMode)
{
	int32_t ret = BSP_ERROR_NONE;
	QSPI_CommandTypeDef s_command;
	uint16_t reg = 0;

	/* Initialize the read volatile configuration register command */
	s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
	s_command.Instruction = MT25TL01G_READ_VOL_CFG_REG_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_4_LINES;
	s_command.DummyCycles = 0;
	s_command.NbData = 2;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return BSP_ERROR_COMPONENT_FAILURE;
	}

	/* Reception of the data */
	if (HAL_QSPI_Receive(&hqspi, (uint8_t *)(&reg), HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return BSP_ERROR_COMPONENT_FAILURE;
	}

	/* Enable write operations */
	if (MT25TL01G_WriteEnable(&hqspi, InterfaceMode) != MT25TL01G_OK)
	{
		return BSP_ERROR_COMPONENT_FAILURE;
	}

	/* Update volatile configuration register (with new dummy cycles) */
	s_command.Instruction = MT25TL01G_WRITE_VOL_CFG_REG_CMD;
	MODIFY_REG(reg, 0xF0F0, ((MT25TL01G_DUMMY_CYCLES_READ_QUAD << 4) | (MT25TL01G_DUMMY_CYCLES_READ_QUAD << 12)));

	/* Configure the write volatile configuration register command */
	if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return BSP_ERROR_COMPONENT_FAILURE;
	}

	/* Transmission of the data */
	if (HAL_QSPI_Transmit(&hqspi, (uint8_t *)(&reg), HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return BSP_ERROR_COMPONENT_FAILURE;
	}

	/* Return BSP status */
	return ret;
}

int32_t MT25TL01G_EnterQPIMode(QSPI_HandleTypeDef *Ctx)
{
	QSPI_CommandTypeDef s_command;

	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = MT25TL01G_ENTER_QUAD_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return MT25TL01G_ERROR_COMMAND;
	}

	return MT25TL01G_OK;
}

int32_t MT25TL01G_ExitQPIMode(QSPI_HandleTypeDef *Ctx)
{
	QSPI_CommandTypeDef s_command;

	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = MT25TL01G_EXIT_QUAD_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return MT25TL01G_ERROR_COMMAND;
	}

	return MT25TL01G_OK;
}

int32_t BSP_QSPI_ConfigFlash(MT25TL01G_Transfer_t TransferRate, MT25TL01G_Interface_t InterfaceMode)
{
	int32_t ret = BSP_ERROR_NONE;

	/* Setup MCU transfer rate setting ***************************************************/
	hqspi.Init.SampleShifting = (TransferRate == MT25TL01G_STR_TRANSFER) ? QSPI_SAMPLE_SHIFTING_HALFCYCLE : QSPI_SAMPLE_SHIFTING_NONE;

	if (HAL_QSPI_Init(&hqspi) != HAL_OK)
	{
		ret = BSP_ERROR_PERIPH_FAILURE;
	}
	else
	{
		/* Setup Flash interface ***************************************************/
		if (InterfaceMode != MT25TL01G_QPI_MODE)
		{
			if (MT25TL01G_ExitQPIMode(&hqspi) != MT25TL01G_OK)
			{
				ret = BSP_ERROR_COMPONENT_FAILURE;
			}
		}
		else
		{
			if (MT25TL01G_EnterQPIMode(&hqspi) != MT25TL01G_OK)
			{
				ret = BSP_ERROR_COMPONENT_FAILURE;
			}
		}
	}

	/* Return BSP status */
	return ret;
}

int32_t BSP_QSPI_Init(MT25TL01G_Transfer_t TransferRate, MT25TL01G_Interface_t InterfaceMode)
{
	int32_t ret = BSP_ERROR_NONE;
	MT25TL01G_Info_t pInfo;
	MX_QSPI_Init_t qspi_init;
	/* Table to handle clock prescalers:
	1: For STR mode to reach max 108Mhz
	3: For DTR mode to reach max 54Mhz
	*/
	static const uint32_t PrescalerTab[2] = {1, 3};

	/* Check if the instance is supported */
	QSPI_MspInit(&hqspi);
	if (ret == BSP_ERROR_NONE)
	{
		/* STM32 QSPI interface initialization */
		(void)MT25TL01G_GetFlashInfo(&pInfo);
		qspi_init.ClockPrescaler = PrescalerTab[TransferRate];
		qspi_init.DualFlashMode = QSPI_DUALFLASH_ENABLE;
		qspi_init.FlashSize = (uint32_t)POSITION_VAL((uint32_t)pInfo.FlashSize) - 1U;
		qspi_init.SampleShifting = (TransferRate == MT25TL01G_STR_TRANSFER) ? QSPI_SAMPLE_SHIFTING_HALFCYCLE : QSPI_SAMPLE_SHIFTING_NONE;

		if (MX_QSPI_Init(&hqspi, &qspi_init) != HAL_OK)
		{
			ret = BSP_ERROR_PERIPH_FAILURE;
		} /* QSPI memory reset */
		else if (QSPI_ResetMemory(InterfaceMode) != BSP_ERROR_NONE)
		{
			ret = BSP_ERROR_COMPONENT_FAILURE;
		} /* Force Flash enter 4 Byte address mode */
		else if (MT25TL01G_AutoPollingMemReady(&hqspi, InterfaceMode) != MT25TL01G_OK)
		{
			ret = BSP_ERROR_COMPONENT_FAILURE;
		}
		else if (MT25TL01G_Enter4BytesAddressMode(&hqspi, InterfaceMode) != MT25TL01G_OK)
		{
			ret = BSP_ERROR_COMPONENT_FAILURE;
		} /* Configuration of the dummy cycles on QSPI memory side */
		else if (QSPI_DummyCyclesCfg(InterfaceMode) != BSP_ERROR_NONE)
		{
			ret = BSP_ERROR_COMPONENT_FAILURE;
		}
		else
		{
			/* Configure Flash to desired mode */
			if (BSP_QSPI_ConfigFlash(TransferRate, InterfaceMode) != BSP_ERROR_NONE)
			{
				ret = BSP_ERROR_COMPONENT_FAILURE;
			}
		}
	}

	/* Return BSP status */
	return ret;
}

/**
 * @brief  Reads an amount of data from the QSPI memory on DTR mode.
 *         SPI/QPI; 1-1-1/1-1-2/1-4-4/4-4-4
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @retval QSPI memory status
 */
int32_t MT25TL01G_EnableMemoryMappedModeDTR(QSPI_HandleTypeDef *Ctx, MT25TL01G_Interface_t Mode)
{
	QSPI_CommandTypeDef s_command;
	QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;
	switch (Mode)
	{
	case MT25TL01G_SPI_MODE: /* 1-1-1 commands, Power on H/W default setting */
		s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		s_command.Instruction = MT25TL01G_FAST_READ_4_BYTE_DTR_CMD;
		s_command.AddressMode = QSPI_ADDRESS_1_LINE;
		s_command.DataMode = QSPI_DATA_1_LINE;

		break;
	case MT25TL01G_SPI_2IO_MODE: /* 1-1-2 read commands */

		s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		s_command.Instruction = MT25TL01G_DUAL_OUT_FAST_READ_DTR_CMD;
		s_command.AddressMode = QSPI_ADDRESS_1_LINE;
		s_command.DataMode = QSPI_DATA_2_LINES;

		break;
	case MT25TL01G_SPI_4IO_MODE: /* 1-4-4 read commands */

		s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_4_BYTE_DTR_CMD;
		s_command.AddressMode = QSPI_ADDRESS_4_LINES;
		s_command.DataMode = QSPI_DATA_4_LINES;

		break;
	case MT25TL01G_QPI_MODE: /* 4-4-4 commands */
		s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
		s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_DTR_CMD;
		s_command.AddressMode = QSPI_ADDRESS_4_LINES;
		s_command.DataMode = QSPI_DATA_4_LINES;

		break;
	}
	/* Configure the command for the read instruction */
	s_command.AddressSize = QSPI_ADDRESS_32_BITS;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DummyCycles = MT25TL01G_DUMMY_CYCLES_READ_QUAD_DTR;
	s_command.DdrMode = QSPI_DDR_MODE_ENABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_HALF_CLK_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the memory mapped mode */
	s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
	s_mem_mapped_cfg.TimeOutPeriod = 0;

	if (HAL_QSPI_MemoryMapped(Ctx, &s_command, &s_mem_mapped_cfg) != HAL_OK)
	{
		return MT25TL01G_ERROR_MEMORYMAPPED;
	}

	return MT25TL01G_OK;
}

/**
 * @brief  Reads an amount of data from the QSPI memory on STR mode.
 *         SPI/QPI; 1-1-1/1-2-2/1-4-4/4-4-4
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @retval QSPI memory status
 */

int32_t MT25TL01G_EnableMemoryMappedModeSTR(QSPI_HandleTypeDef *Ctx, MT25TL01G_Interface_t Mode)
{
	QSPI_CommandTypeDef s_command;
	QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;
	switch (Mode)
	{
	case MT25TL01G_SPI_MODE: /* 1-1-1 read commands */
		s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		s_command.Instruction = MT25TL01G_FAST_READ_4_BYTE_ADDR_CMD;
		s_command.AddressMode = QSPI_ADDRESS_1_LINE;
		s_command.DataMode = QSPI_DATA_1_LINE;

		break;
	case MT25TL01G_SPI_2IO_MODE: /* 1-2-2 read commands */

		s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		s_command.Instruction = MT25TL01G_DUAL_INOUT_FAST_READ_4_BYTE_ADDR_CMD;
		s_command.AddressMode = QSPI_ADDRESS_2_LINES;
		s_command.DataMode = QSPI_DATA_2_LINES;

		break;

	case MT25TL01G_SPI_4IO_MODE: /* 1-4-4 read commands */

		s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_4_BYTE_ADDR_CMD;
		s_command.AddressMode = QSPI_ADDRESS_4_LINES;
		s_command.DataMode = QSPI_DATA_4_LINES;

		break;

	case MT25TL01G_QPI_MODE: /* 4-4-4 commands */
		s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
		s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_CMD;
		s_command.AddressMode = QSPI_ADDRESS_4_LINES;
		s_command.DataMode = QSPI_DATA_4_LINES;

		break;
	}
	/* Configure the command for the read instruction */
	s_command.DummyCycles = MT25TL01G_DUMMY_CYCLES_READ;
	s_command.AddressSize = QSPI_ADDRESS_32_BITS;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the memory mapped mode */
	s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
	s_mem_mapped_cfg.TimeOutPeriod = 0;

	if (HAL_QSPI_MemoryMapped(Ctx, &s_command, &s_mem_mapped_cfg) != HAL_OK)
	{
		return MT25TL01G_ERROR_MEMORYMAPPED;
	}

	return MT25TL01G_OK;
}

int32_t BSP_QSPI_EnableMemoryMappedMode(MT25TL01G_Transfer_t TransferRate, MT25TL01G_Interface_t InterfaceMode)
{
	int32_t ret = BSP_ERROR_NONE;

	if (TransferRate == MT25TL01G_STR_TRANSFER)
	{
		if (MT25TL01G_EnableMemoryMappedModeSTR(&hqspi, InterfaceMode) != MT25TL01G_OK)
		{
			ret = BSP_ERROR_COMPONENT_FAILURE;
		}
	}
	else
	{
		if (MT25TL01G_EnableMemoryMappedModeDTR(&hqspi, InterfaceMode) != MT25TL01G_OK)
		{
			ret = BSP_ERROR_COMPONENT_FAILURE;
		}
	}

	/* Return BSP status */
	return ret;
}

void TargetBSP_Init(void)
{
	MX_FMC_Init();
	if(BSP_QSPI_Init(MT25TL01G_DTR_TRANSFER, MT25TL01G_SPI_4IO_MODE) != BSP_ERROR_NONE)
	{
		error("BSP_QSPI_Init failed");
	}
	if(BSP_QSPI_EnableMemoryMappedMode(MT25TL01G_DTR_TRANSFER, MT25TL01G_SPI_4IO_MODE) != BSP_ERROR_NONE)
	{
		error("BSP_QSPI_EnableMemoryMappedMode failed");
	}
	MPU_Config();
}
