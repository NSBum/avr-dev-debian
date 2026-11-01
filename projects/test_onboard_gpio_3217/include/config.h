#ifndef TEST_ONBOARD_GPIO_3217_CONFIG_H
#define TEST_ONBOARD_GPIO_3217_CONFIG_H

#include <stdint.h>

#define TCB0_TICK_HZ       1000UL
#define GPIO_TOGGLE_TICKS   500U

/* TCB0 uses CLK_PER/2 in periodic interrupt mode. */
#define TCB0_PERIOD ((uint16_t)((F_CPU / 2UL / TCB0_TICK_HZ) - 1UL))

#endif
