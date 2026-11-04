#include <stdint.h>

#include "tinyavr1_dfplayer.h"

enum {
    DFPLAYER_START = 0x7EU,
    DFPLAYER_VERSION = 0xFFU,
    DFPLAYER_LENGTH = 0x06U,
    DFPLAYER_END = 0xEFU,
    DFPLAYER_CMD_NEXT = 0x01U,
    DFPLAYER_CMD_PREVIOUS = 0x02U,
    DFPLAYER_CMD_PLAY_TRACK = 0x03U,
    DFPLAYER_CMD_PLAY_FOLDER_FILE = 0x0FU,
    DFPLAYER_CMD_SET_VOLUME = 0x06U,
    DFPLAYER_CMD_RESUME = 0x0DU,
    DFPLAYER_CMD_PAUSE = 0x0EU,
    DFPLAYER_CMD_STOP = 0x16U
};

static uint16_t checksum(const uint8_t *frame)
{
    uint16_t sum = 0U;
    uint8_t index;

    for (index = 1U; index <= 6U; ++index) {
        sum = (uint16_t)(sum + frame[index]);
    }
    return (uint16_t)(0U - sum);
}

void tinyavr1_dfplayer_init(tinyavr1_dfplayer_t *player,
                            tinyavr1_dfplayer_transport_t transport)
{
    player->transport = transport;
}

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_send_command(
    const tinyavr1_dfplayer_t *player,
    uint8_t command,
    uint16_t parameter,
    uint8_t request_feedback)
{
    uint8_t frame[10];
    uint16_t sum;

    if ((player == 0) || (player->transport.write == 0)) {
        return TINYAVR1_DFPLAYER_INVALID_ARGUMENT;
    }

    frame[0] = DFPLAYER_START;
    frame[1] = DFPLAYER_VERSION;
    frame[2] = DFPLAYER_LENGTH;
    frame[3] = command;
    frame[4] = request_feedback != 0U ? 0x01U : 0x00U;
    frame[5] = (uint8_t)(parameter >> 8U);
    frame[6] = (uint8_t)parameter;
    sum = checksum(frame);
    frame[7] = (uint8_t)(sum >> 8U);
    frame[8] = (uint8_t)sum;
    frame[9] = DFPLAYER_END;

    return player->transport.write(player->transport.context, frame, 10U);
}

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_read_response(
    const tinyavr1_dfplayer_t *player,
    tinyavr1_dfplayer_response_t *response,
    uint16_t timeout_ms)
{
    uint8_t frame[10];
    uint8_t value;
    uint8_t index;
    uint16_t received_checksum;

    if ((player == 0) || (response == 0) ||
        (player->transport.read_byte == 0)) {
        return TINYAVR1_DFPLAYER_INVALID_ARGUMENT;
    }

    do {
        tinyavr1_dfplayer_result_t result =
            player->transport.read_byte(player->transport.context,
                                        &value, timeout_ms);
        if (result != TINYAVR1_DFPLAYER_OK) {
            return result;
        }
    } while (value != DFPLAYER_START);

    frame[0] = value;
    for (index = 1U; index < 10U; ++index) {
        tinyavr1_dfplayer_result_t result =
            player->transport.read_byte(player->transport.context,
                                        &frame[index], timeout_ms);
        if (result != TINYAVR1_DFPLAYER_OK) {
            return result;
        }
    }

    if ((frame[1] != DFPLAYER_VERSION) ||
        (frame[2] != DFPLAYER_LENGTH) ||
        (frame[9] != DFPLAYER_END)) {
        return TINYAVR1_DFPLAYER_FRAME_ERROR;
    }

    received_checksum = ((uint16_t)frame[7] << 8U) | frame[8];
    if (received_checksum != checksum(frame)) {
        return TINYAVR1_DFPLAYER_FRAME_ERROR;
    }

    response->command = frame[3];
    response->feedback = frame[4];
    response->parameter = ((uint16_t)frame[5] << 8U) | frame[6];
    return TINYAVR1_DFPLAYER_OK;
}

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_play_track(
    const tinyavr1_dfplayer_t *player,
    uint16_t track)
{
    return tinyavr1_dfplayer_send_command(player, DFPLAYER_CMD_PLAY_TRACK,
                                          track, 0U);
}

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_play_folder_file(
    const tinyavr1_dfplayer_t *player,
    uint8_t folder,
    uint8_t file)
{
    return tinyavr1_dfplayer_play_folder_file_feedback(player, folder, file,
                                                       0U);
}

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_play_folder_file_feedback(
    const tinyavr1_dfplayer_t *player,
    uint8_t folder,
    uint8_t file,
    uint8_t request_feedback)
{
    return tinyavr1_dfplayer_send_command(
        player, DFPLAYER_CMD_PLAY_FOLDER_FILE,
        ((uint16_t)folder << 8U) | file, request_feedback);
}

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_next(
    const tinyavr1_dfplayer_t *player)
{
    return tinyavr1_dfplayer_send_command(player, DFPLAYER_CMD_NEXT, 0U, 0U);
}

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_previous(
    const tinyavr1_dfplayer_t *player)
{
    return tinyavr1_dfplayer_send_command(player, DFPLAYER_CMD_PREVIOUS,
                                          0U, 0U);
}

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_pause(
    const tinyavr1_dfplayer_t *player)
{
    return tinyavr1_dfplayer_send_command(player, DFPLAYER_CMD_PAUSE, 0U, 0U);
}

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_resume(
    const tinyavr1_dfplayer_t *player)
{
    return tinyavr1_dfplayer_send_command(player, DFPLAYER_CMD_RESUME, 0U, 0U);
}

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_stop(
    const tinyavr1_dfplayer_t *player)
{
    return tinyavr1_dfplayer_send_command(player, DFPLAYER_CMD_STOP, 0U, 0U);
}

tinyavr1_dfplayer_result_t tinyavr1_dfplayer_set_volume(
    const tinyavr1_dfplayer_t *player,
    uint8_t volume)
{
    if (volume > 30U) {
        return TINYAVR1_DFPLAYER_INVALID_ARGUMENT;
    }
    return tinyavr1_dfplayer_send_command(player, DFPLAYER_CMD_SET_VOLUME,
                                          volume, 0U);
}
