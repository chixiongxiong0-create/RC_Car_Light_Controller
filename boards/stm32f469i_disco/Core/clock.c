#include "clock.h"
#include "stm32f4xx_hal.h"

bool f469_clock_init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
#ifdef USE_STM32469I_DISCO_REVA
    osc.PLL.PLLM = 25;
#else
    osc.PLL.PLLM = 8;
#endif
    osc.PLL.PLLN = 360;
    osc.PLL.PLLP = RCC_PLLP_DIV2;
    osc.PLL.PLLQ = 7;
    osc.PLL.PLLR = 6;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK ||
        HAL_PWREx_EnableOverDrive() != HAL_OK) return false;
    RCC_ClkInitTypeDef clocks = {0};
    clocks.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider = RCC_HCLK_DIV4;
    clocks.APB2CLKDivider = RCC_HCLK_DIV2;
    return HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_5) == HAL_OK;
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
