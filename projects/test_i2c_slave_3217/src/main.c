#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "board.h"
#include "config.h"

static uint8_t tx_started;

static void clock_init(void)
{
    /* FUSE.OSCCFG must select OSC20M. */
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);
}

static void gpio_init(void)
{
    DIAGNOSTIC_PORT.OUTCLR = DIAGNOSTIC_PIN;
    DIAGNOSTIC_PORT.DIRSET = DIAGNOSTIC_PIN;
}

static void i2c_init(void)
{
    PORTMUX.CTRLB &= (uint8_t)~PORTMUX_TWI0_bm;
    I2C_PORT.DIRCLR = I2C_SDA_PIN | I2C_SCL_PIN;

    /* SADDR stores the 7-bit address in bits 7:1. */
    TWI0.SADDR = (uint8_t)(I2C_SLAVE_ADDRESS << 1U);
    TWI0.SADDRMASK = 0U;
    TWI0.SCTRLA = TWI_APIEN_bm | TWI_DIEN_bm | TWI_PIEN_bm |
                  TWI_ENABLE_bm;
}

ISR(TWI0_TWIS_vect)
{
    uint8_t status = TWI0.SSTATUS;

    /* Service data first if address and data flags overlap. */
    if ((status & TWI_DIF_bm) != 0U) {
        if ((status & TWI_DIR_bm) != 0U) {
            /* RXACK is not valid until after the first byte is sent. */
            if (tx_started == 0U) {
                TWI0.SDATA = I2C_RESPONSE_BYTE;
                tx_started = 1U;
                TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
            } else if ((status & TWI_RXACK_bm) == 0U) {
                TWI0.SDATA = I2C_RESPONSE_BYTE;
                TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
            } else {
                TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            }
        } else {
            (void)TWI0.SDATA;
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        }
    } else if ((status & TWI_APIF_bm) != 0U) {
        if ((status & TWI_AP_bm) != 0U) {
            tx_started = 0U;
            DIAGNOSTIC_PORT.OUTSET = DIAGNOSTIC_PIN;
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        } else {
            tx_started = 0U;
            DIAGNOSTIC_PORT.OUTCLR = DIAGNOSTIC_PIN;
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        }
    }
}

int main(void)
{
    clock_init();
    gpio_init();
    i2c_init();
    sei();

    for (;;) {
    }
}
