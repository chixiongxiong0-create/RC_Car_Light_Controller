#include "tim.h"

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim24;
DMA_HandleTypeDef hdma_tim2_up;
DMA_HandleTypeDef hdma_tim24_up;

static void ws2812_timer_fail_safe(void) __attribute__((noreturn));

static void ws2812_timer_fail_safe(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_RESET);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
  Error_Handler();
  for (;;) {}
}

uint32_t ws2812_timer_clock_hz(void)
{
  uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
  if ((RCC->D2CFGR & RCC_D2CFGR_D2PPRE1) != RCC_APB1_DIV1)
  {
    if (timer_clock > UINT32_MAX / 2u)
    {
      ws2812_timer_fail_safe();
    }
    timer_clock *= 2u;
  }

  return timer_clock;
}

static uint32_t ws2812_timer_period(void)
{
  const uint32_t timer_clock = ws2812_timer_clock_hz();

  const uint32_t period_ticks = (timer_clock + 400000u) / 800000u;
  if (period_ticks == 0u || period_ticks > 0x10000u)
  {
    ws2812_timer_fail_safe();
  }

  const uint32_t output_frequency = timer_clock / period_ticks;
  if (output_frequency < 790000u || output_frequency > 810000u)
  {
    ws2812_timer_fail_safe();
  }
  return period_ticks - 1u;
}

static void MX_WS2812_TIM_Init(TIM_HandleTypeDef *htim, TIM_TypeDef *instance)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim->Instance = instance;
  htim->Init.Prescaler = 0u;
  htim->Init.CounterMode = TIM_COUNTERMODE_UP;
  htim->Init.Period = ws2812_timer_period();
  htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(htim) != HAL_OK)
  {
    ws2812_timer_fail_safe();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(htim, &sClockSourceConfig) != HAL_OK)
  {
    ws2812_timer_fail_safe();
  }
  if (HAL_TIM_PWM_Init(htim) != HAL_OK)
  {
    ws2812_timer_fail_safe();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(htim, &sMasterConfig) != HAL_OK)
  {
    ws2812_timer_fail_safe();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0u;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, TIM_CHANNEL_1) != HAL_OK ||
      HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    ws2812_timer_fail_safe();
  }
  HAL_TIM_MspPostInit(htim);
}

void MX_TIM2_Init(void)
{
  MX_WS2812_TIM_Init(&htim2, TIM2);
}

void MX_TIM24_Init(void)
{
  MX_WS2812_TIM_Init(&htim24, TIM24);
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *timHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  if (timHandle->Instance == TIM2)
  {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
  else if (timHandle->Instance == TIM24)
  {
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Alternate = GPIO_AF14_TIM24;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
  }
}
