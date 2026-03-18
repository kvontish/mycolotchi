#pragma once

#include "asset_loader.h"
#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

// --- Components ---

struct GameAssets {
    AnimationSet *player{nullptr};
    AnimationSet *coin{nullptr};
    AnimationSet *obstacle{nullptr};
    M5Canvas *bg{nullptr};
    M5Canvas *mid{nullptr};
    M5Canvas *ground{nullptr};
};

struct SpawnState {
    int16_t nextX{200};
};

// --- Systems ---

inline void gameInputSystem(entt::registry *registry, const ButtonEvent &e) {
    if (registry->ctx<SceneManager>().isViewActive())
        return;
    registry->view<Player, Velocity>().each([registry](entt::entity entity, Velocity &vel) {
        if (registry->all_of<Grounded>(entity)) {
            vel.y = -12;
            registry->remove<Grounded>(entity);
        }
    });
}

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

inline void spawn(entt::registry &registry) {
    auto &state = registry.ctx<SpawnState>();
    const auto &camera = registry.ctx<Camera>();
    const auto &assets = registry.ctx<GameAssets>();
    AnimationSet *coinAnimSet = assets.coin;
    AnimationSet *obstacleAnimSet = assets.obstacle;
    int16_t spawnEdge = camera.x + (int16_t)camera.w;

    if (spawnEdge < state.nextX)
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

    state.nextX = spawnEdge + (int16_t)random(60, 140);
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

inline void checkCollisions(entt::registry &registry) {
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
        registry.ctx<SceneManager>().pushView(registry, registry.ctx<GameMap>().gameOverView);
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

class GameScene : public Scene {
  public:
    void load(entt::registry &registry) override {
        registry.set<SpawnState>();
        registry.set<Score>();
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&gameInputSystem>(&registry);

        auto &assets = registry.set<GameAssets>();

        uint16_t bgW, bgH;
        assets.bg = loadSpriteFromSD(
            "/Environments/Forest of Illusion/Forest of Illusion Pack/Layers/back-120-cc.png", bgW, bgH);
        auto bg = registry.create();
        registry.emplace<Position>(bg, int16_t(0), int16_t(0), 0.3f);
        registry.emplace<Sprite>(bg, bgW, bgH, assets.bg);
        registry.emplace<Tiled>(bg, true, false);

        uint16_t midW, midH;
        assets.mid = loadSpriteFromSD(
            "/Environments/Forest of Illusion/Forest of Illusion Pack/Layers/middle-120-cc.png", midW, midH);
        auto mid = registry.create();
        registry.emplace<Position>(mid, int16_t(0), int16_t(0), 0.6f);
        registry.emplace<Sprite>(mid, midW, midH, assets.mid);
        registry.emplace<Tiled>(mid, true, false);

        uint16_t groundW, groundH;
        assets.ground = loadSpriteFromSD(
            "/Environments/Forest of Illusion/Forest of Illusion Pack/Layers/tiles-120-cc.png", groundW, groundH);
        auto ground = registry.create();
        registry.emplace<Position>(ground, int16_t(0), int16_t(90));
        registry.emplace<Sprite>(ground, groundW, groundH, assets.ground);
        registry.emplace<Hitbox>(ground, uint16_t(0), uint16_t(0), int8_t(0), int8_t(6));
        registry.emplace<Tiled>(ground, true, false);
        registry.emplace<Solid>(ground);

        static const char *runPaths[] = {
            "/Characters/Players/Shroom/Sprites/run/player-run-1.png",
            "/Characters/Players/Shroom/Sprites/run/player-run-2.png",
            "/Characters/Players/Shroom/Sprites/run/player-run-3.png",
            "/Characters/Players/Shroom/Sprites/run/player-run-4.png",
            "/Characters/Players/Shroom/Sprites/run/player-run-5.png",
            "/Characters/Players/Shroom/Sprites/run/player-run-6.png",
            "/Characters/Players/Shroom/Sprites/run/player-run-7.png",
            "/Characters/Players/Shroom/Sprites/run/player-run-8.png",
        };
        static const char *jumpPaths[] = {
            "/Characters/Players/Shroom/Sprites/run/player-run-1.png",
            "/Characters/Players/Shroom/Sprites/run/player-run-2.png",
        };

        uint16_t pw = 22, ph = 22;
        Animation *anims = (Animation *)malloc(2 * sizeof(Animation));
        anims[0] = loadAnimationFromSD(runPaths, 8, 100, pw, ph);
        anims[1] = loadAnimationFromSD(jumpPaths, 2, 150, pw, ph);
        anims[1].loop = false;
        assets.player = (AnimationSet *)malloc(sizeof(AnimationSet));
        *assets.player = {anims, 2, pw, ph};

        static const char *gemPaths[] = {
            "/Props Items and VFX/Sunnyland items/Sprites/gem-cc/gem-1.png",
            "/Props Items and VFX/Sunnyland items/Sprites/gem-cc/gem-2.png",
            "/Props Items and VFX/Sunnyland items/Sprites/gem-cc/gem-3.png",
            "/Props Items and VFX/Sunnyland items/Sprites/gem-cc/gem-4.png",
            "/Props Items and VFX/Sunnyland items/Sprites/gem-cc/gem-5.png",
        };
        uint16_t gw, gh;
        Animation *coinAnims = (Animation *)malloc(sizeof(Animation));
        coinAnims[0] = loadAnimationFromSD(gemPaths, 5, 100, gw, gh);
        assets.coin = (AnimationSet *)malloc(sizeof(AnimationSet));
        *assets.coin = {coinAnims, 1, gw, gh};

        static const char *fireballPaths[] = {
            "/Props Items and VFX/fireball-cc/fireball-1.png",
            "/Props Items and VFX/fireball-cc/fireball-2.png",
            "/Props Items and VFX/fireball-cc/fireball-3.png",
            "/Props Items and VFX/fireball-cc/fireball-4.png",
            "/Props Items and VFX/fireball-cc/fireball-5.png",
        };
        uint16_t fw, fh;
        Animation *obstacleAnims = (Animation *)malloc(sizeof(Animation));
        obstacleAnims[0] = loadAnimationFromSD(fireballPaths, 5, 80, fw, fh);
        assets.obstacle = (AnimationSet *)malloc(sizeof(AnimationSet));
        *assets.obstacle = {obstacleAnims, 1, fw, fh};

        auto player = registry.create();
        registry.emplace<Player>(player);
        registry.emplace<Position>(player, int16_t(10), int16_t(74));
        registry.emplace<Sprite>(player, assets.player->w, assets.player->h);
        registry.emplace<AnimationState>(player, assets.player);
        registry.emplace<Velocity>(player, int16_t(3), int16_t(0));
        registry.emplace<Gravity>(player);
        registry.emplace<Grounded>(player);

        auto &camera = registry.ctx<Camera>();
        camera.x = 0;
        camera.y = 0;
        auto cameraTarget = registry.create();
        registry.emplace<CameraTarget>(cameraTarget);
        registry.emplace<Position>(cameraTarget, camera.x, camera.y);
        registry.emplace<Velocity>(cameraTarget, int16_t(3), int16_t(0));
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&gameInputSystem>(&registry);
        registry.unset<SpawnState>();
        auto &assets = registry.ctx<GameAssets>();
        freeAnimationSet(assets.player);
        freeAnimationSet(assets.coin);
        freeAnimationSet(assets.obstacle);
        freeSprite(assets.bg);
        freeSprite(assets.mid);
        freeSprite(assets.ground);
        registry.unset<GameAssets>();
        registry.clear();
    }

    void update(entt::registry &registry) override {
        pollInput(registry);
        physics(registry);
        groundCheck(registry);
        updatePlayerAnimation(registry);
        animateSprites(registry);
        checkCollisions(registry);
        spawn(registry);
        despawn(registry);
        collectCoins(registry);
        moveCamera(registry);
    }
};

inline GameScene gameScene;
