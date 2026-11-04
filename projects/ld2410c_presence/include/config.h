#ifndef LD2410C_PRESENCE_CONFIG_H
#define LD2410C_PRESENCE_CONFIG_H

#include <stdint.h>

#define RADAR_BAUD             256000UL
#define RADAR_RX_BUFFER_SIZE   128U
#define TARGET_RANGE_CM        100U
#define I2C_BASE_ADDRESS       0x30U

/* Test-only DFPlayer feature: set to 0 to compile it out. */
#define DFPLAYER_TEST_ENABLE   1U
#define DFPLAYER_BAUD          9600UL
#define DFPLAYER_TEST_TRACK_1  1U
#define DFPLAYER_TEST_TRACK_2  2U

#define MOVING_BLINK_PERIOD_MS     200U
#define STATIONARY_BLINK_PERIOD_MS 1000U

/* Normal async USART mode: BAUD = 4 * F_CPU / baud, rounded. */
#define USART0_BAUD_VALUE \
    ((uint16_t)((4ULL * F_CPU + RADAR_BAUD / 2UL) / RADAR_BAUD))

#endif
