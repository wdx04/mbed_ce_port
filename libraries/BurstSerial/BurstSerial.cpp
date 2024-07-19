#include "BurstSerial.h"
#include "stm_dma_utils.h"


// Support up to 10 U(S)ARTs and 2 LPUARTs
#define MAX_SERIAL_COUNT 12

struct SerialDMAInfo
{
    int index; // index in SerialDMALinks array, U(S)ART starts at 0, LPUARTs starts at 10
    UARTName name;
    DMALinkInfo tx_dma_info;
    DMALinkInfo rx_dma_info;
    IRQn_Type uartIrqn;
};

extern UART_HandleTypeDef uart_handlers[];
extern "C" int8_t get_uart_index(UARTName uart_name);

#if defined(STM32L0)
static const SerialDMAInfo SerialDMALinks[] = {
        { 0, UART_1, { 1, 4, DMA_REQUEST_3 }, { 1, 5, DMA_REQUEST_3 }, USART1_IRQn }
};
#endif
#if defined(STM32G030xx) || defined(STM32G031xx) || defined(STM32G070xx) || defined(STM32G071xx)
static const SerialDMAInfo SerialDMALinks[] = {
        { 0, UART_1, { 1, 5, DMA_REQUEST_USART1_TX }, { 1, 4, DMA_REQUEST_USART1_RX }, USART1_IRQn }
};
#endif
#if defined(STM32G0B1xx)
static const SerialDMAInfo SerialDMALinks[] = {
        { 0, UART_1, { 1, 5, DMA_REQUEST_USART1_TX }, { 1, 4, DMA_REQUEST_USART1_RX }, USART1_IRQn },
        { 10, LPUART_1, { 2, 5, DMA_REQUEST_LPUART1_TX }, { 2, 4, DMA_REQUEST_LPUART1_RX }, USART3_4_5_6_LPUART1_IRQn }
};
#endif
#if defined(STM32F1) || defined(STM32F3)
static const SerialDMAInfo SerialDMALinks[] = {
        { 0, UART_1, { 1, 4, 0 }, { 1, 5, 0 }, USART1_IRQn }
};
#endif
#if defined(TARGET_STM32F2) || defined(TARGET_STM32F4) || defined(TARGET_STM32F7)
static const SerialDMAInfo SerialDMALinks[] = {
        { 0, UART_1, { 2, 7, 4 }, { 2, 5, 4 }, USART1_IRQn },
        { 1, UART_2, { 1, 6, 4 }, { 1, 5, 4 }, USART2_IRQn },
        { 2, UART_3, { 1, 3, 4 }, { 1, 1, 4 }, USART3_IRQn }
};
#endif
#if defined(STM32L4) && !defined(DMAMUX1)
static const SerialDMAInfo SerialDMALinks[] = {
        { 0, UART_1, { 2, 6, DMA_REQUEST_2 }, { 2, 7, DMA_REQUEST_2 }, USART1_IRQn },
        { 1, UART_2, { 1, 7, DMA_REQUEST_2 }, { 1, 6, DMA_REQUEST_2 }, USART2_IRQn },
        { 10, LPUART_1, { 2, 6, DMA_REQUEST_4 }, { 2, 7, DMA_REQUEST_4 }, LPUART1_IRQn }
};
#endif
#if defined(STM32G4) || defined(STM32L5) || (defined(STM32L4) && defined(DMAMUX1))
static const SerialDMAInfo SerialDMALinks[] = {
        { 0, UART_1, { 2, 6, DMA_REQUEST_USART1_TX }, { 2, 7, DMA_REQUEST_USART1_RX }, USART1_IRQn },
        { 1, UART_2, { 1, 7, DMA_REQUEST_USART1_TX }, { 1, 6, DMA_REQUEST_USART1_RX }, USART2_IRQn }
};
#endif
#if defined(STM32U5)
static const SerialDMAInfo SerialDMALinks[] = {
        { 0, UART_1, { 1, 11, GPDMA1_REQUEST_USART1_TX }, { 1, 10, GPDMA1_REQUEST_USART1_RX }, USART1_IRQn }
};
#endif
#ifdef STM32H7
static const SerialDMAInfo SerialDMALinks[] = {
        { 0, UART_1, { 2, 7, DMA_REQUEST_USART1_TX }, { 2, 5, DMA_REQUEST_USART1_RX }, USART1_IRQn }
};
#endif
static BurstSerial *BurstSerialInstances[MAX_SERIAL_COUNT];

