#include "iwdg.h"

bool MX_IWDG1_Init(void)
{
    /* Nominal 32 kHz LSI / 32 / (1999 + 1) = 2.0 s.  LSI tolerance makes
       hardware acceptance of the actual reset interval mandatory. */
    const uint32_t started_ms = HAL_GetTick();
    __HAL_RCC_LSI_ENABLE();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == 0u) {
        if ((uint32_t)(HAL_GetTick() - started_ms) >= 5u) {
            return false;
        }
    }
    IWDG1->KR = 0xCCCCu;
    IWDG1->KR = 0x5555u;
    IWDG1->PR = 3u; /* divider 32 */
    IWDG1->RLR = 1999u;
    IWDG1->WINR = 0x0FFFu;
    while (IWDG1->SR != 0u) {
        if ((uint32_t)(HAL_GetTick() - started_ms) >= 5u) {
            return false;
        }
    }
    IWDG1->KR = 0xAAAAu;
    return true;
}

void IWDG1_Refresh(void)
{
    IWDG1->KR = 0xAAAAu;
}
