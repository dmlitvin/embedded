#pragma once

#include <array>
#include <cstdint>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

struct Config final {
    // A, B, C, D, E, F, G; common anode, active LOW.
    static constexpr std::array<gpio_num_t, 7> segment_pins{
        GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7,
        GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17};
    static constexpr gpio_num_t red_pin = GPIO_NUM_8;
    static constexpr gpio_num_t yellow_pin = GPIO_NUM_9;
    static constexpr gpio_num_t green_pin = GPIO_NUM_10;
    static constexpr std::array<gpio_num_t, 3> led_pins{
        red_pin, yellow_pin, green_pin};
    static constexpr std::uint32_t segment_on = 0;
    static constexpr std::uint32_t segment_off = 1;
    static constexpr std::uint32_t led_on = 1;
    static constexpr std::uint32_t led_off = 0;
    static constexpr TickType_t scheduler_pause_ticks = 1;
};
