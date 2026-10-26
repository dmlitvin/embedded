#include "traffic_light.hpp"

#include "config.hpp"
#include "gpio_output.hpp"

esp_err_t TrafficLight::init() const {
    return init_outputs(Config::led_pins, Config::led_off);
}

esp_err_t TrafficLight::set(Signals signals) const {
    if (signals.green && (signals.red || signals.yellow)) {
        return ESP_ERR_INVALID_ARG;
    }
    const std::array<bool, Config::led_pins.size()> enabled{
        signals.red, signals.yellow, signals.green};
    // Turn unwanted LEDs off before turning the next signals on.
    for (const bool turn_on : {false, true}) {
        for (std::size_t index = 0; index < enabled.size(); ++index) {
            if (enabled[index] == turn_on) {
                const auto result = gpio_set_level(Config::led_pins[index],
                    turn_on ? Config::led_on : Config::led_off);
                if (result != ESP_OK) {
                    return result;
                }
            }
        }
    }
    return ESP_OK;
}
