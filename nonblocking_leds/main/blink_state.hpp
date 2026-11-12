#pragma once

#include <cstdint>

// One independent schedule. Time comes from a monotonic clock, in milliseconds.
struct BlinkState {
    std::int64_t interval_ms;
    std::int64_t last_ms = 0;
    bool on = false;

    // Return true only when the physical output needs to change.
    bool update(std::int64_t now_ms)
    {
        const auto elapsed_ms = now_ms - last_ms;
        if (elapsed_ms < interval_ms) {
            return false;
        }

        // Keep the original phase despite jitter. If execution was delayed,
        // skip missed intervals in constant time, without a catch-up loop.
        const auto intervals = elapsed_ms / interval_ms;
        last_ms += intervals * interval_ms;
        if (intervals % 2 == 0) {
            return false;
        }

        on = !on;
        return true;
    }
};
