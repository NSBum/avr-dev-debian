#ifndef TINYAVR1_DFPLAYER_SOFT_UART_H
#define TINYAVR1_DFPLAYER_SOFT_UART_H

#include "tinyavr1_dfplayer.h"
#include "../software-uart/tinyavr1_soft_uart.h"

tinyavr1_dfplayer_transport_t tinyavr1_dfplayer_transport_soft_uart(
    tinyavr1_soft_uart_t *uart);

#endif
