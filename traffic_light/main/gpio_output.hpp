#pragma once

#include <array>
#include <cstdint>

#include "driver/gpio.h"

template <std::size_t N>
esp_err_t init_outputs(const std::array<gpio_num_t, N>& pins,
                       std::uint32_t inactive_level) {
    gpio_config_t configuration{};
    configuration.mode = GPIO_MODE_OUTPUT;
    configuration.pull_up_en = GPIO_PULLUP_DISABLE;
    configuration.pull_down_en = GPIO_PULLDOWN_DISABLE;
    configuration.intr_type = GPIO_INTR_DISABLE;
    for (const auto pin : pins) {
        if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) {
            return ESP_ERR_INVALID_ARG;
        }
        configuration.pin_bit_mask |= std::uint64_t{1} << pin;
    }
    // Preload the inactive level before enabling output drivers.
    for (const auto pin : pins) {
        const auto result = gpio_set_level(pin, inactive_level);
        if (result != ESP_OK) {
            return result;
        }
    }
    return gpio_config(&configuration);
}
