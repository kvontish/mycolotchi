#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <M5Unified.h>
#include <entt/entity/registry.hpp>

class ClockScene : public Scene {
    entt::registry *mRegistry{nullptr};
    char mTimeBuf[9];  // "HH:MM AM\0"
    char mDateBuf[12]; // "Jan 15 2025\0"

    void onButton(const ButtonEvent &e) {
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
        mDateBuf[0] = '\0';

        auto timeLabel = registry.create();
        registry.emplace<Position>(timeLabel, int16_t(camera.w / 2), int16_t(45));
        registry.emplace<Label>(timeLabel, (const char *)mTimeBuf, uint16_t(TFT_WHITE), uint8_t(3));

        auto dateLabel = registry.create();
        registry.emplace<Position>(dateLabel, int16_t(camera.w / 2), int16_t(78));
        registry.emplace<Label>(dateLabel, (const char *)mDateBuf, uint16_t(TFT_LIGHTGREY), uint8_t(1));
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&ClockScene::onButton>(this);
        mRegistry = nullptr;
        registry.clear();
    }

    void update(entt::registry &registry) override {
        pollInput(registry);

        static const char *kMonths[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

        auto t = M5.Rtc.getTime();
        int8_t h = t.hours;
        const char *ampm = h >= 12 ? "PM" : "AM";
        h = h % 12;
        if (h == 0)
            h = 12;
        snprintf(mTimeBuf, sizeof(mTimeBuf), "%02d:%02d %s", h, t.minutes, ampm);

        auto d = M5.Rtc.getDate();
        snprintf(mDateBuf, sizeof(mDateBuf), "%s %02d %04d", kMonths[d.month - 1], d.date, d.year);
    }
};

inline ClockScene clockScene;
