#pragma once

#include <stdbool.h>
#include "main.h"
#include "platform/watchdog_status.h"

WatchdogStartResult MX_IWDG1_Init(void);
void IWDG1_Refresh(void);
