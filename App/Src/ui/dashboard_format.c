#include "ui/dashboard_format.h"

#include <stdio.h>

void dashboard_format_mode(char *buffer, size_t size, uint32_t mode_flags)
{
    (void)snprintf(buffer, size, "M:%08lX", (unsigned long)mode_flags);
}
