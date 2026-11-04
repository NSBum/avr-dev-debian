#ifndef LD2410C_PRESENCE_BOARD_H
#define LD2410C_PRESENCE_BOARD_H

#include <avr/io.h>

#define INDICATOR_PORT PORTA
#define INDICATOR_PIN  PIN4_bm

/* USART0 default route on the ATtiny3217. */
#define RADAR_RX_PORT PORTB
#define RADAR_RX_PIN  PIN3_bm

/* TWI0 default route: PB1/SDA and PB0/SCL. */
#define I2C_PORT PORTB
#define I2C_SDA_PIN PIN1_bm
#define I2C_SCL_PIN PIN0_bm

/* External pulldowns hold these address-select inputs low. */
#define I2C_ADDRESS_PORT PORTC
#define I2C_ADDRESS_A0_PIN PIN4_bm
#define I2C_ADDRESS_A1_PIN PIN5_bm

/* Test-only DFPlayer software UART and BUSY input. */
#define DFPLAYER_UART_PORT PORTA
#define DFPLAYER_TX_PIN    PIN5_bm
#define DFPLAYER_RX_PIN    PIN6_bm
#define DFPLAYER_BUSY_PORT PORTA
#define DFPLAYER_BUSY_PIN  PIN7_bm

#endif
