#include "PulseOut.h"
#include <cstdint>

#if defined(TARGET_STM)

namespace
{
  struct CriticalContext final
  {
    CriticalContext()
    {
      __disable_irq();
    }

    ~CriticalContext()
    {
      __enable_irq();
    }
  };
}

#include <pwmout_device.h>
#include <stm_dma_utils.h>

extern const PinMap PinMap_PWM[];
extern const pwm_apb_map_t pwm_apb_map_table[];
extern "C" uint32_t TIM_ChannelConvert_HAL2LL(uint32_t channel, pwmout_t *obj);
constexpr size_t dma_pulse_out_table_size = 4;
static std::pair<TIM_HandleTypeDef *, DMAPulseOut *> dma_pulse_out_table[dma_pulse_out_table_size];

extern "C" void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  for (size_t i = 0; i < dma_pulse_out_table_size; i++)
  {
    if (dma_pulse_out_table[i].first == htim)
    {
      dma_pulse_out_table[i].second->on_pulse_finished();
      break;
    }
  }
}

#if !defined(TARGET_STM32F0) && !defined(TARGET_STM32L0) && !defined(TARGET_STM32L1)

OPMPulseOut::OPMPulseOut(PinName pin)
{
  sleep_manager_lock_deep_sleep();

  int peripheral = (int)pinmap_peripheral(pin, PinMap_PWM);
  int function = (int)pinmap_find_function(pin, PinMap_PWM);

  const PinMap pinmap = {pin, peripheral, function};
  obj.pwm = (PWMName)pinmap.peripheral;

  // Get the functions (timer channel, (non)inverted) from the pin and assign it to the object
  obj.channel = STM_PIN_CHANNEL(function);
  obj.inverted = STM_PIN_INVERTED(function);

  // Enable TIM clock
  bool clock_enabled = false;
#if defined(TIM1_BASE)
  if (obj.pwm == PWM_1)
  {
    __HAL_RCC_TIM1_CLK_ENABLE();
    clock_enabled = true;
  }
#endif
#if defined(TIM8_BASE)
  if (obj.pwm == PWM_8)
  {
    __HAL_RCC_TIM8_CLK_ENABLE();
    clock_enabled = true;
  }
#endif
#if defined(TIM15_BASE)
  if (obj.pwm == PWM_15)
  {
    __HAL_RCC_TIM15_CLK_ENABLE();
    clock_enabled = true;
  }
#endif
#if defined(TIM16_BASE)
  if (obj.pwm == PWM_16)
  {
    __HAL_RCC_TIM16_CLK_ENABLE();
    clock_enabled = true;
  }
#endif
#if defined(TIM17_BASE)
  if (obj.pwm == PWM_17)
  {
    __HAL_RCC_TIM17_CLK_ENABLE();
    clock_enabled = true;
  }
#endif
#if defined(TIM20_BASE)
  if (obj.pwm == PWM_20)
  {
    __HAL_RCC_TIM20_CLK_ENABLE();
    clock_enabled = true;
  }
#endif

  if (!clock_enabled)
  {
    error("PulseOut only supports TIM1,TIM8,TIM15,TIM16,TIM17 and TIM20\n");
  }

  // Configure GPIO
  pin_function(pinmap.pin, pinmap.function);

  obj.pin = pinmap.pin;
  obj.period = 0;
  obj.compare_value = 0;
  obj.top_count = 1;

  memset(&TIM_Handler, 0, sizeof(TIM_HandleTypeDef));
}

OPMPulseOut::~OPMPulseOut()
{
  sleep_manager_unlock_deep_sleep();
  pwmout_free(&obj);
}

void OPMPulseOut::write_us(int period_us, int width_us, int count, int batch_size)
{
  int current_batch_count = count > batch_size ? batch_size : count;
  write_once(period_us, width_us, current_batch_count);
  count -= current_batch_count;
  wait();
  while (count > 0)
  {
    current_batch_count = count > batch_size ? batch_size : count;
    write_more(current_batch_count);
    count -= current_batch_count;
    wait();
  }
}

