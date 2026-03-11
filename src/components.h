#pragma once

#include <M5Unified.h>

struct Position
{
    int16_t x{0};
    int16_t y{0};
};

struct Camera
{
    int16_t x{0};
    int16_t y{0};
    uint16_t w{320};
    uint16_t h{240};
};

struct Sprite
{
    uint16_t w{0};
    uint16_t h{0};
    uint16_t color{TFT_TRANSPARENT};
};
