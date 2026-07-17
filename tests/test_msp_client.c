#include <assert.h>
#include <string.h>

#include "msp/msp_client.h"

enum { MAX_WRITES = 256, COMMAND_COUNT = 256 };

typedef struct {
    uint8_t commands[MAX_WRITES];
    uint32_t times[MAX_WRITES];
    size_t count;
    uint32_t now_ms;
    size_t frames;
} ClientFixture;

typedef struct {
    uint32_t times[MAX_WRITES];
    size_t count;
} CommandHistory;

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

static void tick_at(MspClient *client, ClientFixture *fixture, uint32_t now_ms)
{
    fixture->now_ms = now_ms;
    msp_client_tick(client, now_ms);
}

static void feed_empty_response(MspClient *client, ClientFixture *fixture,
                                uint8_t command, uint8_t checksum)
{
    const uint8_t response[] = {'$', 'M', '>', 0u, command, checksum};
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

    tick_at(&client, &fixture, 0u);
    assert(fixture.count == 1u && fixture.commands[0] == 1u);
    feed_empty_response(&client, &fixture, 1u, 0u);
    assert(client.checksum_errors == 1u);
    assert(client.awaiting);
    assert(fixture.frames == 0u);
    feed_empty_response(&client, &fixture, 1u, 1u);
    assert(!client.awaiting);
    assert(fixture.frames == 1u);

    tick_at(&client, &fixture, 1u);
    assert(fixture.count == 2u && fixture.commands[1] == 2u);
    tick_at(&client, &fixture, 40u);
    assert(fixture.count == 2u);
    tick_at(&client, &fixture, 41u);
    assert(client.timeouts == 1u);
    assert(fixture.count == 3u && fixture.commands[2] == 3u);
    reply_to_active(&client, &fixture);
    assert(!client.awaiting);

    CommandHistory histories[COMMAND_COUNT] = {0};
    const uint32_t schedule_start_ms = 42u;
    const uint32_t schedule_end_ms = 2000u;
    for (uint32_t now_ms = schedule_start_ms; now_ms <= schedule_end_ms; ++now_ms) {
        const size_t before = fixture.count;
        tick_at(&client, &fixture, now_ms);
        if (fixture.count != before) {
            const uint8_t command = fixture.commands[fixture.count - 1u];
            const uint16_t period = period_for(command);
            CommandHistory *history = &histories[command];
            assert(period != 0u);
            assert(history->count < MAX_WRITES);
            if (history->count > 0u) {
                const uint32_t interval = now_ms - history->times[history->count - 1u];
                assert(interval >= period);
                assert(interval <= 2u * period);
            }
            history->times[history->count++] = now_ms;
            reply_to_active(&client, &fixture);
        }
    }

    const uint8_t scheduled_commands[] = {105u, 108u, 110u, 106u, 101u};
    const uint32_t duration_ms = schedule_end_ms - schedule_start_ms + 1u;
    for (size_t i = 0; i < sizeof scheduled_commands; ++i) {
        const uint8_t command = scheduled_commands[i];
        const uint32_t period = period_for(command);
        const uint32_t nominal = duration_ms / period;
        assert(histories[command].count >= nominal - 1u);
        assert(histories[command].count <= nominal + 1u);
    }
    assert(client.requests == fixture.count);
    assert(fixture.frames == fixture.count - 1u);

    ClientFixture overdue_fixture = {0};
    MspClient overdue_client;
    msp_client_init(&overdue_client, capture_write, capture_frame, &overdue_fixture);
    for (uint32_t now_ms = 0u; now_ms < 3u; ++now_ms) {
        tick_at(&overdue_client, &overdue_fixture, now_ms);
        reply_to_active(&overdue_client, &overdue_fixture);
    }
    enum { RC_QUERY = 0, ATTITUDE_QUERY = 1 };
    for (size_t i = 0; i < 5u; ++i) {
        overdue_client.due_ms[i] = 1000u;
    }
    overdue_client.due_ms[RC_QUERY] = 100u;
    overdue_client.due_ms[ATTITUDE_QUERY] = 90u;
    tick_at(&overdue_client, &overdue_fixture, 120u);
    assert(overdue_fixture.commands[overdue_fixture.count - 1u] == 108u);
}
