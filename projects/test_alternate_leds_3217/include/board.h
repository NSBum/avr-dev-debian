#ifndef TEST_ALTERNATE_LEDS_3217_BOARD_H
#define TEST_ALTERNATE_LEDS_3217_BOARD_H

#include <avr/io.h>

#define LED_PORT       PORTB
#define LED_PB5_bm     PIN5_bm
#define LED_PB6_bm     PIN6_bm
#define LED_PINS_bm    (LED_PB5_bm | LED_PB6_bm)

#endif
