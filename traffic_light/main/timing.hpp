#pragma once

#include <cstdint>

struct Timing final {
    static constexpr std::int64_t us_per_second = 1'000'000;
    static constexpr std::int64_t us_per_millisecond = 1'000;
    static constexpr std::int64_t green_us = 9 * us_per_second;
    static constexpr std::int64_t blinking_green_us = 3 * us_per_second;
    static constexpr std::int64_t yellow_us = 3 * us_per_second;
    static constexpr std::int64_t red_us = 9 * us_per_second;
    static constexpr std::int64_t red_yellow_us = 2 * us_per_second;
    static constexpr std::int64_t blink_half_period_us = 500 * us_per_millisecond;
    static constexpr std::uint8_t max_digit = 9;
};

constexpr bool fits_display(std::int64_t duration_us) {
    return duration_us > 0 &&
           duration_us <= Timing::max_digit * Timing::us_per_second;
}

static_assert(fits_display(Timing::green_us));
static_assert(fits_display(Timing::blinking_green_us));
static_assert(fits_display(Timing::yellow_us));
static_assert(fits_display(Timing::red_us));
static_assert(fits_display(Timing::red_yellow_us));
static_assert(Timing::blink_half_period_us > 0);
