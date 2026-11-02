#ifndef TEST_SERIAL_IO_CONFIG_H
#define TEST_SERIAL_IO_CONFIG_H

#include <stdint.h>

#define SERIAL_BAUD 9600UL
#define RX_BUFFER_SIZE 64U

/* Normal async mode: BAUD = 64 * F_CPU / (16 * baud), rounded. */
#define USART0_BAUD_VALUE \
    ((uint16_t)((4ULL * F_CPU + SERIAL_BAUD / 2UL) / SERIAL_BAUD))

#endif
