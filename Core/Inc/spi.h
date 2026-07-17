#pragma once
#include "stm32h7xx_hal.h"
extern SPI_HandleTypeDef hspi3;
extern DMA_HandleTypeDef hdma_spi3_tx;
void MX_SPI3_Init(void);
