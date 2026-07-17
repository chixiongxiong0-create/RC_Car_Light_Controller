#include <assert.h>
#include <string.h>

#include "platform/ws2812_port.h"

typedef struct {
    unsigned calls;
    bool succeed;
} FakeDma;

static bool fake_start(const uint8_t *data, size_t length, void *ctx)
{
    FakeDma *fake = ctx;
    assert(data != NULL);
    assert(length >= WS2812_RESET_BYTES);
    fake->calls++;
    return fake->succeed;
}

void test_ws2812_encoder(void)
{
    const LedRgb pixel = {0x80u, 0x01u, 0xFFu};
    uint8_t buffer[WS2812_TX_BYTES + 2u];
    memset(buffer, 0xA5, sizeof buffer);

    const size_t length = ws2812_encode_grb(&pixel, 1u, buffer + 1u,
                                            WS2812_TX_BYTES);
    assert(length == WS2812_BYTES_PER_PIXEL + WS2812_RESET_BYTES);
    /* GRB: 0x01, 0x80, 0xFF; 0 -> 1000, 1 -> 1110. */
    const uint8_t expected[] = {
        0x88u, 0x88u, 0x88u, 0x8Eu,
        0xE8u, 0x88u, 0x88u, 0x88u,
        0xEEu, 0xEEu, 0xEEu, 0xEEu
    };
    assert(memcmp(buffer + 1u, expected, sizeof expected) == 0);
    for (size_t i = sizeof expected; i < length; ++i) {
        assert(buffer[1u + i] == 0u);
    }
    assert(buffer[0] == 0xA5u);
    assert(buffer[WS2812_TX_BYTES + 1u] == 0xA5u);

    assert(ws2812_encode_grb(&pixel, LED_MAX_PIXELS + 1u, buffer,
                             WS2812_TX_BYTES) == 0u);
    assert(ws2812_encode_grb(&pixel, 1u, buffer, 8u) == 0u);
    assert(ws2812_encode_grb(NULL, 0u, buffer, WS2812_TX_BYTES) ==
           WS2812_RESET_BYTES);
    assert(WS2812_ENCODED_BYTES == 360u);
    assert(WS2812_TX_BYTES == 384u);

    LedRgb strip[LED_MAX_PIXELS] = {0};
    memset(buffer, 0xA5, sizeof buffer);
    assert(ws2812_encode_grb(strip, 10u, buffer + 1u,
                             WS2812_TX_BYTES) == 144u);
    assert(buffer[0] == 0xA5u);
    assert(buffer[WS2812_TX_BYTES + 1u] == 0xA5u);
    assert(ws2812_encode_grb(strip, LED_MAX_PIXELS, buffer + 1u,
                             WS2812_TX_BYTES) == WS2812_TX_BYTES);
    assert(buffer[0] == 0xA5u);
    assert(buffer[WS2812_TX_BYTES + 1u] == 0xA5u);

    assert(!ws2812_can_submit(100u, 0u, false));
    assert(!ws2812_can_submit(133u, 100u, true));
    assert(ws2812_can_submit(134u, 100u, true));
    assert(ws2812_can_submit(20u, UINT32_MAX - 20u, true));

    int spi3_handle;
    int spi3_instance;
    int other_handle;
    int other_instance;
    Ws2812Transport transport;
    FakeDma dma = {0u, true};
    uint8_t tx[WS2812_TX_BYTES];
    ws2812_transport_init(&transport, &spi3_handle, &spi3_instance);
    assert(ws2812_transport_submit(&transport, 0u, strip, 10u, tx, sizeof tx,
                                   fake_start, &dma));
    assert(!ws2812_transport_submit(&transport, 34u, strip, 10u, tx, sizeof tx,
                                    fake_start, &dma));
    ws2812_transport_error(&transport, &other_handle, &other_instance);
    assert(!ws2812_transport_submit(&transport, 34u, strip, 10u, tx, sizeof tx,
                                    fake_start, &dma));
    ws2812_transport_error(&transport, &spi3_handle, &spi3_instance);
    assert(ws2812_transport_errors(&transport) == 1u);
    assert(ws2812_transport_submit(&transport, 34u, strip, 10u, tx, sizeof tx,
                                   fake_start, &dma));
    ws2812_transport_abort_complete(&transport, &spi3_handle, &spi3_instance);
    assert(ws2812_transport_submit(&transport, 68u, strip, 10u, tx, sizeof tx,
                                   fake_start, &dma));

    ws2812_transport_init(&transport, &spi3_handle, &spi3_instance);
    dma.succeed = false;
    assert(!ws2812_transport_submit(&transport, 0u, strip, 10u, tx, sizeof tx,
                                    fake_start, &dma));
    dma.succeed = true;
    assert(ws2812_transport_submit(&transport, 0u, strip, 10u, tx, sizeof tx,
                                   fake_start, &dma));
}
