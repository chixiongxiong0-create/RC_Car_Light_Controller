#include "lamp_f469.h"
#include "stm32f4xx_hal.h"

void f469_lamp_init_off(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13 | GPIO_PIN_1, GPIO_PIN_RESET);
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_13 | GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
}

void f469_lamp_apply(uint16_t front_duty, uint16_t roof_duty)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,
                      front_duty >= 500u ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1,
                      roof_duty >= 500u ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
