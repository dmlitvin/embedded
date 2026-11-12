#include <cstdio>
#include <cstdlib>
#include "debounce.hpp"

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "Failed line %d: %s\n", __LINE__, #condition); \
    std::abort(); } } while (false)

void time_filter() {
    button::TimeFilter filter;
    CHECK(filter.edge(0)); // First edge must work even at boot.
    CHECK(!filter.edge(49'999));
    CHECK(!filter.edge(50'000)); // Relative to previous edge, not accepted edge.
    CHECK(filter.edge(100'000)); // Exactly 50 ms is accepted.
    CHECK(filter.edge(200'000)); // Release bounce is deliberately not suppressed.
    filter = button::TimeFilter{};
    CHECK(filter.edge(3'000'000'000LL)); // Beyond signed 32-bit microseconds.
    CHECK(!filter.edge(3'000'000'001LL));
}

void stable_trace(bool interrupt_mode) {
    button::StableButton filter;
    int count = 0;
    // 5 ms samples: press bounce, held LOW, release bounce, stable release,
    // second press, held LOW. Long enough stable intervals separate presses.
    bool previous_low = false;
    for (int ms = 0; ms <= 600; ms += 5) {
        const bool low = (ms >= 10 && ms < 15) || (ms >= 20 && ms < 25) ||
                         (ms >= 30 && ms < 200) || (ms >= 205 && ms < 210) ||
                         (ms >= 220 && ms < 225) || (ms >= 350);
        const bool edge = low && !previous_low;
        const bool accepted = filter.sample(low, ms * 1000LL, edge, interrupt_mode);
        if (accepted) {
            CHECK(low);
            CHECK(ms == 80 || ms == 400);
            ++count;
        }
        previous_low = low;
    }
    CHECK(count == 2);
}

void event_and_release_rules() {
    button::StableButton filter;
    CHECK(!filter.sample(false, 0, true, true)); // Event already HIGH: ignore.
    CHECK(!filter.sample(true, 100'000, false, true)); // No stale pending event.
    CHECK(!filter.sample(true, 105'000, true, true));
    CHECK(!filter.sample(true, 154'999, false, true));
    CHECK(filter.sample(true, 155'000, false, true));
    CHECK(!filter.sample(true, 250'000, true, true)); // Extra falling while held.
    CHECK(!filter.sample(false, 300'000, false, true));
    CHECK(!filter.sample(true, 345'000, true, true)); // Release <50ms: no rearm.
    CHECK(!filter.sample(true, 400'000, false, true));
    CHECK(!filter.sample(false, 450'000, false, true));
    CHECK(!filter.sample(false, 500'000, false, true));
    CHECK(filter.state() == button::State::released);
    CHECK(!filter.sample(true, 600'000, true, true));
    CHECK(filter.sample(true, 650'000, false, true));
}

void startup_held_and_short_pulse(bool interrupt_mode) {
    button::StableButton filter{true};
    CHECK(!filter.sample(true, 0, true, interrupt_mode));
    CHECK(!filter.sample(true, 100'000, true, interrupt_mode));
    CHECK(!filter.sample(false, 200'000, false, interrupt_mode));
    CHECK(!filter.sample(false, 250'000, false, interrupt_mode));
    CHECK(!filter.sample(true, 300'000, true, interrupt_mode));
    CHECK(!filter.sample(false, 345'000, false, interrupt_mode)); // Short press rejected.
    CHECK(!filter.sample(true, 400'000, true, interrupt_mode));
    CHECK(filter.sample(true, 450'000, false, interrupt_mode));
}

int main() {
    time_filter();
    stable_trace(true);
    stable_trace(false);
    event_and_release_rules();
    startup_held_and_short_pulse(true);
    startup_held_and_short_pulse(false);
    std::puts("PASS: time boundaries, consecutive edges, bounce traces, event gating, release, startup");
}
