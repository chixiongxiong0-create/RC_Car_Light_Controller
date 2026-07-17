#include "platform/ws2812_port.h"

#include <string.h>

static void encode_byte(uint8_t value, uint8_t out[4])
{
  uint32_t encoded = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit) {
        encoded <<= 4u;
        encoded |= (value & 0x80u) != 0u ? 0xEu : 0x8u;
        value <<= 1u;
    }
    out[0] = (uint8_t)(encoded >> 24u);
    out[1] = (uint8_t)(encoded >> 16u);
    out[2] = (uint8_t)(encoded >> 8u);
    out[3] = (uint8_t)encoded;
}

size_t ws2812_encode_grb(const LedRgb *pixels, size_t count,
                         uint8_t *out, size_t capacity)
{
    if (out == NULL || count > LED_MAX_PIXELS ||
        (count != 0u && pixels == NULL)) {
        return 0u;
    }
    const size_t length = count * WS2812_BYTES_PER_PIXEL + WS2812_RESET_BYTES;
    if (capacity < length) {
        return 0u;
    }
    size_t offset = 0u;
    for (size_t i = 0u; i < count; ++i) {
        encode_byte(pixels[i].g, &out[offset]);
        encode_byte(pixels[i].r, &out[offset + 4u]);
        encode_byte(pixels[i].b, &out[offset + 8u]);
        offset += WS2812_BYTES_PER_PIXEL;
    }
    memset(&out[offset], 0, WS2812_RESET_BYTES);
    return length;
}

bool ws2812_can_submit(uint32_t now_ms, uint32_t last_submit_ms, bool is_idle)
{
    return is_idle && (uint32_t)(now_ms - last_submit_ms) >= 34u;
}

void ws2812_transport_init(Ws2812Transport *transport,
                            const void *expected_handle,
                            const void *expected_instance)
{
    if (transport == NULL) {
        return;
    }
    transport->idle = true;
    transport->last_submit_ms = UINT32_MAX - 33u;
    transport->error_count = 0u;
    transport->expected_handle = expected_handle;
    transport->expected_instance = expected_instance;
}

bool ws2812_transport_submit(Ws2812Transport *transport, uint32_t now_ms,
                             const LedRgb *pixels, size_t count,
                             uint8_t *tx_buffer, size_t capacity,
                             Ws2812DmaStart start, void *ctx)
{
    if (transport == NULL || start == NULL ||
        !ws2812_can_submit(now_ms, transport->last_submit_ms, transport->idle)) {
        return false;
    }
    const size_t length = ws2812_encode_grb(pixels, count, tx_buffer, capacity);
    if (length == 0u) {
        return false;
    }
    transport->idle = false;
    if (!start(tx_buffer, length, ctx)) {
        transport->idle = true;
        return false;
    }
    transport->last_submit_ms = now_ms;
    return true;
}

static bool transport_matches(const Ws2812Transport *transport,
                              const void *handle, const void *instance)
{
    return transport != NULL && handle == transport->expected_handle &&
           instance == transport->expected_instance;
}

void ws2812_transport_complete(Ws2812Transport *transport,
                               const void *handle, const void *instance)
{
    if (transport_matches(transport, handle, instance)) {
        transport->idle = true;
    }
}

void ws2812_transport_error(Ws2812Transport *transport,
                            const void *handle, const void *instance)
{
    if (transport_matches(transport, handle, instance)) {
        transport->error_count++;
        transport->idle = true;
    }
}

void ws2812_transport_abort_complete(Ws2812Transport *transport,
                                     const void *handle, const void *instance)
{
    ws2812_transport_complete(transport, handle, instance);
}

uint32_t ws2812_transport_errors(const Ws2812Transport *transport)
{
    return transport != NULL ? transport->error_count : 0u;
}

#ifndef WS2812_HOST_TEST
#include "main.h"
#include "spi.h"

static uint8_t tx[WS2812_TX_BYTES] __attribute__((aligned(32)));
static Ws2812Transport transport;

static bool start_dma(const uint8_t *data, size_t length, void *ctx)
{
    (void)ctx;
    SCB_CleanDCache_by_Addr((uint32_t *)data, WS2812_TX_BYTES);
    return HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *)data, (uint16_t)length) == HAL_OK;
}

void ws2812_port_init(void)
{
    MX_SPI3_Init();
    ws2812_transport_init(&transport, &hspi3, SPI3);
}

bool ws2812_port_submit(uint32_t now_ms, const LedRgb *pixels, size_t count)
{
    return ws2812_transport_submit(&transport, now_ms, pixels, count,
                                    tx, sizeof tx, start_dma, NULL);
}

void ws2812_port_tx_complete(void *spi_handle)
{
    SPI_HandleTypeDef *hspi = spi_handle;
    ws2812_transport_complete(&transport, hspi, hspi != NULL ? hspi->Instance : NULL);
}

void ws2812_port_error(void *spi_handle)
{
    SPI_HandleTypeDef *hspi = spi_handle;
    ws2812_transport_error(&transport, hspi, hspi != NULL ? hspi->Instance : NULL);
}

void ws2812_port_abort_complete(void *spi_handle)
{
    SPI_HandleTypeDef *hspi = spi_handle;
    ws2812_transport_abort_complete(&transport, hspi,
                                     hspi != NULL ? hspi->Instance : NULL);
}
#endif
