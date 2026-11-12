#pragma once

#include <cstdint>

namespace button {
constexpr std::int64_t debounce_us = 50'000;

// Compare consecutive interrupt timestamps, including ignored interrupts.
class TimeFilter {
public:
    bool edge(std::int64_t now) {
        const bool accept = !seen_ || now - previous_ >= debounce_us;
        previous_ = now;
        seen_ = true;
        return accept;
    }
private:
    bool seen_ = false;
    std::int64_t previous_ = 0;
};

enum class State { released, press_candidate, pressed, release_candidate };

class StableButton {
public:
    // A button held at reset is not a new press. Require release first.
    explicit StableButton(bool initially_low = false)
        : state_(initially_low ? State::pressed : State::released) {}

    // Called by a task every 5 ms. event gates only entry into press_candidate.
    bool sample(bool low, std::int64_t now, bool event, bool require_event) {
        switch (state_) {
        case State::released:
            if (low && (!require_event || event)) {
                state_ = State::press_candidate;
                since_ = now;
            }
            break;
        case State::press_candidate:
            if (!low) {
                state_ = State::released;
            } else if (now - since_ >= debounce_us) {
                state_ = State::pressed;
                return true;
            }
            break;
        case State::pressed:
            if (!low) {
                state_ = State::release_candidate;
                since_ = now;
            }
            break;
        case State::release_candidate:
            if (low) {
                state_ = State::pressed;
            } else if (now - since_ >= debounce_us) {
                state_ = State::released;
            }
            break;
        }
        return false;
    }
    State state() const { return state_; }
private:
    State state_;
    std::int64_t since_ = 0;
};
} // namespace button
