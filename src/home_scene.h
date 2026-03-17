#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

// --- Components ---

enum HomeSpriteId : uint8_t { HomeBgSprite, HomeMidSprite, HomeGroundSprite };
enum HomeAnimId : uint8_t { HomeShroomAnim };

// --- Systems ---

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

inline void homeInputSystem(entt::registry *registry, const ButtonEvent &e) {
    if (e.button == ButtonEvent::Button::C)
        registry->ctx<SceneManager>().transition(registry->ctx<GameMap>().clockScene);
}

class HomeScene : public Scene {
  public:
    void load(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&homeInputSystem>(&registry);

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
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&homeInputSystem>(&registry);
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
