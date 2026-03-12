#pragma once

#include <entt/entity/registry.hpp>
#include "components.h"
#include "systems.h"
#include "scene.h"

class GameOverScene : public Scene
{
    entt::registry *mRegistry{nullptr};
    char mScoreText[24]{};

    void onButton(const ButtonEvent &e)
    {
        if (e.action != ButtonEvent::Action::Pressed)
            return;
        mRegistry->ctx<SceneManager>().transition(nextScene);
    }

public:
    Scene *nextScene{nullptr};

    void load(entt::registry &registry) override
    {
        mRegistry = &registry;
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&GameOverScene::onButton>(this);

        uint32_t score = 0;
        if (auto *s = registry.try_ctx<Score>()) score = s->value;
        snprintf(mScoreText, sizeof(mScoreText), "score: %lu", score);

        auto title = registry.create();
        registry.emplace<Position>(title, int16_t(80), int16_t(40));
        registry.emplace<Label>(title, "game over", uint16_t(TFT_RED), uint8_t(2));

        auto scoreLabel = registry.create();
        registry.emplace<Position>(scoreLabel, int16_t(80), int16_t(65));
        registry.emplace<Label>(scoreLabel, mScoreText, uint16_t(TFT_WHITE), uint8_t(1));

        auto prompt = registry.create();
        registry.emplace<Position>(prompt, int16_t(80), int16_t(90));
        registry.emplace<Label>(prompt, "tap to retry", uint16_t(TFT_LIGHTGREY), uint8_t(1));
    }

    void unload(entt::registry &registry) override
    {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&GameOverScene::onButton>(this);
        mRegistry = nullptr;
        registry.clear();
    }

    void update(entt::registry &registry) override
    {
        pollInput(registry);
    }
};

inline GameOverScene gameOverScene;
