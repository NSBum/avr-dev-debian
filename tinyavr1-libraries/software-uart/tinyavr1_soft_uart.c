#include <util/delay.h>
#include <stdint.h>

#include "tinyavr1_soft_uart.h"

static void bit_delay(void)
{
    _delay_us(TINYAVR1_SOFT_UART_BIT_US);
}

static void half_bit_delay(void)
{
    _delay_us(TINYAVR1_SOFT_UART_HALF_BIT_US);
}

static uint8_t rx_is_low(const tinyavr1_soft_uart_t *uart)
{
    return (uart->rx_port->IN & uart->rx_mask) == 0U;
}

void tinyavr1_soft_uart_init(tinyavr1_soft_uart_t *uart,
                             PORT_t *tx_port,
                             uint8_t tx_mask,
                             PORT_t *rx_port,
                             uint8_t rx_mask)
{
    uart->tx_port = tx_port;
    uart->tx_mask = tx_mask;
    uart->rx_port = rx_port;
    uart->rx_mask = rx_mask;

    uart->tx_port->DIRSET = uart->tx_mask;
    uart->tx_port->OUTSET = uart->tx_mask;
    uart->rx_port->DIRCLR = uart->rx_mask;
}

void tinyavr1_soft_uart_putc(const tinyavr1_soft_uart_t *uart,
                             uint8_t value)
{
    uint8_t bit;

    /* Start bit. */
    uart->tx_port->OUTCLR = uart->tx_mask;
    bit_delay();

    for (bit = 0U; bit < 8U; ++bit) {
        if ((value & 0x01U) != 0U) {
            uart->tx_port->OUTSET = uart->tx_mask;
        } else {
            uart->tx_port->OUTCLR = uart->tx_mask;
        }
        bit_delay();
        value >>= 1U;
    }

    /* Stop bit and idle state. */
    uart->tx_port->OUTSET = uart->tx_mask;
    bit_delay();
}

void tinyavr1_soft_uart_write(const tinyavr1_soft_uart_t *uart,
                              const uint8_t *data,
                              uint8_t length)
{
    while (length != 0U) {
        tinyavr1_soft_uart_putc(uart, *data);
        ++data;
        --length;
    }
}

static enum tinyavr1_soft_uart_result receive_after_start(
    const tinyavr1_soft_uart_t *uart,
    uint8_t *value)
{
    uint8_t bit;
    uint8_t received = 0U;

    /* Sample the middle of the start bit. */
    half_bit_delay();
    if (!rx_is_low(uart)) {
        return TINYAVR1_SOFT_UART_FRAME_ERROR;
    }

    /* Move to the center of each data bit. */
    for (bit = 0U; bit < 8U; ++bit) {
        bit_delay();
        if (rx_is_low(uart)) {
            received &= (uint8_t)~(1U << bit);
        } else {
            received |= (uint8_t)(1U << bit);
        }
    }

    /* Check the stop bit. */
    bit_delay();
    if (rx_is_low(uart)) {
        return TINYAVR1_SOFT_UART_FRAME_ERROR;
    }

    *value = received;
    return TINYAVR1_SOFT_UART_OK;
}

enum tinyavr1_soft_uart_result tinyavr1_soft_uart_getc(
    const tinyavr1_soft_uart_t *uart,
    uint8_t *value)
{
    while (!rx_is_low(uart)) {
        /* Deliberately polling: this library owns no timer or ISR vector. */
    }

    return receive_after_start(uart, value);
}

enum tinyavr1_soft_uart_result tinyavr1_soft_uart_try_getc(
    const tinyavr1_soft_uart_t *uart,
    uint8_t *value)
{
    if (!rx_is_low(uart)) {
        return TINYAVR1_SOFT_UART_NO_DATA;
    }

    return receive_after_start(uart, value);
}

enum tinyavr1_soft_uart_result tinyavr1_soft_uart_getc_timeout(
    const tinyavr1_soft_uart_t *uart,
    uint8_t *value,
    uint16_t timeout_ms)
{
    while (timeout_ms != 0U) {
        uint16_t microseconds;

        for (microseconds = 0U; microseconds < 1000U; ++microseconds) {
            if (rx_is_low(uart)) {
                return receive_after_start(uart, value);
            }
            _delay_us(1);
        }
        --timeout_ms;
    }

    return TINYAVR1_SOFT_UART_TIMEOUT;
}
