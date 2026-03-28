#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

// --- Components ---

struct MenuItem {
    const char     *label;
    const MenuItem *children; // nullptr if leaf
    uint8_t         childCount;
    void (*action)(entt::registry &); // called on leaf selection, nullptr if has children
};

struct MenuState {
    struct Frame {
        const MenuItem *node;
        uint8_t         selectedIdx;
    };

    static constexpr uint8_t kMaxDepth   = 4;
    static constexpr uint8_t kMaxVisible = 6;

    Frame   stack[kMaxDepth]{};
    uint8_t depth{0};
    bool    dirty{true};

    entt::entity titleEntity{entt::null};
    entt::entity itemEntities[kMaxVisible]{};

    const MenuItem *currentNode() const { return stack[depth - 1].node; }
    uint8_t        &selectedIdx() { return stack[depth - 1].selectedIdx; }
};

// --- Menu tree ---

static void menuPlay(entt::registry &r) { r.ctx<SceneManager>().transition(r.ctx<GameMap>().gameScene); }
static void menuWalk(entt::registry &r) { r.ctx<SceneManager>().transition(r.ctx<GameMap>().walkScene); }
static void menuStatus(entt::registry &r) { r.ctx<SceneManager>().pushView(r, r.ctx<GameMap>().statusView); }

static void menuTendSubstrate(entt::registry &r) {
    auto &pet     = r.ctx<Pet>();
    pet.substrate = (uint8_t)min((int)pet.substrate + 20, 100);
    r.ctx<SceneManager>().transition(r.ctx<GameMap>().homeScene);
}

static void menuTendMist(entt::registry &r) {
    auto &pet    = r.ctx<Pet>();
    pet.moisture = (uint8_t)min((int)pet.moisture + 15, 100);
    r.ctx<SceneManager>().transition(r.ctx<GameMap>().homeScene);
}

static const MenuItem kMenuTendItems[] = {
    {"Substrate", nullptr, 0, menuTendSubstrate},
    {"Mist", nullptr, 0, menuTendMist},
};

static const MenuItem kMenuPlayItems[] = {
    {"Game", nullptr, 0, menuPlay},
    {"Walk", nullptr, 0, menuWalk},
};

static const MenuItem kMenuRootItems[] = {
    {"Status", nullptr, 0, menuStatus},
    {"Tend", kMenuTendItems, 2, nullptr},
    {"Play", kMenuPlayItems, 2, nullptr},
};

static const MenuItem kMenuRoot = {"Menu", kMenuRootItems, 3, nullptr};

// --- Systems ---

static constexpr int16_t kMenuTitleY     = 14;
static constexpr int16_t kMenuItemStartY = 38;
static constexpr int16_t kMenuItemStepY  = 14;

inline void refreshMenuLabels(entt::registry &registry) {
    auto &state = registry.ctx<MenuState>();
    if (!state.dirty) return;

    const MenuItem *node = state.currentNode();

    registry.get<Label>(state.titleEntity).text = node->label;

    for (uint8_t i = 0; i < MenuState::kMaxVisible; i++) {
        auto &label = registry.get<Label>(state.itemEntities[i]);
        if (node->children && i < node->childCount) {
            label.text  = node->children[i].label;
            label.color = (i == state.selectedIdx()) ? uint16_t(TFT_YELLOW) : uint16_t(TFT_WHITE);
        } else {
            label.text  = "";
            label.color = uint16_t(TFT_BLACK);
        }
    }

    state.dirty = false;
}

inline void menuInputSystem(entt::registry *registry, const ButtonEvent &e) {
    if (registry->ctx<SceneManager>().isViewActive()) return;
    auto           &state = registry->ctx<MenuState>();
    const MenuItem *node  = state.currentNode();

    switch (e.button) {
    case ButtonEvent::Button::A: // cycle through items
        if (node->childCount > 0) state.selectedIdx() = (state.selectedIdx() + 1) % node->childCount;
        state.dirty = true;
        break;

    case ButtonEvent::Button::B: { // select / enter submenu
        if (!node->children) break;
        const MenuItem &item = node->children[state.selectedIdx()];
        if (item.childCount > 0 && item.children) {
            if (state.depth < MenuState::kMaxDepth) {
                state.stack[state.depth] = {&item, 0};
                state.depth++;
                state.dirty = true;
            }
        } else if (item.action) {
            item.action(*registry);
        }
        break;
    }

    case ButtonEvent::Button::C: // back
        if (state.depth > 1) {
            state.depth--;
            state.dirty = true;
        } else {
            registry->ctx<SceneManager>().transition(registry->ctx<GameMap>().homeScene);
        }
        break;
    }
}

class MenuScene : public Scene {
  public:
    void load(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&menuInputSystem>(&registry);

        auto &state    = registry.set<MenuState>();
        state.stack[0] = {&kMenuRoot, 0};
        state.depth    = 1;
        state.dirty    = true;

        state.titleEntity = registry.create();
        registry.emplace<Position>(state.titleEntity, int16_t(80), int16_t(kMenuTitleY));
        registry.emplace<Label>(state.titleEntity, kMenuRoot.label, uint16_t(TFT_WHITE), uint8_t(2));

        for (uint8_t i = 0; i < MenuState::kMaxVisible; i++) {
            state.itemEntities[i] = registry.create();
            registry.emplace<Position>(
                state.itemEntities[i], int16_t(80), int16_t(kMenuItemStartY + i * kMenuItemStepY));
            registry.emplace<Label>(state.itemEntities[i], "", uint16_t(TFT_BLACK), uint8_t(1));
        }
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&menuInputSystem>(&registry);
        registry.unset<MenuState>();
        registry.clear();
    }

    void update(entt::registry &registry) override {
        pollInput(registry);
        refreshMenuLabels(registry);
    }
};

inline MenuScene menuScene;
