#include <stdio.h>

void test_app_time(void);
void test_msp_codec(void);
void test_msp_client(void);
void test_vehicle_state(void);
void test_msp_uart(void);
void test_lvgl_port_math(void);
void test_display_metrics(void);
void test_input_manager(void);
void test_button_input(void);
void test_dashboard_format(void);
void test_face_model(void);
void test_low_battery_policy(void);

int main(void)
{
    test_app_time();
    test_msp_codec();
    test_msp_client();
    test_vehicle_state();
    test_msp_uart();
    test_lvgl_port_math();
    test_display_metrics();
    test_input_manager();
    test_button_input();
    test_dashboard_format();
    test_face_model();
    test_low_battery_policy();
    puts("all tests passed");
    return 0;
}
