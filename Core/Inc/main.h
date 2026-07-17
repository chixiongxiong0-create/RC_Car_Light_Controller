/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "wio_lite_ai.h"
#include "wio_lite_ai_ospi.h"
/* USER CODE END Includes */


/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define EXTMEM __attribute__((section(".externalram")))
#define DTCMMEM __attribute__((section(".dtcmram")))
#define ITCMMEM __attribute__((section(".itcmram")))
#define DMA_BUFFER __attribute__((section(".dma_buffer")))

/****************************/
/***Display related defines**/
/****************************/
#define QVGA_RES_WIDTH  320
#define QVGA_RES_HEIGHT 240

#define RGB_565_BPP 2
#define RGB_888_BPP 3
#define ARGB8888_BPP 4

/*QVGA capture*/
#if ASPECT_RATIO_MODE == ASPECT_RATIO_CROP
  #define CAM_RES_WIDTH  QVGA_RES_HEIGHT
  #define CAM_RES_HEIGHT QVGA_RES_HEIGHT
#else
  #define CAM_RES_WIDTH  QVGA_RES_WIDTH
  #define CAM_RES_HEIGHT QVGA_RES_HEIGHT
#endif

#if ASPECT_RATIO_MODE == ASPECT_RATIO_PADDING
  #define CAM_RES_WITH_BORDERS  QVGA_RES_WIDTH
#endif

#define CAM_LINE_SIZE  (CAM_RES_WIDTH * RGB_565_BPP) /* 16-bit per px in RGB565 */

#define JPEG_BUFFER_SIZE      (16384)

/**************************/
/***LCD related defines****/
/**************************/
#define LCD_BRIGHTNESS_MIN 0
#define LCD_BRIGHTNESS_MAX 100
#define LCD_BRIGHTNESS_MID 50
#define LCD_BRIGHTNESS_STEP 10

#define LCD_RES_WIDTH QVGA_RES_WIDTH
#define LCD_RES_HEIGHT QVGA_RES_HEIGHT
#define LCD_BPP RGB_565_BPP
#define LCD_FRAME_BUFFER_SIZE (LCD_RES_WIDTH * LCD_RES_HEIGHT * LCD_BPP)

/*******************/
/****BSP defines****/
/*******************/


#ifdef BSP_CONFIG_SEEDSTUDIO
  #ifdef BSP_CONFIG_VITTASCIENCE
    #error "Only one BSP configuration is allowed !"
  #else
    #define BSP_CONFIG_SCREEN_LTDC 1
  #endif
#else //BSP_CONFIG_VITTASCIENCE
  #define BSP_CONFIG_SCREEN_SPI 1
#endif


#ifdef BSP_CONFIG_SCREEN_SPI
#ifdef SCREEN_ARDUINO_PIN
/* ILI93441 related macros */
#define ILI9341_SCK_Pin         GPIO_PIN_3
#define ILI9341_SCK_GPIO_Port   GPIOB
#define ILI9341_MOSI_Pin        GPIO_PIN_5
#define ILI9341_MOSI_GPIO_Port  GPIOB
#define ILI9341_LED_Pin         GPIO_PIN_13
#define ILI9341_LED_GPIO_Port   GPIOF
#define ILI9341_RST_Pin         GPIO_PIN_0
#define ILI9341_RST_GPIO_Port   GPIOC
#define ILI9341_CS_Pin          GPIO_PIN_11
#define ILI9341_CS_GPIO_Port    GPIOF
#define ILI9341_DC_Pin          GPIO_PIN_12
#define ILI9341_DC_GPIO_Port    GPIOF
#else
/* ILI93441 related macros */
#define ILI9341_SCK_Pin         GPIO_PIN_5
#define ILI9341_SCK_GPIO_Port   GPIOA
#define ILI9341_MOSI_Pin        GPIO_PIN_7
#define ILI9341_MOSI_GPIO_Port  GPIOA
#define ILI9341_LED_Pin         GPIO_PIN_11
#define ILI9341_LED_GPIO_Port   GPIOB
#define ILI9341_RST_Pin         GPIO_PIN_10
#define ILI9341_RST_GPIO_Port   GPIOB
#define ILI9341_CS_Pin          GPIO_PIN_3
#define ILI9341_CS_GPIO_Port    GPIOA
#define ILI9341_DC_Pin          GPIO_PIN_8
#define ILI9341_DC_GPIO_Port    GPIOA
#endif
#endif

  /* Exported functions prototypes ---------------------------------------------*/
  void Error_Handler(void);

  /* USER CODE BEGIN EFP */

  /* USER CODE END EFP */

  /* Private defines -----------------------------------------------------------*/

  /* USER CODE BEGIN Private defines */

  /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
