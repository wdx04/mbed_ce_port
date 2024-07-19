#include "mbed_error.h"
#include "bsp_ospi_w25q128.h"
#include "octospi.h"

#if MAP_QSPI_FLASH
void EnableCache()
{
    DCACHE_HandleTypeDef hdcache;

    /* Enable ICACHE */
    HAL_ICACHE_Enable();

    /* Enable DCACHE clock */
    __HAL_RCC_DCACHE1_CLK_ENABLE();

    /* Enable DCACHE */
    hdcache.Instance = DCACHE1;
    hdcache.Init.ReadBurstType = DCACHE_READ_BURST_WRAP;
    HAL_DCACHE_Enable(&hdcache);
}

void TargetBSP_Init(void)
{
    MX_OCTOSPI1_Init();
    if(OSPI_W25Qxx_Init() != OSPI_W25Qxx_OK)
    {
        error("TargetBSP_Init");
    }
    if(OSPI_W25Qxx_MemoryMappedMode() != OSPI_W25Qxx_OK)
    {
        error("TargetBSP_Init");
    }
    EnableCache();
}
#endif
