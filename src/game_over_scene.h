#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

// --- Components ---

struct GameOverState {
    char scoreText[24]{};
};

// --- Systems ---

inline void gameOverInputSystem(entt::registry *registry, const ButtonEvent &e) {
    registry->ctx<SceneManager>().transition(registry->ctx<GameMap>().gameScene);
}

class GameOverScene : public Scene {
  public:
    void load(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&gameOverInputSystem>(&registry);

        auto &state = registry.set<GameOverState>();
        uint32_t score = 0;
        if (auto *s = registry.try_ctx<Score>())
            score = s->value;
        snprintf(state.scoreText, sizeof(state.scoreText), "score: %lu", score);

        auto title = registry.create();
        registry.emplace<Position>(title, int16_t(80), int16_t(40));
        registry.emplace<Label>(title, "game over", uint16_t(TFT_RED), uint8_t(2));

        auto scoreLabel = registry.create();
        registry.emplace<Position>(scoreLabel, int16_t(80), int16_t(65));
        registry.emplace<Label>(scoreLabel, state.scoreText, uint16_t(TFT_WHITE), uint8_t(1));

        auto prompt = registry.create();
        registry.emplace<Position>(prompt, int16_t(80), int16_t(90));
        registry.emplace<Label>(prompt, "tap to retry", uint16_t(TFT_LIGHTGREY), uint8_t(1));
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&gameOverInputSystem>(&registry);
        registry.unset<GameOverState>();
        registry.clear();
    }
};

inline GameOverScene gameOverScene;
