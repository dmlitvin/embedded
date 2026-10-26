#pragma once

#include <cstdint>
#include "esp_err.h"

class SevenSegment final {
public:
    esp_err_t init() const;
    esp_err_t show(std::uint8_t digit) const;
};
