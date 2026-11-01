#include <avr/cpufunc.h>
#include <avr/io.h>
#include <util/delay.h>

#include "board.h"
#include "config.h"

static void clock_init(void)
{
    /* Select OSC20M; FUSE.OSCCFG must select its 20 MHz setting. */
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);

    /* Run CPU and synchronous peripherals without the reset prescaler. */
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);
}

static void usart0_init(void)
{
    /* USART0 default route: PB2/TXD and PB3/RXD. */
    USART_TX_PORT.DIRSET = USART_TX_PIN;

    USART0.BAUD = USART0_BAUD_VALUE;
    USART0.CTRLC = USART_CHSIZE_8BIT_gc;
    USART0.CTRLB = USART_TXEN_bm;
}

static void usart0_write_char(char value)
{
    while ((USART0.STATUS & USART_DREIF_bm) == 0) {
    }

    USART0.TXDATAL = (uint8_t)value;
}

static void usart0_write_string(const char *text)
{
    while (*text != '\0') {
        usart0_write_char(*text++);
    }
}

int main(void)
{
    clock_init();
    usart0_init();

    for (;;) {
        usart0_write_string("ATtiny3217 serial OK\r\n");
        _delay_ms(MESSAGE_INTERVAL_MS);
    }
}
