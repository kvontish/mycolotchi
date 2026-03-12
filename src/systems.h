#pragma once

#include <M5Unified.h>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include "components.h"
#include "events.h"

inline void renderTiled(const Tiled &tiled, const Sprite &sprite, int16_t baseX, int16_t baseY, uint16_t camW, uint16_t camH, M5Canvas &canvas)
{
    int16_t startX = tiled.x ? (baseX % (int16_t)sprite.w + sprite.w) % sprite.w - sprite.w : baseX;
    int16_t startY = tiled.y ? (baseY % (int16_t)sprite.h + sprite.h) % sprite.h - sprite.h : baseY;
    int16_t endX = tiled.x ? camW : baseX + sprite.w;
    int16_t endY = tiled.y ? camH : baseY + sprite.h;

    for (int16_t ty = startY; ty < endY; ty += sprite.h)
        for (int16_t tx = startX; tx < endX; tx += sprite.w)
            canvas.fillRect(tx, ty, sprite.w, sprite.h, sprite.color);
}

inline void pollInput(entt::registry &registry)
{
    auto &dispatcher = registry.ctx<entt::dispatcher>();

    auto poll = [&](m5::Button_Class &btn, ButtonEvent::Button id)
    {
        if (btn.wasPressed())
            dispatcher.enqueue<ButtonEvent>({id, ButtonEvent::Action::Pressed});
        if (btn.wasReleased())
            dispatcher.enqueue<ButtonEvent>({id, ButtonEvent::Action::Released});
    };

    poll(M5.BtnA, ButtonEvent::Button::A);
    poll(M5.BtnB, ButtonEvent::Button::B);
    poll(M5.BtnC, ButtonEvent::Button::C);

    dispatcher.update<ButtonEvent>();
}

inline void showDebugOverlay(entt::registry &registry)
{
    static uint32_t lastCheck = 0;
    static uint16_t fps = 0;
    static uint16_t frameCount = 0;
    static int8_t batteryLevel = -1;
    static bool charging = false;

    uint32_t now = millis();
    frameCount++;

    if (now - lastCheck >= 1000)
    {
        fps = frameCount * 1000 / (now - lastCheck);
        frameCount = 0;
        batteryLevel = M5.Power.getBatteryLevel();
        charging = M5.Power.isCharging();
        lastCheck = now;
    }

    auto &canvas = registry.ctx<M5Canvas>();
    const auto &camera = registry.ctx<Camera>();

    canvas.setTextSize(1);
    canvas.setTextDatum(TL_DATUM);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);

    // FPS — top left
    canvas.setCursor(2, 2);
    canvas.printf("FPS:%3u", fps);

    // battery icon — top right
    if (batteryLevel >= 0)
    {
        int16_t x = camera.w - 18;
        int16_t y = 2;
        uint16_t color = charging ? TFT_GREEN : (batteryLevel < 20 ? TFT_RED : TFT_WHITE);
        canvas.drawRect(x, y, 14, 7, TFT_WHITE);
        canvas.fillRect(x + 14, y + 2, 2, 3, TFT_WHITE);
        canvas.fillRect(x + 1, y + 1, batteryLevel * 12 / 100, 5, color);
    }
}

inline void debugPanCamera(entt::registry &registry)
{
    static int16_t dir = 1;
    auto &camera = registry.ctx<Camera>();

    camera.x += dir;
    if (camera.x >= 50 || camera.x <= -50)
        dir = -dir;
}

inline void render(entt::registry &registry)
{
    const auto &camera = registry.ctx<Camera>();
    auto &canvas = registry.ctx<M5Canvas>();

    canvas.clear();

    auto view = registry.view<Position, Sprite>();
    view.each([&registry, &camera, &canvas](entt::entity entity, const Position &pos, const Sprite &sprite)
              {
        int16_t baseX = pos.x - (int16_t)(camera.x * pos.parallax);
        int16_t baseY = pos.y - (int16_t)(camera.y * pos.parallax);

        const Tiled *tiled = registry.try_get<Tiled>(entity);
        if (tiled)
        {
            renderTiled(*tiled, sprite, baseX, baseY, camera.w, camera.h, canvas);
            return;
        }

        if (baseX + sprite.w <= 0 || baseX >= camera.w) return;
        if (baseY + sprite.h <= 0 || baseY >= camera.h) return;

        canvas.fillRect(baseX, baseY, sprite.w, sprite.h, sprite.color); });

    auto labelView = registry.view<Position, Label>();
    labelView.each([&canvas](const Position &pos, const Label &label) {
        canvas.setTextSize(label.size);
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextColor(label.color);
        canvas.drawString(label.text, pos.x, pos.y);
    });
}

inline void present(entt::registry &registry)
{
    const auto &camera = registry.ctx<Camera>();
    auto &canvas = registry.ctx<M5Canvas>();

    canvas.pushRotateZoom(
        M5.Display.width() / 2,
        M5.Display.height() / 2,
        0.0f,
        camera.scale,
        camera.scale);
}
