#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

// --- Systems ---

inline void titleInputSystem(entt::registry *registry, const ButtonEvent &e) {
    registry->ctx<SceneManager>().transition(registry->ctx<GameMap>().gameScene);
}

class TitleScene : public Scene {
  public:
    void load(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&titleInputSystem>(&registry);

        auto title = registry.create();
        registry.emplace<Position>(title, int16_t(80), int16_t(45));
        registry.emplace<Label>(title, "mycolotchi", uint16_t(TFT_WHITE), uint8_t(2));

        auto prompt = registry.create();
        registry.emplace<Position>(prompt, int16_t(80), int16_t(80));
        registry.emplace<Label>(prompt, "tap to start!", uint16_t(TFT_LIGHTGREY), uint8_t(1));
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&titleInputSystem>(&registry);
        registry.clear();
    }

    void update(entt::registry &registry) override { pollInput(registry); }
};

inline TitleScene titleScene;
