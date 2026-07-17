#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "msp/msp_codec.h"

typedef bool (*MspWriteFn)(const uint8_t *data, size_t length, void *ctx);
typedef void (*MspFrameFn)(const MspFrame *frame, uint32_t now_ms, void *ctx);

typedef struct {
    MspCodec codec;
    MspWriteFn write;
    MspFrameFn on_frame;
    void *ctx;
    uint32_t sent_at_ms;
    uint32_t last_valid_ms;
    uint32_t due_ms[5];
    uint8_t active_query;
    uint8_t startup_query;
    bool awaiting;
    uint32_t requests;
    uint32_t timeouts;
    uint32_t checksum_errors;
} MspClient;

void msp_client_init(MspClient *client, MspWriteFn write,
                     MspFrameFn on_frame, void *ctx);
void msp_client_rx_byte(MspClient *client, uint8_t byte, uint32_t now_ms);
void msp_client_tick(MspClient *client, uint32_t now_ms);
