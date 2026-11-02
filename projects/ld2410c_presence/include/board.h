#ifndef LD2410C_PRESENCE_BOARD_H
#define LD2410C_PRESENCE_BOARD_H

#include <avr/io.h>

#define INDICATOR_PORT PORTA
#define INDICATOR_PIN  PIN4_bm

/* USART0 default route on the ATtiny3217. */
#define RADAR_RX_PORT PORTB
#define RADAR_RX_PIN  PIN3_bm

#endif
