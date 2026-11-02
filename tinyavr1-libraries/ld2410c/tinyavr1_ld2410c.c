#include <stdint.h>

#include "tinyavr1_ld2410c.h"

static uint16_t little_endian_u16(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U);
}

static void reset_parser(tinyavr1_ld2410c_parser_t *parser)
{
    parser->state = 0U;
    parser->header_index = 0U;
    parser->tail_index = 0U;
    parser->frame_index = 0U;
    parser->payload_length = 0U;
}

static void emit_report(tinyavr1_ld2410c_parser_t *parser)
{
    tinyavr1_ld2410c_report_t report;

    if ((parser->payload_length < 13U) ||
        (parser->frame[0] != 0x01U && parser->frame[0] != 0x02U) ||
        (parser->frame[1] != 0xAAU) ||
        (parser->frame[11] != 0x55U) ||
        (parser->frame[12] != 0x00U)) {
        return;
    }

    report.target_state = parser->frame[2];
    report.moving_distance_cm = little_endian_u16(&parser->frame[3]);
    report.stationary_distance_cm = little_endian_u16(&parser->frame[6]);

    if (parser->report_callback != 0) {
        parser->report_callback(&report);
    }
}

void tinyavr1_ld2410c_init(
    tinyavr1_ld2410c_parser_t *parser,
    tinyavr1_ld2410c_report_callback_t report_callback)
{
    parser->report_callback = report_callback;
    reset_parser(parser);
}

void tinyavr1_ld2410c_receive_byte(
    tinyavr1_ld2410c_parser_t *parser,
    uint8_t value)
{
    static const uint8_t header[4] = {0xF4U, 0xF3U, 0xF2U, 0xF1U};
    static const uint8_t tail[4] = {0xF8U, 0xF7U, 0xF6U, 0xF5U};

    if (parser->state == 0U) {
        if (value == header[parser->header_index]) {
            ++parser->header_index;
            if (parser->header_index == 4U) {
                parser->state = 1U;
                parser->frame_index = 0U;
                parser->payload_length = 0U;
            }
        } else {
            parser->header_index = (value == header[0]) ? 1U : 0U;
        }
        return;
    }

    if (parser->state == 1U) {
        parser->frame[parser->frame_index++] = value;
        if (parser->frame_index == 2U) {
            parser->payload_length = little_endian_u16(parser->frame);
            if ((parser->payload_length == 0U) ||
                (parser->payload_length > 56U)) {
                reset_parser(parser);
            } else {
                parser->frame_index = 0U;
                parser->state = 2U;
            }
        }
        return;
    }

    if (parser->state == 2U) {
        parser->frame[parser->frame_index++] = value;
        if (parser->frame_index >= parser->payload_length) {
            parser->frame_index = 0U;
            parser->tail_index = 0U;
            parser->state = 3U;
        }
        return;
    }

    if (value != tail[parser->tail_index]) {
        reset_parser(parser);
        if (value == header[0]) {
            parser->header_index = 1U;
        }
        return;
    }

    ++parser->tail_index;
    if (parser->tail_index == 4U) {
        emit_report(parser);
        reset_parser(parser);
    }
}
