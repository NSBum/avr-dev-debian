#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <stdint.h>

#include "board.h"
#include "config.h"

#include "tinyavr1_dfplayer.h"
#include "tinyavr1_dfplayer_soft_uart.h"
#include "tinyavr1_soft_uart.h"
#include "tinyavr1_ld2410c.h"
#include "tinyavr1_twi_slave.h"

static volatile uint8_t rx_buffer[RADAR_RX_BUFFER_SIZE];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;

/* The main loop is the only writer. TWI0 reads these in its ISR. */
static volatile uint8_t latest_status;
static volatile uint8_t latest_raw_state;
static volatile uint16_t latest_distance_cm;
static volatile uint16_t latest_moving_distance_cm;
static volatile uint16_t latest_stationary_distance_cm;
static tinyavr1_ld2410c_parser_t radar_parser;

static tinyavr1_soft_uart_t dfplayer_serial;
static tinyavr1_dfplayer_t dfplayer;

/* TWI callbacks only stage bytes. DFPlayer I/O is performed by main(). */
static volatile uint8_t df_command;
static volatile uint8_t df_feedback;
static volatile uint8_t df_parameter_hi;
static volatile uint8_t df_parameter_lo;
static volatile uint8_t df_track_hi;
static volatile uint8_t df_track_lo;
static volatile uint8_t df_folder;
static volatile uint8_t df_file;
static volatile uint8_t df_pending;
static volatile uint8_t df_status;
static volatile uint8_t df_result;
static volatile uint8_t df_response_command;
static volatile uint8_t df_response_feedback;
static volatile uint8_t df_response_hi;
static volatile uint8_t df_response_lo;
static volatile uint8_t df_rejected;
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
    DFPLAYER_BUSY_PORT.DIRCLR = DFPLAYER_BUSY_PIN;
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
    case DFPLAYER_REG_STATUS:
        return df_status;
    case DFPLAYER_REG_RESULT:
        return df_result;
    case DFPLAYER_REG_RESPONSE_CMD:
        return df_response_command;
    case DFPLAYER_REG_RESPONSE_FB:
        return df_response_feedback;
    case DFPLAYER_REG_RESPONSE_HI:
        return df_response_hi;
    case DFPLAYER_REG_RESPONSE_LO:
        return df_response_lo;
    case DFPLAYER_REG_BUSY:
        return (DFPLAYER_BUSY_PORT.IN & DFPLAYER_BUSY_PIN) != 0U ? 0U : 1U;
    case DFPLAYER_REG_REJECTED:
        return df_rejected;
    default:
        return 0U;
    }
}

static void dfplayer_queue(uint8_t command, uint16_t parameter,
                           uint8_t feedback)
{
    if (df_pending != 0U) {
        ++df_rejected;
        df_result = TINYAVR1_DFPLAYER_TRANSPORT_ERROR;
        return;
    }

    df_command = command;
    df_parameter_hi = (uint8_t)(parameter >> 8U);
    df_parameter_lo = (uint8_t)parameter;
    df_feedback = feedback != 0U ? 1U : 0U;
    df_status = 1U;
    df_pending = 1U;
}

static void i2c_write_register(uint8_t reg, uint8_t value)
{
    switch (reg) {
    case DFPLAYER_REG_COMMAND:
        df_command = value;
        break;
    case DFPLAYER_REG_FEEDBACK:
        df_feedback = value != 0U ? 1U : 0U;
        break;
    case DFPLAYER_REG_PARAMETER_HI:
        df_parameter_hi = value;
        break;
    case DFPLAYER_REG_PARAMETER_LO:
        df_parameter_lo = value;
        break;
    case DFPLAYER_REG_EXECUTE:
        if (value != 0U) {
            dfplayer_queue(df_command,
                           ((uint16_t)df_parameter_hi << 8U) |
                           df_parameter_lo, df_feedback);
        }
        break;
    case DFPLAYER_REG_TRACK_HI:
        df_track_hi = value;
        break;
    case DFPLAYER_REG_TRACK_LO:
        df_track_lo = value;
        break;
    case DFPLAYER_REG_PLAY_TRACK:
        if (value != 0U) {
            dfplayer_queue(0x03U,
                           ((uint16_t)df_track_hi << 8U) | df_track_lo, 0U);
        }
        break;
    case DFPLAYER_REG_FOLDER:
        df_folder = value;
        break;
    case DFPLAYER_REG_FILE:
        df_file = value;
        break;
    case DFPLAYER_REG_PLAY_FILE:
        if (value != 0U) {
            dfplayer_queue(0x0FU, ((uint16_t)df_folder << 8U) | df_file, 0U);
        }
        break;
    case DFPLAYER_REG_VOLUME:
        if (value <= 30U) {
            dfplayer_queue(0x06U, value, 0U);
        } else {
            df_result = TINYAVR1_DFPLAYER_INVALID_ARGUMENT;
            df_status = 5U;
        }
        break;
    default:
        break;
    }
}

static void dfplayer_service(void)
{
    uint8_t command;
    uint8_t feedback;
    uint16_t parameter;
    tinyavr1_dfplayer_result_t result;

    if (df_pending == 0U) {
        return;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        command = df_command;
        feedback = df_feedback;
        parameter = ((uint16_t)df_parameter_hi << 8U) | df_parameter_lo;
        df_pending = 0U;
    }

    df_status = 2U;
    result = tinyavr1_dfplayer_send_command(&dfplayer, command, parameter,
                                            feedback);
    if (result != TINYAVR1_DFPLAYER_OK) {
        df_result = (uint8_t)result;
        df_status = 5U;
        return;
    }

    if (feedback == 0U) {
        df_result = TINYAVR1_DFPLAYER_OK;
        df_status = 3U;
        return;
    }

    {
        tinyavr1_dfplayer_response_t response;
        result = tinyavr1_dfplayer_read_response(
            &dfplayer, &response, DFPLAYER_RESPONSE_TIMEOUT_MS);
        if (result == TINYAVR1_DFPLAYER_OK) {
            df_response_command = response.command;
            df_response_feedback = response.feedback;
            df_response_hi = (uint8_t)(response.parameter >> 8U);
            df_response_lo = (uint8_t)response.parameter;
        }
        df_result = (uint8_t)result;
        df_status = result == TINYAVR1_DFPLAYER_OK ? 4U : 5U;
    }
}

static void radar_report_received(const tinyavr1_ld2410c_report_t *report)
{
    update_radar_snapshot(report->target_state,
                           report->moving_distance_cm,
                           report->stationary_distance_cm);
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

int main(void)
{
    uint8_t value;
    uint8_t i2c_address;

    clock_init();
    gpio_init();
    usart0_init();
    i2c_address = i2c_address_init();
    tinyavr1_twi_slave_init(i2c_address, i2c_read_register,
                            i2c_write_register);
    tinyavr1_ld2410c_init(&radar_parser, radar_report_received);
    tinyavr1_soft_uart_init(&dfplayer_serial,
                            &DFPLAYER_UART_PORT, DFPLAYER_TX_PIN,
                            &DFPLAYER_UART_PORT, DFPLAYER_RX_PIN);
    tinyavr1_dfplayer_init(
        &dfplayer,
        tinyavr1_dfplayer_transport_soft_uart(&dfplayer_serial));
    sei();

    for (;;) {
        dfplayer_service();
        if (rx_read(&value) != 0U) {
            tinyavr1_ld2410c_receive_byte(&radar_parser, value);
        }
    }
}
