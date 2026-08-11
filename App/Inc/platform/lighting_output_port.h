#pragma once

#include <stdbool.h>
#include <stdint.h>

#define LIGHTING_OUTPUT_DUTY_MAX 1000u
#define LIGHTING_OUTPUT_ON_THRESHOLD 500u

#ifndef LIGHTING_OUTPUT_PORT_HOST_TEST
#include "main.h"

#define LIGHTING_OUTPUT_FRONT_GPIO_PORT GPIOF
#define LIGHTING_OUTPUT_FRONT_GPIO_PIN GPIO_PIN_3
#define LIGHTING_OUTPUT_ROOF_GPIO_PORT GPIOE
#define LIGHTING_OUTPUT_ROOF_GPIO_PIN GPIO_PIN_10
#endif

bool lighting_output_port_duty_is_on(uint16_t duty);
void lighting_output_port_init(void);
void lighting_output_port_apply(uint16_t front_duty, uint16_t roof_duty);
