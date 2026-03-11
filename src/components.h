#pragma once

#include <M5Unified.h>

struct Position
{
    int16_t x{0};
    int16_t y{0};
};

struct Sprite
{
    uint16_t w{};
    uint16_t h{};
    uint16_t color{TFT_TRANSPARENT};
};
