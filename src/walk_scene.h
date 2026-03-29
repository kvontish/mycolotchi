#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

// --- Components ---

struct WalkState {
    uint32_t     steps{0};
    char         stepsText[24]{};
    entt::entity stepsLabel{entt::null};
};

// --- Systems ---

inline void syncSteps(entt::registry &registry) {
    auto &state = registry.ctx<WalkState>();
    state.steps = sDetectedSteps;
    snprintf(state.stepsText, sizeof(state.stepsText), "steps: %lu", state.steps);
    registry.get<Label>(state.stepsLabel).text = state.stepsText;
}

inline void walkInputSystem(entt::registry *r, const ButtonEvent &e) {
    if (e.button == ButtonEvent::Button::C) r->ctx<SceneManager>().transition(r->ctx<GameMap>().homeScene);
}

class WalkScene : public Scene {
  public:
    void load(entt::registry &registry) override {
        sDetectedSteps      = 0;
        sStepCountingActive = true;
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&walkInputSystem>(&registry);

        auto &state = registry.set<WalkState>();
        snprintf(state.stepsText, sizeof(state.stepsText), "steps: 0");

        auto title = registry.create();
        registry.emplace<Position>(title, int16_t(80), int16_t(40));
        registry.emplace<Label>(title, "walking", uint16_t(TFT_WHITE), uint8_t(2));

        state.stepsLabel = registry.create();
        registry.emplace<Position>(state.stepsLabel, int16_t(80), int16_t(70));
        registry.emplace<Label>(state.stepsLabel, state.stepsText, uint16_t(TFT_WHITE), uint8_t(1));
    }

    void unload(entt::registry &registry) override {
        sStepCountingActive = false;
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&walkInputSystem>(&registry);
        registry.unset<WalkState>();
        registry.clear();
    }

    void update(entt::registry &registry) override { syncSteps(registry); }
};

inline WalkScene walkScene;
