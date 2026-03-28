#pragma once

#include <entt/entity/registry.hpp>

struct Scene {
    virtual void load(entt::registry &) {}
    virtual void unload(entt::registry &) {}
    virtual void update(entt::registry &) {}
    virtual ~Scene() = default;
};

struct View {
    virtual void load(entt::registry &) {}
    virtual void unload(entt::registry &) {}
    virtual void update(entt::registry &) {}
    virtual ~View() = default;
};

struct SceneManager {
    Scene *current{nullptr};
    Scene *next{nullptr};

    static constexpr uint8_t kMaxViewDepth = 4;
    View                    *viewStack[kMaxViewDepth]{};
    uint8_t                  viewDepth{0};
    bool                     justPopped{false}; // stays true until the end of the current update tick
    bool                     pendingPop{false};

    void transition(Scene *scene) { next = scene; }

    void pushView(entt::registry &registry, View *view) {
        if (viewDepth < kMaxViewDepth) {
            viewStack[viewDepth++] = view;
            view->load(registry);
        }
    }

    void popView() { pendingPop = true; }

    // Returns true if a view is active OR was popped this tick (prevents the
    // underlying scene's input system from consuming the same event that triggered the pop)
    bool isViewActive() const { return viewDepth > 0 || justPopped; }

    View *activeView() const { return viewDepth > 0 ? viewStack[viewDepth - 1] : nullptr; }

    void update(entt::registry &registry) {
        justPopped = false;
        if (pendingPop) {
            pendingPop = false;
            if (viewDepth > 0) {
                viewStack[--viewDepth]->unload(registry);
                justPopped = true;
            }
        }
        if (next) {
            while (viewDepth > 0)
                viewStack[--viewDepth]->unload(registry);
            if (current) current->unload(registry);
            current = next;
            next    = nullptr;
            current->load(registry);
        }
        if (viewDepth > 0)
            viewStack[viewDepth - 1]->update(registry);
        else if (current)
            current->update(registry);
    }
};
