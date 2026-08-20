
/**
 ******************************************************************************
 * @file         stm32h7xx_hal_msp.c
 * @brief        This file provides code for the MSP Initialization
 *               and de-Initialization codes.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* External functions --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_spi1_tx;
extern DMA_HandleTypeDef hdma_tim2_up;
extern DMA_HandleTypeDef hdma_tim3_up;

static void ws2812_dma_fail_safe(void) __attribute__((noreturn));

static void ws2812_dma_fail_safe(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5,
                    GPIO_PIN_RESET);
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(GPIOA, &gpio);
  gpio.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
  HAL_GPIO_Init(GPIOB, &gpio);
  Error_Handler();
  for (;;) {}
}

/**
 * Initializes the Global MSP.
 */
void HAL_MspInit(void)
{

  __HAL_RCC_SYSCFG_CLK_ENABLE();

  /* System interrupt init*/
}

/**
* @brief SPI MSP Initialization
* This function configures the hardware resources used in this example
* @param hspi: SPI handle pointer
* @retval None
*/
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
	  GPIO_InitTypeDef GPIO_InitStruct = {0};
	  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
	  if(hspi->Instance==SPI1)
	  {
#ifdef BSP_CONFIG_SCREEN_SPI
	  /* USER CODE BEGIN SPI1_MspInit 0 */

	  /* USER CODE END SPI1_MspInit 0 */

	  /** Initializes the peripherals clock
	  */
	    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
	    PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL;
	    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	    {
	      Error_Handler();
	    }

		/* Peripheral clock enable */
		__HAL_RCC_SPI1_CLK_ENABLE();

		__HAL_RCC_GPIOB_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();
		__HAL_RCC_GPIOG_CLK_ENABLE();
		/**SPI1 GPIO Configuration*/
		GPIO_InitStruct.Pin = GPIO_PIN_4;
	    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	    GPIO_InitStruct.Pull = GPIO_NOPULL;
	    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
	    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	    GPIO_InitStruct.Pin = ILI9341_MOSI_Pin;
	    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	    GPIO_InitStruct.Pull = GPIO_NOPULL;
	    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
	    HAL_GPIO_Init(ILI9341_MOSI_GPIO_Port, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = ILI9341_SCK_Pin;
	    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	    GPIO_InitStruct.Pull = GPIO_NOPULL;
	    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
	    HAL_GPIO_Init(ILI9341_SCK_GPIO_Port, &GPIO_InitStruct);

	    /* SPI1 DMA Init */
	    /* SPI1_TX Init */
	    hdma_spi1_tx.Instance = DMA1_Stream0;
	    hdma_spi1_tx.Init.Request = DMA_REQUEST_SPI1_TX;
	    hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
	    hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
	    hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
	    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	    hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	    hdma_spi1_tx.Init.Mode = DMA_NORMAL;
	    hdma_spi1_tx.Init.Priority = DMA_PRIORITY_HIGH;
	    hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	    if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
	    {
	      Error_Handler();
	    }

		__HAL_LINKDMA(hspi, hdmatx, hdma_spi1_tx);

		/* DMA controller clock enable */
		__HAL_RCC_DMA1_CLK_ENABLE();

		/* DMA interrupt init */
		/* DMA1_Stream0_IRQn interrupt configuration */
		HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

		/* SPI1 interrupt Init */
		HAL_NVIC_SetPriority(SPI1_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(SPI1_IRQn);
#endif
	  }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  DMA_HandleTypeDef *hdma = NULL;

  if (htim->Instance == TIM2)
  {
    __HAL_RCC_TIM2_CLK_ENABLE();
    hdma = &hdma_tim2_up;
    hdma->Instance = DMA1_Stream2;
    hdma->Init.Request = DMA_REQUEST_TIM2_UP;
  }
  else if (htim->Instance == TIM3)
  {
    __HAL_RCC_TIM3_CLK_ENABLE();
    hdma = &hdma_tim3_up;
    hdma->Instance = DMA1_Stream3;
    hdma->Init.Request = DMA_REQUEST_TIM3_UP;
  }
  else
  {
    return;
  }

  __HAL_RCC_DMA1_CLK_ENABLE();
  hdma->Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma->Init.PeriphInc = DMA_PINC_DISABLE;
  hdma->Init.MemInc = DMA_MINC_ENABLE;
  hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma->Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma->Init.Mode = DMA_NORMAL;
  hdma->Init.Priority = DMA_PRIORITY_HIGH;
  hdma->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(hdma) != HAL_OK)
  {
    ws2812_dma_fail_safe();
  }
  htim->hdma[TIM_DMA_ID_UPDATE] = hdma;
  hdma->Parent = htim;

  if (htim->Instance == TIM2)
  {
    HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
  }
  else
  {
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    __HAL_RCC_TIM2_CLK_DISABLE();
    HAL_DMA_DeInit(&hdma_tim2_up);
    HAL_NVIC_DisableIRQ(DMA1_Stream2_IRQn);
  }
  else if (htim->Instance == TIM3)
  {
    __HAL_RCC_TIM3_CLK_DISABLE();
    HAL_DMA_DeInit(&hdma_tim3_up);
    HAL_NVIC_DisableIRQ(DMA1_Stream3_IRQn);
  }
}
