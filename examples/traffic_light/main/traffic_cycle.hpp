#pragma once

#include "timing.hpp"

enum class TrafficState : std::uint8_t {
    Green, BlinkingGreen, Yellow, Red, RedYellow
};

struct Signals {
    bool red;
    bool yellow;
    bool green;
};

struct TrafficSnapshot {
    TrafficState state;
    std::uint8_t seconds_left;
    Signals signals;
};

// Pure timing logic: the caller supplies monotonic microseconds.
class TrafficCycle final {
public:
    explicit TrafficCycle(std::int64_t start_us) : state_started_us_{start_us} {}

    TrafficSnapshot update(std::int64_t now_us) {
        // Preserve scheduled boundaries even when the superloop runs late.
        while (now_us - state_started_us_ >= duration()) {
            state_started_us_ += duration();
            advance();
        }
        const auto elapsed_us = now_us - state_started_us_;
        const auto remaining_us = duration() - elapsed_us;
        const auto seconds = static_cast<std::uint8_t>(
            (remaining_us + Timing::us_per_second - 1) / Timing::us_per_second);
        const bool green_on = state_ == TrafficState::Green ||
            (state_ == TrafficState::BlinkingGreen &&
             (elapsed_us / Timing::blink_half_period_us) % 2 == 0);
        return {state_, seconds, {
            state_ == TrafficState::Red || state_ == TrafficState::RedYellow,
            state_ == TrafficState::Yellow || state_ == TrafficState::RedYellow,
            green_on}};
    }

private:
    std::int64_t duration() const {
        switch (state_) {
        case TrafficState::Green: return Timing::green_us;
        case TrafficState::BlinkingGreen: return Timing::blinking_green_us;
        case TrafficState::Yellow: return Timing::yellow_us;
        case TrafficState::Red: return Timing::red_us;
        case TrafficState::RedYellow: return Timing::red_yellow_us;
        }
        return Timing::red_us;
    }

    void advance() {
        switch (state_) {
        case TrafficState::Green: state_ = TrafficState::BlinkingGreen; break;
        case TrafficState::BlinkingGreen: state_ = TrafficState::Yellow; break;
        case TrafficState::Yellow: state_ = TrafficState::Red; break;
        case TrafficState::Red: state_ = TrafficState::RedYellow; break;
        case TrafficState::RedYellow: state_ = TrafficState::Green; break;
        }
    }

    TrafficState state_{TrafficState::Green};
    std::int64_t state_started_us_;
};
