#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <stdint.h>

#include "board.h"
#include "config.h"

#if DFPLAYER_TEST_ENABLE != 0U
#include <util/delay.h>
#endif

#if DFPLAYER_TEST_ENABLE != 0U
#include "tinyavr1_dfplayer.h"
#include "tinyavr1_dfplayer_soft_uart.h"
#include "tinyavr1_soft_uart.h"
#endif
#include "tinyavr1_ld2410c.h"
#include "tinyavr1_twi_slave.h"

enum target_mode {
    TARGET_NONE = 0,
    TARGET_MOVING,
    TARGET_STATIONARY
};

static volatile uint8_t rx_buffer[RADAR_RX_BUFFER_SIZE];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;

static volatile uint8_t blink_active;
static volatile uint16_t blink_half_period_ms;
static volatile uint16_t blink_elapsed_ms;

/* The main loop is the only writer. TWI0 reads these in its ISR. */
static volatile uint8_t latest_status;
static volatile uint8_t latest_raw_state;
static volatile uint16_t latest_distance_cm;
static volatile uint16_t latest_moving_distance_cm;
static volatile uint16_t latest_stationary_distance_cm;
static tinyavr1_ld2410c_parser_t radar_parser;

#if DFPLAYER_TEST_ENABLE != 0U
static tinyavr1_soft_uart_t dfplayer_serial;
static tinyavr1_dfplayer_t dfplayer;
static volatile uint16_t dfplayer_test_elapsed_ms;
static volatile uint8_t dfplayer_test_due;
#endif
static void clock_init(void)
{
    /* FUSE.OSCCFG must select OSC20M. */
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);
}

static void gpio_init(void)
{
    INDICATOR_PORT.OUTCLR = INDICATOR_PIN;
    INDICATOR_PORT.DIRSET = INDICATOR_PIN;
#if DFPLAYER_TEST_ENABLE != 0U
    DFPLAYER_BUSY_PORT.DIRCLR = DFPLAYER_BUSY_PIN;
#endif
}

static void usart0_init(void)
{
    /* USART0 default route: PB2/TXD and PB3/RXD. TX is unused here. */
    PORTMUX.CTRLB &= (uint8_t)~PORTMUX_USART0_bm;
    RADAR_RX_PORT.DIRCLR = RADAR_RX_PIN;

    USART0.BAUD = USART0_BAUD_VALUE;
    USART0.CTRLC = USART_CHSIZE_8BIT_gc;
    USART0.CTRLB = USART_RXEN_bm;
    USART0.CTRLA = USART_RXCIE_bm;
}

static uint8_t i2c_address_init(void)
{
    I2C_ADDRESS_PORT.DIRCLR = I2C_ADDRESS_A0_PIN | I2C_ADDRESS_A1_PIN;

    return (uint8_t)(I2C_BASE_ADDRESS |
                     ((I2C_ADDRESS_PORT.IN & I2C_ADDRESS_A0_PIN) != 0U ?
                      0x01U : 0U) |
                     ((I2C_ADDRESS_PORT.IN & I2C_ADDRESS_A1_PIN) != 0U ?
                      0x02U : 0U));
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

static uint8_t rx_read(uint8_t *value)
{
    uint8_t available = 0U;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (rx_tail != rx_head) {
            *value = rx_buffer[rx_tail];
            rx_tail = (uint8_t)((rx_tail + 1U) & (RADAR_RX_BUFFER_SIZE - 1U));
            available = 1U;
        }
    }

    return available;
}

static void set_blink_mode(enum target_mode mode)
{
    uint16_t full_period_ms;

    if (mode == TARGET_MOVING) {
        full_period_ms = MOVING_BLINK_PERIOD_MS;
    } else if (mode == TARGET_STATIONARY) {
        full_period_ms = STATIONARY_BLINK_PERIOD_MS;
    } else {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            blink_active = 0U;
            INDICATOR_PORT.OUTCLR = INDICATOR_PIN;
        }
        return;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        /* Start each state transition with a deterministic low phase. */
        INDICATOR_PORT.OUTCLR = INDICATOR_PIN;
        blink_half_period_ms = (uint16_t)(full_period_ms / 2U);
        blink_elapsed_ms = 0U;
        blink_active = 1U;
    }
}

