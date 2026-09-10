#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern SPI_HandleTypeDef hspi6;

#define IMU_CS_Pin GPIO_PIN_13
#define IMU_CS_GPIO_Port GPIOF
#define IMU_INT_Pin GPIO_PIN_4
#define IMU_INT_GPIO_Port GPIOC

void MX_SPI6_Init(void);

static inline void IMU_Select(void)
{
  HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_RESET);
}

static inline void IMU_Deselect(void)
{
  HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_SET);
}

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */
