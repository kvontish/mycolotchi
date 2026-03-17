#pragma once

#include <entt/entity/registry.hpp>

struct Scene {
    virtual void load(entt::registry &) {}
    virtual void unload(entt::registry &) {}
    virtual void update(entt::registry &) {}
    virtual ~Scene() = default;
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