BurstSerial::BurstSerial(PinName tx, PinName rx, int bauds, PinName direction)
    : SerialBase(tx, rx, bauds), shared_queue(mbed_event_queue())
{
    if(direction != PinName::NC)
    {
        p_direction = std::make_unique<DigitalOut>(direction);
        p_direction->write(0);
    }
}

BurstSerial::~BurstSerial()
{
    dma_uninit();
}

bool BurstSerial::dma_init()
{
    if(dma_initialized)
    {
        return true;
    }
    #if defined(STM32F3) || defined(STM32H7)
    UARTName uart_name = _serial.uart;
    #else
    UARTName uart_name = _serial.serial.uart;
    #endif
    int uart_index = get_uart_index(uart_name);
    uart_handle = &uart_handlers[uart_index];
    for(const SerialDMAInfo &info: SerialDMALinks)
    {
        if(info.name == uart_name)
        {
            #if defined(__HAL_RCC_DMAMUX1_CLK_ENABLE)
            __HAL_RCC_DMAMUX1_CLK_ENABLE();
            #endif            
            #ifdef DMA1
            if(stm_get_dma_instance(&info.tx_dma_info) == DMA1) __HAL_RCC_DMA1_CLK_ENABLE();
            #endif
            #ifdef DMA2
            if(stm_get_dma_instance(&info.tx_dma_info) == DMA2) __HAL_RCC_DMA2_CLK_ENABLE();
            #endif
            #ifdef GPDMA1
            if(stm_get_dma_instance(&info.tx_dma_info) == GPDMA1) __HAL_RCC_GPDMA1_CLK_ENABLE();
            #endif
            hdma_tx.Instance = stm_get_dma_channel(&info.tx_dma_info);
            #if defined(STM32G0) || defined(STM32L0) || defined(STM32L4) || defined(STM32G4) || defined(STM32L5) || defined(STM32U5) || defined(STM32H7)
            hdma_tx.Init.Request = info.tx_dma_info.sourceNumber;
            #endif
            #if defined(TARGET_STM32F2) || defined(TARGET_STM32F4) || defined(TARGET_STM32F7)
            hdma_tx.Init.Channel = info.tx_dma_info.sourceNumber << DMA_SxCR_CHSEL_Pos;
            #endif
            hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
            hdma_tx.Init.Mode = DMA_NORMAL;
            #if !defined(TARGET_STM32U5)
            hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
            hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
            hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
            hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
            hdma_tx.Init.Priority = DMA_PRIORITY_LOW;
            #else
            hdma_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
            hdma_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
            hdma_tx.Init.DestInc = DMA_DINC_FIXED;
            hdma_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
            hdma_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
            hdma_tx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
            hdma_tx.Init.SrcBurstLength = 1;
            hdma_tx.Init.DestBurstLength = 1;
            hdma_tx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
            hdma_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
            #endif

            if (HAL_DMA_Init(&hdma_tx) != HAL_OK)
            {
                return false;
            }
            __HAL_LINKDMA(uart_handle, hdmatx, hdma_tx);

            hdma_rx.Instance = stm_get_dma_channel(&info.rx_dma_info);
            #if defined(STM32G0) || defined(STM32L0) || defined(STM32L4) || defined(STM32G4) || defined(STM32L5) || defined(STM32H7)
            hdma_rx.Init.Request = info.rx_dma_info.sourceNumber;
            #endif
            #if defined(TARGET_STM32F2) || defined(TARGET_STM32F4) || defined(TARGET_STM32F7)
            hdma_rx.Init.Channel = info.rx_dma_info.sourceNumber << DMA_SxCR_CHSEL_Pos;
            #endif
            #if !defined(TARGET_STM32U5)
            hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
            hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
            hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
            hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
            hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
            hdma_rx.Init.Mode = DMA_CIRCULAR;
            hdma_rx.Init.Priority = DMA_PRIORITY_HIGH;
            if (HAL_DMA_Init(&hdma_rx) != HAL_OK)
            {
                return false;
            }
            __HAL_LINKDMA(uart_handle, hdmarx, hdma_rx);
            #else
            DMA_NodeConfTypeDef NodeConfig;
            NodeConfig.NodeType = DMA_GPDMA_LINEAR_NODE;
            NodeConfig.Init.Request = info.rx_dma_info.sourceNumber;
            NodeConfig.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
            NodeConfig.Init.Direction = DMA_PERIPH_TO_MEMORY;
            NodeConfig.Init.SrcInc = DMA_SINC_FIXED;
            NodeConfig.Init.DestInc = DMA_DINC_INCREMENTED;
            NodeConfig.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
            NodeConfig.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
            NodeConfig.Init.SrcBurstLength = 1;
            NodeConfig.Init.DestBurstLength = 1;
            NodeConfig.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
            NodeConfig.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
            NodeConfig.Init.Mode = DMA_NORMAL;
            NodeConfig.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
            NodeConfig.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
            NodeConfig.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
            if (HAL_DMAEx_List_BuildNode(&NodeConfig, &dma_rx_node) != HAL_OK)
            {
                return false;
            }
            if (HAL_DMAEx_List_InsertNode(&dma_rx_list, NULL, &dma_rx_node) != HAL_OK)
            {
                return false;
            }
            if (HAL_DMAEx_List_SetCircularMode(&dma_rx_list) != HAL_OK)
            {
                return false;
            }
            hdma_rx.Instance = stm_get_dma_channel(&info.rx_dma_info);
            hdma_rx.InitLinkedList.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
            hdma_rx.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
            hdma_rx.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
            hdma_rx.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
            hdma_rx.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;
            if (HAL_DMAEx_List_Init(&hdma_rx) != HAL_OK)
            {
                return false;
            }
            if (HAL_DMAEx_List_LinkQ(&hdma_rx, &dma_rx_list) != HAL_OK)
            {
                return false;
            }
            __HAL_LINKDMA(uart_handle, hdmarx, hdma_rx);
            if (HAL_DMA_ConfigChannelAttributes(&hdma_rx, DMA_CHANNEL_NPRIV) != HAL_OK)
            {
                return false;
            }
            #endif

            /* Put DMA handles into matrix */
            if(!stm_set_dma_handle_for_link(&info.tx_dma_info, &hdma_tx))
            {
                return false;
            }
            if(!stm_set_dma_handle_for_link(&info.rx_dma_info, &hdma_rx))
            {
                return false;
            }

            /* DMA interrupt configuration */
            IRQn_Type txIrqn = stm_get_dma_irqn(&info.tx_dma_info);
            IRQn_Type rxIrqn = stm_get_dma_irqn(&info.rx_dma_info);
            HAL_NVIC_SetPriority(rxIrqn, 0, 0);
            HAL_NVIC_EnableIRQ(rxIrqn);            
            if(rxIrqn != txIrqn)
            {
                HAL_NVIC_SetPriority(txIrqn, 5, 0);
                HAL_NVIC_EnableIRQ(txIrqn);            
            }
            HAL_NVIC_SetPriority(info.uartIrqn, 0, 1);
            HAL_NVIC_EnableIRQ(info.uartIrqn);
            BurstSerialInstances[info.index] = this;
            dma_initialized = HAL_UARTEx_ReceiveToIdle_DMA(uart_handle, &rx_buffer[0], rx_buffer_size) == HAL_OK;
            if(dma_initialized)
            {
                sleep_manager_lock_deep_sleep();
            }
            return dma_initialized;
        }
    }
    return false;
}

