#include "tinyavr1_dfplayer_soft_uart.h"

static tinyavr1_dfplayer_result_t soft_write(void *context,
                                                const uint8_t *data,
                                                uint8_t length)
{
    tinyavr1_soft_uart_write((const tinyavr1_soft_uart_t *)context,
                             data, length);
    return TINYAVR1_DFPLAYER_OK;
}

static tinyavr1_dfplayer_result_t soft_read_byte(void *context,
                                                    uint8_t *value,
                                                    uint16_t timeout_ms)
{
    enum tinyavr1_soft_uart_result result =
        tinyavr1_soft_uart_getc_timeout(
            (const tinyavr1_soft_uart_t *)context, value, timeout_ms);

    if (result == TINYAVR1_SOFT_UART_OK) {
        return TINYAVR1_DFPLAYER_OK;
    }
    if (result == TINYAVR1_SOFT_UART_TIMEOUT) {
        return TINYAVR1_DFPLAYER_TIMEOUT;
    }
    return TINYAVR1_DFPLAYER_FRAME_ERROR;
}

tinyavr1_dfplayer_transport_t tinyavr1_dfplayer_transport_soft_uart(
    tinyavr1_soft_uart_t *uart)
{
    tinyavr1_dfplayer_transport_t transport = {
        uart, soft_write, soft_read_byte
    };
    return transport;
}
