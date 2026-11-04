#ifndef TINYAVR1_SOFT_UART_H
#define TINYAVR1_SOFT_UART_H

#include <avr/io.h>
#include <stdint.h>

/* Override this before including the header for another baud rate. */
#ifndef TINYAVR1_SOFT_UART_BAUD
#define TINYAVR1_SOFT_UART_BAUD 9600UL
#endif

#define TINYAVR1_SOFT_UART_BIT_US \
    ((1000000UL + (TINYAVR1_SOFT_UART_BAUD / 2UL)) / \
     TINYAVR1_SOFT_UART_BAUD)

#define TINYAVR1_SOFT_UART_HALF_BIT_US \
    ((TINYAVR1_SOFT_UART_BIT_US + 1UL) / 2UL)

enum tinyavr1_soft_uart_result {
    TINYAVR1_SOFT_UART_OK = 0,
    TINYAVR1_SOFT_UART_NO_DATA,
    TINYAVR1_SOFT_UART_TIMEOUT,
    TINYAVR1_SOFT_UART_FRAME_ERROR
};

typedef struct {
    PORT_t *tx_port;
    uint8_t tx_mask;
    PORT_t *rx_port;
    uint8_t rx_mask;
} tinyavr1_soft_uart_t;

/* Configure one 8-N-1 software UART instance. */
void tinyavr1_soft_uart_init(tinyavr1_soft_uart_t *uart,
                             PORT_t *tx_port,
                             uint8_t tx_mask,
                             PORT_t *rx_port,
                             uint8_t rx_mask);

/* Blocking transmit operations. */
void tinyavr1_soft_uart_putc(const tinyavr1_soft_uart_t *uart,
                             uint8_t value);
void tinyavr1_soft_uart_write(const tinyavr1_soft_uart_t *uart,
                              const uint8_t *data,
                              uint8_t length);

/* Blocking receive; waits indefinitely for a start bit. */
enum tinyavr1_soft_uart_result tinyavr1_soft_uart_getc(
    const tinyavr1_soft_uart_t *uart,
    uint8_t *value);

/* Poll for a start bit, then receive one byte if one is present. */
enum tinyavr1_soft_uart_result tinyavr1_soft_uart_try_getc(
    const tinyavr1_soft_uart_t *uart,
    uint8_t *value);

/* Poll for up to timeout_ms milliseconds for one received byte. */
enum tinyavr1_soft_uart_result tinyavr1_soft_uart_getc_timeout(
    const tinyavr1_soft_uart_t *uart,
    uint8_t *value,
    uint16_t timeout_ms);

#endif
