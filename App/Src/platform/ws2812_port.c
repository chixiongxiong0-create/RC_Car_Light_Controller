#include "platform/ws2812_port.h"

#include <string.h>

static size_t encode_component_pair(uint8_t first, uint8_t second,
                                    uint32_t duty_0, uint32_t duty_1,
                                    uint32_t *interleaved, size_t offset)
{
    for (uint8_t bit = 0x80u; bit != 0u; bit >>= 1u) {
        interleaved[offset++] = (first & bit) != 0u ? duty_1 : duty_0;
        interleaved[offset++] = (second & bit) != 0u ? duty_1 : duty_0;
    }
    return offset;
}

size_t ws2812_encode_pair(const LedRgb *first, const LedRgb *second,
                          size_t pixel_count, uint32_t duty_0,
                          uint32_t duty_1, size_t reset_slots,
                          uint32_t *interleaved, size_t capacity_words)
{
    if (first == NULL || second == NULL || interleaved == NULL ||
        pixel_count == 0u || reset_slots < WS2812_MIN_RESET_SLOTS ||
        duty_0 >= duty_1 || pixel_count > (SIZE_MAX - reset_slots) / 24u) {
        return 0u;
    }

    const size_t slots = pixel_count * 24u + reset_slots;
    if (slots > SIZE_MAX / 2u || capacity_words < slots * 2u) {
        return 0u;
    }

    size_t offset = 0u;
    for (size_t pixel = 0u; pixel < pixel_count; ++pixel) {
        offset = encode_component_pair(first[pixel].g, second[pixel].g,
                                       duty_0, duty_1, interleaved, offset);
        offset = encode_component_pair(first[pixel].r, second[pixel].r,
                                       duty_0, duty_1, interleaved, offset);
        offset = encode_component_pair(first[pixel].b, second[pixel].b,
                                       duty_0, duty_1, interleaved, offset);
    }
    memset(&interleaved[offset], 0, reset_slots * 2u * sizeof *interleaved);
    return slots;
}

bool ws2812_can_submit(uint32_t now_ms, uint32_t last_submit_ms, bool idle)
{
    return idle && (uint32_t)(now_ms - last_submit_ms) >=
                       WS2812_RATE_LIMIT_MS;
}

void ws2812_transport_init(Ws2812Transport *transport)
{
    if (transport == NULL) {
        return;
    }

    transport->state = WS2812_TRANSPORT_IDLE;
    transport->pair_complete[0] = false;
    transport->pair_complete[1] = false;
    transport->last_submit_ms = UINT32_MAX - (WS2812_RATE_LIMIT_MS - 1u);
    transport->error_count = 0u;
}

static bool transport_pairs_complete(const Ws2812Transport *transport)
{
    return transport->pair_complete[0] && transport->pair_complete[1];
}

static void transport_fail(Ws2812Transport *transport, Ws2812PairStopFn stop,
                           void *ctx, bool stop_pair_0, bool stop_pair_1)
{
    transport->pair_complete[0] = false;
    transport->pair_complete[1] = false;
    transport->state = WS2812_TRANSPORT_IDLE;
    transport->error_count++;

    if (stop_pair_0) {
        stop(0u, ctx);
    }
    if (stop_pair_1) {
        stop(1u, ctx);
    }
}

bool ws2812_transport_submit(Ws2812Transport *transport, uint32_t now_ms,
                             const Ws2812Frame *frame, uint32_t duty_0,
                             uint32_t duty_1, size_t reset_slots,
                             uint32_t *pair_0_words,
                             size_t pair_0_capacity_words,
                             uint32_t *pair_1_words,
                             size_t pair_1_capacity_words,
                             Ws2812PairStartFn start,
                             Ws2812PairStopFn stop, void *ctx)
{
    if (transport == NULL || frame == NULL || start == NULL || stop == NULL ||
        transport->state != WS2812_TRANSPORT_IDLE ||
        !ws2812_can_submit(now_ms, transport->last_submit_ms, true)) {
        return false;
    }

    const size_t pair_0_slots = ws2812_encode_pair(
        frame->groups[0], frame->groups[1], WS2812_GROUP_LENGTHS[0], duty_0,
        duty_1, reset_slots, pair_0_words, pair_0_capacity_words);
    const size_t pair_1_slots = ws2812_encode_pair(
        frame->groups[2], frame->groups[3], WS2812_GROUP_LENGTHS[2], duty_0,
        duty_1, reset_slots, pair_1_words, pair_1_capacity_words);
    if (pair_0_slots == 0u || pair_1_slots == 0u) {
        return false;
    }

    transport->pair_complete[0] = false;
    transport->pair_complete[1] = false;
    transport->state = WS2812_TRANSPORT_STARTING;
    const bool pair_0_started = start(0u, pair_0_words, pair_0_slots, ctx);
    if (transport->state != WS2812_TRANSPORT_STARTING) {
        return false;
    }
    if (!pair_0_started) {
        transport_fail(transport, stop, ctx, false, false);
        return false;
    }

    const bool pair_1_started = start(1u, pair_1_words, pair_1_slots, ctx);
    if (pair_1_started && transport->state == WS2812_TRANSPORT_IDLE &&
        transport_pairs_complete(transport)) {
        transport->last_submit_ms = now_ms;
        return true;
    }
    if (transport->state != WS2812_TRANSPORT_STARTING) {
        return false;
    }
    if (!pair_1_started) {
        transport_fail(transport, stop, ctx, true, false);
        return false;
    }

    transport->last_submit_ms = now_ms;
    transport->state = transport_pairs_complete(transport)
                           ? WS2812_TRANSPORT_IDLE
                           : WS2812_TRANSPORT_ACTIVE;
    return true;
}

void ws2812_transport_complete(Ws2812Transport *transport, unsigned pair)
{
    if (transport == NULL || transport->state == WS2812_TRANSPORT_IDLE ||
        pair >= WS2812_PAIR_COUNT) {
        return;
    }

    transport->pair_complete[pair] = true;
    if (transport_pairs_complete(transport)) {
        transport->state = WS2812_TRANSPORT_IDLE;
    }
}

void ws2812_transport_error(Ws2812Transport *transport, unsigned pair,
                            Ws2812PairStopFn stop, void *ctx)
{
    if (transport == NULL || transport->state == WS2812_TRANSPORT_IDLE ||
        pair >= WS2812_PAIR_COUNT || stop == NULL) {
        return;
    }

    transport_fail(transport, stop, ctx, true, true);
}

uint32_t ws2812_transport_errors(const Ws2812Transport *transport)
{
    return transport != NULL ? transport->error_count : 0u;
}
