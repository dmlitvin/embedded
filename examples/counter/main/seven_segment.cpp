#include "seven_segment.hpp"

#include "config.hpp"

namespace {
// Bits from right to left: A, B, C, D, E, F, G. A set bit means lit.
constexpr std::uint8_t zero_segments = 0b0111111;
constexpr std::uint8_t one_segments = 0b0000110;
} // namespace

esp_err_t SevenSegment::init() const {
    gpio_config_t configuration{};
    configuration.mode = GPIO_MODE_OUTPUT;
    configuration.pull_up_en = GPIO_PULLUP_DISABLE;
    configuration.pull_down_en = GPIO_PULLDOWN_DISABLE;
    configuration.intr_type = GPIO_INTR_DISABLE;

    for (const auto pin : Config::segment_pins) {
        if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) {
            return ESP_ERR_INVALID_ARG;
        }
        configuration.pin_bit_mask |= std::uint64_t{1} << pin;
    }

    // Preload HIGH before enabling outputs so all segments start off.
    for (const auto pin : Config::segment_pins) {
        const auto result = gpio_set_level(pin, Config::segment_off_level);
        if (result != ESP_OK) {
            return result;
        }
    }
    return gpio_config(&configuration);
}

esp_err_t SevenSegment::show(Digit digit) const {
    std::uint8_t segments;
    switch (digit) {
    case Digit::Zero:
        segments = zero_segments;
        break;
    case Digit::One:
        segments = one_segments;
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    for (std::size_t index = 0; index < Config::segment_pins.size(); ++index) {
        const bool is_on = (segments & (std::uint8_t{1} << index)) != 0;
        const auto result = gpio_set_level(
            Config::segment_pins[index],
            is_on ? Config::segment_on_level : Config::segment_off_level);
        if (result != ESP_OK) {
            return result;
        }
    }
    return ESP_OK;
}
