#ifndef TEST_ALTERNATE_LEDS_3217_CONFIG_H
#define TEST_ALTERNATE_LEDS_3217_CONFIG_H

#include <stdint.h>

#define TCB0_TICK_HZ          1000UL
#define LED_SWITCH_TICKS       500U

/* TCB0 periodic interrupt mode runs from CLK_PER / 2. */
#define TCB0_PERIOD ((uint16_t)((F_CPU / 2UL / TCB0_TICK_HZ) - 1UL))

#endif