void BurstSerial::dma_uninit()
{
    if(dma_initialized)
    {
        sleep_manager_unlock_deep_sleep();
        #if defined(STM32F3) || defined(STM32H7)
        UARTName uart_name = _serial.uart;
        #else
        UARTName uart_name = _serial.serial.uart;
        #endif
        for(const SerialDMAInfo &info: SerialDMALinks)
        {
            if(info.name == uart_name)
            {
                IRQn_Type txIrqn = stm_get_dma_irqn(&info.tx_dma_info);
                IRQn_Type rxIrqn = stm_get_dma_irqn(&info.rx_dma_info);
                HAL_NVIC_DisableIRQ(rxIrqn);            
                if(rxIrqn != txIrqn)
                {
                    HAL_NVIC_DisableIRQ(txIrqn);
                }
                HAL_NVIC_DisableIRQ(info.uartIrqn);
                HAL_DMA_DeInit(&hdma_rx);
                HAL_DMA_DeInit(&hdma_tx);
                BurstSerialInstances[info.index] = nullptr;
                stm_set_dma_handle_for_link(&info.tx_dma_info, nullptr);
                stm_set_dma_handle_for_link(&info.rx_dma_info, nullptr);
                break;
            }
        }
        dma_initialized = false;
        return;
    }
}

