#pragma once

#include "components.h"
#include "events.h"
#include "scene.h"
#include <M5Unified.h>
#include <algorithm>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <esp_sleep.h>
#include <vector>

// =============================================================================
// Input
// =============================================================================

static uint32_t sLastActivityMs = 0;

inline void pollInput(entt::registry &registry) {
    if (gDiscardNextInput) {
        gDiscardNextInput = false;
        for (uint8_t i = 0; i < 3; i++)
            gBtnPressed[i] = false;
        return;
    }

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

    gDiscardNextInput = true;
    sLastActivityMs = millis();
    M5.Display.wakeup();
    M5.Display.setBrightness(brightness);
}

// =============================================================================
// Simulation
// =============================================================================

// --- Physics ---

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
    registry.view<Solid, Position>().each([&registry, &groundY](entt::entity e, const Position &pos) {
        const Hitbox *hb = registry.try_get<Hitbox>(e);
        int16_t y = pos.y + (hb ? hb->oy : int8_t(0));
        if (y < groundY)
            groundY = y;
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

// --- NPC Behavior ---

// Moves the player horizontally and bounces at a one-sprite-width margin from each camera edge.
// Also mirrors the sprite to face the direction of travel.
inline void walkAndBounce(entt::registry &registry) {
    const auto &camera = registry.ctx<Camera>();
    registry.view<Shroom, Position, Velocity, Sprite>().each(
        [&camera](Position &pos, Velocity &vel, const Sprite &sprite) {
        pos.x += vel.x;
        const int16_t margin = (int16_t)sprite.w;
        if (vel.x > 0 && pos.x + (int16_t)sprite.w >= (int16_t)camera.w - margin) {
            pos.x = (int16_t)camera.w - (int16_t)sprite.w - margin;
            vel.x = -vel.x;
        } else if (vel.x < 0 && pos.x <= margin) {
            pos.x = margin;
            vel.x = -vel.x;
        }
        pos.scaleX = vel.x < 0 ? -1 : 1;
    });
}

// --- Camera ---

inline void moveCamera(entt::registry &registry) {
    auto &camera = registry.ctx<Camera>();
    auto view = registry.view<CameraTarget, Position>();
    view.each([&camera](const Position &pos) {
        camera.x = pos.x;
        camera.y = pos.y;
    });
}

// --- World ---

inline void spawn(entt::registry &registry, int16_t &nextSpawnX) {
    const auto &camera = registry.ctx<Camera>();
    const auto &lib = registry.ctx<AssetLibrary>();
    AnimationSet *coinAnimSet = lib.animSets[CoinAnim];
    AnimationSet *obstacleAnimSet = lib.animSets[ObstacleAnim];
    int16_t spawnEdge = camera.x + (int16_t)camera.w;

    if (spawnEdge < nextSpawnX)
        return;

    auto e = registry.create();
    registry.emplace<Despawnable>(e);

    if (random(5) < 2) {                                       // 40% obstacle, 60% coin
        registry.emplace<Position>(e, spawnEdge, int16_t(74)); // bottom capped at groundY - obstacleH
        registry.emplace<Obstacle>(e);
        registry.emplace<Sprite>(e, obstacleAnimSet->w, obstacleAnimSet->h);
        registry.emplace<Hitbox>(e, uint16_t(16), uint16_t(14), int8_t(5), int8_t(6));
        registry.emplace<AnimationState>(e, obstacleAnimSet);
    } else {
        registry.emplace<Position>(
            e, spawnEdge, (int16_t)random(50, 84)); // bottom capped at groundHitboxY(96) - coinH(13) = 83
        registry.emplace<Coin>(e);
        registry.emplace<Sprite>(e, coinAnimSet->w, coinAnimSet->h);
        registry.emplace<AnimationState>(e, coinAnimSet);
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

// --- Game Logic ---

static void getBounds(const Position &pos,
                      const Sprite &sprite,
                      const Hitbox *hb,
                      int16_t &x,
                      int16_t &y,
                      int16_t &w,
                      int16_t &h) {
    if (hb) {
        x = pos.x + hb->ox;
        y = pos.y + hb->oy;
        w = hb->w;
        h = hb->h;
    } else {
        x = pos.x;
        y = pos.y;
        w = sprite.w;
        h = sprite.h;
    }
}

inline void checkCollisions(entt::registry &registry, Scene *onHit) {
    auto players = registry.view<Player, Position, Sprite>();
    auto obstacles = registry.view<Obstacle, Position, Sprite>();

    bool hit = false;
    players.each([&](entt::entity pe, const Position &pp, const Sprite &ps) {
        int16_t px, py, pw, ph;
        getBounds(pp, ps, registry.try_get<Hitbox>(pe), px, py, pw, ph);
        obstacles.each([&](entt::entity oe, const Position &op, const Sprite &os) {
            int16_t ox, oy, ow, oh;
            getBounds(op, os, registry.try_get<Hitbox>(oe), ox, oy, ow, oh);
            if (px < ox + ow && px + pw > ox && py < oy + oh && py + ph > oy)
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
    players.each([&](entt::entity pe, const Position &pp, const Sprite &ps) {
        int16_t px, py, pw, ph;
        getBounds(pp, ps, registry.try_get<Hitbox>(pe), px, py, pw, ph);
        coins.each([&](entt::entity coin, const Position &cp, const Sprite &cs) {
            int16_t cx, cy, cw, ch;
            getBounds(cp, cs, registry.try_get<Hitbox>(coin), cx, cy, cw, ch);
            if (px < cx + cw && px + pw > cx && py < cy + ch && py + ph > cy)
                collected.push_back(coin);
        });
    });

    for (auto e : collected) {
        registry.destroy(e);
        registry.ctx<Score>().value++;
        M5.Speaker.tone(900, 60, 0, true);   // lower note
        M5.Speaker.tone(1400, 80, 0, false); // higher note, queued after
    }
}

// --- Animation ---

// Switches the player between animation 0 (grounded) and animation 1 (airborne)
inline void updatePlayerAnimation(entt::registry &registry) {
    registry.view<Player, AnimationState>().each([&registry](entt::entity e, AnimationState &state) {
        uint8_t desired = registry.all_of<Grounded>(e) ? 0 : 1;
        if (state.currentAnimation != desired) {
            state.currentAnimation = desired;
            state.currentFrame = 0;
            state.lastFrameMs = millis();
        }
    });
}

inline void animateSprites(entt::registry &registry) {
    uint32_t now = millis();
    registry.view<AnimationState, Sprite>().each([now](AnimationState &state, Sprite &sprite) {
        const Animation &anim = state.set->animations[state.currentAnimation];
        if (now - state.lastFrameMs >= anim.frameDurationMs) {
            if (anim.loop || state.currentFrame + 1 < anim.frameCount)
                state.currentFrame = (state.currentFrame + 1) % anim.frameCount;
            state.lastFrameMs = now;
        }
        sprite.data = anim.frames[state.currentFrame];
    });
}

// =============================================================================
// Rendering
// =============================================================================

inline void drawSprite(M5Canvas &canvas,
                       const Sprite &sprite,
                       int16_t x,
                       int16_t y,
                       int8_t scaleX = 1,
                       int8_t scaleY = 1) {
    if (sprite.color != TFT_TRANSPARENT)
        canvas.fillRect(x, y, sprite.w, sprite.h, sprite.color);
    if (!sprite.data)
        return;
    if (scaleX == 1 && scaleY == 1) {
        sprite.data->pushSprite(&canvas, x, y, TFT_TRANSPARENT);
    } else {
        // pushRotateZoom takes the destination center point; use abs(scale) so negative (mirror) scales
        // don't shift the sprite off its bounding box
        float cx = x + sprite.w * (scaleX < 0 ? -scaleX : scaleX) * 0.5f;
        float cy = y + sprite.h * (scaleY < 0 ? -scaleY : scaleY) * 0.5f;
        sprite.data->pushRotateZoom(&canvas, cx, cy, 0.0f, (float)scaleX, (float)scaleY, (uint16_t)TFT_TRANSPARENT);
    }
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
            drawSprite(canvas, sprite, tx, ty); // tiled backgrounds are never scaled
}

inline void drawEntity(entt::registry &registry,
                       const Camera &camera,
                       M5Canvas &canvas,
                       entt::entity entity,
                       const Position &pos,
                       const Sprite &sprite) {
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

    drawSprite(canvas, sprite, baseX, baseY, pos.scaleX, pos.scaleY);
}

inline void render(entt::registry &registry) {
    const auto &camera = registry.ctx<Camera>();
    auto &canvas = registry.ctx<M5Canvas>();

    canvas.clear();

    // Entities are drawn in reverse creation order — later-created entities appear behind earlier ones.
    // Z-order is controlled by the order entities are created in each scene's load().
    auto view = registry.view<Position, Sprite>();
    std::for_each(view.rbegin(), view.rend(), [&](entt::entity e) {
        drawEntity(registry, camera, canvas, e, view.get<Position>(e), view.get<Sprite>(e));
    });

    auto labelView = registry.view<Position, Label>();
    labelView.each([&canvas](const Position &pos, const Label &label) {
        canvas.setTextSize(label.size);
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextColor(label.color);
        canvas.drawString(label.text, pos.x, pos.y);
    });
}

inline void present(entt::registry &registry) {
    const auto &camera = registry.ctx<Camera>();
    auto &canvas = registry.ctx<M5Canvas>();

    const int32_t dstX = (M5.Display.width() - camera.w * camera.scale) / 2;
    const int32_t dstY = (M5.Display.height() - camera.h * camera.scale) / 2;
    const uint16_t *src = static_cast<const uint16_t *>(canvas.getBuffer());

    static uint16_t rowBuf[320]; // one scaled row, bounded by display width

    M5.Display.startWrite();
    M5.Display.setWindow(dstX, dstY, dstX + camera.w * camera.scale - 1, dstY + camera.h * camera.scale - 1);

    for (uint16_t sy = 0; sy < camera.h; sy++) {
        const uint16_t *srcRow = src + sy * camera.w;

        // Expand each pixel horizontally by `scale`
        uint16_t *p = rowBuf;
        for (uint16_t sx = 0; sx < camera.w; sx++) {
            uint16_t px = srcRow[sx];
            for (uint8_t s = 0; s < camera.scale; s++)
                *p++ = px;
        }

        // Repeat the same expanded row `scale` times for vertical scaling
        for (uint8_t s = 0; s < camera.scale; s++)
            M5.Display.writePixels(rowBuf, camera.w * camera.scale);
    }

    M5.Display.endWrite();
}

// --- Debug ---

inline void renderHitboxes(entt::registry &registry) {
    const auto &camera = registry.ctx<Camera>();
    auto &canvas = registry.ctx<M5Canvas>();

    registry.view<Position, Sprite>().each([&](entt::entity e, const Position &pos, const Sprite &sprite) {
        int16_t sx = pos.x - camera.x;
        int16_t sy = pos.y - camera.y;
        // canvas.drawRect(sx, sy, sprite.w, sprite.h, TFT_CYAN);

        if (const Hitbox *hb = registry.try_get<Hitbox>(e))
            canvas.drawRect(sx + hb->ox, sy + hb->oy, hb->w, hb->h, TFT_WHITE);
    });
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
