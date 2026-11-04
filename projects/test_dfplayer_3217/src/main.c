#include <avr/cpufunc.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "board.h"
#include "config.h"
#include "tinyavr1_dfplayer.h"
#include "tinyavr1_dfplayer_soft_uart.h"
#include "tinyavr1_soft_uart.h"

static tinyavr1_soft_uart_t serial;
static tinyavr1_dfplayer_t player;

static void clock_init(void)
{
    /* FUSE.OSCCFG must select OSC20M. */
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0U);
}

static void gpio_init(void)
{
    ERROR_LED_PORT.OUTCLR = ERROR_LED_PIN;
    ERROR_LED_PORT.DIRSET = ERROR_LED_PIN;
    DFPLAYER_BUSY_PORT.DIRCLR = DFPLAYER_BUSY_PIN;
}

static void flash_error(void)
{
    uint8_t count;

    ERROR_LED_PORT.OUTCLR = ERROR_LED_PIN;
    for (count = 0U; count < 3U; ++count) {
        ERROR_LED_PORT.OUTSET = ERROR_LED_PIN;
        _delay_ms(100);
        ERROR_LED_PORT.OUTCLR = ERROR_LED_PIN;
        _delay_ms(100);
    }
}

static void flash_success(void)
{
    ERROR_LED_PORT.OUTSET = ERROR_LED_PIN;
    _delay_ms(100);
    ERROR_LED_PORT.OUTCLR = ERROR_LED_PIN;
}

static void try_play_file(void)
{
    tinyavr1_dfplayer_response_t response;
    tinyavr1_dfplayer_result_t result;

    result = tinyavr1_dfplayer_play_folder_file_feedback(
        &player, DFPLAYER_TEST_FOLDER, DFPLAYER_TEST_FILE, 1U);
    if (result == TINYAVR1_DFPLAYER_OK) {
        result = tinyavr1_dfplayer_read_response(
            &player, &response, DFPLAYER_RESPONSE_TIMEOUT_MS);
        if ((result == TINYAVR1_DFPLAYER_OK) &&
            (response.command != 0x41U)) {
            result = TINYAVR1_DFPLAYER_TRANSPORT_ERROR;
        }
    }

    if (result != TINYAVR1_DFPLAYER_OK) {
        flash_error();
    } else {
        flash_success();
    }
}

int main(void)
{
    clock_init();
    gpio_init();
    tinyavr1_soft_uart_init(&serial,
                            &DFPLAYER_UART_PORT, DFPLAYER_TX_PIN,
                            &DFPLAYER_UART_PORT, DFPLAYER_RX_PIN);
    tinyavr1_dfplayer_init(
        &player,
        tinyavr1_dfplayer_transport_soft_uart(&serial));

    for (;;) {
        _delay_ms(DFPLAYER_TEST_INTERVAL_MS);
        try_play_file();
    }
}
