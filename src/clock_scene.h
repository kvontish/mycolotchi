#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <M5Unified.h>
#include <entt/entity/registry.hpp>

class ClockScene : public Scene {
    entt::registry *mRegistry{nullptr};
    char mTimeBuf[9]; // "HH:MM:SS\0"

    void onButton(const ButtonEvent &e) {
        if (e.action != ButtonEvent::Action::Pressed)
            return;

        if (e.button == ButtonEvent::Button::C)
            mRegistry->ctx<SceneManager>().transition(prevScene);
    }

  public:
    Scene *prevScene{nullptr};

    void load(entt::registry &registry) override {
        mRegistry = &registry;
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&ClockScene::onButton>(this);

        const auto &camera = registry.ctx<Camera>();

        mTimeBuf[0] = '\0';
        auto timeLabel = registry.create();
        registry.emplace<Position>(timeLabel, int16_t(camera.w / 2), int16_t(camera.h / 2));
        registry.emplace<Label>(timeLabel, (const char *)mTimeBuf, uint16_t(TFT_WHITE), uint8_t(3));
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&ClockScene::onButton>(this);
        mRegistry = nullptr;
        registry.clear();
    }

    void update(entt::registry &registry) override {
        pollInput(registry);

        auto t = M5.Rtc.getTime();
        snprintf(mTimeBuf, sizeof(mTimeBuf), "%02d:%02d:%02d", t.hours, t.minutes, t.seconds);
    }
};

inline ClockScene clockScene;