void OPMPulseOut::write_once(int period_us, int width_us, int count)
{
  TIM_Handler.Instance = (TIM_TypeDef *)(obj.pwm);
  __HAL_TIM_DISABLE(&TIM_Handler);

  // Get clock configuration
  // Note: PclkFreq contains here the Latency (not used after)
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  uint32_t PclkFreq = 0;
  uint32_t APBxCLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &PclkFreq);

  uint8_t i = 0;
  while (pwm_apb_map_table[i].pwm != obj.pwm)
  {
    i++;
  }

  if (pwm_apb_map_table[i].pwm == 0)
  {
    error("Unknown PWM instance\n");
  }

  if (pwm_apb_map_table[i].pwmoutApb == PWMOUT_ON_APB1)
  {
    PclkFreq = HAL_RCC_GetPCLK1Freq();
    APBxCLKDivider = RCC_ClkInitStruct.APB1CLKDivider;
  }
  else
  {
#if !defined(PWMOUT_APB2_NOT_SUPPORTED)
    PclkFreq = HAL_RCC_GetPCLK2Freq();
    APBxCLKDivider = RCC_ClkInitStruct.APB2CLKDivider;
#endif
  }

  /* By default use, 1us as SW pre-scaler */
  obj.top_count = 1;
  // TIMxCLK = PCLKx when the APB prescaler = 1 else TIMxCLK = 2 * PCLKx
  if (APBxCLKDivider == RCC_HCLK_DIV1)
  {
    TIM_Handler.Init.Prescaler = (((PclkFreq) / 1000000)) - 1; // 1 us tick
  }
  else
  {
    TIM_Handler.Init.Prescaler = (((PclkFreq * 2) / 1000000)) - 1; // 1 us tick
  }
  TIM_Handler.Init.Period = (period_us - 1);
  /*  In case period or pre-scalers are out of range, loop-in to get valid values */
  while ((TIM_Handler.Init.Period > 0xFFFF) || (TIM_Handler.Init.Prescaler > 0xFFFF))
  {
    obj.top_count = obj.top_count * 2;
    if (APBxCLKDivider == RCC_HCLK_DIV1)
    {
      TIM_Handler.Init.Prescaler = (((PclkFreq) / 1000000) * obj.top_count) - 1;
    }
    else
    {
      TIM_Handler.Init.Prescaler = (((PclkFreq * 2) / 1000000) * obj.top_count) - 1;
    }
    TIM_Handler.Init.Period = (period_us - 1) / obj.top_count;
    /*  Period decreases and prescaler increases over loops, so check for
     *  possible out of range cases */
    if ((TIM_Handler.Init.Period < 0xFFFF) && (TIM_Handler.Init.Prescaler > 0xFFFF))
    {
      error("Cannot initialize PWM\n");
      break;
    }
  }

  TIM_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
  TIM_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  TIM_Handler.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  TIM_Handler.Init.RepetitionCounter = count - 1;

  if (HAL_TIM_OnePulse_Init(&TIM_Handler, TIM_OPMODE_SINGLE) != HAL_OK)
  {
    error("HAL_TIM_OnePulse_Init failed\n");
  }

  TIM_OC_InitTypeDef sConfig;
  sConfig.OCMode = TIM_OCMODE_PWM1;
  sConfig.OCPolarity = TIM_OCPOLARITY_LOW;
  sConfig.OCFastMode = TIM_OCFAST_DISABLE;
  sConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  sConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfig.Pulse = (period_us - width_us) * (TIM_Handler.Init.Period + 1) / period_us;

  int channel = 0;
  switch (obj.channel)
  {
  case 1:
    channel = TIM_CHANNEL_1;
    break;
  case 2:
    channel = TIM_CHANNEL_2;
    break;
  default:
    error("Only Channe 1&2 supports OPM\n");
    return;
  }

  if (HAL_TIM_PWM_ConfigChannel(&TIM_Handler, &sConfig, channel) != HAL_OK)
  {
    error("HAL_TIM_PWM_ConfigChannel failed\n");
  }

  if (obj.inverted)
  {
    if (HAL_TIMEx_PWMN_Start(&TIM_Handler, channel) != HAL_OK)
    {
      error("HAL_TIM_PWN_Start failed\n");
    }
  }
  else
  {
    if (HAL_TIM_PWM_Start(&TIM_Handler, channel) != HAL_OK)
    {
      error("HAL_TIM_PWM_Start failed\n");
    }
  }
}

