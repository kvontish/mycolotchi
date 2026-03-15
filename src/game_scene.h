#pragma once

#include "components.h"
#include "game_over_scene.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

class GameScene : public Scene {
    entt::registry *mRegistry{nullptr};
    int16_t mNextSpawnX{200};
    AnimationSet *mPlayerAnimSet{nullptr};

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

        auto ground = registry.create();
        registry.emplace<Position>(ground, int16_t(0), int16_t(100));
        registry.emplace<Sprite>(ground, uint16_t(160), uint16_t(20), uint16_t(TFT_DARKGREEN));
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
        mPlayerAnimSet = (AnimationSet *)malloc(sizeof(AnimationSet));
        *mPlayerAnimSet = {anims, 2};

        auto player = registry.create();
        registry.emplace<Player>(player);
        registry.emplace<Position>(player, int16_t(10), int16_t(68));
        registry.emplace<Sprite>(player, pw, ph, uint16_t(TFT_TRANSPARENT), nullptr, false);
        registry.emplace<AnimationState>(player, mPlayerAnimSet);
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
        if (mPlayerAnimSet) {
            for (uint8_t i = 0; i < mPlayerAnimSet->animationCount; i++) {
                Animation &anim = mPlayerAnimSet->animations[i];
                for (uint8_t f = 0; f < anim.frameCount; f++)
                    free(anim.frames[f]);
                free(anim.frames);
            }
            free(mPlayerAnimSet->animations);
            free(mPlayerAnimSet);
            mPlayerAnimSet = nullptr;
        }
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
