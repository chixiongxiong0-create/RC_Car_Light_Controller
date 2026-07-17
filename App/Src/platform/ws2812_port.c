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

#ifndef WS2812_HOST_TEST
#include "main.h"
#include "spi.h"

static uint8_t tx[WS2812_TX_BYTES] __attribute__((aligned(32)));
static volatile bool idle = true;
static uint32_t last_submit_ms;

void ws2812_port_init(void)
{
    MX_SPI3_Init();
    idle = true;
    last_submit_ms = UINT32_MAX - 33u;
}

bool ws2812_port_submit(uint32_t now_ms, const LedRgb *pixels, size_t count)
{
    if (!ws2812_can_submit(now_ms, last_submit_ms, idle)) {
        return false;
    }
    const size_t length = ws2812_encode_grb(pixels, count, tx, sizeof tx);
    if (length == 0u) {
        return false;
    }
    /* tx resides in cacheable D1 SRAM; clean complete cache lines before DMA. */
    SCB_CleanDCache_by_Addr((uint32_t *)tx, sizeof tx);
    idle = false;
    if (HAL_SPI_Transmit_DMA(&hspi3, tx, (uint16_t)length) != HAL_OK) {
        idle = true;
        return false;
    }
    last_submit_ms = now_ms;
    return true;
}

void ws2812_port_tx_complete(void *spi_handle)
{
    if (spi_handle == &hspi3) {
        idle = true;
    }
}
#endif
