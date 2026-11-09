#ifndef LD2410C_I2C_MASTER_CONFIG_H
#define LD2410C_I2C_MASTER_CONFIG_H

#include <stdint.h>

#define I2C_SLAVE_ADDRESS 0x30U
#define I2C_BUS_HZ        100000UL
#define I2C_POLL_PERIOD_MS 50U
#define I2C_TIMEOUT_MS     20U

#define DFPLAYER_FOLDER              4U
#define DFPLAYER_FIRST_FILE          1U
#define DFPLAYER_LAST_FILE           5U
#define DFPLAYER_PLAY_INTERVAL_MS    5000U
#define DFPLAYER_PLAY_REGISTER       0x23U

/* TWI master BAUD = F_CPU / (2 * SCL) - 5, neglecting rise/fall time. */
#define TWI0_BAUD_VALUE \
    ((uint8_t)((F_CPU / (2UL * I2C_BUS_HZ)) - 5UL))

#endif