static uint8_t status_for_state(uint8_t raw_state,
                                uint16_t moving_distance,
                                uint16_t stationary_distance,
                                uint16_t *selected_distance)
{
    if ((raw_state == 0x01U) || (raw_state == 0x03U)) {
        *selected_distance = moving_distance;
        return 0x01U;
    }

    if (raw_state == 0x02U) {
        *selected_distance = stationary_distance;
        return 0x02U;
    }

    *selected_distance = 0U;
    return 0x00U;
}

static void update_radar_snapshot(uint8_t raw_state,
                                  uint16_t moving_distance,
                                  uint16_t stationary_distance)
{
    uint16_t selected_distance;
    uint8_t status = status_for_state(raw_state,
                                      moving_distance,
                                      stationary_distance,
                                      &selected_distance);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        latest_raw_state = raw_state;
        latest_status = status;
        latest_distance_cm = selected_distance;
        latest_moving_distance_cm = moving_distance;
        latest_stationary_distance_cm = stationary_distance;
    }
}

#if DFPLAYER_TEST_ENABLE != 0U && DFPLAYER_TEST_PERIODIC_ENABLE != 0U
static void flash_dfplayer_error(void)
{
    uint8_t flash;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        blink_active = 0U;
        INDICATOR_PORT.OUTCLR = INDICATOR_PIN;
    }

    for (flash = 0U; flash < 3U; ++flash) {
        INDICATOR_PORT.OUTSET = INDICATOR_PIN;
        _delay_ms(100);
        INDICATOR_PORT.OUTCLR = INDICATOR_PIN;
        _delay_ms(100);
    }
}

static void run_dfplayer_periodic_test(void)
{
    tinyavr1_dfplayer_response_t response;
    tinyavr1_dfplayer_result_t result;

    result = tinyavr1_dfplayer_play_folder_file_feedback(
        &dfplayer, 1U, DFPLAYER_TEST_TRACK_1, 1U);
    if (result == TINYAVR1_DFPLAYER_OK) {
        result = tinyavr1_dfplayer_read_response(
            &dfplayer, &response, DFPLAYER_TEST_RESPONSE_TIMEOUT_MS);
        if ((result == TINYAVR1_DFPLAYER_OK) &&
            (response.command == 0x40U)) {
            result = TINYAVR1_DFPLAYER_TRANSPORT_ERROR;
        }
    }

    if (result != TINYAVR1_DFPLAYER_OK) {
        flash_dfplayer_error();
    }
}
#endif

static uint8_t i2c_read_register(uint8_t reg)
{
    switch (reg) {
    case 0x00U:
        return latest_status;
    case 0x01U:
        return (uint8_t)latest_distance_cm;
    case 0x02U:
        return (uint8_t)(latest_distance_cm >> 8U);
    case 0x03U:
        return (uint8_t)latest_moving_distance_cm;
    case 0x04U:
        return (uint8_t)(latest_moving_distance_cm >> 8U);
    case 0x05U:
        return (uint8_t)latest_stationary_distance_cm;
    case 0x06U:
        return (uint8_t)(latest_stationary_distance_cm >> 8U);
    case 0x07U:
        return latest_raw_state;
    default:
        return 0U;
    }
}

