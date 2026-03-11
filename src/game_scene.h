#pragma once

#include <entt/entity/registry.hpp>
#include "components.h"
#include "systems.h"
#include "scene.h"

namespace GameScene
{

    void load(entt::registry &registry)
    {
        auto purpleBox = registry.create();
        registry.emplace<Position>(purpleBox, int16_t(10), int16_t(10), 0.5f);
        registry.emplace<Sprite>(purpleBox, uint16_t(32), uint16_t(32), uint16_t(TFT_PURPLE));

        auto blueBox = registry.create();
        registry.emplace<Position>(blueBox, int16_t(50), int16_t(50));
        registry.emplace<Sprite>(blueBox, uint16_t(32), uint16_t(32), uint16_t(TFT_BLUE));
    }

    void unload(entt::registry &registry)
    {
        registry.clear();
    }

    void update(entt::registry &registry)
    {
        pollInput(registry);
        debugPanCamera(registry);
    }

    constexpr Scene scene{load, unload, update};
}
