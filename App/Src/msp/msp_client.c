#include "msp/msp_client.h"

#include <string.h>

#include "app_time.h"

enum {
    MSP_API_VERSION = 1,
    MSP_FC_VARIANT = 2,
    MSP_FC_VERSION = 3,
    MSP_STATUS = 101,
    MSP_RAW_GPS = 106,
    MSP_ATTITUDE = 108,
    MSP_ANALOG = 110,
    MSP_RC = 105,
    STARTUP_QUERY_COUNT = 3,
    SCHEDULED_QUERY_COUNT = 5,
    REQUEST_TIMEOUT_MS = 40
};

typedef struct {
    uint8_t command;
    uint16_t period_ms;
} QueryDef;

static const uint8_t startup_queries[STARTUP_QUERY_COUNT] = {
    MSP_API_VERSION, MSP_FC_VARIANT, MSP_FC_VERSION
};

static const QueryDef queries[SCHEDULED_QUERY_COUNT] = {
    {MSP_RC, 40u},
    {MSP_ATTITUDE, 50u},
    {MSP_ANALOG, 200u},
    {MSP_RAW_GPS, 200u},
    {MSP_STATUS, 500u}
};

static bool send_request(MspClient *client, uint8_t command, uint32_t now_ms)
{
    uint8_t request[6];
    const size_t length = msp_v1_encode_request(command, request);
    if (client->write == NULL || !client->write(request, length, client->ctx)) {
        return false;
    }
    client->active_query = command;
    client->sent_at_ms = now_ms;
    client->awaiting = true;
    ++client->requests;
    return true;
}

void msp_client_init(MspClient *client, MspWriteFn write,
                     MspFrameFn on_frame, void *ctx)
{
    memset(client, 0, sizeof *client);
    client->write = write;
    client->on_frame = on_frame;
    client->ctx = ctx;
    msp_codec_reset(&client->codec);
}

void msp_client_rx_byte(MspClient *client, uint8_t byte, uint32_t now_ms)
{
    MspFrame frame;
    const MspCodecFeedResult result = msp_codec_feed(&client->codec, byte, &frame);
    if (result == MSP_CODEC_CHECKSUM_ERROR) {
        ++client->checksum_errors;
        return;
    }
    if (result != MSP_CODEC_FRAME) {
        return;
    }

    client->last_valid_ms = now_ms;
    if (client->on_frame != NULL) {
        client->on_frame(&frame, now_ms, client->ctx);
    }
    if (client->awaiting && frame.command == client->active_query) {
        client->awaiting = false;
    }
}

void msp_client_tick(MspClient *client, uint32_t now_ms)
{
    if (client->awaiting) {
        if (elapsed_ms(now_ms, client->sent_at_ms) < REQUEST_TIMEOUT_MS) {
            return;
        }
        client->awaiting = false;
        ++client->timeouts;
    }

    if (client->startup_query < STARTUP_QUERY_COUNT) {
        const uint8_t command = startup_queries[client->startup_query];
        if (send_request(client, command, now_ms)) {
            ++client->startup_query;
            if (client->startup_query == STARTUP_QUERY_COUNT) {
                for (size_t i = 0; i < SCHEDULED_QUERY_COUNT; ++i) {
                    client->due_ms[i] = now_ms;
                }
            }
        }
        return;
    }

    size_t selected = SCHEDULED_QUERY_COUNT;
    uint32_t greatest_lateness = 0u;
    for (size_t i = 0; i < SCHEDULED_QUERY_COUNT; ++i) {
        if (!time_reached(now_ms, client->due_ms[i])) {
            continue;
        }
        const uint32_t lateness = elapsed_ms(now_ms, client->due_ms[i]);
        if (selected == SCHEDULED_QUERY_COUNT || lateness > greatest_lateness) {
            selected = i;
            greatest_lateness = lateness;
        }
    }
    if (selected == SCHEDULED_QUERY_COUNT) {
        return;
    }

    if (send_request(client, queries[selected].command, now_ms)) {
        client->due_ms[selected] = now_ms + queries[selected].period_ms;
    }
}
