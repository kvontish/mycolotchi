#pragma once

#include "components.h"
#include "events.h"
#include "scene.h"
#include <M5Unified.h>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <esp_sleep.h>
#include <vector>

static uint32_t sLastActivityMs = 0;

inline void pollInput(entt::registry &registry) {
    auto &dispatcher = registry.ctx<entt::dispatcher>();

    for (uint8_t i = 0; i < 3; i++) {
        if (gBtnPressed[i]) {
            gBtnPressed[i] = false;
            sLastActivityMs = millis();
            dispatcher.enqueue<ButtonEvent>({ButtonEvent::Button(i), ButtonEvent::Action::Pressed});
        }
    }

    dispatcher.update<ButtonEvent>();
}

// FT6336 touch controller INT pin — fires low on any touch, usable as ext0 wake source
static constexpr gpio_num_t kTouchIntPin = GPIO_NUM_39;
static constexpr uint32_t kSleepTimeoutMs = 30000;

inline void sleepIfInactive() {
    if (millis() - sLastActivityMs < kSleepTimeoutMs)
        return;

    uint8_t brightness = M5.Display.getBrightness();
    M5.Display.setBrightness(0);
    M5.Display.sleep();

    esp_sleep_enable_ext0_wakeup(kTouchIntPin, 0);
    esp_light_sleep_start();
    // Execution resumes here after any touch

    sLastActivityMs = millis();
    M5.Display.wakeup();
    M5.Display.setBrightness(brightness);
}

