#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>
#include <vector>

// --- Components ---

struct GameOverState {
    char scoreText[24]{};
};

// --- Systems ---

inline void gameOverInputSystem(entt::registry *r, const ButtonEvent &) {
    r->ctx<SceneManager>().transition(r->ctx<GameMap>().homeScene);
}

class GameOverView : public View {
  public:
    void load(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&gameOverInputSystem>(&registry);

        auto &camera = registry.ctx<Camera>();
        camera.x = camera.y = 0; // reset camera position on game over

        auto    &state = registry.set<GameOverState>();
        uint32_t score = 0;
        if (auto *s = registry.try_ctx<Score>()) score = s->value;

        auto &pet     = registry.ctx<Pet>();
        pet.happiness = (uint8_t)min((int)pet.happiness + (int)score, 100);
        snprintf(state.scoreText, sizeof(state.scoreText), "score: %lu", score);

        auto title = registry.create();
        registry.emplace<ViewOwner>(title, this);
        registry.emplace<Position>(title, int16_t(80), int16_t(40));
        registry.emplace<Label>(title, "game over", uint16_t(TFT_RED), uint8_t(2));

        auto scoreLabel = registry.create();
        registry.emplace<ViewOwner>(scoreLabel, this);
        registry.emplace<Position>(scoreLabel, int16_t(80), int16_t(65));
        registry.emplace<Label>(scoreLabel, state.scoreText, uint16_t(TFT_WHITE), uint8_t(1));

        auto prompt = registry.create();
        registry.emplace<ViewOwner>(prompt, this);
        registry.emplace<Position>(prompt, int16_t(80), int16_t(90));
        registry.emplace<Label>(prompt, "tap to continue", uint16_t(TFT_LIGHTGREY), uint8_t(1));
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&gameOverInputSystem>(&registry);

        std::vector<entt::entity> toDestroy;
        registry.view<ViewOwner>().each([&](entt::entity e, const ViewOwner &owner) {
            if (owner.view == this) toDestroy.push_back(e);
        });
        registry.destroy(toDestroy.begin(), toDestroy.end());

        registry.unset<GameOverState>();
    }

    void update(entt::registry &registry) override { pollInput(registry); }
};

inline GameOverView gameOverView;
