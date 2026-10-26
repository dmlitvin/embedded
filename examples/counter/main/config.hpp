#pragma once

#include <array>
#include <cstdint>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

struct Config final {
    // Segment order: A, B, C, D, E, F, G. Common anode: LOW lights a segment.
    static constexpr std::array<gpio_num_t, 7> segment_pins{
        GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7,
        GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17};
    static constexpr std::array<gpio_num_t, 3> led_pins{
        GPIO_NUM_8, GPIO_NUM_9, GPIO_NUM_10};
    static constexpr std::uint32_t segment_on_level = 0;
    static constexpr std::uint32_t segment_off_level = 1;
    static constexpr std::uint32_t led_off_level = 0;

    static constexpr std::int64_t microseconds_per_second = 1'000'000;
    static constexpr std::int64_t count_interval_us = microseconds_per_second;
    static constexpr TickType_t scheduler_pause_ticks = 1;
};
