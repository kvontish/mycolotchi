#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>
#include <vector>

// --- Components ---

struct StatusState {
    entt::entity substrateSegs[kFogSegments]{};
    entt::entity substrateCursor{entt::null};
    entt::entity moistureSegs[kFogSegments]{};
    entt::entity moistureCursor{entt::null};
    entt::entity happinessFill{entt::null};
};

// --- Constants ---

static constexpr int16_t  kStatusBarX    = 10;
static constexpr uint16_t kStatusBarMaxW = 140;
static constexpr uint16_t kStatusBarH    = 7;
static constexpr uint16_t kSegW          = kStatusBarMaxW / kFogSegments; // 14px per segment
static constexpr uint16_t kSegFillW      = kSegW - 1;                     // 1px gap between segments
static constexpr uint16_t kCursorW       = 2;
static constexpr uint16_t kCursorH       = kStatusBarH + 2;

static constexpr int16_t kSubstrateBarY = 36;
static constexpr int16_t kMoistureBarY  = 60;
static constexpr int16_t kHappinessBarY = 84;

static uint16_t statusBarColor(uint8_t value) {
    if (value > 50) return TFT_GREEN;
    if (value > 20) return TFT_YELLOW;
    return TFT_RED;
}

// Returns the display color for a revealed fog segment given its bucket midpoint and the ideal range.
static uint16_t fogSegColor(uint8_t bucket, uint8_t idealLo, uint8_t idealHi, uint8_t largeDev) {
    uint8_t mid = bucket * 10 + 5;
    uint8_t dev = rangeDeviation(mid, idealLo, idealHi);
    if (dev == 0) return TFT_GREEN;
    if (dev <= largeDev) return TFT_YELLOW;
    return TFT_RED;
}

// --- Systems ---

inline void updateStatusBars(entt::registry &registry) {
    const auto    &pet   = registry.ctx<Pet>();
    const auto    &state = registry.ctx<StatusState>();
    const Species &sp    = *pet.species;

    // Fog-of-war bars: color each segment based on revealed/ideal-range state
    auto updateFogBar = [&](const entt::entity segs[],
                            entt::entity       cursor,
                            uint8_t            value,
                            uint16_t           seen,
                            uint8_t            idealLo,
                            uint8_t            idealHi,
                            uint8_t            largeDev) {
        for (uint8_t i = 0; i < kFogSegments; i++) {
            uint16_t color = (seen & (1u << i)) ? fogSegColor(i, idealLo, idealHi, largeDev) : uint16_t(TFT_DARKGREY);
            registry.get<Sprite>(segs[i]).color = color;
        }
        // Cursor: position at current value
        int16_t cx                       = kStatusBarX + (int16_t)(value * kStatusBarMaxW / 100) - kCursorW / 2;
        registry.get<Position>(cursor).x = cx;
    };

    updateFogBar(state.substrateSegs,
                 state.substrateCursor,
                 pet.substrate,
                 pet.substrateSeen,
                 sp.substrateIdealLo,
                 sp.substrateIdealHi,
                 sp.substrateLargeDev);

    updateFogBar(state.moistureSegs,
                 state.moistureCursor,
                 pet.moisture,
                 pet.moistureSeen,
                 sp.moistureIdealLo,
                 sp.moistureIdealHi,
                 sp.moistureLargeDev);

    // Simple fill bar for happiness
    auto &happinessSprite = registry.get<Sprite>(state.happinessFill);
    happinessSprite.w     = (uint16_t)((uint32_t)pet.happiness * kStatusBarMaxW / 100);
    happinessSprite.color = statusBarColor(pet.happiness);
}

// --- Input ---

inline void statusInputSystem(entt::registry *r, const ButtonEvent &e) {
    if (e.button == ButtonEvent::Button::C) r->ctx<SceneManager>().popView();
}

// Helper: create the 10 segment entities and 1 cursor entity for a fog-of-war bar.
static void createFogBar(entt::registry &registry,
                         View           *view,
                         entt::entity    segs[],
                         entt::entity   &cursor,
                         int16_t         barY) {
    for (uint8_t i = 0; i < kFogSegments; i++) {
        segs[i] = registry.create();
        registry.emplace<ViewOwner>(segs[i], view);
        registry.emplace<Position>(segs[i], int16_t(kStatusBarX + i * kSegW), barY, 0.0f);
        registry.emplace<Sprite>(segs[i], kSegFillW, kStatusBarH, nullptr, uint16_t(TFT_DARKGREY));
    }
    cursor = registry.create();
    registry.emplace<ViewOwner>(cursor, view);
    registry.emplace<Position>(cursor, int16_t(kStatusBarX), int16_t(barY - 1), 0.0f);
    registry.emplace<Sprite>(cursor, kCursorW, kCursorH, nullptr, uint16_t(TFT_WHITE));
}