inline void showDebugOverlay(entt::registry &registry) {
    static uint32_t lastCheck = 0;
    static uint16_t fps = 0;
    static uint16_t frameCount = 0;
    static int8_t batteryLevel = -1;
    static bool charging = false;

    uint32_t now = millis();
    frameCount++;

    if (now - lastCheck >= 1000) {
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
    if (batteryLevel >= 0) {
        int16_t x = camera.w - 18;
        int16_t y = 2;
        uint16_t color = charging ? TFT_GREEN : (batteryLevel < 20 ? TFT_RED : TFT_WHITE);
        canvas.drawRect(x, y, 14, 7, TFT_WHITE);
        canvas.fillRect(x + 14, y + 2, 2, 3, TFT_WHITE);
        canvas.fillRect(x + 1, y + 1, batteryLevel * 12 / 100, 5, color);
    }
}

inline void physics(entt::registry &registry) {
    const int16_t gravity = 1;

    auto view = registry.view<Position, Velocity>();
    view.each([&registry, gravity](entt::entity entity, Position &pos, Velocity &vel) {
        if (registry.all_of<Gravity>(entity))
            vel.y += gravity;
        pos.x += vel.x;
        pos.y += vel.y;
    });
}

inline void groundCheck(entt::registry &registry) {
    int16_t groundY = INT16_MAX;
    registry.view<Solid, Position>().each([&groundY](const Position &pos) {
        if (pos.y < groundY)
            groundY = pos.y;
    });

    registry.view<Player, Position, Velocity, Sprite>().each(
        [&registry, groundY](entt::entity entity, Position &pos, Velocity &vel, const Sprite &sprite) {
        if (vel.y >= 0 && pos.y + (int16_t)sprite.h >= groundY) {
            pos.y = groundY - (int16_t)sprite.h;
            vel.y = 0;
            registry.emplace_or_replace<Grounded>(entity);
        } else if (registry.all_of<Grounded>(entity)) {
            registry.remove<Grounded>(entity);
        }
    });
}

inline void moveCamera(entt::registry &registry) {
    auto &camera = registry.ctx<Camera>();
    auto view = registry.view<CameraTarget, Position>();
    view.each([&camera](const Position &pos) {
        camera.x = pos.x;
        camera.y = pos.y;
    });
}

inline void spawn(entt::registry &registry, int16_t &nextSpawnX) {
    const auto &camera = registry.ctx<Camera>();
    int16_t spawnEdge = camera.x + (int16_t)camera.w;

    if (spawnEdge < nextSpawnX)
        return;

    auto e = registry.create();
    registry.emplace<Position>(e, spawnEdge, (int16_t)random(70, 90));
    registry.emplace<Despawnable>(e);

    if (random(2) == 0) {
        registry.emplace<Obstacle>(e);
        registry.emplace<Sprite>(e, uint16_t(10), uint16_t(10), uint16_t(TFT_RED));
    } else {
        registry.emplace<Coin>(e);
        registry.emplace<Sprite>(e, uint16_t(10), uint16_t(10), uint16_t(TFT_GOLD));
    }

    nextSpawnX = spawnEdge + (int16_t)random(60, 140);
}

inline void despawn(entt::registry &registry) {
    const auto &camera = registry.ctx<Camera>();

    std::vector<entt::entity> toDestroy;
    registry.view<Despawnable, Position, Sprite>().each([&](entt::entity e, const Position &pos, const Sprite &sprite) {
        if (pos.x + (int16_t)sprite.w < camera.x - 16)
            toDestroy.push_back(e);
    });

    for (auto e : toDestroy)
        registry.destroy(e);
}

inline void checkCollisions(entt::registry &registry, Scene *onHit) {
    auto players = registry.view<Player, Position, Sprite>();
    auto obstacles = registry.view<Obstacle, Position, Sprite>();

    bool hit = false;
    players.each([&](const Position &pp, const Sprite &ps) {
        obstacles.each([&](const Position &op, const Sprite &os) {
            if (pp.x < op.x + (int16_t)os.w && pp.x + (int16_t)ps.w > op.x && pp.y < op.y + (int16_t)os.h &&
                pp.y + (int16_t)ps.h > op.y)
                hit = true;
        });
    });

    if (hit)
        registry.ctx<SceneManager>().transition(onHit);
}

inline void collectCoins(entt::registry &registry) {
    auto players = registry.view<Player, Position, Sprite>();
    auto coins = registry.view<Coin, Position, Sprite>();

    std::vector<entt::entity> collected;
    players.each([&](const Position &pp, const Sprite &ps) {
        coins.each([&](entt::entity coin, const Position &cp, const Sprite &cs) {
            if (pp.x < cp.x + (int16_t)cs.w && pp.x + (int16_t)ps.w > cp.x && pp.y < cp.y + (int16_t)cs.h &&
                pp.y + (int16_t)ps.h > cp.y)
                collected.push_back(coin);
        });
    });

    for (auto e : collected) {
        registry.destroy(e);
        registry.ctx<Score>().value++;
    }
}

inline void freeSprites(entt::registry &registry) {
    registry.view<Sprite>().each([](Sprite &sprite) {
        if (sprite.data) {
            free(const_cast<uint16_t *>(sprite.data));
            sprite.data = nullptr;
        }
    });
}

inline void drawSprite(M5Canvas &canvas, const Sprite &sprite, int16_t x, int16_t y) {
    if (sprite.color != TFT_TRANSPARENT)
        canvas.fillRect(x, y, sprite.w, sprite.h, sprite.color);
    if (sprite.data)
        canvas.pushImage(x, y, sprite.w, sprite.h, sprite.data);
}

inline void renderTiled(const Tiled &tiled,
                        const Sprite &sprite,
                        int16_t baseX,
                        int16_t baseY,
                        uint16_t camW,
                        uint16_t camH,
                        M5Canvas &canvas) {
    int16_t startX = tiled.x ? (baseX % (int16_t)sprite.w + sprite.w) % sprite.w - sprite.w : baseX;
    int16_t startY = tiled.y ? (baseY % (int16_t)sprite.h + sprite.h) % sprite.h - sprite.h : baseY;
    int16_t endX = tiled.x ? camW : baseX + sprite.w;
    int16_t endY = tiled.y ? camH : baseY + sprite.h;

    for (int16_t ty = startY; ty < endY; ty += sprite.h)
        for (int16_t tx = startX; tx < endX; tx += sprite.w)
            drawSprite(canvas, sprite, tx, ty);
}

inline void render(entt::registry &registry) {
    const auto &camera = registry.ctx<Camera>();
    auto &canvas = registry.ctx<M5Canvas>();

    canvas.clear();

    auto view = registry.view<Position, Sprite>();
    view.each([&registry, &camera, &canvas](entt::entity entity, const Position &pos, const Sprite &sprite) {
        int16_t baseX = pos.x - (int16_t)(camera.x * pos.parallax);
        int16_t baseY = pos.y - (int16_t)(camera.y * pos.parallax);

        const Tiled *tiled = registry.try_get<Tiled>(entity);
        if (tiled) {
            renderTiled(*tiled, sprite, baseX, baseY, camera.w, camera.h, canvas);
            return;
        }

        if (baseX + sprite.w <= 0 || baseX >= camera.w)
            return;
        if (baseY + sprite.h <= 0 || baseY >= camera.h)
            return;

        drawSprite(canvas, sprite, baseX, baseY);
    });

    auto labelView = registry.view<Position, Label>();
    labelView.each([&canvas](const Position &pos, const Label &label) {
        canvas.setTextSize(label.size);
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextColor(label.color);
        canvas.drawString(label.text, pos.x, pos.y);
    });
}

inline void pushScaled(M5Canvas &canvas, uint16_t srcW, uint16_t srcH, uint8_t scale) {
    const int32_t dstX = (M5.Display.width() - srcW * scale) / 2;
    const int32_t dstY = (M5.Display.height() - srcH * scale) / 2;

    const uint16_t *src = static_cast<const uint16_t *>(canvas.getBuffer());

    static uint16_t rowBuf[320]; // one scaled row, bounded by display width

    M5.Display.startWrite();
    M5.Display.setWindow(dstX, dstY, dstX + srcW * scale - 1, dstY + srcH * scale - 1);

    for (uint16_t sy = 0; sy < srcH; sy++) {
        const uint16_t *srcRow = src + sy * srcW;

        // Expand each pixel horizontally by `scale`
        uint16_t *p = rowBuf;
        for (uint16_t sx = 0; sx < srcW; sx++) {
            uint16_t px = srcRow[sx];
            for (uint8_t s = 0; s < scale; s++)
                *p++ = px;
        }

        // Repeat the same expanded row `scale` times for vertical scaling
        for (uint8_t s = 0; s < scale; s++)
            M5.Display.writePixels(rowBuf, srcW * scale);
    }

    M5.Display.endWrite();
}

inline void present(entt::registry &registry) {
    const auto &camera = registry.ctx<Camera>();
    auto &canvas = registry.ctx<M5Canvas>();

    pushScaled(canvas, camera.w, camera.h, camera.scale);
}
