#include "stm32f4xx_hal.h"

int main(void)
{
    HAL_Init();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13 | GPIO_PIN_1, GPIO_PIN_RESET);
    GPIO_InitTypeDef output = {0};
    output.Pin = GPIO_PIN_13 | GPIO_PIN_1;
    output.Mode = GPIO_MODE_OUTPUT_PP;
    output.Pull = GPIO_NOPULL;
    output.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &output);
    for (;;) {
        __WFI();
    }
}
