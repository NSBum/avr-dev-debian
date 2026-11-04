#ifndef TINYAVR1_DFPLAYER_USART_H
#define TINYAVR1_DFPLAYER_USART_H

#include <avr/io.h>

#include "tinyavr1_dfplayer.h"

void tinyavr1_dfplayer_usart_init(USART_t *usart,
                                  uint32_t f_cpu,
                                  uint32_t baud);

tinyavr1_dfplayer_transport_t tinyavr1_dfplayer_transport_usart(
    USART_t *usart);

#endif
