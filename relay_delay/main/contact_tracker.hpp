#pragma once

#include <cstdint>

// Caller serializes all accesses between the ISR and measurement task.
struct ContactTracker {
    int level = 1;
    int target = 0;
    int64_t start_us = 0;
    int64_t last_edge_us = 0;
    int64_t first_target_us = 0;
    uint32_t edges = 0;
    bool saw_target = false;

    void reset(int64_t now, int initial_level, int target_level) {
        level = initial_level;
        target = target_level;
        start_us = last_edge_us = now;
        first_target_us = 0;
        edges = 0;
        saw_target = false;
    }

    void on_edge(int64_t now, int new_level) {
        // Even duplicate sampled levels restart quiet time: an ISR means
        // activity occurred, possibly faster than the software could sample.
        level = new_level;
        last_edge_us = now;
        ++edges;
        if (new_level == target && !saw_target) {
            saw_target = true;
            first_target_us = now;
        }
    }

    bool stable(int64_t now, int observed_level, int64_t quiet_us) const {
        return level == target && observed_level == target &&
               now - last_edge_us >= quiet_us;
    }

    bool complete(int64_t now, int observed_level, int64_t quiet_us,
                  int64_t timeout_us) const {
        return saw_target && stable(now, observed_level, quiet_us) &&
               last_edge_us + quiet_us <= start_us + timeout_us;
    }
};
