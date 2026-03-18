#pragma once

#include <M5Unified.h>

struct Scene; // forward declaration for GameMap
struct View;  // forward declaration for GameMap

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

struct Camera {
    int16_t x{0};
    int16_t y{0};
    uint16_t w{160};
    uint16_t h{120};
    uint8_t scale{2};
};

struct Clock {
    uint16_t year{2025};
    uint8_t month{1}, day{1};
    uint8_t hours{0}, minutes{0}, seconds{0};
    uint32_t timestamp{0}; // Unix time, seconds since 1970-01-01
};

struct StepCounter {
    uint32_t steps{0};
};

struct GameMap {
    Scene *homeScene{nullptr};
    Scene *titleScene{nullptr};
    Scene *gameScene{nullptr};
    Scene *clockScene{nullptr};
    Scene *menuScene{nullptr};
    View *gameOverView{nullptr};
    View *statusView{nullptr};
};

struct Hitbox {
    uint16_t w{0};
    uint16_t h{0};
    int8_t ox{0}; // x offset from Position
    int8_t oy{0}; // y offset from Position
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
    int8_t scaleX{1};
    int8_t scaleY{1};
};

struct Pet {
    uint8_t hunger{100};    // 0 = starving, 100 = full
    uint8_t happiness{100}; // 0 = miserable, 100 = ecstatic
};

struct Score {
    uint32_t value{0};
};

struct Sprite {
    uint16_t w{0};
    uint16_t h{0};
    M5Canvas *data{nullptr}; // borrowed; points to current frame canvas owned by AnimationSet
    uint16_t color{TFT_TRANSPARENT};
};

struct Tiled {
    bool x{true};
    bool y{true};
};

struct Velocity {
    int16_t x{0};
    int16_t y{0};
};

struct ViewOwner {
    View *view{nullptr}; // links an entity to the View overlay that owns it
};

// --- Tags ---
struct CameraTarget {};
struct Coin {};
struct Despawnable {};
struct Gravity {};
struct Grounded {};
struct Obstacle {};
struct Player {};
struct Shroom {};
struct Solid {};