static void radar_report_received(const tinyavr1_ld2410c_report_t *report)
{
    static enum target_mode current_mode = TARGET_NONE;
#if DFPLAYER_TEST_ENABLE != 0U && DFPLAYER_TEST_PERIODIC_ENABLE == 0U
    static uint8_t sequence_stage;
    static uint8_t next_track = DFPLAYER_TEST_TRACK_1;
#endif
    enum target_mode mode = TARGET_NONE;

    update_radar_snapshot(report->target_state,
                           report->moving_distance_cm,
                           report->stationary_distance_cm);

    if (((report->target_state == TINYAVR1_LD2410C_STATE_MOVING) ||
         (report->target_state == TINYAVR1_LD2410C_STATE_BOTH)) &&
        (report->moving_distance_cm <= TARGET_RANGE_CM)) {
        mode = TARGET_MOVING;
    } else if (((report->target_state == TINYAVR1_LD2410C_STATE_STATIONARY) ||
                (report->target_state == TINYAVR1_LD2410C_STATE_BOTH)) &&
               (report->stationary_distance_cm <= TARGET_RANGE_CM)) {
        mode = TARGET_STATIONARY;
    }

    if (mode != current_mode) {
        set_blink_mode(mode);
        current_mode = mode;
    }

#if DFPLAYER_TEST_ENABLE != 0U && DFPLAYER_TEST_PERIODIC_ENABLE == 0U
    /* Test sequence: moving -> stationary -> all clear. */
    if (mode == TARGET_MOVING) {
        if (sequence_stage == 0U) {
            sequence_stage = 1U;
        } else if (sequence_stage == 2U) {
            sequence_stage = 1U;
        }
    } else if (mode == TARGET_STATIONARY) {
        if (sequence_stage == 1U) {
            sequence_stage = 2U;
        }
    } else if ((mode == TARGET_NONE) && (sequence_stage == 2U)) {
        /* BUSY is active low. Do not interrupt a track during the test. */
        if ((DFPLAYER_BUSY_PORT.IN & DFPLAYER_BUSY_PIN) != 0U) {
            (void)tinyavr1_dfplayer_play_folder_file(&dfplayer, 1U,
                                                      next_track);
            next_track = (next_track == DFPLAYER_TEST_TRACK_1) ?
                         DFPLAYER_TEST_TRACK_2 : DFPLAYER_TEST_TRACK_1;
        }
        sequence_stage = 0U;
    }
#endif
}

ISR(USART0_RXC_vect)
{
    uint8_t status = USART0.RXDATAH;
    uint8_t data = USART0.RXDATAL;

    if ((status & (USART_FERR_bm | USART_PERR_bm | USART_BUFOVF_bm)) != 0U) {
        return;
    }

    uint8_t next = (uint8_t)((rx_head + 1U) & (RADAR_RX_BUFFER_SIZE - 1U));
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
            INDICATOR_PORT.OUTTGL = INDICATOR_PIN;
        }
    }
#if DFPLAYER_TEST_ENABLE != 0U && DFPLAYER_TEST_PERIODIC_ENABLE != 0U
    if (dfplayer_test_elapsed_ms < DFPLAYER_TEST_INTERVAL_MS) {
        ++dfplayer_test_elapsed_ms;
    } else {
        dfplayer_test_due = 1U;
    }
#endif
}

int main(void)
{
    uint8_t value;
    uint8_t i2c_address;

    clock_init();
    gpio_init();
    usart0_init();
    i2c_address = i2c_address_init();
    tinyavr1_twi_slave_init(i2c_address, i2c_read_register, 0);
    tinyavr1_ld2410c_init(&radar_parser, radar_report_received);
#if DFPLAYER_TEST_ENABLE != 0U
    tinyavr1_soft_uart_init(&dfplayer_serial,
                            &DFPLAYER_UART_PORT, DFPLAYER_TX_PIN,
                            &DFPLAYER_UART_PORT, DFPLAYER_RX_PIN);
    tinyavr1_dfplayer_init(
        &dfplayer,
        tinyavr1_dfplayer_transport_soft_uart(&dfplayer_serial));
#endif
    timer_init();
    sei();

    for (;;) {
        if (rx_read(&value) != 0U) {
            tinyavr1_ld2410c_receive_byte(&radar_parser, value);
        }
#if DFPLAYER_TEST_ENABLE != 0U && DFPLAYER_TEST_PERIODIC_ENABLE != 0U
        if (dfplayer_test_due != 0U) {
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                dfplayer_test_due = 0U;
                dfplayer_test_elapsed_ms = 0U;
            }
            run_dfplayer_periodic_test();
        }
#endif
    }
}
