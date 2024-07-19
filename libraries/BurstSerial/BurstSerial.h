#pragma once

// Serial(UART) DMA Driver for STM32
// By default USART1 is enabled for G03x/G07x/G0B1/G4/F1/F2/F3/F4/F7/L0/L4/L4+/L5/U5/H7 MCUs
// Other U(SART) peripherals can be enabled by adding the SPITxDMALinks entries and UART/DMA interrupt handlers
// RS485 flow control is supported with any GPIO as the Drive Enable pin
// Receiver Timeout interrupt can be enabled on supported devices
// Note 1: this library uses relatively newer HAL APIs such as HAL_UARTEx_ReceiveToIdle_DMA
//       a manual update of STM32Cube firmware package is required for some devices(F1/F2/F4)
// Note 2: LPUART peripherals on some low power models may run into overrun(ORE) errors

#include "mbed.h"
#include <memory>

#if defined(TARGET_STM) && !defined(TARGET_STM32F1) && !defined(TARGET_STM32F2) && !defined(TARGET_STM32F4) && !defined(TARGET_STM32L1) 
#define HAS_RECEIVER_TIMEOUT 1
#endif
#ifndef BURSTSERIAL_RX_BUFFER_SIZE
#define BURSTSERIAL_RX_BUFFER_SIZE 128
#endif
#ifndef BURSTSERIAL_USER_BUFFER_SIZE
#define BURSTSERIAL_USER_BUFFER_SIZE 256
#endif

class BurstSerial : 
    public SerialBase
{
public:
    constexpr static uint32_t rx_buffer_size = BURSTSERIAL_RX_BUFFER_SIZE;
    constexpr static uint32_t user_buffer_size = BURSTSERIAL_USER_BUFFER_SIZE;
    constexpr static uint32_t rx_callback_unused = 0x00;
    constexpr static uint32_t rx_callback_in_queue = 0x01;
    constexpr static uint32_t rx_callback_in_isr = 0x02;
    constexpr static uint32_t rx_callback_flag_mask = 0x03;
    constexpr static uint32_t tx_callback_unused = 0x00;
    constexpr static uint32_t tx_callback_in_queue = 0x04;
    constexpr static uint32_t tx_callback_in_isr = 0x08;
    constexpr static uint32_t tx_callback_flag_mask = 0x0C;

    CircularBuffer<uint8_t, user_buffer_size> user_buffer;

    BurstSerial(PinName tx, PinName rx, int bauds = 115200, PinName direction = PinName::NC);
    
    ~BurstSerial();

    // initialize DMA, return true on success
    bool dma_init();
    
    // deinitialize DMA, automatically called in destructor
    void dma_uninit();

    // enable/disable receiver timeouts
#if HAS_RECEIVER_TIMEOUT    
    void enable_receiver_timeout(uint32_t rto_length);
#endif

    // send data using DMA transfer
    bool write(const uint8_t* data, uint16_t size);
    bool write(const char *data);

    // set callback function when new data are received
    // when in_queue is true, the callback function will be called inside mbed_event_queue()
    void set_rx_callback(mbed::Callback<void(bool/*is_timeout*/)> callback_, bool in_queue = true);

    // set callback function when data transmition is complete
    // when in_queue is true, the callback function will be called inside mbed_event_queue()
    void set_tx_callback(mbed::Callback<void()> callback_, bool in_queue = true);

private:
    alignas(32U)  uint8_t rx_buffer[rx_buffer_size];
    UART_HandleTypeDef * uart_handle;
    DMA_HandleTypeDef hdma_rx = { 0 };
    DMA_HandleTypeDef hdma_tx = { 0 };
    #if defined(STM32U5)
    DMA_NodeTypeDef dma_rx_node;
    DMA_QListTypeDef dma_rx_list;
    #endif
    uint16_t dma_initialized = 0;
    uint16_t rx_buffer_index = 0;
    mbed::Callback<void(bool)> rx_callback;
    mbed::Callback<void()> tx_callback;
    uint32_t callback_flags = 0;
    std::unique_ptr<DigitalOut> p_direction;
    EventQueue *shared_queue;

public:
    void on_uart_received(uint16_t size);

    void on_uart_tx_complete();

    void on_dma_interrupt_tx();

    void on_dma_interrupt_rx();

    void on_uart_interrupt();
};
