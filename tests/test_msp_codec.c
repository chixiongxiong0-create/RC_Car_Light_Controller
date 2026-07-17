#include <assert.h>
#include <string.h>

#include "msp/msp_codec.h"

static MspCodecFeedResult feed_bytes(MspCodec *codec, const uint8_t *bytes,
                                     size_t length, MspFrame *frame)
{
    MspCodecFeedResult result = MSP_CODEC_INCOMPLETE;
    for (size_t i = 0; i < length; ++i) {
        const MspCodecFeedResult current = msp_codec_feed(codec, bytes[i], frame);
        if (current != MSP_CODEC_INCOMPLETE) {
            result = current;
        }
    }
    return result;
}

void test_msp_codec(void)
{
    uint8_t request[6];
    assert(msp_v1_encode_request(105u, request) == 6u);
    assert(memcmp(request,
                  (uint8_t[]){'$', 'M', '<', 0u, 105u, 105u},
                  sizeof request) == 0);

    MspCodec codec = {0};
    MspFrame frame = {0};
    const uint8_t response[] = {
        '$', 'M', '>', 2u, 108u, 0x10u, 0x20u,
        (uint8_t)(2u ^ 108u ^ 0x10u ^ 0x20u)
    };
    for (size_t i = 0; i < sizeof response - 1u; ++i) {
        assert(msp_codec_feed(&codec, response[i], &frame) == MSP_CODEC_INCOMPLETE);
    }
    assert(msp_codec_feed(&codec, response[sizeof response - 1u], &frame) ==
           MSP_CODEC_FRAME);
    assert(frame.command == 108u);
    assert(frame.length == 2u);
    assert(frame.payload[0] == 0x10u);
    assert(frame.payload[1] == 0x20u);

    const uint8_t bad_checksum[] = {'$', 'M', '>', 0u, 101u, 0u};
    assert(feed_bytes(&codec, bad_checksum, sizeof bad_checksum, &frame) ==
           MSP_CODEC_CHECKSUM_ERROR);

    const uint8_t oversize[] = {'$', 'M', '>', MSP_MAX_PAYLOAD + 1u};
    assert(feed_bytes(&codec, oversize, sizeof oversize, &frame) ==
           MSP_CODEC_INCOMPLETE);

    const uint8_t noisy_valid[] = {
        0x00u, 'x', '$', 'x', '$', 'M', '<', '$',
        '$', 'M', '>', 0u, 101u, 101u
    };
    assert(feed_bytes(&codec, noisy_valid, sizeof noisy_valid, &frame) ==
           MSP_CODEC_FRAME);
    assert(frame.command == 101u);
    assert(frame.length == 0u);

    const uint8_t bad_then_valid[] = {
        '$', 'M', '>', 0u, 105u, 0u,
        '$', 'M', '>', 0u, 108u, 108u
    };
    assert(feed_bytes(&codec, bad_then_valid, sizeof bad_then_valid, &frame) ==
           MSP_CODEC_FRAME);
    assert(frame.command == 108u);
}
