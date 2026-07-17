/**
 ******************************************************************************
 * @file    ltdc.c
 * @brief   This file provides code for the configuration
 *          of the LTDC instances.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "ltdc.h"

#include "BSP_LCD.h"

static inline void wait(uint32_t us)
{
  for (int j = 0; j < 55; j++)
  {
    asm volatile("nop");
  }
}

void SPI_WriteComm(uint8_t val)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); // CS

  for (uint8_t bit = 0u; bit < 9u; bit++)
  {
    if (bit == 0)
    {
      HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2, GPIO_PIN_RESET);
    }
    else
    {
      HAL_GPIO_WritePin(
          GPIOH, GPIO_PIN_2,
          ((val & (1 << (8 - bit))) ? GPIO_PIN_SET : GPIO_PIN_RESET));
    }
    wait(1);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_RESET);
    wait(1);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_SET);
  }
  HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2, GPIO_PIN_RESET); // SDO
  HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_RESET); // CLK

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); // CS
}

void SPI_WriteData(uint8_t val)
{

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); // CS
  // HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_SET); // DC

  for (uint8_t bit = 0u; bit < 9u; bit++)
  {
    if (bit == 0)
    {
      HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2, GPIO_PIN_SET);
    }
    else
    {
      HAL_GPIO_WritePin(
          GPIOH, GPIO_PIN_2,
          ((val & (1 << (8 - bit))) ? GPIO_PIN_SET : GPIO_PIN_RESET));
    }
    wait(1);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    wait(1);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_SET);
  }
  HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2, GPIO_PIN_RESET); // SDO
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_7, GPIO_PIN_RESET); // CLK

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); // CS
}

void ST7789V_Init()
{
  GPIO_InitTypeDef GPIO_InitStruct =
      {0};

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); // CS

  HAL_Delay(1);
  //----------------Star Initial Sequence-------//
  SPI_WriteComm(0x11);
  HAL_Delay(120); // Delay 120ms

  SPI_WriteComm(0x13);

  SPI_WriteComm(0xB0);
  SPI_WriteData(0x11);
  SPI_WriteData(0xf0); // RGB interface		 //00

  SPI_WriteComm(0xB1);
  SPI_WriteData(0x40); // 40=RGB-DE-mode/60=RGB HV MODE
  SPI_WriteData(0x02); // VBP	  //02
  SPI_WriteData(0x14); // HBP	  //14

  SPI_WriteComm(0xB2);
  SPI_WriteData(0x0C);
  SPI_WriteData(0x0C);
  SPI_WriteData(0x00);
  SPI_WriteData(0x33);
  SPI_WriteData(0x33);

  SPI_WriteComm(0xB2);
  SPI_WriteData(0x0C);

  SPI_WriteComm(0xB7);
  SPI_WriteData(0x35);

  SPI_WriteComm(0xBB);
  SPI_WriteData(0x28); // VCOM	   //28

  SPI_WriteComm(0xC0);
  SPI_WriteData(0x2C);

  SPI_WriteComm(0xC2);
  SPI_WriteData(0x01);

  SPI_WriteComm(0xc3);
  SPI_WriteData(0x0B); // GVDD/GVCL�?�?调亮

  // SPI_Write_index(0xc5);
  // SPI_Write_data(0x28);//VCOM 调闪

  SPI_WriteComm(0xC4);
  SPI_WriteData(0x20);

  SPI_WriteComm(0xC6);
  SPI_WriteData(0x00);

  SPI_WriteComm(0xD0);
  SPI_WriteData(0xA4);
  SPI_WriteData(0xA1);
  ////////////////////////////////////GAMMA
  SPI_WriteComm(0xE0);
  SPI_WriteData(0xD0);
  SPI_WriteData(0x01);
  SPI_WriteData(0x08);
  SPI_WriteData(0x0F);
  SPI_WriteData(0x11);
  SPI_WriteData(0x2A);
  SPI_WriteData(0x36);
  SPI_WriteData(0x55);
  SPI_WriteData(0x44);
  SPI_WriteData(0x3A);
  SPI_WriteData(0x0B);
  SPI_WriteData(0x06);
  SPI_WriteData(0x11);
  SPI_WriteData(0x20);

  SPI_WriteComm(0xE1);
  SPI_WriteData(0xD0);
  SPI_WriteData(0x02);
  SPI_WriteData(0x07);
  SPI_WriteData(0x0A);
  SPI_WriteData(0x0B);
  SPI_WriteData(0x18);
  SPI_WriteData(0x34);
  SPI_WriteData(0x43);
  SPI_WriteData(0x4A);
  SPI_WriteData(0x2B);
  SPI_WriteData(0x1B);
  SPI_WriteData(0x1C);
  SPI_WriteData(0x22);
  SPI_WriteData(0x1F);
  /////////////////////////////////////

  SPI_WriteComm(0x3A);
  SPI_WriteData(0x55); // 16-Bit RGB/65k

  SPI_WriteComm(0x36);
  SPI_WriteData(0x00);

  SPI_WriteComm(0x29); // display on
  HAL_Delay(50);
}

LTDC_HandleTypeDef hltdc;

/* LTDC init function */
void MX_LTDC_Init(void)
{

  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_5, GPIO_PIN_RESET); // RESET
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_5, GPIO_PIN_SET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_5, GPIO_PIN_SET); // 使能的LCD背光 BL

  HAL_Delay(100);

  ST7789V_Init();

  LTDC_LayerCfgTypeDef pLayerCfg = {0};

  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 9;
  hltdc.Init.VerticalSync = 3;
  hltdc.Init.AccumulatedHBP = 19;
  hltdc.Init.AccumulatedVBP = 5;
  hltdc.Init.AccumulatedActiveW = 259;
  hltdc.Init.AccumulatedActiveH = 325;
  hltdc.Init.TotalWidth = 297;
  hltdc.Init.TotalHeigh = 327;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 240;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 320;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = (uint32_t)(uintptr_t)LCD_BUFFER;
  pLayerCfg.ImageWidth = 240;
  pLayerCfg.ImageHeight = 320;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_LTDC_MspInit(LTDC_HandleTypeDef *ltdcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if (ltdcHandle->Instance == LTDC)
  {

    /** Initializes the peripherals clock
     */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLL3.PLL3M = 5;
    PeriphClkInitStruct.PLL3.PLL3N = 48;
    PeriphClkInitStruct.PLL3.PLL3P = 2;
    PeriphClkInitStruct.PLL3.PLL3Q = 5;
    PeriphClkInitStruct.PLL3.PLL3R = 20;
    PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_2;
    PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
    PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* LTDC clock enable */
    __HAL_RCC_LTDC_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    /**LTDC GPIO Configuration
    PB8     ------> LTDC_B6
    PH13     ------> LTDC_G2
    PB9     ------> LTDC_B7
    PA15(JTDI)     ------> LTDC_R3
    PA8     ------> LTDC_R6
    PC6     ------> LTDC_HSYNC
    PC7     ------> LTDC_G6
    PG7     ------> LTDC_CLK
    PA7     ------> LTDC_VSYNC
    PD10     ------> LTDC_B3
    PA5     ------> LTDC_R4
    PE12     ------> LTDC_B4
    PB10     ------> LTDC_G4
    PH11     ------> LTDC_R5
    PB11     ------> LTDC_G5
    PB15     ------> LTDC_G7
    PA3     ------> LTDC_B5
    PC5     ------> LTDC_DE
    PE11     ------> LTDC_G3
    PE15     ------> LTDC_R7
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF9_LTDC;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_7 | GPIO_PIN_5 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_11 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  }
}

void HAL_LTDC_MspDeInit(LTDC_HandleTypeDef *ltdcHandle)
{

  if (ltdcHandle->Instance == LTDC)
  {

    /* Peripheral clock disable */
    __HAL_RCC_LTDC_CLK_DISABLE();

    /**LTDC GPIO Configuration
    PB8     ------> LTDC_B6
    PH13     ------> LTDC_G2
    PB9     ------> LTDC_B7
    PA15(JTDI)     ------> LTDC_R3
    PA8     ------> LTDC_R6
    PC6     ------> LTDC_HSYNC
    PC7     ------> LTDC_G6
    PG7     ------> LTDC_CLK
    PA7     ------> LTDC_VSYNC
    PD10     ------> LTDC_B3
    PA5     ------> LTDC_R4
    PE12     ------> LTDC_B4
    PB10     ------> LTDC_G4
    PH11     ------> LTDC_R5
    PB11     ------> LTDC_G5
    PB15     ------> LTDC_G7
    PA3     ------> LTDC_B5
    PC5     ------> LTDC_DE
    PE11     ------> LTDC_G3
    PE15     ------> LTDC_R7
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_13 | GPIO_PIN_11);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_15 | GPIO_PIN_8 | GPIO_PIN_7 | GPIO_PIN_5 | GPIO_PIN_3);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_5);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_7);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_10);

    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_12 | GPIO_PIN_11 | GPIO_PIN_15);
  }
}
