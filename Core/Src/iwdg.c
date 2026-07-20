#include "iwdg.h"

enum {
    LSI_READY_TIMEOUT_MS = 100u,
    /* Mirrors STM32H7 HAL_IWDG_DEFAULT_TIMEOUT for LSI_VALUE=32 kHz,
       rounded up and kept independent from the LSI-ready deadline. */
    IWDG_UPDATE_TIMEOUT_MS = 6145u
};

WatchdogStartResult MX_IWDG1_Init(void)
{
    /* Nominal 32 kHz LSI / 32 / (1999 + 1) = 2.0 s.  LSI tolerance makes
       hardware acceptance of the actual reset interval mandatory. */
    const uint32_t lsi_started_ms = HAL_GetTick();
    __HAL_RCC_LSI_ENABLE();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == 0u) {
        if ((uint32_t)(HAL_GetTick() - lsi_started_ms) >= LSI_READY_TIMEOUT_MS) {
            return WATCHDOG_NOT_STARTED_FAIL;
        }
    }
#if defined(DEBUG)
    __HAL_DBGMCU_FREEZE_IWDG1();
#endif
    /* STM32H7 HAL_IWDG_Init starts first; SR propagation only occurs once
       the IWDG kernel/LSI domain is running. */
    IWDG1->KR = 0xCCCCu;
    IWDG1->KR = 0x5555u;
    IWDG1->PR = 3u; /* divider 32 */
    IWDG1->RLR = 1999u;
    IWDG1->WINR = 0x0FFFu;
    const uint32_t update_started_ms = HAL_GetTick();
    while (IWDG1->SR != 0u) {
        if ((uint32_t)(HAL_GetTick() - update_started_ms) >= IWDG_UPDATE_TIMEOUT_MS) {
            return WATCHDOG_STARTED_CONFIG_FAIL;
        }
    }
    IWDG1->KR = 0xAAAAu;
    return WATCHDOG_STARTED_OK;
}

void IWDG1_Refresh(void)
{
    IWDG1->KR = 0xAAAAu;
}
