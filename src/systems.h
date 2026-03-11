#pragma once

#include <M5Unified.h>
#include <entt/entity/registry.hpp>
#include "components.h"

inline void renderTiled(const Tiled &tiled, const Sprite &sprite, int16_t baseX, int16_t baseY, uint16_t camW, uint16_t camH)
{
    int16_t startX = tiled.x ? (baseX % (int16_t)sprite.w + sprite.w) % sprite.w - sprite.w : baseX;
    int16_t startY = tiled.y ? (baseY % (int16_t)sprite.h + sprite.h) % sprite.h - sprite.h : baseY;
    int16_t endX   = tiled.x ? camW : baseX + sprite.w;
    int16_t endY   = tiled.y ? camH : baseY + sprite.h;

    for (int16_t ty = startY; ty < endY; ty += sprite.h)
        for (int16_t tx = startX; tx < endX; tx += sprite.w)
            M5.Display.fillRect(tx, ty, sprite.w, sprite.h, sprite.color);
}

inline void render(entt::registry &registry)
{
    const auto &camera = registry.ctx<Camera>();

    auto view = registry.view<Position, Sprite>();
    view.each([&registry, &camera](entt::entity entity, const Position &pos, const Sprite &sprite) {
        int16_t baseX = pos.x - (int16_t)(camera.x * pos.parallax);
        int16_t baseY = pos.y - (int16_t)(camera.y * pos.parallax);

        const Tiled *tiled = registry.try_get<Tiled>(entity);
        if (tiled)
        {
            renderTiled(*tiled, sprite, baseX, baseY, camera.w, camera.h);
            return;
        }

        if (baseX + sprite.w <= 0 || baseX >= camera.w) return;
        if (baseY + sprite.h <= 0 || baseY >= camera.h) return;

        M5.Display.fillRect(baseX, baseY, sprite.w, sprite.h, sprite.color);
    });
}
