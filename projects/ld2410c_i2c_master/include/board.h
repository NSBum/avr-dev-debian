#ifndef LD2410C_I2C_MASTER_BOARD_H
#define LD2410C_I2C_MASTER_BOARD_H

#include <avr/io.h>

#define STATUS_PORT PORTA
#define MOVING_PIN  PIN5_bm
#define STILL_PIN   PIN6_bm
#define NONE_PIN    PIN7_bm

/* TWI0 default route: PB1/SDA and PB0/SCL. */
#define I2C_PORT PORTB
#define I2C_SDA_PIN PIN1_bm
#define I2C_SCL_PIN PIN0_bm

#endif
