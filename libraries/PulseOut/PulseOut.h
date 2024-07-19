#pragma once

// PulseOut library
// Generate a number of pulses on a specific digital output pin

#include <mbed.h>
#include <SingletonPtr.h>

#ifndef PULSEOUT_MAX_PULSE_COUNT
// Max number of pins for output
#define PULSEOUT_MAX_PULSE_COUNT 4
#endif

#ifndef PULSEOUT_TICKER_INTERVAL_US
// Ticker interrput interval
#define PULSEOUT_TICKER_INTERVAL_US 10
#endif

#if defined(TARGET_STM)

#if !defined(TARGET_STM32F0) && !defined(TARGET_STM32L0) && !defined(TARGET_STM32L1)

// Pulse Generator class based on STM32 Timer's One-Pulse Mode for driving stepper motors
// Advantages: NO DMA, NO Interrupts, LOW CPU Usage 
// Disadvantages: Fixed Duty, ONLY support Channel 1/2 of Timers with the RCR register(TIM1/TIM8/TIM15/TIM16/TIM17/TIM20)
// Example Usage: Drive a screw stepper motor
// Deep-sleep mode is disabled when using this class
class OPMPulseOut
    : public mbed::NonCopyable<OPMPulseOut>
{
public:
  // limitation on 'pin':
  // the pin should be connected to channel 1/2 of TIM1/TIM8/TIM15/TIM16/TIM17/TIM20
  OPMPulseOut(PinName pin);

  ~OPMPulseOut();

  // send any number of pulses in batches(blocking)
  void write_us(int period_us, int width_us, int count, int batch_size = 256);

  // send a number of pulses limited by hardware(non-blocking)
  // limitation on 'count':
  // 1~65536 for TIM1,TIM8 and TIM20 on F3/F7/G0/G4/L4/L4+/L5/U5/H7
  // 1~256 for all timers other than above
  void write_once(int period_us, int width_us, int count);

  // send more pulses following previous period and width settings(non-blocking)
  void write_more(int count);

  // wait until all pulses were sent
  void wait();

private:
  TIM_HandleTypeDef TIM_Handler;
  pwmout_t obj;
};
#endif

// Pulse Generator class based on STM32 Timer's PWM output mode and DMA
// Advantages: NO Interrupts, LOW CPU Usage, High Resolution, Variable Duty, Usable on any PWM output pin
// Disadvantages: Requires DMA, NOT Memory efficient compared to SPI
// Example Usage: Drive a WS2812 LED array, or a NEC IR emitter
// Deep-sleep mode is disabled when using this class
class DMAPulseOut
    : public mbed::NonCopyable<DMAPulseOut>
{
public:
  // constructor
  // params: dma link information(dma instance index, dma stream index, dma request), output pin, period in nanoseconds
  DMAPulseOut(const DMALinkInfo& dma_info, PinName pin, uint32_t period_ns);

  // destructor
  ~DMAPulseOut();

  // convert a list of duty in nanoseconds to internal ticks
  void convert_ns_to_ticks(const uint32_t *in_ns, uint32_t *out_ticks, uint16_t count);

  // send a number of pulses with varible duties in ticks
  void write(const uint32_t *duties_ticks, uint16_t count);

  // wait until all pulses were sent
  void wait();

  // pulse finished callback
  void on_pulse_finished();

private:
  TIM_HandleTypeDef TIM_Handler;
  pwmout_t obj;
  float ns_per_tick = 1.0f;
  DMALinkInfo dma_info;
};

#endif

// General Pulse Generator class based on Ticker
struct PulseOutParams
{
    DigitalOut out { NC };
    PinName pin = NC;
    int period_us = 0;
    int width_us = 0;
    int count = 0;
    int phase = 0;
};

// Pulse Generator class based on mbed Ticker, allows output to any GPIO pin
// CPU intensive, may not be suitable for output frequencies of 10KHz and higher
class TickerPulseOut
    : public mbed::NonCopyable<TickerPulseOut>
{
public:
    static TickerPulseOut* get_instance();

    // schedule pulse output(non-blocking)
    bool write_us(PinName pin, int period_us, int width_us, int count);

private:
    TickerPulseOut() = default;

    friend class SingletonPtr<TickerPulseOut>;

    void ticker_callback();

    Ticker ticker;
    bool ticker_running = false;
    PulseOutParams pulses[PULSEOUT_MAX_PULSE_COUNT];
};
