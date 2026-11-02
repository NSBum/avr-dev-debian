#ifndef TEST_I2C_SLAVE_3217_BOARD_H
#define TEST_I2C_SLAVE_3217_BOARD_H

#include <avr/io.h>

#define DIAGNOSTIC_PORT PORTA
#define DIAGNOSTIC_PIN  PIN4_bm

/* TWI0 default route: PB1/SDA and PB0/SCL. */
#define I2C_PORT PORTB
#define I2C_SDA_PIN PIN1_bm
#define I2C_SCL_PIN PIN0_bm

#endif
