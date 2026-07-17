#include <stdio.h>

void test_app_time(void);
void test_msp_codec(void);
void test_msp_client(void);
void test_vehicle_state(void);
void test_msp_uart(void);

int main(void)
{
    test_app_time();
    test_msp_codec();
    test_msp_client();
    test_vehicle_state();
    test_msp_uart();
    puts("all tests passed");
    return 0;
}
