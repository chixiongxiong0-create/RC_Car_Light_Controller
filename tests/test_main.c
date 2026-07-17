#include <stdio.h>

void test_app_time(void);
void test_msp_codec(void);
void test_msp_client(void);

int main(void)
{
    test_app_time();
    test_msp_codec();
    test_msp_client();
    puts("all tests passed");
    return 0;
}
