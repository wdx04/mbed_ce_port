#include "stm32h7xx.h"
#include "mbed_error.h"

#if MBED_CONF_TARGET_ENABLE_OVERDRIVE_MODE
#define FLASH_LATENCY FLASH_LATENCY_4
#else
#define FLASH_LATENCY FLASH_LATENCY_2
#endif

/******************************************************************************/
/*            PLL (clocked by HSE) used as System clock source                */
/******************************************************************************/
uint8_t SetSysClock_PLL_HSE(uint8_t bypass)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /* Configure the main internal regulator output voltage */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

#if MBED_CONF_TARGET_ENABLE_OVERDRIVE_MODE
    /* Enable overdrive mode (needed to hit 480MHz).  Note that on STM32H74x/5x,
       unlike other STM32H7x devices, you have to switch to VOS1 first, then switch to VOS0. */
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
#endif

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48;
    if (bypass) {
        RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    } else {
        RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    }
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

    if(HSE_VALUE % 2000000 == 0)
    {
        // Clock divisible by 2.  Divide down to 2MHz and then multiply up again.
        RCC_OscInitStruct.PLL.PLLM = (HSE_VALUE / 2000000); // PLL1 input clock = 2MHz
        RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1; // PLL1 input clock is between 2 and 4 MHz

#if MBED_CONF_TARGET_ENABLE_OVERDRIVE_MODE
        RCC_OscInitStruct.PLL.PLLN = 480; // PLL1 internal (VCO) clock = 960 MHz
#else
        RCC_OscInitStruct.PLL.PLLN = 400; // PLL1 internal (VCO) clock = 800 MHz
#endif
    }
    else if(HSE_VALUE % 5000000 == 0)
    {
        RCC_OscInitStruct.PLL.PLLM = (HSE_VALUE / 5000000); // PLL1 input clock = 5MHz
        RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2; // PLL1 input clock is between 4 and 8 MHz

#if MBED_CONF_TARGET_ENABLE_OVERDRIVE_MODE
        RCC_OscInitStruct.PLL.PLLN = 192; // 960 MHz
#else
        RCC_OscInitStruct.PLL.PLLN = 160; // 800 MHz
#endif
    }
    else
    {
        error("HSE_VALUE not divisible by 2MHz or 5MHz\n");
    }

    RCC_OscInitStruct.PLL.PLLP = 2;   // PLLCLK = SYSCLK = 480/400 MHz

#if MBED_CONF_TARGET_ENABLE_OVERDRIVE_MODE
    RCC_OscInitStruct.PLL.PLLQ = 10;  // PLL1Q used for SPI123 = 96 MHz
#else
    RCC_OscInitStruct.PLL.PLLQ = 10;  // PLL1Q used for SPI123 = 80 MHz
#endif

    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE; // PLL1 VCO clock is between 192 and 960 MHz
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        return 0; // FAIL
    }

    /* Select PLL as system clock source and configure bus clocks dividers */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                  RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_D3PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY) != HAL_OK) {
        return 0; // FAIL
    }

#if DEVICE_USBDEVICE
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        return 0; // FAIL
    }

    HAL_PWREx_EnableUSBVoltageDetector();
#endif /* DEVICE_USBDEVICE */

    return 1; // OK
}
