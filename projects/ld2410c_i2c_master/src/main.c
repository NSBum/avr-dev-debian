#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <stdint.h>

#include "board.h"
#include "config.h"

enum radar_status {
    RADAR_NOT_DETECTED = 0x00U,
    RADAR_MOVING = 0x01U,
    RADAR_STATIONARY = 0x02U
};

static volatile uint16_t system_millis;

static void clock_init(void)
{
    /* FUSE.OSCCFG must select OSC20M. */
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);
}

static void gpio_init(void)
{
    STATUS_PORT.OUTCLR = MOVING_PIN | STILL_PIN | NONE_PIN;
    STATUS_PORT.DIRSET = MOVING_PIN | STILL_PIN | NONE_PIN;
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

static uint16_t millis_now(void)
{
    uint16_t value;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        value = system_millis;
    }

    return value;
}

static uint8_t deadline_reached(uint16_t deadline)
{
    return ((int16_t)(millis_now() - deadline) >= 0) ? 1U : 0U;
}

static void i2c_init(void)
{
    PORTMUX.CTRLB &= (uint8_t)~PORTMUX_TWI0_bm;
    I2C_PORT.DIRCLR = I2C_SDA_PIN | I2C_SCL_PIN;

    TWI0.CTRLA = TWI_SDAHOLD_50NS_gc;
    TWI0.MBAUD = TWI0_BAUD_VALUE;
    TWI0.MCTRLA = TWI_ENABLE_bm;
    TWI0.MCTRLB = 0;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

static void i2c_stop(void)
{
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}

static uint8_t i2c_wait_for(uint8_t flag, uint16_t deadline)
{
    for (;;) {
        uint8_t status = TWI0.MSTATUS;

        if ((status & (TWI_BUSERR_bm | TWI_ARBLOST_bm)) != 0U) {
            i2c_stop();
            return 0U;
        }

        if ((status & flag) != 0U) {
            return 1U;
        }

        if (deadline_reached(deadline) != 0U) {
            i2c_stop();
            return 0U;
        }
    }
}

static uint8_t i2c_read_register(uint8_t reg, uint8_t *value)
{
    uint16_t deadline = (uint16_t)(millis_now() + I2C_TIMEOUT_MS);

    TWI0.MADDR = (uint8_t)(I2C_SLAVE_ADDRESS << 1U);
    if (i2c_wait_for(TWI_WIF_bm, deadline) == 0U) {
        return 0U;
    }
    if ((TWI0.MSTATUS & TWI_RXACK_bm) != 0U) {
        i2c_stop();
        return 0U;
    }

    /* Prime the TWI data transfer before loading MDATA. */
    TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
    TWI0.MDATA = reg;
    if (i2c_wait_for(TWI_WIF_bm, deadline) == 0U) {
        return 0U;
    }
    if ((TWI0.MSTATUS & TWI_RXACK_bm) != 0U) {
        i2c_stop();
        return 0U;
    }

    TWI0.MADDR = (uint8_t)((I2C_SLAVE_ADDRESS << 1U) | 0x01U);
    if (i2c_wait_for(TWI_RIF_bm, deadline) == 0U) {
        return 0U;
    }
    if ((TWI0.MSTATUS & TWI_RXACK_bm) != 0U) {
        i2c_stop();
        return 0U;
    }

    *value = TWI0.MDATA;
    TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;
    return 1U;
}

static uint8_t i2c_write_registers(uint8_t reg,
                                   const uint8_t *data,
                                   uint8_t length)
{
    uint16_t deadline = (uint16_t)(millis_now() + I2C_TIMEOUT_MS);
    uint8_t index;

    TWI0.MADDR = (uint8_t)(I2C_SLAVE_ADDRESS << 1U);
    if (i2c_wait_for(TWI_WIF_bm, deadline) == 0U) {
        return 0U;
    }
    if ((TWI0.MSTATUS & TWI_RXACK_bm) != 0U) {
        i2c_stop();
        return 0U;
    }

    /* Prime the TWI data transfer before loading MDATA. */
    TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
    TWI0.MDATA = reg;
    if (i2c_wait_for(TWI_WIF_bm, deadline) == 0U) {
        return 0U;
    }
    if ((TWI0.MSTATUS & TWI_RXACK_bm) != 0U) {
        i2c_stop();
        return 0U;
    }

    for (index = 0U; index < length; ++index) {
        TWI0.MDATA = data[index];
        if (i2c_wait_for(TWI_WIF_bm, deadline) == 0U) {
            return 0U;
        }
        if ((TWI0.MSTATUS & TWI_RXACK_bm) != 0U) {
            i2c_stop();
            return 0U;
        }
    }

    i2c_stop();
    return 1U;
}

static void set_status_outputs(uint8_t status)
{
    STATUS_PORT.OUTCLR = MOVING_PIN | STILL_PIN | NONE_PIN;

    if (status == RADAR_MOVING) {
        STATUS_PORT.OUTSET = MOVING_PIN;
    } else if (status == RADAR_STATIONARY) {
        STATUS_PORT.OUTSET = STILL_PIN;
    } else {
        STATUS_PORT.OUTSET = NONE_PIN;
    }
}

static uint8_t play_next_dfplayer_file(uint8_t file)
{
    const uint8_t command_data[] = {
        DFPLAYER_FOLDER, file, 1U
    };

    /* Starting at 0x23 writes folder, file, then execute at 0x25. */
    return i2c_write_registers(DFPLAYER_PLAY_REGISTER,
                               command_data,
                               sizeof(command_data));
}

ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;
    ++system_millis;
}

int main(void)
{
    uint16_t next_poll;
    uint16_t next_play;
    uint8_t status;
    uint8_t next_file = DFPLAYER_FIRST_FILE;

    clock_init();
    gpio_init();
    timer_init();
    i2c_init();
    sei();

    next_poll = 0U;
    next_play = DFPLAYER_PLAY_INTERVAL_MS;
    for (;;) {
        if (deadline_reached(next_poll) != 0U) {
            next_poll = (uint16_t)(millis_now() + I2C_POLL_PERIOD_MS);
            if (i2c_read_register(0x00U, &status) != 0U) {
                set_status_outputs(status);
            } else {
                /* A missing/unreadable slave is treated as no detected target. */
                set_status_outputs(RADAR_NOT_DETECTED);
            }
        }

        if (deadline_reached(next_play) != 0U) {
            next_play = (uint16_t)(millis_now() + DFPLAYER_PLAY_INTERVAL_MS);
            if (play_next_dfplayer_file(next_file) != 0U) {
                ++next_file;
                if (next_file > DFPLAYER_LAST_FILE) {
                    next_file = DFPLAYER_FIRST_FILE;
                }
            }
        }
    }
}