void OPMPulseOut::write_more(int count)
{
  TIM_Handler.Instance->RCR = uint32_t(count - 1);
  TIM_Handler.Instance->EGR = TIM_EGR_UG;
  __HAL_TIM_ENABLE(&TIM_Handler);
}

void OPMPulseOut::wait()
{
  while ((TIM_Handler.Instance->CR1 & TIM_CR1_CEN_Msk) == TIM_CR1_CEN)
  {
    ThisThread::yield();
  }
}

#endif

void pwmout_dma_init_channel(TIM_HandleTypeDef *TIM_Handler, pwmout_t *obj, DMA_HandleTypeDef *dmaHandle)
{
  TIM_OC_InitTypeDef sConfig;
  int channel = 0;
  int dma_id = 0;

  // Configure channels
  sConfig.OCMode = TIM_OCMODE_PWM1;
  sConfig.Pulse = 0;
  sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfig.OCFastMode = TIM_OCFAST_DISABLE;
#if defined(TIM_OCIDLESTATE_RESET)
  sConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
#endif
#if defined(TIM_OCNIDLESTATE_RESET)
  sConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
#endif

  switch (obj->channel)
  {
  case 1:
    channel = TIM_CHANNEL_1;
    dma_id = TIM_DMA_ID_CC1;
    break;
  case 2:
    channel = TIM_CHANNEL_2;
    dma_id = TIM_DMA_ID_CC2;
    break;
  case 3:
    channel = TIM_CHANNEL_3;
    dma_id = TIM_DMA_ID_CC3;
    break;
  case 4:
    channel = TIM_CHANNEL_4;
    dma_id = TIM_DMA_ID_CC4;
    break;
  default:
    return;
  }

  __HAL_LINKDMA(TIM_Handler, hdma[dma_id], *dmaHandle);

  if (LL_TIM_CC_IsEnabledChannel(TIM_Handler->Instance, TIM_ChannelConvert_HAL2LL(channel, obj)) == 0)
  {
    // If channel is not enabled, proceed to channel configuration
    if (HAL_TIM_PWM_ConfigChannel(TIM_Handler, &sConfig, channel) != HAL_OK)
    {
      error("Cannot initialize PWM\n");
    }
  }
  else
  {
    // If channel already enabled, only update compare value to avoid glitch
    __HAL_TIM_SET_COMPARE(TIM_Handler, channel, sConfig.Pulse);
    return;
  }
}

float pwmout_dma_period_ns(TIM_HandleTypeDef *TIM_Handler, const DMALinkInfo *link_info, pwmout_t *obj, uint32_t period_ns)
{
  float ns_per_tick = 1.0f;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  uint32_t PclkFreq = 0;
  uint32_t APBxCLKDivider = RCC_HCLK_DIV1;
  uint8_t i = 0;

  TIM_Handler->Instance = (TIM_TypeDef *)(obj->pwm);

  __HAL_TIM_DISABLE(TIM_Handler);

  // Get clock configuration
  // Note: PclkFreq contains here the Latency (not used after)
  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &PclkFreq);

  /*  Parse the pwm / apb mapping table to find the right entry */
  while (pwm_apb_map_table[i].pwm != obj->pwm)
  {
    i++;
  }

  if (pwm_apb_map_table[i].pwm == 0)
  {
    error("Unknown PWM instance");
  }

  if (pwm_apb_map_table[i].pwmoutApb == PWMOUT_ON_APB1)
  {
    PclkFreq = HAL_RCC_GetPCLK1Freq();
    APBxCLKDivider = RCC_ClkInitStruct.APB1CLKDivider;
  }
  else
  {
#if !defined(PWMOUT_APB2_NOT_SUPPORTED)
    PclkFreq = HAL_RCC_GetPCLK2Freq();
    APBxCLKDivider = RCC_ClkInitStruct.APB2CLKDivider;
#endif
  }

  obj->top_count = 1;
  // TIMxCLK = PCLKx when the APB prescaler = 1 else TIMxCLK = 2 * PCLKx
  uint32_t timx_clk = APBxCLKDivider == RCC_HCLK_DIV1 ? PclkFreq : PclkFreq * 2;
  ns_per_tick = 1.0e9f / float(timx_clk);
  // Use max Resolution
  TIM_Handler->Init.Prescaler = 0;
  TIM_Handler->Init.Period = 0;
  if (float(period_ns) >= ns_per_tick)
  {
    TIM_Handler->Init.Period = int(period_ns / ns_per_tick) - 1;
  }

  /*  In case period or pre-scalers are out of range, loop-in to get valid values */
  while ((TIM_Handler->Init.Period > 0xFFFF) || (TIM_Handler->Init.Prescaler > 0xFFFF))
  {
    obj->top_count = obj->top_count * 2;
    timx_clk /= 2;
    ns_per_tick = 1.0e9f / float(timx_clk);
    TIM_Handler->Init.Prescaler = (TIM_Handler->Init.Prescaler + 1) * 2 - 1;
    TIM_Handler->Init.Period = 0;
    if (float(period_ns) >= ns_per_tick)
    {
      TIM_Handler->Init.Period = int(period_ns / ns_per_tick) - 1;
    }
    /*  Period decreases and prescaler increases over loops, so check for
     *  possible out of range cases */
    if ((TIM_Handler->Init.Period < 0xFFFF) && (TIM_Handler->Init.Prescaler > 0xFFFF))
    {
      error("Cannot initialize PWM\n");
      break;
    }
  }

  TIM_Handler->Init.ClockDivision = 0;
  TIM_Handler->Init.CounterMode = TIM_COUNTERMODE_UP;

  if (HAL_TIM_PWM_Init(TIM_Handler) != HAL_OK)
  {
    error("Cannot initialize PWM\n");
  }

  DMA_HandleTypeDef *dmaHandle = stm_init_dma_link(link_info, DMA_MEMORY_TO_PERIPH, false, true, 4, 4);
  pwmout_dma_init_channel(TIM_Handler, obj, dmaHandle);

  // Save for future use
  obj->period = period_ns / 1000;

  return ns_per_tick;
}

