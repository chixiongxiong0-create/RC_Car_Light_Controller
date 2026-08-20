#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "led/ws2812_frame.h"

enum {
    WS2812_PAIR_COUNT = 2u,
    WS2812_MIN_RESET_SLOTS = 64u,
    WS2812_RATE_LIMIT_MS = 34u
};

size_t ws2812_encode_pair(const LedRgb *first, const LedRgb *second,
                          size_t pixel_count, uint32_t duty_0,
                          uint32_t duty_1, size_t reset_slots,
                          uint32_t *interleaved, size_t capacity_words);

bool ws2812_can_submit(uint32_t now_ms, uint32_t last_submit_ms, bool idle);

typedef bool (*Ws2812PairStartFn)(unsigned pair, const uint32_t *words,
                                  size_t slots, void *ctx);
typedef void (*Ws2812PairStopFn)(unsigned pair, void *ctx);

typedef struct {
    bool idle;
    uint8_t complete_mask;
    uint32_t last_submit_ms;
    uint32_t error_count;
} Ws2812Transport;

void ws2812_transport_init(Ws2812Transport *transport);
bool ws2812_transport_submit(Ws2812Transport *transport, uint32_t now_ms,
                             const Ws2812Frame *frame, uint32_t duty_0,
                             uint32_t duty_1, size_t reset_slots,
                             uint32_t *pair_0_words,
                             size_t pair_0_capacity_words,
                             uint32_t *pair_1_words,
                             size_t pair_1_capacity_words,
                             Ws2812PairStartFn start,
                             Ws2812PairStopFn stop, void *ctx);
void ws2812_transport_complete(Ws2812Transport *transport, unsigned pair);
void ws2812_transport_error(Ws2812Transport *transport, unsigned pair,
                            Ws2812PairStopFn stop, void *ctx);
uint32_t ws2812_transport_errors(const Ws2812Transport *transport);
