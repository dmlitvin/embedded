#pragma once

#include "esp_err.h"
#include "traffic_cycle.hpp"

class TrafficLight final {
public:
    esp_err_t init() const;
    esp_err_t set(Signals signals) const;
};