DMAPulseOut::DMAPulseOut(const DMALinkInfo &dma_info, PinName pin, uint32_t period_ns)
    : dma_info(dma_info)
{
  sleep_manager_lock_deep_sleep();
  pwmout_init(&obj, pin);
  memset(&TIM_Handler, 0, sizeof(TIM_HandleTypeDef));
  TIM_Handler.Instance = (TIM_TypeDef *)(obj.pwm);
  ns_per_tick = pwmout_dma_period_ns(&TIM_Handler, &dma_info, &obj, period_ns);
  bool table_entry_found = false;
  {
    CriticalContext cctx;
    for(size_t i = 0; i < dma_pulse_out_table_size; i++)
    {
      if(dma_pulse_out_table[i].first == nullptr)
      {
        dma_pulse_out_table[i].first = &TIM_Handler;
        dma_pulse_out_table[i].second = this;
        table_entry_found = true;
        break;
      }
    }
  }
  if(!table_entry_found)
  {
    error("DMA pulse out table is full");
  }
}

DMAPulseOut::~DMAPulseOut()
{
  stm_free_dma_link(&dma_info);
  pwmout_free(&obj);
  {
    CriticalContext cctx;
    for(size_t i = 0; i < dma_pulse_out_table_size; i++)
    {
      if(dma_pulse_out_table[i].second == this)
      {
        dma_pulse_out_table[i].first = nullptr;
        dma_pulse_out_table[i].second = nullptr;
        break;
      }
    }
  }  
  sleep_manager_unlock_deep_sleep();
}

void DMAPulseOut::convert_ns_to_ticks(const uint32_t *in_ns, uint32_t *out_ticks, uint16_t count)
{
  for (uint16_t i = 0; i < count; i++)
  {
    out_ticks[i] = uint32_t(float(in_ns[i]) / ns_per_tick);
  }
}

