#pragma once

#include <entt/entity/registry.hpp>

struct Scene
{
    void (*load)(entt::registry &){nullptr};
    void (*unload)(entt::registry &){nullptr};
    void (*update)(entt::registry &){nullptr};
};

struct SceneManager
{
    const Scene *current{nullptr};
    const Scene *next{nullptr};

    void transition(const Scene *scene) { next = scene; }

    void update(entt::registry &registry)
    {
        if (next)
        {
            if (current && current->unload)
                current->unload(registry);

            current = next;
            next = nullptr;
            
            if (current->load)
                current->load(registry);
        }
        if (current && current->update)
            current->update(registry);
    }
};
