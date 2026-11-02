#include <avr/interrupt.h>
#include <avr/io.h>
#include <stddef.h>
#include <stdint.h>

#include "tinyavr1_twi_slave.h"

static tinyavr1_twi_slave_read_callback_t read_callback;
static tinyavr1_twi_slave_write_callback_t write_callback;
static uint8_t register_pointer;
static uint8_t pointer_received;
static uint8_t tx_started;

void tinyavr1_twi_slave_init(
    uint8_t address,
    tinyavr1_twi_slave_read_callback_t configured_read_callback,
    tinyavr1_twi_slave_write_callback_t configured_write_callback)
{
    /* Verify the default route against the exact device header/datasheet. */
    PORTMUX.CTRLB &= (uint8_t)~PORTMUX_TWI0_bm;

    /* SADDR stores the 7-bit address in bits 7:1; bit 0 disables general call. */
    TWI0.SADDR = (uint8_t)(address << 1U);
    TWI0.SADDRMASK = 0U;

    read_callback = configured_read_callback;
    write_callback = configured_write_callback;
    register_pointer = 0U;
    pointer_received = 0U;
    tx_started = 0U;

    TWI0.SCTRLA = TWI_APIEN_bm | TWI_DIEN_bm | TWI_PIEN_bm |
                  TWI_ENABLE_bm;
}

ISR(TWI0_TWIS_vect)
{
    uint8_t status = TWI0.SSTATUS;

    if ((status & (TWI_COLL_bm | TWI_BUSERR_bm)) != 0U) {
        /* COLL and BUSERR are write-one-to-clear status flags. */
        TWI0.SSTATUS = status & (TWI_COLL_bm | TWI_BUSERR_bm);
        pointer_received = 0U;
        tx_started = 0U;
        TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        return;
    }

    /* Follow the APIF/DIF precedence defined for the exact target. */
    if ((status & TWI_APIF_bm) != 0U) {
        if ((status & TWI_AP_bm) == 0U) {
            /* STOP: SCMD clears the pending APIF and completes the client. */
            pointer_received = 0U;
            tx_started = 0U;
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        } else {
            /* A repeated START with read direction retains the pointer. */
            tx_started = 0U;
            if ((status & TWI_DIR_bm) == 0U) {
                pointer_received = 0U;
            }
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        }
    } else if ((status & TWI_DIF_bm) != 0U) {
        if ((status & TWI_DIR_bm) == 0U) {
            uint8_t value = TWI0.SDATA;

            if (pointer_received == 0U) {
                register_pointer = value;
                pointer_received = 1U;
            } else if (write_callback != NULL) {
                write_callback(register_pointer, value);
                ++register_pointer;
            }
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        } else if (tx_started == 0U) {
            /* RXACK is not valid until after the first byte is transmitted. */
            TWI0.SDATA = (read_callback != NULL) ?
                         read_callback(register_pointer) : 0U;
            tx_started = 1U;
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        } else if ((status & TWI_RXACK_bm) == 0U) {
            ++register_pointer;
            TWI0.SDATA = (read_callback != NULL) ?
                         read_callback(register_pointer) : 0U;
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        } else {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        }
    }
}
