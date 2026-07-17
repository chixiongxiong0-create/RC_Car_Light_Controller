#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MSP_MAX_PAYLOAD 64u

typedef struct {
    uint16_t command;
    uint8_t length;
    uint8_t payload[MSP_MAX_PAYLOAD];
} MspFrame;

typedef struct {
    uint8_t state;
    uint8_t length;
    uint8_t command;
    uint8_t offset;
    uint8_t checksum;
    MspFrame frame;
} MspCodec;

typedef enum {
    MSP_CODEC_INCOMPLETE = 0,
    MSP_CODEC_FRAME,
    MSP_CODEC_CHECKSUM_ERROR
} MspCodecFeedResult;

void msp_codec_reset(MspCodec *codec);
MspCodecFeedResult msp_codec_feed(MspCodec *codec, uint8_t byte, MspFrame *out);
size_t msp_v1_encode_request(uint8_t command, uint8_t out[6]);
