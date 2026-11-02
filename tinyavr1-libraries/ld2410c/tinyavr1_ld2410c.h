#ifndef TINYAVR1_LD2410C_H
#define TINYAVR1_LD2410C_H

#include <stdint.h>

enum tinyavr1_ld2410c_target_state {
    TINYAVR1_LD2410C_STATE_NONE = 0x00U,
    TINYAVR1_LD2410C_STATE_MOVING = 0x01U,
    TINYAVR1_LD2410C_STATE_STATIONARY = 0x02U,
    TINYAVR1_LD2410C_STATE_BOTH = 0x03U
};

typedef struct {
    uint8_t target_state;
    uint16_t moving_distance_cm;
    uint16_t stationary_distance_cm;
} tinyavr1_ld2410c_report_t;

typedef void (*tinyavr1_ld2410c_report_callback_t)(
    const tinyavr1_ld2410c_report_t *report);

typedef struct {
    tinyavr1_ld2410c_report_callback_t report_callback;
    uint8_t frame[64];
    uint8_t header_index;
    uint8_t tail_index;
    uint8_t frame_index;
    uint8_t state;
    uint16_t payload_length;
} tinyavr1_ld2410c_parser_t;

void tinyavr1_ld2410c_init(
    tinyavr1_ld2410c_parser_t *parser,
    tinyavr1_ld2410c_report_callback_t report_callback);

void tinyavr1_ld2410c_receive_byte(
    tinyavr1_ld2410c_parser_t *parser,
    uint8_t value);

#endif
