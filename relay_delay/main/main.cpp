#include <cinttypes>
#include <cstdio>

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "contact_tracker.hpp"

namespace {
constexpr auto kDrive = static_cast<gpio_num_t>(CONFIG_RELAY_DRIVE_GPIO);
constexpr auto kContact = static_cast<gpio_num_t>(CONFIG_RELAY_CONTACT_GPIO);
constexpr int kOff = 1 - CONFIG_RELAY_ACTIVE_LEVEL;
constexpr int64_t kQuietUs = CONFIG_RELAY_DEBOUNCE_MS * 1000LL;
constexpr int64_t kTimeoutUs = CONFIG_RELAY_TIMEOUT_MS * 1000LL;
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
ContactTracker tracker;

struct Result {
    int64_t first_us;
    int64_t settled_us;
    uint32_t edges;
};

struct Statistics {
    int count = 0;
    int64_t first_sum = 0;
    int64_t settled_sum = 0;

    void add(const Result &result) {
        ++count;
        first_sum += result.first_us;
        settled_sum += result.settled_us;
    }

    void print(const char *name) const {
        if (count == 0) {
            std::printf("%s: valid=0; average unavailable\n", name);
            return;
        }
        std::printf("%s: valid=%d; mean_first=%.1f us; mean_settled=%.1f us\n",
                    name, count, static_cast<double>(first_sum) / count,
                    static_cast<double>(settled_sum) / count);
    }
};

// Conservative exclusions for this S3 with octal PSRAM; actual board wiring
// must still be checked. Keep strapping, memory, USB and console pins free.
bool allowed_pin(int pin) {
    return GPIO_IS_VALID_OUTPUT_GPIO(pin) && pin != 0 && pin != 3 &&
           pin != 19 && pin != 20 && !(pin >= 26 && pin <= 37) &&
           !(pin >= 43 && pin <= 46);
}

uint64_t pin_mask(gpio_num_t pin) {
    return 1ULL << static_cast<unsigned>(pin);
}

void contact_isr(void *) {
    portENTER_CRITICAL_ISR(&lock);
    const int64_t now = esp_timer_get_time();
    tracker.on_edge(now, gpio_get_level(kContact));
    portEXIT_CRITICAL_ISR(&lock);
}

bool wait_initial_off() {
    const int64_t deadline = esp_timer_get_time() + kTimeoutUs;
    while (true) {
        portENTER_CRITICAL(&lock);
        const int64_t now = esp_timer_get_time();
        const bool ready = tracker.stable(now, gpio_get_level(kContact), kQuietUs);
        portEXIT_CRITICAL(&lock);
        if (ready) return true;
        if (now >= deadline) return false;
        vTaskDelay(1);
    }
}

bool measure(bool energize, Result &result) {
    const int target = energize ? 0 : 1; // COM=GND, NO=input + pull-up
    portENTER_CRITICAL(&lock);
    const int initial = gpio_get_level(kContact);
    const int64_t now = esp_timer_get_time();
    const bool baseline_ok = initial == 1 - target &&
                             tracker.level == initial &&
                             now - tracker.last_edge_us >= kQuietUs;
    if (!baseline_ok) {
        portEXIT_CRITICAL(&lock);
        std::printf("ERROR: contact is not stable in the expected initial state\n");
        return false;
    }
    // Timestamp immediately before the command, with the ISR excluded until
    // the new measurement state and GPIO command are both in place.
    tracker.reset(esp_timer_get_time(), initial, target);
    const esp_err_t command = gpio_set_level(
        kDrive, energize ? CONFIG_RELAY_ACTIVE_LEVEL : kOff);
    portEXIT_CRITICAL(&lock);
    if (command != ESP_OK) return false;

    while (true) {
        portENTER_CRITICAL(&lock);
        const int64_t check_us = esp_timer_get_time();
        const bool done = tracker.complete(check_us, gpio_get_level(kContact),
                                           kQuietUs, kTimeoutUs);
        const bool expired = check_us - tracker.start_us >= kTimeoutUs;
        if (done) {
            result = {tracker.first_target_us - tracker.start_us,
                      tracker.last_edge_us - tracker.start_us, tracker.edges};
        }
        portEXIT_CRITICAL(&lock);
        if (done) return true;
        if (expired) {
            std::printf("ERROR: %s timeout; no confirmed stable transition\n",
                        energize ? "ON" : "OFF");
            return false;
        }
        vTaskDelay(1);
    }
}

void print_result(int cycle, const char *phase, const Result &r) {
    std::printf("%d,%s,%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRIu32 "\n",
                cycle, phase, r.first_us, r.settled_us,
                r.settled_us - r.first_us, r.edges);
}
} // namespace

