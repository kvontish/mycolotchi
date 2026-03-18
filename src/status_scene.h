#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>
#include <vector>

// --- Components ---

struct StatusState {
    entt::entity hungerFill{entt::null};
    entt::entity happinessFill{entt::null};
};

// --- Constants ---

static constexpr int16_t kStatusBarX = 10;
static constexpr uint16_t kStatusBarMaxW = 140;
static constexpr uint16_t kStatusBarH = 7;

static uint16_t statusBarColor(uint8_t value) {
    if (value > 50)
        return TFT_GREEN;
    if (value > 20)
        return TFT_YELLOW;
    return TFT_RED;
}

// --- Systems ---

inline void updateStatusBars(entt::registry &registry) {
    const auto &pet = registry.ctx<Pet>();
    const auto &state = registry.ctx<StatusState>();

    auto &hungerSprite = registry.get<Sprite>(state.hungerFill);
    hungerSprite.w = (uint16_t)((uint32_t)pet.hunger * kStatusBarMaxW / 100);
    hungerSprite.color = statusBarColor(pet.hunger);

    auto &happinessSprite = registry.get<Sprite>(state.happinessFill);
    happinessSprite.w = (uint16_t)((uint32_t)pet.happiness * kStatusBarMaxW / 100);
    happinessSprite.color = statusBarColor(pet.happiness);
}

// --- Input ---

inline void statusInputSystem(entt::registry *r, const ButtonEvent &e) {
    if (e.button == ButtonEvent::Button::C)
        r->ctx<SceneManager>().popView();
}

class StatusView : public View {
  public:
    void load(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&statusInputSystem>(&registry);

        auto &state = registry.set<StatusState>();

        // Sprite creation order matters: rbegin/rend iterates oldest-first (drawn first = behind).
        // So the black background must be the OLDEST sprite entity — created first.
        const auto &camera = registry.ctx<Camera>();
        auto bg = registry.create();
        registry.emplace<ViewOwner>(bg, this);
        registry.emplace<Position>(bg, int16_t(0), int16_t(0), 0.0f);
        registry.emplace<Sprite>(bg, camera.w, camera.h, nullptr, uint16_t(TFT_BLACK));

        // Bar bgs before fills so fills render on top
        auto hungerBg = registry.create();
        registry.emplace<ViewOwner>(hungerBg, this);
        registry.emplace<Position>(hungerBg, int16_t(kStatusBarX), int16_t(44), 0.0f);
        registry.emplace<Sprite>(hungerBg, kStatusBarMaxW, kStatusBarH, nullptr, uint16_t(TFT_DARKGREY));

        state.hungerFill = registry.create();
        registry.emplace<ViewOwner>(state.hungerFill, this);
        registry.emplace<Position>(state.hungerFill, int16_t(kStatusBarX), int16_t(44), 0.0f);
        registry.emplace<Sprite>(state.hungerFill, uint16_t(0), kStatusBarH, nullptr, uint16_t(TFT_GREEN));

        auto happinessBg = registry.create();
        registry.emplace<ViewOwner>(happinessBg, this);
        registry.emplace<Position>(happinessBg, int16_t(kStatusBarX), int16_t(74), 0.0f);
        registry.emplace<Sprite>(happinessBg, kStatusBarMaxW, kStatusBarH, nullptr, uint16_t(TFT_DARKGREY));

        state.happinessFill = registry.create();
        registry.emplace<ViewOwner>(state.happinessFill, this);
        registry.emplace<Position>(state.happinessFill, int16_t(kStatusBarX), int16_t(74), 0.0f);
        registry.emplace<Sprite>(state.happinessFill, uint16_t(0), kStatusBarH, nullptr, uint16_t(TFT_GREEN));

        // Labels are rendered in a separate pass after sprites, so creation order doesn't
        // affect their layering relative to sprites.
        auto title = registry.create();
        registry.emplace<ViewOwner>(title, this);
        registry.emplace<Position>(title, int16_t(80), int16_t(14), 0.0f);
        registry.emplace<Label>(title, "Status", uint16_t(TFT_WHITE), uint8_t(2));

        auto hungerLabel = registry.create();
        registry.emplace<ViewOwner>(hungerLabel, this);
        registry.emplace<Position>(hungerLabel, int16_t(80), int16_t(36), 0.0f);
        registry.emplace<Label>(hungerLabel, "Hunger", uint16_t(TFT_WHITE), uint8_t(1));

        auto happinessLabel = registry.create();
        registry.emplace<ViewOwner>(happinessLabel, this);
        registry.emplace<Position>(happinessLabel, int16_t(80), int16_t(66), 0.0f);
        registry.emplace<Label>(happinessLabel, "Happiness", uint16_t(TFT_WHITE), uint8_t(1));
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&statusInputSystem>(&registry);

        std::vector<entt::entity> toDestroy;
        registry.view<ViewOwner>().each([&](entt::entity e, const ViewOwner &owner) {
            if (owner.view == this)
                toDestroy.push_back(e);
        });
        registry.destroy(toDestroy.begin(), toDestroy.end());

        registry.unset<StatusState>();
    }

    void update(entt::registry &registry) override {
        pollInput(registry);
        updateStatusBars(registry);
    }
};

inline StatusView statusView;
