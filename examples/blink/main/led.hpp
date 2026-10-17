#pragma once

#include <cstdint>

#include "driver/gpio.h"
#include "esp_err.h"

enum class LedState : std::uint8_t { Off, On };

class Led final {
public:
    constexpr explicit Led(gpio_num_t pin, bool active_high)
        : pin_(pin), active_high_(active_high) {}

    // Call init() successfully before set().
    esp_err_t init() const;
    esp_err_t set(LedState state) const;

private:
    const gpio_num_t pin_;
    const bool active_high_;
};
