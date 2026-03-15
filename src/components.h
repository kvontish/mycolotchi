#pragma once

#include <M5Unified.h>

struct Animation {
    M5Canvas **frames{nullptr}; // heap-allocated array of M5Canvas sprites, one per frame
    uint8_t frameCount{0};
    uint16_t frameDurationMs{100};
    bool loop{true};
};

struct AnimationSet {
    Animation *animations{nullptr}; // heap-allocated array of animations, indexed by state; owned by the scene
    uint8_t animationCount{0};
    uint16_t w{0};
    uint16_t h{0};
};

struct AnimationState {
    AnimationSet *set{nullptr}; // borrowed; owned and freed by the scene
    uint8_t currentAnimation{0};
    uint8_t currentFrame{0};
    uint32_t lastFrameMs{0};
};

struct Hitbox {
    uint16_t w{0};
    uint16_t h{0};
    int8_t ox{0}; // x offset from Position
    int8_t oy{0}; // y offset from Position
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
    M5Canvas *data{nullptr};       // borrowed; points to current frame canvas owned by AnimationSet
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
