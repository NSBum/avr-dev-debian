#ifndef TEST_DFPLAYER_3217_BOARD_H
#define TEST_DFPLAYER_3217_BOARD_H

#include <avr/io.h>

#define ERROR_LED_PORT PORTA
#define ERROR_LED_PIN  PIN4_bm

/* Software UART: MCU TX -> DFPlayer RX, DFPlayer TX -> MCU RX. */
#define DFPLAYER_UART_PORT PORTA
#define DFPLAYER_TX_PIN    PIN5_bm
#define DFPLAYER_RX_PIN    PIN6_bm

/* DFPlayer BUSY is active low while a track is playing. */
#define DFPLAYER_BUSY_PORT PORTA
#define DFPLAYER_BUSY_PIN  PIN7_bm

#endif
