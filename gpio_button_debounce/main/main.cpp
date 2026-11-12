#include <cinttypes>
#include <cstdint>

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "soc/uart_pins.h"
#include "debounce.hpp"

#if !CONFIG_ESP_CONSOLE_UART_DEFAULT
#error "This project requires the default UART console"
#endif

namespace {
constexpr char tag[] = "button";
constexpr int pin = CONFIG_BUTTON_GPIO;
constexpr auto console = static_cast<uart_port_t>(CONFIG_ESP_CONSOLE_UART_NUM);
constexpr TickType_t sample_ticks = pdMS_TO_TICKS(5);
static_assert(sample_ticks > 0, "5 ms polling needs a faster FreeRTOS tick");

struct Edge { std::int64_t time_us; std::uint32_t number; };
QueueHandle_t events;
portMUX_TYPE counter_lock = portMUX_INITIALIZER_UNLOCKED;
std::uint32_t raw_edges = 0;
std::uint32_t lost_events = 0;
std::uint64_t accepted = 0;
int mode = CONFIG_BUTTON_START_MODE;
bool hardware = false;
bool isr_installed = false;
bool rc_label = false; // Experiment label only; never changes electrical wiring.
button::TimeFilter time_filter;
button::StableButton stable_button;

bool is_low() { return gpio_get_level(static_cast<gpio_num_t>(pin)) == 0; }

// ISR records edges only: no delay, debounce decision, or logging here.
void on_falling(void *) {
    const auto timestamp = esp_timer_get_time();
    portENTER_CRITICAL_ISR(&counter_lock);
    const Edge edge{timestamp, ++raw_edges};
    portEXIT_CRITICAL_ISR(&counter_lock);
    BaseType_t wake = pdFALSE;
    if (xQueueSendFromISR(events, &edge, &wake) != pdTRUE) {
        portENTER_CRITICAL_ISR(&counter_lock);
        ++lost_events;
        portEXIT_CRITICAL_ISR(&counter_lock);
    }
    portYIELD_FROM_ISR(wake);
}

void stats() {
    portENTER_CRITICAL(&counter_lock);
    const auto raw = raw_edges;
    const auto lost = lost_events;
    portEXIT_CRITICAL(&counter_lock);
    ESP_LOGI(tag, "mode=%d rc=%s GPIO=%d enabled=%d raw=%" PRIu32
             " accepted=%" PRIu64 " queue_lost=%" PRIu32 " level=%d",
             mode, rc_label ? "yes" : "no", pin, hardware, raw, accepted, lost,
             hardware ? gpio_get_level(static_cast<gpio_num_t>(pin)) : -1);
    if (lost != 0) {
        ESP_LOGW(tag, "Queue overflow: log/accepted counts incomplete; repeat this trial");
    }
}

void help() {
    ESP_LOGI(tag, "Commands: 1 raw FALLING, 2 time 50ms, 3 state, 4 polling 5ms");
    ESP_LOGI(tag, "s statistics; r reset trial; c toggle RC label + reset; h help");
}

void restart_trial(int next_mode) {
    if (isr_installed) {
        ESP_ERROR_CHECK(gpio_intr_disable(static_cast<gpio_num_t>(pin)));
        ESP_ERROR_CHECK(gpio_isr_handler_remove(static_cast<gpio_num_t>(pin)));
        ESP_ERROR_CHECK(gpio_uninstall_isr_service());
        isr_installed = false;
    }
    xQueueReset(events);
    portENTER_CRITICAL(&counter_lock);
    raw_edges = 0;
    lost_events = 0;
    portEXIT_CRITICAL(&counter_lock);
    accepted = 0;
    mode = next_mode;
    time_filter = button::TimeFilter{};
    stable_button = button::StableButton{hardware && is_low()};
    if (hardware && mode != 4) {
        ESP_ERROR_CHECK(gpio_set_intr_type(static_cast<gpio_num_t>(pin), GPIO_INTR_NEGEDGE));
        ESP_ERROR_CHECK(gpio_install_isr_service(0));
        ESP_ERROR_CHECK(gpio_isr_handler_add(static_cast<gpio_num_t>(pin), on_falling, nullptr));
        isr_installed = true;
        ESP_ERROR_CHECK(gpio_intr_enable(static_cast<gpio_num_t>(pin)));
    } else if (hardware) {
        ESP_ERROR_CHECK(gpio_set_intr_type(static_cast<gpio_num_t>(pin), GPIO_INTR_DISABLE));
    }
    stats();
}

void count_press() {
    ++accepted;
    ESP_LOGI(tag, "mode=%d count=%" PRIu64, mode, accepted);
}

bool configure_button(int gpio_number) {
    if (gpio_number < 0) {
        ESP_LOGW(tag, "GPIO disabled. Confirm wiring, set Button debounce assignment / GPIO in menuconfig.");
        return false;
    }
    // Exclude memory, USB, strapping and the configured console pins.
    if (!GPIO_IS_VALID_GPIO(gpio_number) || gpio_number == 0 || gpio_number == 3 ||
        gpio_number == 19 || gpio_number == 20 || (gpio_number >= 26 && gpio_number <= 37) ||
        gpio_number == 45 || gpio_number == 46 ||
        gpio_number == U0TXD_GPIO_NUM || gpio_number == U0RXD_GPIO_NUM) {
        ESP_LOGE(tag, "GPIO %d is invalid or reserved by this project; input stays disabled", pin);
        return false;
    }
    gpio_config_t config{};
    config.pin_bit_mask = 1ULL << gpio_number;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&config));
    return true;
}
} // namespace

extern "C" void app_main() {
    events = xQueueCreate(256, sizeof(Edge));
    ESP_ERROR_CHECK(events ? ESP_OK : ESP_ERR_NO_MEM);
    // Keep default UART console pins and baud rate configured by ESP-IDF.
    ESP_ERROR_CHECK(uart_driver_install(console, 256, 0, 0, nullptr, 0));
    hardware = configure_button(pin);
    help();
    restart_trial(mode);
    TickType_t last_wake = xTaskGetTickCount();
    while (true) {
        std::uint8_t command;
        // One command per iteration prevents serial traffic from starving sampling.
        if (uart_read_bytes(console, &command, 1, 0) == 1) {
            if (command >= '1' && command <= '4') { restart_trial(command - '0'); }
            else if (command == 'r') { restart_trial(mode); }
            else if (command == 'c') { rc_label = !rc_label; restart_trial(mode); }
            else if (command == 's') { stats(); }
            else if (command == 'h') { help(); }
        }
        bool pending_edge = false;
        Edge edge{};
        // Bound work even if the input is noisy. Queue loss is reported separately.
        for (int i = 0; i < 256 && xQueueReceive(events, &edge, 0) == pdTRUE; ++i) {
            pending_edge = true;
            if (mode == 1) {
                accepted = edge.number;
                ESP_LOGI(tag, "mode=1 interrupt_count=%" PRIu32, edge.number);
            } else if (mode == 2 && time_filter.edge(edge.time_us)) {
                count_press();
            }
        }
        if (hardware && (mode == 3 || mode == 4) &&
            stable_button.sample(is_low(), esp_timer_get_time(), pending_edge, mode == 3)) {
            count_press();
        }
        xTaskDelayUntil(&last_wake, sample_ticks);
    }
}