void DMAPulseOut::write(const uint32_t *duties_ticks, uint16_t count)
{
#if defined(DMA_IP_VERSION_V3)
  constexpr uint16_t length_multiplier = sizeof(uint32_t);
#else
  constexpr uint16_t length_multiplier = 1;
#endif
  uint32_t channel = 0;
  switch (obj.channel)
  {
  case 1:
    channel = TIM_CHANNEL_1;
    break;
  case 2:
    channel = TIM_CHANNEL_2;
    break;
  case 3:
    channel = TIM_CHANNEL_3;
    break;
  case 4:
    channel = TIM_CHANNEL_4;
    break;
  default:
    return;
  }
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  uint32_t alignedAddr = (uint32_t)duties_ticks & ~0x1F;
  SCB_CleanDCache_by_Addr((uint32_t *)alignedAddr, count * sizeof(uint32_t) + ((uint32_t)duties_ticks - alignedAddr));
#endif
  HAL_StatusTypeDef startDMARet = HAL_OK;
  if (obj.inverted)
  {
    if (TIM_CHANNEL_N_STATE_GET(&TIM_Handler, channel) == HAL_TIM_CHANNEL_STATE_BUSY)
    {
      wait();
    }
    startDMARet = HAL_TIMEx_PWMN_Start_DMA(&TIM_Handler, channel, const_cast<uint32_t *>(duties_ticks), count * length_multiplier);
  }
  else
  {
    if (TIM_CHANNEL_STATE_GET(&TIM_Handler, channel) == HAL_TIM_CHANNEL_STATE_BUSY)
    {
      wait();
    }
    startDMARet = HAL_TIM_PWM_Start_DMA(&TIM_Handler, channel, const_cast<uint32_t *>(duties_ticks), count * length_multiplier);
  }
  if (startDMARet != HAL_OK)
  {
    error("HAL_TIM_PWM_Start_DMA failed");
  }
}

void DMAPulseOut::on_pulse_finished()
{
  uint32_t channel = 0;
  switch (obj.channel)
  {
  case 1:
    channel = TIM_CHANNEL_1;
    break;
  case 2:
    channel = TIM_CHANNEL_2;
    break;
  case 3:
    channel = TIM_CHANNEL_3;
    break;
  case 4:
    channel = TIM_CHANNEL_4;
    break;
  default:
    return;
  }
  if(obj.inverted)
  {
    HAL_TIMEx_PWMN_Stop_DMA(&TIM_Handler, channel);
  }
  else
  {
    HAL_TIM_PWM_Stop_DMA(&TIM_Handler, channel);
  }
}

void DMAPulseOut::wait()
{
  while ((TIM_Handler.Instance->CR1 & TIM_CR1_CEN_Msk) == TIM_CR1_CEN)
  {
    ThisThread::yield();
  }
}

#endif

SingletonPtr<TickerPulseOut> ticker_pulse_out_ptr;

TickerPulseOut *TickerPulseOut::get_instance()
{
  return ticker_pulse_out_ptr.get();
}

void TickerPulseOut::ticker_callback()
{
  bool has_pulses = false;
  for (int i = 0; i < PULSEOUT_MAX_PULSE_COUNT; i++)
  {
    if (pulses[i].count > 0)
    {
      pulses[i].phase += PULSEOUT_TICKER_INTERVAL_US;
      if (pulses[i].phase >= pulses[i].period_us)
      {
        pulses[i].phase = 0;
        pulses[i].count--;
      }
      if (pulses[i].count > 0)
      {
        if (pulses[i].phase == 0)
        {
          pulses[i].out.write(1);
        }
        else if (pulses[i].phase >= pulses[i].width_us &&
                 pulses[i].phase < pulses[i].width_us + PULSEOUT_TICKER_INTERVAL_US)
        {
          pulses[i].out.write(0);
        }
        has_pulses = true;
      }
    }
  }
  if (!has_pulses)
  {
    ticker.detach();
    ticker_running = false;
  }
}

bool TickerPulseOut::write_us(PinName pin, int period_us, int width_us, int count)
{
  CriticalSectionLock critical;
  for (int i = 0; i < PULSEOUT_MAX_PULSE_COUNT; i++)
  {
    if (pulses[i].count <= 0)
    {
      pulses[i].out.~DigitalOut();
      new (&pulses[i].out) DigitalOut(pin);
      pulses[i].pin = pin;
      pulses[i].period_us = period_us;
      pulses[i].width_us = width_us;
      pulses[i].count = count;
      pulses[i].phase = -PULSEOUT_TICKER_INTERVAL_US;
      pulses[i].out.write(0);
      if (!ticker_running)
      {
        ticker.attach(callback(this, &TickerPulseOut::ticker_callback), chrono::microseconds(PULSEOUT_TICKER_INTERVAL_US));
        ticker_running = true;
      }
      return true;
    }
  }
  return false;
}
