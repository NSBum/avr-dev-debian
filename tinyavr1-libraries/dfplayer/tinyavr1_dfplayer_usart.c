#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "tinyavr1_dfplayer_usart.h"

static tinyavr1_dfplayer_result_t usart_write(void *context,
                                                 const uint8_t *data,
                                                 uint8_t length)
{
    USART_t *usart = (USART_t *)context;

    while (length != 0U) {
        while ((usart->STATUS & USART_DREIF_bm) == 0U) {
        }
        usart->TXDATAL = *data++;
        --length;
    }
    return TINYAVR1_DFPLAYER_OK;
}

static tinyavr1_dfplayer_result_t usart_read_byte(void *context,
                                                     uint8_t *value,
                                                     uint16_t timeout_ms)
{
    USART_t *usart = (USART_t *)context;

    while (timeout_ms != 0U) {
        uint16_t microseconds;

        for (microseconds = 0U; microseconds < 1000U; ++microseconds) {
            if ((usart->STATUS & USART_RXCIF_bm) != 0U) {
                uint8_t status = usart->RXDATAH;
                *value = usart->RXDATAL;
                if ((status & (USART_FERR_bm | USART_PERR_bm |
                               USART_BUFOVF_bm)) != 0U) {
                    return TINYAVR1_DFPLAYER_TRANSPORT_ERROR;
                }
                return TINYAVR1_DFPLAYER_OK;
            }
            _delay_us(1);
        }
        --timeout_ms;
    }

    return TINYAVR1_DFPLAYER_TIMEOUT;
}

void tinyavr1_dfplayer_usart_init(USART_t *usart,
                                  uint32_t f_cpu,
                                  uint32_t baud)
{
    usart->BAUD = (uint16_t)((4ULL * f_cpu + baud / 2UL) / baud);
    usart->CTRLA = 0U;
    usart->CTRLC = USART_CHSIZE_8BIT_gc;
    usart->CTRLB = USART_RXEN_bm | USART_TXEN_bm;
}

tinyavr1_dfplayer_transport_t tinyavr1_dfplayer_transport_usart(
    USART_t *usart)
{
    tinyavr1_dfplayer_transport_t transport = {
        usart, usart_write, usart_read_byte
    };
    return transport;
}
