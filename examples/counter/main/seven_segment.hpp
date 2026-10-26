#pragma once

#include <cstdint>

#include "esp_err.h"

enum class Digit : std::uint8_t { Zero, One };

class SevenSegment final {
public:
    esp_err_t init() const;
    esp_err_t show(Digit digit) const;
};
