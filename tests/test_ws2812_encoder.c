#include <assert.h>

#include "led/ws2812_frame.h"
#include "platform/ws2812_port.h"

enum {
    RESET_SLOTS = 64u,
    DUTY_0 = 96u,
    DUTY_1 = 193u,
    PAIR_0_PIXELS = 4u,
    PAIR_1_PIXELS = 8u,
    PAIR_0_WORDS = 2u * (PAIR_0_PIXELS * 24u + RESET_SLOTS),
    PAIR_1_WORDS = 2u * (PAIR_1_PIXELS * 24u + RESET_SLOTS)
};

typedef struct {
    unsigned start_calls[2];
    unsigned stop_calls[2];
    bool start_succeeds[2];
    bool error_during_pair_0_start;
    bool become_busy_on_critical_enter;
    const uint32_t *started_words[2];
    size_t started_slots[2];
    Ws2812Transport *transport;
    unsigned critical_enter_calls;
    unsigned critical_exit_calls;
} FakePairs;

static void fake_stop(unsigned pair, void *ctx)
{
    FakePairs *fake = ctx;
    assert(pair < 2u);
    fake->stop_calls[pair]++;
}

static uintptr_t fake_critical_enter(void *ctx)
{
    FakePairs *fake = ctx;
    fake->critical_enter_calls++;
    if (fake->become_busy_on_critical_enter) {
        fake->transport->state = WS2812_TRANSPORT_ACTIVE;
    }
    return 0xA55Au;
}

static void fake_critical_exit(uintptr_t saved_state, void *ctx)
{
    FakePairs *fake = ctx;
    assert(saved_state == 0xA55Au);
    fake->critical_exit_calls++;
}

static bool fake_start(unsigned pair, const uint32_t *words, size_t slots,
                       void *ctx)
{
    FakePairs *fake = ctx;
    assert(pair < 2u);
    assert(words != NULL);
    assert(slots >= RESET_SLOTS);
    fake->start_calls[pair]++;
    fake->started_words[pair] = words;
    fake->started_slots[pair] = slots;
    if (pair == 0u && fake->error_during_pair_0_start) {
        ws2812_transport_error(fake->transport, pair, fake_stop, fake);
    }
    return fake->start_succeeds[pair];
}

