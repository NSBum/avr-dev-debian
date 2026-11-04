#ifndef TINYAVR1_DFPLAYER_H
#define TINYAVR1_DFPLAYER_H

#include <stdint.h>

typedef enum tinyavr1_dfplayer_result {
    TINYAVR1_DFPLAYER_OK = 0,
    TINYAVR1_DFPLAYER_TIMEOUT,
    TINYAVR1_DFPLAYER_FRAME_ERROR,
    TINYAVR1_DFPLAYER_TRANSPORT_ERROR,
    TINYAVR1_DFPLAYER_INVALID_ARGUMENT
} tinyavr1_dfplayer_result_t;

typedef tinyavr1_dfplayer_result_t (*tinyavr1_dfplayer_write_fn)(
    void *context,
    const uint8_t *data,
    uint8_t length);

typedef tinyavr1_dfplayer_result_t (*tinyavr1_dfplayer_read_byte_fn)(
    void *context,
    uint8_t *value,
    uint16_t timeout_ms);

typedef struct {
    void *context;
    tinyavr1_dfplayer_write_fn write;
    tinyavr1_dfplayer_read_byte_fn read_byte;
} tinyavr1_dfplayer_transport_t;

typedef struct {
    tinyavr1_dfplayer_transport_t transport;
} tinyavr1_dfplayer_t;

typedef struct {
    uint8_t command;
    uint8_t feedback;
    uint16_t parameter;
} tinyavr1_dfplayer_response_t;

void tinyavr1_dfplayer_init(tinyavr1_dfplayer_t *player,
                            tinyavr1_dfplayer_transport_t transport);

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_send_command(
    const tinyavr1_dfplayer_t *player,
    uint8_t command,
    uint16_t parameter,
    uint8_t request_feedback);

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_read_response(
    const tinyavr1_dfplayer_t *player,
    tinyavr1_dfplayer_response_t *response,
    uint16_t timeout_ms);

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_play_track(
    const tinyavr1_dfplayer_t *player,
    uint16_t track);
tinyavr1_dfplayer_result_t tinyavr1_dfplayer_play_folder_file(
    const tinyavr1_dfplayer_t *player,
    uint8_t folder,
    uint8_t file);
tinyavr1_dfplayer_result_t tinyavr1_dfplayer_next(
    const tinyavr1_dfplayer_t *player);
tinyavr1_dfplayer_result_t tinyavr1_dfplayer_previous(
    const tinyavr1_dfplayer_t *player);
tinyavr1_dfplayer_result_t tinyavr1_dfplayer_pause(
    const tinyavr1_dfplayer_t *player);
tinyavr1_dfplayer_result_t tinyavr1_dfplayer_resume(
    const tinyavr1_dfplayer_t *player);
tinyavr1_dfplayer_result_t tinyavr1_dfplayer_stop(
    const tinyavr1_dfplayer_t *player);
tinyavr1_dfplayer_result_t tinyavr1_dfplayer_set_volume(
    const tinyavr1_dfplayer_t *player,
    uint8_t volume);

#endif
