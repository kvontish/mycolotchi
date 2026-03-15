#pragma once

#include <M5Unified.h>
#include <SD.h>
#include <entt/entity/registry.hpp>

struct Scene {
    virtual void load(entt::registry &) {}
    virtual void unload(entt::registry &) {}
    virtual void update(entt::registry &) {}
    virtual ~Scene() = default;

    // Loads a PNG from the SD card into a heap-allocated RGB565 buffer.
    // outW and outH are set to the image dimensions on success.
    // Returns nullptr on failure. Caller must free() the buffer in unload().
    static uint16_t *loadSpriteFromSD(const char *path, uint16_t &outW, uint16_t &outH) {
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

        // Decode PNG into a temporary canvas via Stream interface (File inherits Stream),
        // avoiding drawPngFile's DataWrapperT<fs::FS> which fails to instantiate on abstract types
        M5Canvas tmp(&M5.Display);
        tmp.createSprite(outW, outH);
        if (!tmp.getBuffer()) {
            f.close();
            return nullptr;
        }

        tmp.drawPng(&f, 0, 0, outW, outH);
        f.close();

        uint16_t *buf = (uint16_t *)malloc(outW * outH * sizeof(uint16_t));
        if (!buf) {
            tmp.deleteSprite();
            return nullptr;
        }

        memcpy(buf, tmp.getBuffer(), outW * outH * sizeof(uint16_t));
        tmp.deleteSprite();
        return buf;
    }

    // Loads multiple PNGs from the SD card into a heap-allocated AnimationClip.
    // outW and outH are set from the first frame. Caller must free via freeAnimations().
    static Animation loadAnimationFromSD(const char * const *paths, uint8_t count,
                                          uint16_t frameDurationMs,
                                          uint16_t &outW, uint16_t &outH) {
        uint16_t **frames = (uint16_t **)malloc(count * sizeof(uint16_t *));
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
        if (!set) return;
        for (uint8_t i = 0; i < set->animationCount; i++) {
            Animation &anim = set->animations[i];
            for (uint8_t f = 0; f < anim.frameCount; f++)
                free(anim.frames[f]);
            free(anim.frames);
        }
        free(set->animations);
        free(set);
        set = nullptr;
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
