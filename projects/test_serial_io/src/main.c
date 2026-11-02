#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <stdint.h>

#include "board.h"
#include "config.h"

static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;

static volatile uint8_t blink_active;
static volatile uint16_t blink_half_period_ms;
static volatile uint16_t blink_elapsed_ms;

static void clock_init(void)
{
    /* Select OSC20M; FUSE.OSCCFG must select its 20 MHz setting. */
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);

    /* Run CPU and synchronous peripherals without the reset prescaler. */
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);
}

static void gpio_init(void)
{
    GPIO_A4_PORT.OUTCLR = GPIO_A4_PIN;
    GPIO_A4_PORT.DIRSET = GPIO_A4_PIN;
}

static void usart0_init(void)
{
    /* USART0 default route: PB2/TXD and PB3/RXD. */
    PORTMUX.CTRLB &= (uint8_t)~PORTMUX_USART0_bm;
    USART0_TX_PORT.DIRSET = USART0_TX_PIN;

    USART0.BAUD = USART0_BAUD_VALUE;
    USART0.CTRLC = USART_CHSIZE_8BIT_gc;
    USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;
    USART0.CTRLA = USART_RXCIE_bm;
}

static void timer_init(void)
{
    TCB0.CTRLA = 0;
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    /* CLK_PER/2 = 10 MHz; 10,000 ticks = 1 ms. */
    TCB0.CCMP = 9999U;
    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
}

static void usart0_write_char(uint8_t value)
{
    while ((USART0.STATUS & USART_DREIF_bm) == 0) {
    }

    USART0.TXDATAL = value;
}

static void usart0_write_text(const char *text)
{
    while (*text != '\0') {
        usart0_write_char((uint8_t)*text++);
    }
}

static uint8_t usart0_read_char(uint8_t *value)
{
    uint8_t available = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (rx_tail != rx_head) {
            *value = rx_buffer[rx_tail];
            rx_tail = (uint8_t)((rx_tail + 1U) & (RX_BUFFER_SIZE - 1U));
            available = 1;
        }
    }

    return available;
}

static uint16_t blink_half_period_for_command(uint8_t command)
{
    switch (command) {
    case '1':
        return 500U;
    case '2':
        return 375U;
    case '3':
        return 250U;
    case '4':
        return 125U;
    default:
        return 0U;
    }
}

static void set_a4(uint8_t high)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        blink_active = 0;
        if (high != 0U) {
            GPIO_A4_PORT.OUTSET = GPIO_A4_PIN;
        } else {
            GPIO_A4_PORT.OUTCLR = GPIO_A4_PIN;
        }
    }
}

static void start_a4_blink(uint16_t half_period_ms)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        GPIO_A4_PORT.OUTCLR = GPIO_A4_PIN;
        blink_half_period_ms = half_period_ms;
        blink_elapsed_ms = 0U;
        blink_active = 1U;
    }
}

static uint8_t command_is(const uint8_t *command, const char *expected)
{
    uint8_t index;

    for (index = 0U; index < 6U; ++index) {
        if (command[index] != (uint8_t)expected[index]) {
            return 0U;
        }
    }

    return 1U;
}

static void process_command(const uint8_t *command)
{
    uint16_t half_period_ms;

    if (command_is(command, "A4STA1")) {
        set_a4(1U);
    } else if (command_is(command, "A4STA0")) {
        set_a4(0U);
    } else if ((command[0] == 'A') &&
               (command[1] == '4') &&
               (command[2] == 'B') &&
               (command[3] == 'L') &&
               (command[4] == 'K')) {
        half_period_ms = blink_half_period_for_command(command[5]);
        if (half_period_ms == 0U) {
            usart0_write_text("ERR\r\n");
            return;
        }
        start_a4_blink(half_period_ms);
    } else {
        usart0_write_text("ERR\r\n");
        return;
    }

    usart0_write_text("ACK");
    for (uint8_t index = 0U; index < 6U; ++index) {
        usart0_write_char(command[index]);
    }
    usart0_write_text("\r\n");
}

ISR(USART0_RXC_vect)
{
    uint8_t status = USART0.RXDATAH;
    uint8_t data = USART0.RXDATAL;

    if ((status & (USART_FERR_bm | USART_PERR_bm | USART_BUFOVF_bm)) != 0U) {
        return;
    }

    uint8_t next = (uint8_t)((rx_head + 1U) & (RX_BUFFER_SIZE - 1U));
    if (next == rx_tail) {
        return;
    }

    rx_buffer[rx_head] = data;
    rx_head = next;
}

ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;

    if (blink_active != 0U) {
        ++blink_elapsed_ms;
        if (blink_elapsed_ms >= blink_half_period_ms) {
            blink_elapsed_ms = 0U;
            GPIO_A4_PORT.OUTTGL = GPIO_A4_PIN;
        }
    }
}

int main(void)
{
    uint8_t command[6];
    uint8_t command_length = 0U;
    uint8_t value;

    clock_init();
    gpio_init();
    usart0_init();
    timer_init();
    sei();

    for (;;) {
        if (usart0_read_char(&value) == 0U) {
            continue;
        }

        if ((value == '\r') || (value == '\n')) {
            if (command_length == 6U) {
                process_command(command);
            } else if (command_length != 0U) {
                usart0_write_text("ERR\r\n");
            }
            command_length = 0U;
        } else if (command_length < 6U) {
            command[command_length++] = value;
        } else {
            /* Discard the rest of an overlength line until its terminator. */
            command_length = 7U;
        }
    }
}
