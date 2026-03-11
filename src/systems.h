#pragma once

#include <M5Unified.h>
#include <entt/entity/registry.hpp>
#include "components.h"

inline void render(entt::registry &registry)
{
    const auto &camera = registry.ctx<Camera>();

    auto view = registry.view<Position, Sprite>();
    view.each([&camera](const Position &pos, const Sprite &sprite) {
        int16_t screenX = pos.x - camera.x;
        int16_t screenY = pos.y - camera.y;

        if (screenX + sprite.w <= 0 || screenX >= camera.w) return;
        if (screenY + sprite.h <= 0 || screenY >= camera.h) return;

        M5.Display.fillRect(screenX, screenY, sprite.w, sprite.h, sprite.color);
    });
}
