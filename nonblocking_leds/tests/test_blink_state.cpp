#include <array>
#include <cassert>
#include <cstdio>

#include "blink_state.hpp"

int main()
{
    // Ten seconds of the actual shared-loop cadence: verify every transition,
    // including simultaneous deadlines, and the resulting counts.
    std::array<BlinkState, 3> leds{{{200}, {500}, {1000}}};
    std::array<int, 3> changes{};
    for (int ms = 0; ms <= 10'000; ms += 10) {
        for (unsigned i = 0; i < leds.size(); ++i) {
            const bool changed = leds[i].update(ms);
            assert(changed == (ms > 0 && ms % leds[i].interval_ms == 0));
            assert(leds[i].on == ((ms / leds[i].interval_ms) % 2 != 0));
            changes[i] += changed;
        }
    }
    assert((changes == std::array<int, 3>{50, 20, 10}));

    BlinkState jitter{200, 1234};
    assert(!jitter.update(1433));
    assert(jitter.update(1441));
    assert(jitter.last_ms == 1434);
    assert(!jitter.update(1633));
    assert(jitter.update(1634)); // late execution did not move the next deadline

    BlinkState late{200};
    assert(!late.update(450)); // two missed toggles leave the output OFF
    assert(late.last_ms == 400 && !late.on);
    assert(late.update(1050)); // three more intervals leave the output ON
    assert(late.last_ms == 1000 && late.on);
    assert(!late.update(1050)); // repeated timestamp cannot toggle twice

    constexpr std::int64_t after_32bit_ms = 4'294'967'200LL;
    BlinkState long_uptime{200, after_32bit_ms};
    assert(!long_uptime.update(after_32bit_ms + 199));
    assert(long_uptime.update(after_32bit_ms + 200));

    std::puts("PASS: independent 200/500/1000 ms schedules, boundaries, jitter, missed intervals, long uptime");
}
