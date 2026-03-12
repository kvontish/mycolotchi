#pragma once

#include <entt/entity/registry.hpp>
#include "components.h"
#include "systems.h"
#include "scene.h"

class TitleScene : public Scene
{
    entt::registry *mRegistry{nullptr};

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
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&TitleScene::onButton>(this);

        auto title = registry.create();
        registry.emplace<Position>(title, int16_t(80), int16_t(45));
        registry.emplace<Label>(title, "mycolotchi", uint16_t(TFT_WHITE), uint8_t(2));

        auto prompt = registry.create();
        registry.emplace<Position>(prompt, int16_t(80), int16_t(80));
        registry.emplace<Label>(prompt, "tap to start!", uint16_t(TFT_LIGHTGREY), uint8_t(1));
    }

    void unload(entt::registry &registry) override
    {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&TitleScene::onButton>(this);
        mRegistry = nullptr;
        registry.clear();
    }

    void update(entt::registry &registry) override
    {
        pollInput(registry);
    }
};

inline TitleScene titleScene;