#if HAS_RECEIVER_TIMEOUT
void BurstSerial::enable_receiver_timeout(uint32_t rto_length)
{
    if(rto_length != 0)
    {
        // enable
        LL_USART_SetRxTimeout(uart_handle->Instance, rto_length);
        LL_USART_EnableRxTimeout(uart_handle->Instance);
        LL_USART_ClearFlag_RTO(uart_handle->Instance);
        LL_USART_EnableIT_RTO(uart_handle->Instance);
    }
    else
    {
        // disable
        LL_USART_DisableRxTimeout(uart_handle->Instance);
        LL_USART_DisableIT_RTO(uart_handle->Instance);
    }
}
#endif

bool BurstSerial::write(const uint8_t* data, uint16_t size)
{
    #if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
        uint32_t alignedAddr = (uint32_t)data &  ~0x1F;
        SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, size + ((uint32_t)data - alignedAddr));
    #endif
    if(p_direction)
    {
        p_direction->write(1);
    }
    return HAL_UART_Transmit_DMA(uart_handle, (uint8_t*)data, size) == HAL_OK;
}

bool BurstSerial::write(const char *data)
{
    int data_len = strlen(data);
    return write((const uint8_t*)data, (uint16_t)data_len);
}

void BurstSerial::set_rx_callback(mbed::Callback<void(bool/*is_timeout*/)> callback_, bool in_queue)
{
    rx_callback = callback_;
    callback_flags &= ~rx_callback_flag_mask;
    callback_flags |= (in_queue ? rx_callback_in_queue: rx_callback_in_isr);
}

void BurstSerial::set_tx_callback(mbed::Callback<void()> callback_, bool in_queue)
{
    tx_callback = callback_;
    callback_flags &= ~tx_callback_flag_mask;
    callback_flags |= (in_queue ? tx_callback_in_queue: tx_callback_in_isr);
}

