#pragma once

#include <cstdint>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

struct Config final {
    // External LED: GPIO4 -> resistor -> LED anode; cathode -> GND.
    static constexpr gpio_num_t led_pin = GPIO_NUM_4;
    static constexpr bool led_active_high = true;
    static constexpr std::uint64_t blink_interval_ms = 500;
    static constexpr std::uint64_t microseconds_per_millisecond = 1'000;
    static constexpr TickType_t scheduler_pause_ticks = 1;
};

static_assert(GPIO_IS_VALID_OUTPUT_GPIO(Config::led_pin),
              "LED pin must support GPIO output");
static_assert(Config::blink_interval_ms > 0, "Blink interval must be positive");
static_assert(Config::scheduler_pause_ticks > 0, "Superloop must yield to FreeRTOS");
