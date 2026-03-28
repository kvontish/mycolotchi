#pragma once

#include "asset_loader.h"
#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

// --- Components ---

struct HomeAssets {
    AnimationSet *shroom{nullptr};
    M5Canvas     *bg{nullptr};
    M5Canvas     *mid{nullptr};
    M5Canvas     *ground{nullptr};
};

struct HomeSporeSpawner {
    uint32_t lastSpawnMs{0};
};

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

inline void spawnSpores(entt::registry &registry) {
    const auto &pet     = registry.ctx<Pet>();
    auto       &spawner = registry.ctx<HomeSporeSpawner>();
    uint32_t    now     = millis();

    // Base interval: generate one spore per spore generation interval from stat system
    // At minimum mycelium: sporeBaseMs (5 min); higher mycelium = faster (proportional)
    const Species &sp            = *pet.species;
    uint32_t       sporeInterval = sp.sporeBaseMs / (1 + pet.mycelium / sp.sporeMyceliumDiv);

    if (now - spawner.lastSpawnMs >= sporeInterval) {
        // Count spore entities currently on screen to prevent clutter
        uint32_t sporeCount = 0;
        registry.view<Spore>().each([&](entt::entity e) { sporeCount++; });

        if (sporeCount < 50) { // cap visual spore entities at 50
            auto e = registry.create();
            registry.emplace<Spore>(e);
            registry.emplace<Position>(e, int16_t(random(20, 140)), int16_t(random(50, 80)));
            registry.emplace<Sprite>(e, uint16_t(8), uint16_t(8), nullptr, uint16_t(TFT_WHITE));
        }
        spawner.lastSpawnMs = now;
    }
}

inline void homeTouchSystem(entt::registry *registry, const TouchEvent &e) {
    auto       &pet    = registry->ctx<Pet>();
    const auto &camera = registry->ctx<Camera>();

    // Convert touch coordinates from screen space to camera space
    int16_t displayW = M5.Display.width();
    int16_t displayH = M5.Display.height();
    int16_t scaledW  = camera.w * camera.scale;
    int16_t scaledH  = camera.h * camera.scale;
    int16_t offsetX  = (displayW - scaledW) / 2;
    int16_t offsetY  = (displayH - scaledH) / 2;

    // Only process touches within the camera viewport
    if (e.x < offsetX || e.x >= offsetX + scaledW || e.y < offsetY || e.y >= offsetY + scaledH) return;

    // Convert display coordinates to camera coordinates
    int16_t cameraX = (e.x - offsetX) / camera.scale;
    int16_t cameraY = (e.y - offsetY) / camera.scale;

    // Check collision with spores and collect
    std::vector<entt::entity> collected;
    registry->view<Spore, Position, Sprite>().each(
        [&](entt::entity spore, const Position &sporePos, const Sprite &sporeSprite) {
        int16_t sx = sporePos.x;
        int16_t sy = sporePos.y;
        int16_t sw = sporeSprite.w;
        int16_t sh = sporeSprite.h;

        // Check if touch is within spore bounds
        if (cameraX >= sx && cameraX < sx + sw && cameraY >= sy && cameraY < sy + sh) {
            collected.push_back(spore);
        }
    });

    for (auto entity : collected) {
        registry->destroy(entity);
        pet.spores++;
    }
}

inline void homeInputSystem(entt::registry *registry, const ButtonEvent &e) {
    if (e.button == ButtonEvent::Button::A)
        registry->ctx<SceneManager>().transition(registry->ctx<GameMap>().menuScene);
    else if (e.button == ButtonEvent::Button::C)
        registry->ctx<SceneManager>().transition(registry->ctx<GameMap>().clockScene);
}

class HomeScene : public Scene {
  public:
    void load(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&homeInputSystem>(&registry);
        registry.ctx<entt::dispatcher>().sink<TouchEvent>().connect<&homeTouchSystem>(&registry);

        registry.set<HomeSporeSpawner>();
        auto &assets = registry.set<HomeAssets>();

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
        uint16_t   pw, ph;
        Animation *anims = (Animation *)malloc(sizeof(Animation));
        anims[0]         = loadAnimationFromSD(runPaths, 8, 100, pw, ph);
        assets.shroom    = (AnimationSet *)malloc(sizeof(AnimationSet));
        *assets.shroom   = {anims, 1, pw, ph};

        auto &camera = registry.ctx<Camera>();
        camera.x     = 0;
        camera.y     = 0;

        // Mushroom starts at x=10, grounded at y=74 (ground hitbox top 96 - sprite height 22)
        auto shroom = registry.create();
        registry.emplace<Shroom>(shroom);
        registry.emplace<Position>(shroom, int16_t(10), int16_t(74));
        registry.emplace<Sprite>(shroom, pw, ph);
        registry.emplace<AnimationState>(shroom, assets.shroom);
        registry.emplace<Velocity>(shroom, int16_t(2), int16_t(0));
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&homeInputSystem>(&registry);
        registry.ctx<entt::dispatcher>().sink<TouchEvent>().disconnect<&homeTouchSystem>(&registry);
        auto &assets = registry.ctx<HomeAssets>();
        freeAnimationSet(assets.shroom);
        freeSprite(assets.bg);
        freeSprite(assets.mid);
        freeSprite(assets.ground);
        registry.unset<HomeAssets>();
        registry.unset<HomeSporeSpawner>();
        registry.clear();
    }

    void update(entt::registry &registry) override {
        pollInput(registry);
        spawnSpores(registry);
        walkAndBounce(registry);
        animateSprites(registry);
    }
};

inline HomeScene homeScene;
