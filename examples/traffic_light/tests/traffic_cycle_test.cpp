#include <cassert>
#include <cstdint>
#include <cstdio>

#include "traffic_cycle.hpp"

int main() {
    constexpr std::int64_t start_us = 123'456'789;
    TrafficCycle cycle{start_us};
    const auto check = [&](std::int64_t elapsed_us, TrafficState state,
                           unsigned seconds, bool red, bool yellow, bool green) {
        const auto result = cycle.update(start_us + elapsed_us);
        assert(result.state == state);
        assert(result.seconds_left == seconds);
        assert(result.signals.red == red);
        assert(result.signals.yellow == yellow);
        assert(result.signals.green == green);
    };
    check(0, TrafficState::Green, 9, false, false, true);
    check(999'999, TrafficState::Green, 9, false, false, true);
    check(1'000'000, TrafficState::Green, 8, false, false, true);
    check(8'999'999, TrafficState::Green, 1, false, false, true);
    check(9'000'000, TrafficState::BlinkingGreen, 3, false, false, true);
    check(9'499'999, TrafficState::BlinkingGreen, 3, false, false, true);
    check(9'500'000, TrafficState::BlinkingGreen, 3, false, false, false);
    check(10'000'000, TrafficState::BlinkingGreen, 2, false, false, true);
    check(10'500'000, TrafficState::BlinkingGreen, 2, false, false, false);
    check(11'000'000, TrafficState::BlinkingGreen, 1, false, false, true);
    check(11'999'999, TrafficState::BlinkingGreen, 1, false, false, false);
    check(12'000'000, TrafficState::Yellow, 3, false, true, false);
    check(14'999'999, TrafficState::Yellow, 1, false, true, false);
    check(15'000'000, TrafficState::Red, 9, true, false, false);
    check(23'999'999, TrafficState::Red, 1, true, false, false);
    check(24'000'000, TrafficState::RedYellow, 2, true, true, false);
    check(25'000'000, TrafficState::RedYellow, 1, true, true, false);
    check(26'000'000, TrafficState::Green, 9, false, false, true);
    // A delayed update skips elapsed phases without shifting the schedule.
    check(52'250'000, TrafficState::Green, 9, false, false, true);
    check(53'000'000, TrafficState::Green, 8, false, false, true);

    TrafficCycle sampled{0};
    for (std::int64_t time = 0; time < 78'000'000; time += 1'000) {
        const auto result = sampled.update(time);
        assert(result.seconds_left >= 1 && result.seconds_left <= 9);
        assert(!(result.signals.green &&
                 (result.signals.red || result.signals.yellow)));
    }
    std::puts("Traffic cycle tests passed");
}
