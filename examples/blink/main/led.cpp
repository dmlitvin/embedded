#include "led.hpp"

esp_err_t Led::init() const {
    // Validate before shifting the pin mask or accessing the peripheral.
    if (!GPIO_IS_VALID_OUTPUT_GPIO(pin_)) {
        return ESP_ERR_INVALID_ARG;
    }

    // Preload the inactive level before enabling output to avoid an on pulse.
    const esp_err_t result = set(LedState::Off);
    if (result != ESP_OK) {
        return result;
    }

    gpio_config_t configuration{};
    configuration.pin_bit_mask = std::uint64_t{1} << pin_;
    configuration.mode = GPIO_MODE_OUTPUT;
    configuration.pull_up_en = GPIO_PULLUP_DISABLE;
    configuration.pull_down_en = GPIO_PULLDOWN_DISABLE;
    configuration.intr_type = GPIO_INTR_DISABLE;
    return gpio_config(&configuration);
}

esp_err_t Led::set(LedState state) const {
    const bool is_on = state == LedState::On;
    const auto level = static_cast<std::uint32_t>(is_on == active_high_);
    return gpio_set_level(pin_, level);
}