class StatusView : public View {
  public:
    void load(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&statusInputSystem>(&registry);

        auto &state = registry.set<StatusState>();

        // Sprite creation order matters: rbegin/rend iterates oldest-first (drawn first = behind).
        // So the black background must be the OLDEST sprite entity — created first.
        const auto &camera = registry.ctx<Camera>();
        auto        bg     = registry.create();
        registry.emplace<ViewOwner>(bg, this);
        registry.emplace<Position>(bg, int16_t(0), int16_t(0), 0.0f);
        registry.emplace<Sprite>(bg, camera.w, camera.h, nullptr, uint16_t(TFT_BLACK));

        // Substrate bar background, segments, and cursor
        auto subBg = registry.create();
        registry.emplace<ViewOwner>(subBg, this);
        registry.emplace<Position>(subBg, int16_t(kStatusBarX), int16_t(kSubstrateBarY), 0.0f);
        registry.emplace<Sprite>(subBg, kStatusBarMaxW, kStatusBarH, nullptr, uint16_t(TFT_DARKGREY));
        createFogBar(registry, this, state.substrateSegs, state.substrateCursor, kSubstrateBarY);

        // Moisture bar background, segments, and cursor
        auto moistBg = registry.create();
        registry.emplace<ViewOwner>(moistBg, this);
        registry.emplace<Position>(moistBg, int16_t(kStatusBarX), int16_t(kMoistureBarY), 0.0f);
        registry.emplace<Sprite>(moistBg, kStatusBarMaxW, kStatusBarH, nullptr, uint16_t(TFT_DARKGREY));
        createFogBar(registry, this, state.moistureSegs, state.moistureCursor, kMoistureBarY);

        // Happiness bar background and fill
        auto happinessBg = registry.create();
        registry.emplace<ViewOwner>(happinessBg, this);
        registry.emplace<Position>(happinessBg, int16_t(kStatusBarX), int16_t(kHappinessBarY), 0.0f);
        registry.emplace<Sprite>(happinessBg, kStatusBarMaxW, kStatusBarH, nullptr, uint16_t(TFT_DARKGREY));

        state.happinessFill = registry.create();
        registry.emplace<ViewOwner>(state.happinessFill, this);
        registry.emplace<Position>(state.happinessFill, int16_t(kStatusBarX), int16_t(kHappinessBarY), 0.0f);
        registry.emplace<Sprite>(state.happinessFill, uint16_t(0), kStatusBarH, nullptr, uint16_t(TFT_GREEN));

        // Labels rendered after sprites so they appear on top
        auto title = registry.create();
        registry.emplace<ViewOwner>(title, this);
        registry.emplace<Position>(title, int16_t(80), int16_t(12), 0.0f);
        registry.emplace<Label>(title, "Status", uint16_t(TFT_WHITE), uint8_t(2));

        auto subLabel = registry.create();
        registry.emplace<ViewOwner>(subLabel, this);
        registry.emplace<Position>(subLabel, int16_t(80), int16_t(28), 0.0f);
        registry.emplace<Label>(subLabel, "Substrate", uint16_t(TFT_WHITE), uint8_t(1));

        auto moistLabel = registry.create();
        registry.emplace<ViewOwner>(moistLabel, this);
        registry.emplace<Position>(moistLabel, int16_t(80), int16_t(52), 0.0f);
        registry.emplace<Label>(moistLabel, "Moisture", uint16_t(TFT_WHITE), uint8_t(1));

        auto happinessLabel = registry.create();
        registry.emplace<ViewOwner>(happinessLabel, this);
        registry.emplace<Position>(happinessLabel, int16_t(80), int16_t(76), 0.0f);
        registry.emplace<Label>(happinessLabel, "Happiness", uint16_t(TFT_WHITE), uint8_t(1));
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&statusInputSystem>(&registry);

        std::vector<entt::entity> toDestroy;
        registry.view<ViewOwner>().each([&](entt::entity e, const ViewOwner &owner) {
            if (owner.view == this) toDestroy.push_back(e);
        });
        registry.destroy(toDestroy.begin(), toDestroy.end());

        registry.unset<StatusState>();
    }

    void update(entt::registry &registry) override { updateStatusBars(registry); }
};

inline StatusView statusView;
