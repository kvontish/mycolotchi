#pragma once

#include <M5Unified.h>

struct Position
{
    int16_t x{0};
    int16_t y{0};
    float parallax{1.0f};
};

struct Camera
{
    int16_t x{0};
    int16_t y{0};
    uint16_t w{160};
    uint16_t h{120};
    uint8_t scale{2};
};

struct Tiled
{
    bool x{true};
    bool y{true};
};

struct Sprite
{
    uint16_t w{0};
    uint16_t h{0};
    uint16_t color{TFT_TRANSPARENT};
};
