#include <assert.h>
#include <string.h>

#include "ui/dashboard_format.h"

void test_dashboard_format(void)
{
    char text[16];
    dashboard_format_mode(text, sizeof text, 0x89abcdefu);
    assert(strcmp(text, "M:89ABCDEF") == 0);
}