extern "C" void app_main() {
    // Allow time to attach the serial monitor after flashing/reset.
    vTaskDelay(pdMS_TO_TICKS(2000));
    std::printf("\nRelay mechanical delay measurement (ESP-IDF, microseconds)\n");
#ifndef CONFIG_RELAY_WIRING_CONFIRMED
    std::printf("DISABLED: wiring is not confirmed. No relay GPIO configured.\n"
                "Set verified GPIOs, active level and wiring confirmation in menuconfig.\n");
    return;
#endif
    if (!allowed_pin(CONFIG_RELAY_DRIVE_GPIO) ||
        !allowed_pin(CONFIG_RELAY_CONTACT_GPIO) || kDrive == kContact) {
        std::printf("ERROR: unspecified, identical or reserved GPIOs. No GPIO configured.\n");
        return;
    }

    // Set the output latch before enabling output to avoid an active pulse.
    ESP_ERROR_CHECK(gpio_set_level(kDrive, kOff));
    gpio_config_t output{};
    output.pin_bit_mask = pin_mask(kDrive);
    output.mode = GPIO_MODE_OUTPUT;
    ESP_ERROR_CHECK(gpio_config(&output));

    gpio_config_t input{};
    input.pin_bit_mask = pin_mask(kContact);
    input.mode = GPIO_MODE_INPUT;
    input.pull_up_en = GPIO_PULLUP_ENABLE;
    input.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&input));
    tracker.reset(esp_timer_get_time(), gpio_get_level(kContact), 1);
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(kContact, contact_isr, nullptr));
    ESP_ERROR_CHECK(gpio_set_intr_type(kContact, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_intr_enable(kContact));

    Statistics on, off;
    bool success = wait_initial_off();
    if (!success) {
        std::printf("ERROR: NO contact must be stably open with the relay OFF\n");
    } else {
        std::printf("drive=%d; contact=%d; active=%d; quiet=%d ms; cycles=%d\n",
                    CONFIG_RELAY_DRIVE_GPIO, CONFIG_RELAY_CONTACT_GPIO,
                    CONFIG_RELAY_ACTIVE_LEVEL, CONFIG_RELAY_DEBOUNCE_MS,
                    CONFIG_RELAY_CYCLES);
        std::printf("cycle,phase,first_us,settled_us,bounce_us,interrupts\n");
    }
    for (int cycle = 1; success && cycle <= CONFIG_RELAY_CYCLES; ++cycle) {
        Result result{};
        success = measure(true, result);
        if (!success) break;
        on.add(result);
        print_result(cycle, "ON", result);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_RELAY_HOLD_MS));
        success = measure(false, result);
        if (!success) break;
        off.add(result);
        print_result(cycle, "OFF", result);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_RELAY_HOLD_MS));
    }
    // Always request OFF on completion or a measurement failure.
    ESP_ERROR_CHECK(gpio_set_level(kDrive, kOff));
    ESP_ERROR_CHECK(gpio_intr_disable(kContact));
    ESP_ERROR_CHECK(gpio_isr_handler_remove(kContact));
    gpio_uninstall_isr_service();
    on.print("ON");
    off.print("OFF");
    std::printf("%s; relay commanded OFF. Reset to repeat.\n",
                success ? "COMPLETE" : "INCOMPLETE (check wiring/contact/timeout)");
}
