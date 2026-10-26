#include "config.hpp"
#include "seven_segment.hpp"

#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/task.h"

namespace {
esp_err_t init_leds_off() {
    gpio_config_t configuration{};
    configuration.mode = GPIO_MODE_OUTPUT;
    configuration.pull_up_en = GPIO_PULLUP_DISABLE;
    configuration.pull_down_en = GPIO_PULLDOWN_DISABLE;
    configuration.intr_type = GPIO_INTR_DISABLE;

    for (const auto pin : Config::led_pins) {
        if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) {
            return ESP_ERR_INVALID_ARG;
        }
        configuration.pin_bit_mask |= std::uint64_t{1} << pin;
        const auto result = gpio_set_level(pin, Config::led_off_level);
        if (result != ESP_OK) {
            return result;
        }
    }
    return gpio_config(&configuration);
}
} // namespace

extern "C" void app_main() {
    ESP_ERROR_CHECK(init_leds_off());
    const SevenSegment display;
    ESP_ERROR_CHECK(display.init());

    Digit digit = Digit::Zero;
    ESP_ERROR_CHECK(display.show(digit));
    auto last_count_us = esp_timer_get_time();

    for (;;) {
        const auto now_us = esp_timer_get_time();
        if (now_us - last_count_us >= Config::count_interval_us) {
            last_count_us = now_us;
            digit = digit == Digit::Zero ? Digit::One : Digit::Zero;
            ESP_ERROR_CHECK(display.show(digit));
        }

        // Yield to the scheduler; the timer above controls the counting rate.
        vTaskDelay(Config::scheduler_pause_ticks);
    }
}
