#pragma once

#include <entt/entity/registry.hpp>
#include "components.h"
#include "systems.h"
#include "scene.h"

class GameScene : public Scene
{
    entt::registry *mRegistry{nullptr};

    void onButton(const ButtonEvent &e)
    {
        if (e.action != ButtonEvent::Action::Pressed)
            return;

        // TODO jump
    }

public:
    void load(entt::registry &registry) override
    {
        mRegistry = &registry;
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&GameScene::onButton>(this);

        auto ground = registry.create();
        registry.emplace<Position>(ground, int16_t(0), int16_t(100));
        registry.emplace<Sprite>(ground, uint16_t(160), uint16_t(20), uint16_t(TFT_DARKGREEN));
        registry.emplace<Tiled>(ground, true, false);

        auto player = registry.create();
        registry.emplace<Player>(player);
        registry.emplace<Position>(player, int16_t(10), int16_t(68));
        registry.emplace<Sprite>(player, uint16_t(32), uint16_t(32), uint16_t(TFT_PURPLE));
        registry.emplace<Velocity>(player, int16_t(1), int16_t(0));

        auto camera = registry.ctx<Camera>();
        auto cameraTarget = registry.create();
        registry.emplace<CameraTarget>(cameraTarget);
        registry.emplace<Position>(cameraTarget, camera.x, camera.y);
        registry.emplace<Velocity>(cameraTarget, int16_t(1), int16_t(0));
    }

    void unload(entt::registry &registry) override
    {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&GameScene::onButton>(this);
        mRegistry = nullptr;
        registry.clear();
    }

    void update(entt::registry &registry) override
    {
        pollInput(registry);
        physics(registry);
        moveCamera(registry);
    }
};

inline GameScene gameScene;
