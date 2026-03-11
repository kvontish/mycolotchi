#pragma once

#include <M5Unified.h>
#include <entt/entity/registry.hpp>
#include "components.h"

inline void render(entt::registry &registry)
{
    auto view = registry.view<Position, Sprite>();
    view.each([](const Position &pos, const Sprite &sprite) {
        M5.Display.fillRect(pos.x, pos.y, sprite.w, sprite.h, sprite.color);
    });
}
