#include "platform/lighting_output_port.h"

bool lighting_output_port_duty_is_on(uint16_t duty)
{
    if (duty > LIGHTING_OUTPUT_DUTY_MAX) {
        duty = LIGHTING_OUTPUT_DUTY_MAX;
    }
    return duty >= LIGHTING_OUTPUT_ON_THRESHOLD;
}

#ifndef LIGHTING_OUTPUT_PORT_HOST_TEST
void lighting_output_port_init(void)
{
    GPIO_InitTypeDef config = {0};

    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    HAL_GPIO_WritePin(LIGHTING_OUTPUT_FRONT_GPIO_PORT,
                      LIGHTING_OUTPUT_FRONT_GPIO_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LIGHTING_OUTPUT_ROOF_GPIO_PORT,
                      LIGHTING_OUTPUT_ROOF_GPIO_PIN, GPIO_PIN_RESET);

    config.Mode = GPIO_MODE_OUTPUT_PP;
    config.Pull = GPIO_NOPULL;
    config.Speed = GPIO_SPEED_FREQ_LOW;
    config.Pin = LIGHTING_OUTPUT_FRONT_GPIO_PIN;
    HAL_GPIO_Init(LIGHTING_OUTPUT_FRONT_GPIO_PORT, &config);
    config.Pin = LIGHTING_OUTPUT_ROOF_GPIO_PIN;
    HAL_GPIO_Init(LIGHTING_OUTPUT_ROOF_GPIO_PORT, &config);
}

void lighting_output_port_apply(uint16_t front_duty, uint16_t roof_duty)
{
    HAL_GPIO_WritePin(LIGHTING_OUTPUT_FRONT_GPIO_PORT,
                      LIGHTING_OUTPUT_FRONT_GPIO_PIN,
                      lighting_output_port_duty_is_on(front_duty)
                          ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LIGHTING_OUTPUT_ROOF_GPIO_PORT,
                      LIGHTING_OUTPUT_ROOF_GPIO_PIN,
                      lighting_output_port_duty_is_on(roof_duty)
                          ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
#endif
