#pragma once

#include <M5Unified.h>

struct AnimationClip {
    uint16_t **frames{nullptr}; // heap-allocated array of decoded frame buffers
    uint8_t frameCount{0};
    uint16_t frameDurationMs{100};
    bool loop{true};
};

struct Animation {
    AnimationClip *clips{nullptr}; // heap-allocated array of clips, indexed by state
    uint8_t clipCount{0};
    uint8_t currentClip{0};
    uint8_t currentFrame{0};
    uint32_t lastFrameMs{0};
};

struct Camera {
    int16_t x{0};
    int16_t y{0};
    uint16_t w{160};
    uint16_t h{120};
    uint8_t scale{2};
};

struct Label {
    const char *text{nullptr};
    uint16_t color{TFT_WHITE};
    uint8_t size{1};
};

struct Position {
    int16_t x{0};
    int16_t y{0};
    float parallax{1.0f};
};

struct Score {
    uint32_t value{0};
};

struct Sprite {
    uint16_t w{0};
    uint16_t h{0};
    uint16_t color{TFT_TRANSPARENT};
    const uint16_t *data{nullptr}; // decoded RGB565 pixels, row-major; nullptr falls back to color
    bool ownsData{true};           // false = data is borrowed; freeSprites will not free it
};

struct Tiled {
    bool x{true};
    bool y{true};
};

struct Velocity {
    int16_t x{0};
    int16_t y{0};
};

// --- Tags ---
struct CameraTarget {};
struct Coin {};
struct Despawnable {};
struct Gravity {};
struct Grounded {};
struct Obstacle {};
struct Player {};
struct Solid {};