void BurstSerial::on_uart_received(uint16_t size)
{
    if (size != rx_buffer_index)
    {
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
        SCB_InvalidateDCache_by_Addr((uint32_t*)rx_buffer, rx_buffer_size);
#endif
        if (size > rx_buffer_index)
        {
            uint16_t received_chars = size - rx_buffer_index;
            user_buffer.push(&rx_buffer[rx_buffer_index],  received_chars);
        }
        else
        {
            uint16_t received_chars = rx_buffer_size - rx_buffer_index;
            if(received_chars > 0)
            {
                user_buffer.push(&rx_buffer[rx_buffer_index],  received_chars);
            }
            if (size > 0)
            {
                user_buffer.push(&rx_buffer[0],  size);
            }
        }
        rx_buffer_index = size;
        uint32_t rx_callback_flag = callback_flags & rx_callback_flag_mask;
        if(rx_callback_flag == rx_callback_in_queue)
        {
            shared_queue->call(rx_callback, false);
        }
        else if(rx_callback_flag == rx_callback_in_isr)
        {
            rx_callback(false);
        }
    }
}

void BurstSerial::on_uart_tx_complete()
{
    if(p_direction)
    {
        p_direction->write(0);
    }
    uint32_t tx_callback_flag = callback_flags & tx_callback_flag_mask;
    if(tx_callback_flag == tx_callback_in_queue)
    {
        shared_queue->call(tx_callback);
    }
    else if(tx_callback_flag == tx_callback_in_isr)
    {
        tx_callback();
    }
}

void BurstSerial::on_uart_interrupt()
{
#if HAS_RECEIVER_TIMEOUT
    if(LL_USART_IsActiveFlag_RTO(uart_handle->Instance))
    {
        LL_USART_ClearFlag_RTO(uart_handle->Instance);
        uint32_t rx_callback_flag = callback_flags & rx_callback_flag_mask;
        if(rx_callback_flag == rx_callback_in_queue)
        {
            shared_queue->call(rx_callback, true);
        }
        else if(rx_callback_flag == rx_callback_in_isr)
        {
            rx_callback(true);
        }        
    }
#endif
    HAL_UART_IRQHandler(uart_handle);
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    if(huart->Instance == USART1)
    {
        BurstSerialInstances[0]->on_uart_received(size);
        return;
    }
#ifdef USART2
    if(huart->Instance == USART2)
    {
        BurstSerialInstances[1]->on_uart_received(size);
        return;
    }
#endif
#ifdef USART3
    if(huart->Instance == USART3)
    {
        BurstSerialInstances[2]->on_uart_received(size);
        return;
    }
#endif
#ifdef LPUART1
    if(huart->Instance == LPUART1)
    {
        BurstSerialInstances[10]->on_uart_received(size);
        return;
    }
#endif
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        BurstSerialInstances[0]->on_uart_tx_complete();
        return;
    }
#ifdef USART2
    if(huart->Instance == USART2)
    {
        BurstSerialInstances[1]->on_uart_tx_complete();
        return;
    }
#endif
#ifdef USART3
    if(huart->Instance == USART3)
    {
        BurstSerialInstances[2]->on_uart_tx_complete();
        return;
    }
#endif
#ifdef LPUART1
    if(huart->Instance == LPUART1)
    {
        BurstSerialInstances[10]->on_uart_tx_complete();
        return;
    }
#endif
}

extern "C" void USART1_IRQHandler(void)
{
    BurstSerialInstances[0]->on_uart_interrupt();
}

#ifdef USART2
extern "C" void USART2_IRQHandler(void)
{
    BurstSerialInstances[1]->on_uart_interrupt();
}
#endif

#ifdef USART3
extern "C" void USART3_IRQHandler(void)
{
    BurstSerialInstances[2]->on_uart_interrupt();
}
#endif

#if defined(LPUART1) && !defined(STM32G0B1xx)
extern "C" void LPUART1_IRQHandler(void)
{
    if(BurstSerialInstances[10] != nullptr)
    {
        BurstSerialInstances[10]->on_uart_interrupt();
    }
}
#endif

#if defined(STM32G0B1xx)
extern "C" void USART3_4_5_6_LPUART1_IRQHandler(void)
{
    if(BurstSerialInstances[10] != nullptr)
    {
        BurstSerialInstances[10]->on_uart_interrupt();
    }
}
#endif
