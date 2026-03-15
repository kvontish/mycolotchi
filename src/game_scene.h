#pragma once

#include "components.h"
#include "game_over_scene.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

class GameScene : public Scene {
    entt::registry *mRegistry{nullptr};
    int16_t mNextSpawnX{200};

    void onButton(const ButtonEvent &e) {
        if (e.action != ButtonEvent::Action::Pressed)
            return;

        mRegistry->view<Player, Velocity>().each([this](entt::entity entity, Velocity &vel) {
            if (mRegistry->all_of<Grounded>(entity)) {
                vel.y = -12;
                mRegistry->remove<Grounded>(entity);
            }
        });
    }

  public:
    void load(entt::registry &registry) override {
        mRegistry = &registry;
        mNextSpawnX = 200;
        registry.set<Score>();
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&GameScene::onButton>(this);

        auto &lib = registry.ctx<AssetLibrary>();

        uint16_t bgW, bgH;
        lib.sprites[BgSprite] =
            loadSpriteFromSD("/Environments/Forest of Illusion/Forest of Illusion Pack/Layers/back-120.png", bgW, bgH);
        auto bg = registry.create();
        registry.emplace<Position>(bg, int16_t(0), int16_t(0), 0.3f);
        registry.emplace<Sprite>(bg, bgW, bgH, lib.sprites[BgSprite]);
        registry.emplace<Tiled>(bg, true, false);

        uint16_t midW, midH;
        lib.sprites[MidSprite] = loadSpriteFromSD(
            "/Environments/Forest of Illusion/Forest of Illusion Pack/Layers/middle-120.png", midW, midH);
        auto mid = registry.create();
        registry.emplace<Position>(mid, int16_t(0), int16_t(0), 0.6f);
        registry.emplace<Sprite>(mid, midW, midH, lib.sprites[MidSprite]);
        registry.emplace<Tiled>(mid, true, false);

        uint16_t groundW, groundH;
        lib.sprites[GroundSprite] = loadSpriteFromSD(
            "/Environments/Forest of Illusion/Forest of Illusion Pack/Layers/tiles-120.png", groundW, groundH);
        auto ground = registry.create();
        registry.emplace<Position>(ground, int16_t(0), int16_t(90));
        registry.emplace<Sprite>(ground, groundW, groundH, lib.sprites[GroundSprite]);
        registry.emplace<Hitbox>(ground, uint16_t(0), uint16_t(0), int8_t(0), int8_t(6));
        registry.emplace<Tiled>(ground, true, false);
        registry.emplace<Solid>(ground);

        static const char *runPaths[] = {
            "/Characters/Players/Foxy/Sprites/run/player-run-1.png",
            "/Characters/Players/Foxy/Sprites/run/player-run-2.png",
            "/Characters/Players/Foxy/Sprites/run/player-run-3.png",
            "/Characters/Players/Foxy/Sprites/run/player-run-4.png",
            "/Characters/Players/Foxy/Sprites/run/player-run-5.png",
            "/Characters/Players/Foxy/Sprites/run/player-run-6.png",
        };
        static const char *jumpPaths[] = {
            "/Characters/Players/Foxy/Sprites/jump/player-jump-1.png",
            "/Characters/Players/Foxy/Sprites/jump/player-jump-2.png",
        };

        uint16_t pw = 32, ph = 32;
        Animation *anims = (Animation *)malloc(2 * sizeof(Animation));
        anims[0] = loadAnimationFromSD(runPaths, 6, 100, pw, ph);
        anims[1] = loadAnimationFromSD(jumpPaths, 2, 150, pw, ph);
        anims[1].loop = false;
        lib.animSets[PlayerAnim] = (AnimationSet *)malloc(sizeof(AnimationSet));
        *lib.animSets[PlayerAnim] = {anims, 2, pw, ph};

        static const char *gemPaths[] = {
            "/Props Items and VFX/Sunnyland items/Sprites/gem/gem-1.png",
            "/Props Items and VFX/Sunnyland items/Sprites/gem/gem-2.png",
            "/Props Items and VFX/Sunnyland items/Sprites/gem/gem-3.png",
            "/Props Items and VFX/Sunnyland items/Sprites/gem/gem-4.png",
            "/Props Items and VFX/Sunnyland items/Sprites/gem/gem-5.png",
        };
        uint16_t gw, gh;
        Animation *coinAnims = (Animation *)malloc(sizeof(Animation));
        coinAnims[0] = loadAnimationFromSD(gemPaths, 5, 100, gw, gh);
        lib.animSets[CoinAnim] = (AnimationSet *)malloc(sizeof(AnimationSet));
        *lib.animSets[CoinAnim] = {coinAnims, 1, gw, gh};

        static const char *fireballPaths[] = {
            "/Props Items and VFX/fireball/Sprites/fireball-1.png",
            "/Props Items and VFX/fireball/Sprites/fireball-2.png",
            "/Props Items and VFX/fireball/Sprites/fireball-3.png",
            "/Props Items and VFX/fireball/Sprites/fireball-4.png",
            "/Props Items and VFX/fireball/Sprites/fireball-5.png",
        };
        uint16_t fw, fh;
        Animation *obstacleAnims = (Animation *)malloc(sizeof(Animation));
        obstacleAnims[0] = loadAnimationFromSD(fireballPaths, 5, 80, fw, fh);
        lib.animSets[ObstacleAnim] = (AnimationSet *)malloc(sizeof(AnimationSet));
        *lib.animSets[ObstacleAnim] = {obstacleAnims, 1, fw, fh};

        auto player = registry.create();
        registry.emplace<Player>(player);
        registry.emplace<Position>(player, int16_t(10), int16_t(68));
        registry.emplace<Sprite>(player, lib.animSets[PlayerAnim]->w, lib.animSets[PlayerAnim]->h);
        registry.emplace<Hitbox>(player, uint16_t(20), uint16_t(20), int8_t(5), int8_t(9));
        registry.emplace<AnimationState>(player, lib.animSets[PlayerAnim]);
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
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&GameScene::onButton>(this);
        mRegistry = nullptr;
        auto &lib = registry.ctx<AssetLibrary>();
        for (auto &a : lib.animSets)
            freeAnimationSet(a);
        for (auto &s : lib.sprites)
            freeSprite(s);
        registry.clear();
    }

    void update(entt::registry &registry) override {
        pollInput(registry);
        physics(registry);
        groundCheck(registry);
        updatePlayerAnimation(registry);
        animateSprites(registry);
        checkCollisions(registry, &gameOverScene);
        spawn(registry, mNextSpawnX);
        despawn(registry);
        collectCoins(registry);
        moveCamera(registry);
    }
};

inline GameScene gameScene;
