#include <assert.h>
#include <string.h>

#include "msp/msp_client.h"

enum { MAX_WRITES = 256 };

typedef struct {
    uint8_t commands[MAX_WRITES];
    uint32_t times[MAX_WRITES];
    size_t count;
    uint32_t now_ms;
    size_t frames;
} ClientFixture;

static bool capture_write(const uint8_t *data, size_t length, void *ctx)
{
    ClientFixture *fixture = ctx;
    assert(length == 6u);
    assert(data[0] == '$' && data[1] == 'M' && data[2] == '<');
    assert(data[3] == 0u && data[5] == data[4]);
    assert(fixture->count < MAX_WRITES);
    fixture->commands[fixture->count] = data[4];
    fixture->times[fixture->count] = fixture->now_ms;
    ++fixture->count;
    return true;
}

static void capture_frame(const MspFrame *frame, uint32_t now_ms, void *ctx)
{
    ClientFixture *fixture = ctx;
    (void)frame;
    assert(now_ms == fixture->now_ms);
    ++fixture->frames;
}

static void reply_to_active(MspClient *client, ClientFixture *fixture)
{
    const uint8_t command = fixture->commands[fixture->count - 1u];
    const uint8_t response[] = {'$', 'M', '>', 0u, command, command};
    for (size_t i = 0; i < sizeof response; ++i) {
        msp_client_rx_byte(client, response[i], fixture->now_ms);
    }
}

static uint16_t period_for(uint8_t command)
{
    switch (command) {
    case 105u: return 40u;
    case 108u: return 50u;
    case 110u:
    case 106u: return 200u;
    case 101u: return 500u;
    default: return 0u;
    }
}

void test_msp_client(void)
{
    ClientFixture fixture = {0};
    MspClient client;
    msp_client_init(&client, capture_write, capture_frame, &fixture);

    fixture.now_ms = 0u;
    msp_client_tick(&client, fixture.now_ms);
    assert(fixture.count == 1u && fixture.commands[0] == 1u);
    msp_client_tick(&client, 39u);
    assert(fixture.count == 1u);
    msp_client_tick(&client, 40u);
    assert(client.timeouts == 1u);
    assert(fixture.count == 2u && fixture.commands[1] == 2u);
    reply_to_active(&client, &fixture);
    assert(!client.awaiting);

    fixture.now_ms = 41u;
    msp_client_tick(&client, fixture.now_ms);
    assert(fixture.commands[2] == 3u);
    reply_to_active(&client, &fixture);

    uint32_t last_sent[256] = {0};
    bool seen[256] = {false};
    for (fixture.now_ms = 42u; fixture.now_ms <= 2000u; ++fixture.now_ms) {
        const size_t before = fixture.count;
        msp_client_tick(&client, fixture.now_ms);
        if (fixture.count != before) {
            const uint8_t command = fixture.commands[fixture.count - 1u];
            const uint16_t period = period_for(command);
            assert(period != 0u);
            if (seen[command]) {
                assert(fixture.now_ms - last_sent[command] >= period);
            }
            seen[command] = true;
            last_sent[command] = fixture.now_ms;
            reply_to_active(&client, &fixture);
        }
    }
    assert(seen[105u] && seen[108u] && seen[110u] && seen[106u] && seen[101u]);
    assert(client.requests == fixture.count);
    assert(fixture.frames == fixture.count - 1u);
}
