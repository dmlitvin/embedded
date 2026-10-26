#include "config.hpp"
#include "seven_segment.hpp"
#include "traffic_light.hpp"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"

namespace {
constexpr const char* log_tag = "traffic_light";

const char* state_name(TrafficState state) {
    switch (state) {
    case TrafficState::Green: return "green";
    case TrafficState::BlinkingGreen: return "blinking green";
    case TrafficState::Yellow: return "yellow";
    case TrafficState::Red: return "red";
    case TrafficState::RedYellow: return "red + yellow";
    }
    return "unknown";
}
} // namespace

extern "C" void app_main() {
    const TrafficLight light;
    const SevenSegment display;
    ESP_ERROR_CHECK(light.init());
    ESP_ERROR_CHECK(display.init());
    TrafficCycle cycle{esp_timer_get_time()};
    TrafficSnapshot previous{};
    bool first_update = true;

    for (;;) {
        const auto current = cycle.update(esp_timer_get_time());
        if (first_update || current.state != previous.state ||
            current.signals.green != previous.signals.green) {
            ESP_ERROR_CHECK(light.set(current.signals));
        }
        if (first_update || current.seconds_left != previous.seconds_left) {
            ESP_ERROR_CHECK(display.show(current.seconds_left));
        }
        if (first_update || current.state != previous.state) {
            ESP_LOGI(log_tag, "%s: %u s", state_name(current.state),
                     static_cast<unsigned>(current.seconds_left));
        }
        previous = current;
        first_update = false;
        // Timing is controlled above; yield one tick for idle/watchdog tasks.
        vTaskDelay(Config::scheduler_pause_ticks);
    }
}
