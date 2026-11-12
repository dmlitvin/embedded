#include <array>

#include "blink_state.hpp"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"

namespace {

constexpr char kTag[] = "nonblocking_leds";
constexpr std::uint64_t kLoopPeriodUs = 10'000;

struct Led {
    gpio_num_t pin;
    BlinkState blink;
    unsigned transitions = 0;
};

std::array<Led, 3> leds{{
    {GPIO_NUM_4, {200}},
    {GPIO_NUM_5, {500}},
    {GPIO_NUM_6, {1000}},
}};

esp_timer_handle_t loop_timer = nullptr;
std::int64_t start_ms = 0;
unsigned diagnostic_snapshots = 0;

// ESP-IDF calls this one shared loop from the timer task, never from an ISR.
// There is no time-waiting loop, sleep, or separate task/timer for each LED.
void loop(void *)
{
    const auto now_ms = esp_timer_get_time() / 1000;
    for (auto &led : leds) {
        if (led.blink.update(now_ms)) {
            ESP_ERROR_CHECK(gpio_set_level(led.pin, led.blink.on ? 1 : 0));
            ++led.transitions;
        }
    }

    // Only two startup snapshots, away from LED deadlines. Read the actual
    // digital pads, not just the software states, to help diagnose wiring.
    if (diagnostic_snapshots < 2 &&
        now_ms - start_ms >= 1100 + 1000 * diagnostic_snapshots) {
        std::array<int, 3> levels{};
        bool match = true;
        for (unsigned i = 0; i < leds.size(); ++i) {
            levels[i] = gpio_get_level(leds[i].pin);
            match = match && levels[i] == static_cast<int>(leds[i].blink.on);
        }
        ++diagnostic_snapshots;
        ESP_LOGI(kTag,
                 "CHECK #%u toggles=%u/%u/%u expected=%d%d%d read=%d%d%d %s",
                 diagnostic_snapshots,
                 leds[0].transitions, leds[1].transitions, leds[2].transitions,
                 static_cast<int>(leds[0].blink.on),
                 static_cast<int>(leds[1].blink.on),
                 static_cast<int>(leds[2].blink.on),
                 levels[0], levels[1], levels[2], match ? "MATCH" : "MISMATCH");
    }
}

} // namespace

extern "C" void app_main()
{
    gpio_config_t config{};
    // Input sensing stays enabled so startup diagnostics can read each pad.
    config.mode = GPIO_MODE_INPUT_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    for (const auto &led : leds) {
        // Set the output latch low before enabling the output driver.
        ESP_ERROR_CHECK(gpio_set_level(led.pin, 0));
        config.pin_bit_mask |= 1ULL << led.pin;
    }
    ESP_ERROR_CHECK(gpio_config(&config));

    ESP_LOGI(kTag, "LED1: GPIO4 / 200 ms; LED2: GPIO5 / 500 ms; LED3: GPIO6 / 1000 ms");
    ESP_LOGI(kTag, "Active high; initially OFF. Intervals are time between toggles.");
    ESP_LOGI(kTag, "One shared loop every 10 ms; no sleep or busy waiting.");

    esp_timer_create_args_t timer_args{};
    timer_args.callback = loop;
    timer_args.dispatch_method = ESP_TIMER_TASK;
    timer_args.name = "led_loop";
    timer_args.skip_unhandled_events = true;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &loop_timer));

    start_ms = esp_timer_get_time() / 1000;
    for (auto &led : leds) {
        led.blink.last_ms = start_ms;
    }
    ESP_ERROR_CHECK(esp_timer_start_periodic(loop_timer, kLoopPeriodUs));
    // app_main may return: the timer and LED state have static lifetime.
}
