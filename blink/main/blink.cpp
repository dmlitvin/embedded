#include <cstdint>

#include "config.hpp"
#include "led.hpp"

#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/task.h"

namespace {
// ESP-IDF equivalent of millis(), with a 64-bit monotonic counter.
std::uint64_t millis() {
    return static_cast<std::uint64_t>(esp_timer_get_time()) /
           Config::microseconds_per_millisecond;
}
} // namespace

extern "C" void app_main() {
    const Led led{Config::led_pin, Config::led_active_high};
    ESP_ERROR_CHECK(led.init());

    LedState state = LedState::Off;
    std::uint64_t last_toggle_ms = millis();

    for (;;) {
        const std::uint64_t now_ms = millis();
        if (now_ms - last_toggle_ms >= Config::blink_interval_ms) {
            last_toggle_ms = now_ms;
            state = state == LedState::Off ? LedState::On : LedState::Off;
            ESP_ERROR_CHECK(led.set(state));
        }

        // Other short, nonblocking peripheral updates belong here.
        // Yield one RTOS tick so idle tasks and the watchdog can run.
        // LED timing is determined by elapsed time, not this scheduler pause.
        vTaskDelay(Config::scheduler_pause_ticks);
    }
}