void test_ws2812_encoder(void)
{
    uint32_t calculated_duty_0 = 0u;
    uint32_t calculated_duty_1 = 0u;
    assert(ws2812_compare_ticks(275000000u, 344u, &calculated_duty_0,
                                &calculated_duty_1));
    assert(calculated_duty_0 == 96u);
    assert(calculated_duty_1 == 193u);
    assert(!ws2812_compare_ticks(0u, 344u, &calculated_duty_0,
                                 &calculated_duty_1));
    assert(!ws2812_compare_ticks(275000000u, 193u, &calculated_duty_0,
                                 &calculated_duty_1));
    assert(!ws2812_compare_ticks(275000000u, 344u, NULL,
                                 &calculated_duty_1));

    uint32_t out[2u * (PAIR_1_PIXELS * 24u + RESET_SLOTS) + 2u];
    for (size_t i = 0u; i < sizeof out / sizeof out[0]; ++i) {
        out[i] = 0xA55Au;
    }

    LedRgb first[PAIR_0_PIXELS] = {{.g = 0x80u, .r = 0x40u, .b = 0x01u}};
    LedRgb second[PAIR_0_PIXELS] = {{.g = 0x00u, .r = 0x80u, .b = 0x01u}};
    const size_t slots = ws2812_encode_pair(first, second, PAIR_0_PIXELS,
                                            DUTY_0, DUTY_1, RESET_SLOTS, out,
                                            PAIR_0_WORDS);
    assert(slots == PAIR_0_PIXELS * 24u + RESET_SLOTS);
    /* First bit is green bit 7; output is first channel then second channel. */
    assert(out[0] == DUTY_1);
    assert(out[1] == DUTY_0);
    /* Green bit 6, red bit 7, and blue bit 7 verify GRB/MSB order. */
    assert(out[2] == DUTY_0);
    assert(out[3] == DUTY_0);
    assert(out[16] == DUTY_0);
    assert(out[17] == DUTY_1);
    assert(out[32] == DUTY_0);
    assert(out[33] == DUTY_0);
    for (size_t word = 2u * PAIR_0_PIXELS * 24u;
         word < 2u * slots; ++word) {
        assert(out[word] == 0u);
    }
    assert(out[2u * slots] == 0xA55Au);
    assert(out[2u * slots + 1u] == 0xA55Au);

    assert(ws2812_encode_pair(NULL, second, PAIR_0_PIXELS, DUTY_0, DUTY_1,
                              RESET_SLOTS, out, PAIR_0_WORDS) == 0u);
    assert(ws2812_encode_pair(first, NULL, PAIR_0_PIXELS, DUTY_0, DUTY_1,
                              RESET_SLOTS, out, PAIR_0_WORDS) == 0u);
    assert(ws2812_encode_pair(first, second, 0u, DUTY_0, DUTY_1, RESET_SLOTS,
                              out, PAIR_0_WORDS) == 0u);
    assert(ws2812_encode_pair(first, second, PAIR_0_PIXELS, DUTY_0, DUTY_1,
                              RESET_SLOTS - 1u, out, PAIR_0_WORDS) == 0u);
    assert(ws2812_encode_pair(first, second, PAIR_0_PIXELS, DUTY_1, DUTY_1,
                              RESET_SLOTS, out, PAIR_0_WORDS) == 0u);
    assert(ws2812_encode_pair(first, second, PAIR_0_PIXELS, DUTY_1, DUTY_0,
                              RESET_SLOTS, out, PAIR_0_WORDS) == 0u);
    assert(ws2812_encode_pair(first, second, PAIR_0_PIXELS, DUTY_0, DUTY_1,
                              RESET_SLOTS, out, PAIR_0_WORDS - 1u) == 0u);

    Ws2812Frame frame;
    ws2812_frame_clear(&frame);
    frame.groups[0][0].g = 0x80u;
    frame.groups[1][0].r = 0x80u;
    frame.groups[2][0].b = 0x80u;
    frame.groups[3][0].g = 0x80u;

    uint32_t pair_0_words[PAIR_0_WORDS];
    uint32_t pair_1_words[PAIR_1_WORDS];
    Ws2812Transport transport;
    FakePairs fake = {.start_succeeds = {true, true}};
    ws2812_transport_init(&transport);
    assert(ws2812_transport_busy_drops(&transport) == 0u);

    assert(!ws2812_transport_submit(&transport, 0u, &frame, DUTY_0, DUTY_1,
                                    RESET_SLOTS, pair_0_words,
                                    PAIR_0_WORDS - 1u, pair_1_words,
                                    PAIR_1_WORDS, fake_start, fake_stop,
                                    fake_critical_enter, fake_critical_exit,
                                    &fake));
    assert(transport.state == WS2812_TRANSPORT_IDLE);
    assert(fake.start_calls[0] == 0u);
    assert(fake.start_calls[1] == 0u);
    assert(fake.critical_enter_calls == 0u);
    assert(fake.critical_exit_calls == 0u);
    assert(ws2812_transport_busy_drops(&transport) == 0u);
    assert(ws2812_transport_errors(&transport) == 0u);

    assert(ws2812_transport_submit(&transport, 0u, &frame, DUTY_0, DUTY_1,
                                   RESET_SLOTS, pair_0_words, PAIR_0_WORDS,
                                   pair_1_words, PAIR_1_WORDS, fake_start,
                                   fake_stop, fake_critical_enter,
                                   fake_critical_exit, &fake));
    assert(fake.start_calls[0] == 1u);
    assert(fake.start_calls[1] == 1u);
    assert(fake.started_words[0] == pair_0_words);
    assert(fake.started_words[1] == pair_1_words);
    assert(fake.started_slots[0] == PAIR_0_PIXELS * 24u + RESET_SLOTS);
    assert(fake.started_slots[1] == PAIR_1_PIXELS * 24u + RESET_SLOTS);
    assert(pair_1_words[0] == DUTY_0);
    assert(pair_1_words[1] == DUTY_1);
    assert(pair_1_words[32] == DUTY_1);
    assert(pair_1_words[33] == DUTY_0);
    for (size_t word = 2u * PAIR_0_PIXELS * 24u;
         word < PAIR_0_WORDS; ++word) {
        assert(pair_0_words[word] == 0u);
    }
    for (size_t word = 2u * PAIR_1_PIXELS * 24u;
         word < PAIR_1_WORDS; ++word) {
        assert(pair_1_words[word] == 0u);
    }
    assert(transport.state == WS2812_TRANSPORT_ACTIVE);
    assert(fake.critical_enter_calls == 1u);
    assert(fake.critical_exit_calls == 1u);
    assert(!ws2812_transport_submit(&transport, 34u, &frame, DUTY_0, DUTY_1,
                                    RESET_SLOTS, pair_0_words,
                                    PAIR_0_WORDS - 1u, pair_1_words,
                                    PAIR_1_WORDS, fake_start, fake_stop,
                                    fake_critical_enter, fake_critical_exit,
                                    &fake));
    assert(ws2812_transport_busy_drops(&transport) == 0u);
    assert(!ws2812_transport_submit(&transport, 34u, &frame, DUTY_0, DUTY_1,
                                    RESET_SLOTS, pair_0_words, PAIR_0_WORDS,
                                    pair_1_words, PAIR_1_WORDS, fake_start,
                                    fake_stop, fake_critical_enter,
                                    fake_critical_exit, &fake));
    assert(ws2812_transport_busy_drops(&transport) == 1u);
    assert(ws2812_transport_errors(&transport) == 0u);

    ws2812_transport_complete(&transport, 1u);
    assert(transport.state == WS2812_TRANSPORT_ACTIVE);
    assert(!transport.pair_complete[0]);
    assert(transport.pair_complete[1]);
    ws2812_transport_complete(&transport, 0u);
    assert(transport.state == WS2812_TRANSPORT_IDLE);
    assert(!ws2812_transport_submit(&transport, 33u, &frame, DUTY_0, DUTY_1,
                                    RESET_SLOTS, pair_0_words, PAIR_0_WORDS,
                                    pair_1_words, PAIR_1_WORDS, fake_start,
                                    fake_stop, fake_critical_enter,
                                    fake_critical_exit, &fake));
    assert(ws2812_transport_busy_drops(&transport) == 1u);
    assert(ws2812_transport_errors(&transport) == 0u);
    assert(ws2812_transport_submit(&transport, 34u, &frame, DUTY_0, DUTY_1,
                                   RESET_SLOTS, pair_0_words, PAIR_0_WORDS,
                                   pair_1_words, PAIR_1_WORDS, fake_start,
                                   fake_stop, fake_critical_enter,
                                   fake_critical_exit, &fake));
    ws2812_transport_complete(&transport, 0u);
    ws2812_transport_complete(&transport, 1u);

    fake.start_succeeds[1] = false;
    assert(!ws2812_transport_submit(&transport, 68u, &frame, DUTY_0, DUTY_1,
                                    RESET_SLOTS, pair_0_words, PAIR_0_WORDS,
                                    pair_1_words, PAIR_1_WORDS, fake_start,
                                    fake_stop, fake_critical_enter,
                                    fake_critical_exit, &fake));
    assert(fake.stop_calls[0] == 1u);
    assert(fake.stop_calls[1] == 0u);
    assert(transport.state == WS2812_TRANSPORT_IDLE);
    assert(ws2812_transport_errors(&transport) == 1u);
    assert(ws2812_transport_busy_drops(&transport) == 1u);
    assert(fake.critical_enter_calls == 3u);
    assert(fake.critical_exit_calls == 3u);

    fake.start_succeeds[1] = true;
    assert(ws2812_transport_submit(&transport, 68u, &frame, DUTY_0, DUTY_1,
                                   RESET_SLOTS, pair_0_words, PAIR_0_WORDS,
                                   pair_1_words, PAIR_1_WORDS, fake_start,
                                   fake_stop, fake_critical_enter,
                                   fake_critical_exit, &fake));
    ws2812_transport_error(&transport, 1u, fake_stop, &fake);
    assert(fake.stop_calls[0] == 2u);
    assert(fake.stop_calls[1] == 1u);
    assert(transport.state == WS2812_TRANSPORT_IDLE);
    assert(!transport.pair_complete[0]);
    assert(!transport.pair_complete[1]);
    assert(ws2812_transport_errors(&transport) == 2u);
    ws2812_transport_error(&transport, 0u, fake_stop, &fake);
    assert(ws2812_transport_errors(&transport) == 2u);

    transport.last_submit_ms = UINT32_MAX - 20u;
    assert(ws2812_transport_submit(&transport, 20u, &frame, DUTY_0, DUTY_1,
                                   RESET_SLOTS, pair_0_words, PAIR_0_WORDS,
                                   pair_1_words, PAIR_1_WORDS, fake_start,
                                   fake_stop, fake_critical_enter,
                                   fake_critical_exit, &fake));

    Ws2812Transport reentrant_transport;
    FakePairs reentrant_fake = {
        .start_succeeds = {true, true},
        .error_during_pair_0_start = true,
        .transport = &reentrant_transport
    };
    ws2812_transport_init(&reentrant_transport);
    assert(!ws2812_transport_submit(&reentrant_transport, 0u, &frame, DUTY_0,
                                    DUTY_1, RESET_SLOTS, pair_0_words,
                                    PAIR_0_WORDS, pair_1_words,
                                    PAIR_1_WORDS, fake_start, fake_stop,
                                    fake_critical_enter, fake_critical_exit,
                                    &reentrant_fake));
    assert(reentrant_fake.start_calls[0] == 1u);
    assert(reentrant_fake.start_calls[1] == 0u);
    assert(reentrant_fake.stop_calls[0] == 1u);
    assert(reentrant_fake.stop_calls[1] == 1u);
    assert(reentrant_transport.state == WS2812_TRANSPORT_IDLE);
    assert(ws2812_transport_errors(&reentrant_transport) == 1u);
    assert(ws2812_transport_busy_drops(&reentrant_transport) == 0u);
    assert(reentrant_fake.critical_enter_calls == 1u);
    assert(reentrant_fake.critical_exit_calls == 1u);

    Ws2812Transport critical_transport;
    FakePairs critical_fake = {.start_succeeds = {false, true}};
    ws2812_transport_init(&critical_transport);
    assert(!ws2812_transport_submit(&critical_transport, 0u, &frame, DUTY_0,
                                    DUTY_1, RESET_SLOTS, pair_0_words,
                                    PAIR_0_WORDS, pair_1_words,
                                    PAIR_1_WORDS, fake_start, fake_stop,
                                    fake_critical_enter, fake_critical_exit,
                                    &critical_fake));
    assert(critical_fake.start_calls[0] == 1u);
    assert(critical_fake.start_calls[1] == 0u);
    assert(critical_fake.critical_enter_calls == 1u);
    assert(critical_fake.critical_exit_calls == 1u);

    critical_fake.start_succeeds[0] = true;
    critical_fake.start_succeeds[1] = false;
    assert(!ws2812_transport_submit(&critical_transport, 0u, &frame, DUTY_0,
                                    DUTY_1, RESET_SLOTS, pair_0_words,
                                    PAIR_0_WORDS, pair_1_words,
                                    PAIR_1_WORDS, fake_start, fake_stop,
                                    fake_critical_enter, fake_critical_exit,
                                    &critical_fake));
    assert(critical_fake.critical_enter_calls == 2u);
    assert(critical_fake.critical_exit_calls == 2u);

    critical_fake.start_succeeds[1] = true;
    assert(ws2812_transport_submit(&critical_transport, 0u, &frame, DUTY_0,
                                   DUTY_1, RESET_SLOTS, pair_0_words,
                                   PAIR_0_WORDS, pair_1_words,
                                   PAIR_1_WORDS, fake_start, fake_stop,
                                   fake_critical_enter, fake_critical_exit,
                                   &critical_fake));
    assert(critical_fake.critical_enter_calls == 3u);
    assert(critical_fake.critical_exit_calls == 3u);

    Ws2812Transport raced_transport;
    FakePairs raced_fake = {
        .start_succeeds = {true, true},
        .become_busy_on_critical_enter = true,
        .transport = &raced_transport
    };
    ws2812_transport_init(&raced_transport);
    assert(!ws2812_transport_submit(&raced_transport, 0u, &frame, DUTY_0,
                                    DUTY_1, RESET_SLOTS, pair_0_words,
                                    PAIR_0_WORDS, pair_1_words,
                                    PAIR_1_WORDS, fake_start, fake_stop,
                                    fake_critical_enter, fake_critical_exit,
                                    &raced_fake));
    assert(raced_fake.start_calls[0] == 0u);
    assert(raced_fake.start_calls[1] == 0u);
    assert(raced_fake.critical_enter_calls == 1u);
    assert(raced_fake.critical_exit_calls == 1u);
    assert(ws2812_transport_busy_drops(&raced_transport) == 1u);
    assert(ws2812_transport_errors(&raced_transport) == 0u);
}
