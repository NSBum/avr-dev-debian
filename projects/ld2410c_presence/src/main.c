#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <stdint.h>

#include "board.h"
#include "config.h"

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
static volatile uint8_t i2c_register;
static volatile uint8_t i2c_tx_started;

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

static void i2c_init(void)
{
    uint8_t address = (uint8_t)(I2C_BASE_ADDRESS |
                                ((I2C_ADDRESS_PORT.IN &
                                  I2C_ADDRESS_A0_PIN) != 0U ? 0x01U : 0U) |
                                ((I2C_ADDRESS_PORT.IN &
                                  I2C_ADDRESS_A1_PIN) != 0U ? 0x02U : 0U));

    PORTMUX.CTRLB &= (uint8_t)~PORTMUX_TWI0_bm;
    I2C_PORT.DIRCLR = I2C_SDA_PIN | I2C_SCL_PIN;
    I2C_ADDRESS_PORT.DIRCLR = I2C_ADDRESS_A0_PIN | I2C_ADDRESS_A1_PIN;

    TWI0.SADDR = (uint8_t)(address << 1U);
    TWI0.SADDRMASK = 0U;
    TWI0.SCTRLA = TWI_APIEN_bm | TWI_DIEN_bm | TWI_PIEN_bm |
                  TWI_ENABLE_bm;
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

static uint16_t little_endian_u16(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U);
}

static enum target_mode decode_basic_report(const uint8_t *payload,
                                             uint16_t payload_length,
                                             uint8_t *raw_state_out,
                                             uint16_t *moving_distance_out,
                                             uint16_t *stationary_distance_out)
{
    uint8_t state;
    uint16_t moving_distance;
    uint16_t stationary_distance;

    /* Basic reports contain 13 payload bytes. Engineering reports retain
       these first 13 bytes, so they are safe to decode the same way. */
    if ((payload_length < 13U) ||
        (payload[0] != 0x01U && payload[0] != 0x02U) ||
        (payload[1] != 0xAAU) ||
        (payload[11] != 0x55U) ||
        (payload[12] != 0x00U)) {
        return TARGET_NONE;
    }

    state = payload[2];
    moving_distance = little_endian_u16(&payload[3]);
    stationary_distance = little_endian_u16(&payload[6]);

    *raw_state_out = state;
    *moving_distance_out = moving_distance;
    *stationary_distance_out = stationary_distance;

    /* State 0x03 means both; moving indication has priority. */
    if (((state == 0x01U) || (state == 0x03U)) &&
        (moving_distance <= TARGET_RANGE_CM)) {
        return TARGET_MOVING;
    }

    if (((state == 0x02U) || (state == 0x03U)) &&
        (stationary_distance <= TARGET_RANGE_CM)) {
        return TARGET_STATIONARY;
    }

    return TARGET_NONE;
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

static uint8_t i2c_register_value(uint8_t reg)
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

static void process_report_byte(uint8_t value)
{
    static const uint8_t header[4] = {0xF4U, 0xF3U, 0xF2U, 0xF1U};
    static const uint8_t tail[4] = {0xF8U, 0xF7U, 0xF6U, 0xF5U};
    static uint8_t frame[64];
    static uint8_t header_index;
    static uint8_t tail_index;
    static uint8_t frame_index;
    static uint16_t payload_length;
    static uint8_t state;
    static enum target_mode current_mode = TARGET_NONE;
    static uint8_t raw_state;
    static uint16_t moving_distance;
    static uint16_t stationary_distance;
    enum target_mode mode;

    if (state == 0U) {
        if (value == header[header_index]) {
            ++header_index;
            if (header_index == 4U) {
                state = 1U;
                frame_index = 0U;
                payload_length = 0U;
            }
        } else {
            header_index = (value == header[0]) ? 1U : 0U;
        }
        return;
    }

    if (state == 1U) {
        frame[frame_index++] = value;
        if (frame_index == 2U) {
            payload_length = little_endian_u16(frame);
            if ((payload_length == 0U) || (payload_length > 56U)) {
                state = 0U;
                header_index = 0U;
            } else {
                frame_index = 0U;
                state = 2U;
            }
        }
        return;
    }

    if (state == 2U) {
        frame[frame_index++] = value;
        if (frame_index >= payload_length) {
            frame_index = 0U;
            tail_index = 0U;
            state = 3U;
        }
        return;
    }

    if (value != tail[tail_index]) {
        state = 0U;
        header_index = (value == header[0]) ? 1U : 0U;
        return;
    }

    ++tail_index;
    if (tail_index == 4U) {
        mode = decode_basic_report(frame, payload_length,
                                   &raw_state,
                                   &moving_distance,
                                   &stationary_distance);
        if ((payload_length >= 13U) &&
            ((frame[0] == 0x01U) || (frame[0] == 0x02U)) &&
            (frame[1] == 0xAAU) && (frame[11] == 0x55U) &&
            (frame[12] == 0x00U)) {
            update_radar_snapshot(raw_state,
                                  moving_distance,
                                  stationary_distance);
        }
        if (mode != current_mode) {
            set_blink_mode(mode);
            current_mode = mode;
        }
        state = 0U;
        header_index = 0U;
    }
}

ISR(TWI0_TWIS_vect)
{
    uint8_t status = TWI0.SSTATUS;

    /* When address and data flags are both pending, service DIF first so
       the first byte of a read is loaded before releasing the clock. */
    if ((status & TWI_DIF_bm) != 0U) {
        if ((status & TWI_DIR_bm) == 0U) {
            /* Register pointer write; additional writes are ignored. */
            i2c_register = TWI0.SDATA;
            i2c_tx_started = 0U;
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        } else if (i2c_tx_started == 0U) {
            /* RXACK is not valid until after the first byte is sent. */
            TWI0.SDATA = i2c_register_value(i2c_register);
            i2c_tx_started = 1U;
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        } else if ((status & TWI_RXACK_bm) == 0U) {
            ++i2c_register;
            TWI0.SDATA = i2c_register_value(i2c_register);
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        } else {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        }
    } else if ((status & TWI_APIF_bm) != 0U) {
        if ((status & TWI_AP_bm) == 0U) {
            /* STOP: complete the previous transaction. */
            i2c_tx_started = 0U;
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        } else {
            /* Address match: acknowledge it. Data handling follows in DIF. */
            i2c_tx_started = 0U;
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        }
    }
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
}

int main(void)
{
    uint8_t value;

    clock_init();
    gpio_init();
    usart0_init();
    i2c_init();
    timer_init();
    sei();

    for (;;) {
        if (rx_read(&value) != 0U) {
            process_report_byte(value);
        }
    }
}
