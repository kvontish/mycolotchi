#pragma once

#include "clock_scene.h"
#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

enum HomeSpriteId : uint8_t { HomeBgSprite, HomeMidSprite, HomeGroundSprite };
enum HomeAnimId : uint8_t { HomeShroomAnim };

class HomeScene : public Scene {
    entt::registry *mRegistry{nullptr};

    void onButton(const ButtonEvent &e) {
        if (e.button == ButtonEvent::Button::C) {
            clockScene.prevScene = this;
            mRegistry->ctx<SceneManager>().transition(&clockScene);
        }
    }

  public:
    void load(entt::registry &registry) override {
        mRegistry = &registry;
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&HomeScene::onButton>(this);

        auto &lib = registry.ctx<AssetLibrary>();

        uint16_t bgW, bgH;
        lib.sprites[HomeBgSprite] = loadSpriteFromSD(
            "/Environments/Forest of Illusion/Forest of Illusion Pack/Layers/back-120-cc.png", bgW, bgH);
        auto bg = registry.create();
        registry.emplace<Position>(bg, int16_t(0), int16_t(0), 0.3f);
        registry.emplace<Sprite>(bg, bgW, bgH, lib.sprites[HomeBgSprite]);
        registry.emplace<Tiled>(bg, true, false);

        uint16_t midW, midH;
        lib.sprites[HomeMidSprite] = loadSpriteFromSD(
            "/Environments/Forest of Illusion/Forest of Illusion Pack/Layers/middle-120-cc.png", midW, midH);
        auto mid = registry.create();
        registry.emplace<Position>(mid, int16_t(0), int16_t(0), 0.6f);
        registry.emplace<Sprite>(mid, midW, midH, lib.sprites[HomeMidSprite]);
        registry.emplace<Tiled>(mid, true, false);

        uint16_t groundW, groundH;
        lib.sprites[HomeGroundSprite] = loadSpriteFromSD(
            "/Environments/Forest of Illusion/Forest of Illusion Pack/Layers/tiles-120-cc.png", groundW, groundH);
        auto ground = registry.create();
        registry.emplace<Position>(ground, int16_t(0), int16_t(90));
        registry.emplace<Sprite>(ground, groundW, groundH, lib.sprites[HomeGroundSprite]);
        registry.emplace<Tiled>(ground, true, false);

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
        uint16_t pw, ph;
        Animation *anims = (Animation *)malloc(sizeof(Animation));
        anims[0] = loadAnimationFromSD(runPaths, 8, 100, pw, ph);
        lib.animSets[HomeShroomAnim] = (AnimationSet *)malloc(sizeof(AnimationSet));
        *lib.animSets[HomeShroomAnim] = {anims, 1, pw, ph};

        auto &camera = registry.ctx<Camera>();
        camera.x = 0;
        camera.y = 0;

        // Mushroom starts at x=10, grounded at y=74 (ground hitbox top 96 - sprite height 22)
        auto shroom = registry.create();
        registry.emplace<Shroom>(shroom);
        registry.emplace<Position>(shroom, int16_t(10), int16_t(74));
        registry.emplace<Sprite>(shroom, pw, ph);
        registry.emplace<AnimationState>(shroom, lib.animSets[HomeShroomAnim]);
        registry.emplace<Velocity>(shroom, int16_t(2), int16_t(0));
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&HomeScene::onButton>(this);
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

        walkAndBounce(registry);

        animateSprites(registry);
    }
};

inline HomeScene homeScene;
