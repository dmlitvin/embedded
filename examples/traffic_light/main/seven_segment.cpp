#include "seven_segment.hpp"

#include "config.hpp"
#include "gpio_output.hpp"

namespace {
// Bit order: G F E D C B A. A set bit means the segment is lit.
constexpr std::array<std::uint8_t, 10> digit_segments{
    0b0111111, // 0
    0b0000110, // 1
    0b1011011, // 2
    0b1001111, // 3
    0b1100110, // 4
    0b1101101, // 5
    0b1111101, // 6
    0b0000111, // 7
    0b1111111, // 8
    0b1101111, // 9
};
} // namespace

esp_err_t SevenSegment::init() const {
    return init_outputs(Config::segment_pins, Config::segment_off);
}

esp_err_t SevenSegment::show(std::uint8_t digit) const {
    if (digit >= digit_segments.size()) {
        return ESP_ERR_INVALID_ARG;
    }
    for (std::size_t index = 0; index < Config::segment_pins.size(); ++index) {
        const bool lit = (digit_segments[digit] & (std::uint8_t{1} << index)) != 0;
        const auto result = gpio_set_level(Config::segment_pins[index],
            lit ? Config::segment_on : Config::segment_off);
        if (result != ESP_OK) {
            return result;
        }
    }
    return ESP_OK;
}
