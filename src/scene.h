#pragma once

#include <M5Unified.h>
#include <SD.h>
#include <entt/entity/registry.hpp>

struct Scene {
    virtual void load(entt::registry &) {}
    virtual void unload(entt::registry &) {}
    virtual void update(entt::registry &) {}
    virtual ~Scene() = default;

    // Loads a PNG from the SD card into a heap-allocated M5Canvas sprite.
    // outW and outH are set to the image dimensions on success.
    // Returns nullptr on failure. Caller must call deleteSprite() then delete on the canvas in unload().
    static M5Canvas *loadSpriteFromSD(const char *path, uint16_t &outW, uint16_t &outH) {
        // PNG IHDR chunk stores width and height as big-endian uint32 at bytes 16-23
        File f = SD.open(path);
        if (!f)
            return nullptr;
        f.seek(16);
        uint32_t w = 0, h = 0;
        for (int i = 0; i < 4; i++)
            w = (w << 8) | f.read();
        for (int i = 0; i < 4; i++)
            h = (h << 8) | f.read();
        f.seek(0); // rewind for decode
        outW = (uint16_t)w;
        outH = (uint16_t)h;

        // Decode PNG into a heap-allocated canvas sprite so pushSprite() can be used at draw time,
        // which handles LovyanGFX's internal byte-swap and transparent color comparison correctly.
        M5Canvas *canvas = new M5Canvas(&M5.Display);
        canvas->createSprite(outW, outH);
        if (!canvas->getBuffer()) {
            delete canvas;
            f.close();
            return nullptr;
        }

        canvas->fillSprite(TFT_TRANSPARENT);
        canvas->drawPng(&f, 0, 0, outW, outH);
        f.close();
        return canvas;
    }

    // Loads multiple PNGs from the SD card into a heap-allocated Animation.
    // outW and outH are set from the first frame. Caller must free via freeAnimationSet().
    static Animation loadAnimationFromSD(const char *const *paths,
                                         uint8_t count,
                                         uint16_t frameDurationMs,
                                         uint16_t &outW,
                                         uint16_t &outH) {
        M5Canvas **frames = (M5Canvas **)malloc(count * sizeof(M5Canvas *));
        outW = 0;
        outH = 0;
        for (uint8_t i = 0; i < count; i++) {
            uint16_t w, h;
            frames[i] = loadSpriteFromSD(paths[i], w, h);
            if (i == 0) {
                outW = w;
                outH = h;
            }
        }
        return {frames, count, frameDurationMs};
    }

    static void freeAnimationSet(AnimationSet *&set) {
        if (!set)
            return;
        for (uint8_t i = 0; i < set->animationCount; i++) {
            Animation &anim = set->animations[i];
            for (uint8_t f = 0; f < anim.frameCount; f++) {
                anim.frames[f]->deleteSprite();
                delete anim.frames[f];
            }
            free(anim.frames);
        }
        free(set->animations);
        free(set);
        set = nullptr;
    }

    static void freeSprite(M5Canvas *&canvas) {
        if (!canvas)
            return;
        canvas->deleteSprite();
        delete canvas;
        canvas = nullptr;
    }
};

struct SceneManager {
    Scene *current{nullptr};
    Scene *next{nullptr};

    void transition(Scene *scene) { next = scene; }

    void update(entt::registry &registry) {
        if (next) {
            if (current)
                current->unload(registry);
            current = next;
            next = nullptr;
            current->load(registry);
        }
        if (current)
            current->update(registry);
    }
};
