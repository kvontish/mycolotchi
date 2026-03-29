#pragma once

#include <M5Unified.h>

struct Scene; // forward declaration for GameMap
struct View;  // forward declaration for GameMap

struct Animation {
    M5Canvas **frames{nullptr}; // heap-allocated array of M5Canvas sprites, one per frame
    uint8_t    frameCount{0};
    uint16_t   frameDurationMs{100};
    bool       loop{true};
};

struct AnimationSet {
    Animation *animations{nullptr}; // heap-allocated array of animations, indexed by state; owned by the scene
    uint8_t    animationCount{0};
    uint16_t   w{0};
    uint16_t   h{0};
};

struct AnimationState {
    AnimationSet *set{nullptr}; // borrowed; owned and freed by the scene
    uint8_t       currentAnimation{0};
    uint8_t       currentFrame{0};
    uint32_t      lastFrameMs{0};
};

struct Camera {
    int16_t  x{0};
    int16_t  y{0};
    uint16_t w{160};
    uint16_t h{120};
    uint8_t  scale{2};
};

struct Clock {
    uint16_t year{2025};
    uint8_t  month{1}, day{1};
    uint8_t  hours{0}, minutes{0}, seconds{0};
    uint32_t timestamp{0}; // Unix time, seconds since 1970-01-01
};

struct GameMap {
    Scene *homeScene{nullptr};
    Scene *gameScene{nullptr};
    Scene *clockScene{nullptr};
    Scene *menuScene{nullptr};
    Scene *walkScene{nullptr};
    View  *gameOverView{nullptr};
    View  *statusView{nullptr};
};

struct Hitbox {
    uint16_t w{0};
    uint16_t h{0};
    int8_t   ox{0}; // x offset from Position
    int8_t   oy{0}; // y offset from Position
};

struct Label {
    const char *text{nullptr};
    uint16_t    color{TFT_WHITE};
    uint8_t     size{1};
};

struct Position {
    int16_t x{0};
    int16_t y{0};
    float   parallax{1.0f};
    int8_t  scaleX{1};
    int8_t  scaleY{1};
};

struct Species {
    const char *name;

    // Substrate sweet spot and decay
    uint8_t  substrateIdealLo;  // lower bound of ideal substrate range
    uint8_t  substrateIdealHi;  // upper bound of ideal substrate range
    uint8_t  substrateLargeDev; // deviation from ideal bounds that also damages health
    uint32_t substrateDecayMs;  // ms per -1 substrate tick (consumption rate)

    // Moisture sweet spot and decay
    uint8_t  moistureIdealLo;  // lower bound of ideal moisture range
    uint8_t  moistureIdealHi;  // upper bound of ideal moisture range
    uint8_t  moistureLargeDev; // deviation from ideal bounds that also damages health
    uint32_t moistureDecayMs;  // ms per -1 moisture tick (evaporation rate)

    // Mycelium growth stage thresholds
    uint32_t myceliumStage1; // Colonized → Pinning
    uint32_t myceliumStage2; // Pinning → Young fruiting body
    uint32_t myceliumStage3; // Young → Mature

    // Mycelium resilience
    uint32_t myceliumFloor;      // minimum mycelium once established (never decays below this)
    uint32_t myceliumDecayDenom; // proportional decay: lose 1/N of mycelium per tick

    // Spore generation
    uint32_t sporeBaseMs;      // base spore interval at minimum mycelium
    uint32_t sporeMyceliumDiv; // mycelium units that halve the spore interval
};

// Oyster mushroom (Pleurotus ostreatus) — v1 fixed species
// Real cultivation notes:
//   Substrate: straw or hardwood at field capacity; over-supplementation invites contamination
//   Moisture:  needs 85–95% RH to fruit; struggles badly when it dries out
//   Mycelium:  aggressive, fast coloniser; very resilient once established
static constexpr Species kOysterMushroom = {
    .name               = "Oyster",
    .substrateIdealLo   = 40,
    .substrateIdealHi   = 70,
    .substrateLargeDev  = 20,                  // <20 or >90 damages health
    .substrateDecayMs   = 8UL * 60UL * 1000UL, // -1 per 8 min
    .moistureIdealLo    = 70,                  // oysters need it genuinely humid
    .moistureIdealHi    = 90,
    .moistureLargeDev   = 15,                  // <55 damages health; >100 impossible (excess moisture safe)
    .moistureDecayMs    = 3UL * 60UL * 1000UL, // -1 per 3 min; misting required regularly
    .myceliumStage1     = 1000UL,
    .myceliumStage2     = 3000UL,
    .myceliumStage3     = 6000UL,
    .myceliumFloor      = 100UL,
    .myceliumDecayDenom = 100UL,               // ~1% loss per tick; resilient coloniser
    .sporeBaseMs        = 5UL * 60UL * 1000UL, // 1 spore per 5 min at low mycelium
    .sporeMyceliumDiv   = 1000UL,              // at 1k mycelium: ~2.5 min/spore; at 6k: ~43 sec/spore
};

struct Pet {
    const Species *species{&kOysterMushroom}; // pointer to species data; supports multiple pets

    // Visible stats
    uint8_t substrate{60}; // 0=depleted, 100=saturated; sweet-spot discovered through play
    uint8_t moisture{60};  // 0=bone dry, 100=soaking; sweet-spot discovered through play
    uint8_t happiness{80}; // 0=miserable, 100=ecstatic

    // Hidden stat — drives sick probability, never shown directly
    uint8_t health{100}; // 0=critical, 100=thriving

    // Growth / currency
    uint32_t mycelium{0}; // unbounded; grows with steps, decays without
    uint32_t spores{0};   // tap-to-collect currency

    // State
    uint8_t growthStage{0}; // 0=Colonized, 1=Pinning, 2=Young, 3=Mature
    bool    isSick{false};

    // Fog-of-war discovery: 10 buckets of 10 units (bit i = range [i*10, i*10+10))
    uint16_t substrateSeen{0};
    uint16_t moistureSeen{0};
};

struct Score {
    uint32_t value{0};
};

struct Sprite {
    uint16_t  w{0};
    uint16_t  h{0};
    M5Canvas *data{nullptr}; // borrowed; points to current frame canvas owned by AnimationSet
    uint16_t  color{TFT_TRANSPARENT};
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
struct Spore {};
