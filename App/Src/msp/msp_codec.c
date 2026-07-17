#include "msp/msp_codec.h"

enum {
    WAIT_DOLLAR,
    WAIT_M,
    WAIT_DIRECTION,
    WAIT_LENGTH,
    WAIT_COMMAND,
    WAIT_PAYLOAD,
    WAIT_CHECKSUM
};

void msp_codec_reset(MspCodec *codec)
{
    codec->state = WAIT_DOLLAR;
    codec->length = 0u;
    codec->command = 0u;
    codec->offset = 0u;
    codec->checksum = 0u;
}

static void restart_or_reset(MspCodec *codec, uint8_t byte)
{
    msp_codec_reset(codec);
    if (byte == '$') {
        codec->state = WAIT_M;
    }
}

bool msp_codec_feed(MspCodec *codec, uint8_t byte, MspFrame *out)
{
    switch (codec->state) {
    case WAIT_DOLLAR:
        if (byte == '$') {
            codec->state = WAIT_M;
        }
        break;
    case WAIT_M:
        if (byte == 'M') {
            codec->state = WAIT_DIRECTION;
        } else {
            restart_or_reset(codec, byte);
        }
        break;
    case WAIT_DIRECTION:
        if (byte == '>') {
            codec->state = WAIT_LENGTH;
        } else {
            restart_or_reset(codec, byte);
        }
        break;
    case WAIT_LENGTH:
        if (byte > MSP_MAX_PAYLOAD) {
            restart_or_reset(codec, byte);
            break;
        }
        codec->length = byte;
        codec->frame.length = byte;
        codec->offset = 0u;
        codec->checksum = byte;
        codec->state = WAIT_COMMAND;
        break;
    case WAIT_COMMAND:
        codec->command = byte;
        codec->frame.command = byte;
        codec->checksum ^= byte;
        codec->state = codec->length == 0u ? WAIT_CHECKSUM : WAIT_PAYLOAD;
        break;
    case WAIT_PAYLOAD:
        codec->frame.payload[codec->offset++] = byte;
        codec->checksum ^= byte;
        if (codec->offset == codec->length) {
            codec->state = WAIT_CHECKSUM;
        }
        break;
    case WAIT_CHECKSUM: {
        const bool valid = byte == codec->checksum;
        if (valid) {
            *out = codec->frame;
        }
        msp_codec_reset(codec);
        return valid;
    }
    default:
        msp_codec_reset(codec);
        break;
    }
    return false;
}

size_t msp_v1_encode_request(uint8_t command, uint8_t out[6])
{
    out[0] = '$';
    out[1] = 'M';
    out[2] = '<';
    out[3] = 0u;
    out[4] = command;
    out[5] = command;
    return 6u;
}
