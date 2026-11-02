#ifndef TINYAVR1_TWI_SLAVE_H
#define TINYAVR1_TWI_SLAVE_H

#include <stdint.h>

/* Return the byte exposed at a read-only register address. */
typedef uint8_t (*tinyavr1_twi_slave_read_callback_t)(uint8_t reg);

/* Receive a write to a register. May be NULL for a read-only client. */
typedef void (*tinyavr1_twi_slave_write_callback_t)(uint8_t reg,
                                                     uint8_t value);

/*
 * Initialize TWI0 as a 7-bit client on its device-defined default route.
 * The application must call sei() after all interrupt-driven peripherals are
 * initialized.
 */
void tinyavr1_twi_slave_init(
    uint8_t address,
    tinyavr1_twi_slave_read_callback_t read_callback,
    tinyavr1_twi_slave_write_callback_t write_callback);

#endif
