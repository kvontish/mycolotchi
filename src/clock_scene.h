#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

class ClockScene : public Scene {
  public:
    Scene *prevScene{nullptr};

    void load(entt::registry &registry) override {
        auto &edit = registry.set<ClockEditState>();
        edit.prevScene = prevScene;
        registry.set<ClockBuffers>();
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&clockInputSystem>(&registry);

        auto &bufs = registry.ctx<ClockBuffers>();

        // Time line — "HH:MM AM" = 144px wide, centered in 160px → left edge x=8
        // Size 3: each char 18px wide. Centers: HH=26, ':'=53, MM=80, AM=134
        auto hour = registry.create();
        registry.emplace<Position>(hour, int16_t(26), int16_t(45));
        registry.emplace<Label>(hour, (const char *)bufs.hour, uint16_t(TFT_WHITE), uint8_t(3));
        registry.emplace<ClockFieldLabel>(hour, ClockFieldLabel::Field::Hours, uint16_t(TFT_WHITE));

        auto colon = registry.create();
        registry.emplace<Position>(colon, int16_t(53), int16_t(45));
        registry.emplace<Label>(colon, ":", uint16_t(TFT_WHITE), uint8_t(3));

        auto min = registry.create();
        registry.emplace<Position>(min, int16_t(80), int16_t(45));
        registry.emplace<Label>(min, (const char *)bufs.min, uint16_t(TFT_WHITE), uint8_t(3));
        registry.emplace<ClockFieldLabel>(min, ClockFieldLabel::Field::Minutes, uint16_t(TFT_WHITE));

        auto ampm = registry.create();
        registry.emplace<Position>(ampm, int16_t(134), int16_t(45));
        registry.emplace<Label>(ampm, (const char *)bufs.ampm, uint16_t(TFT_WHITE), uint8_t(3));
        registry.emplace<ClockFieldLabel>(ampm, ClockFieldLabel::Field::AmPm, uint16_t(TFT_WHITE));

        // Date line — "Jan 15 2025" = 66px wide, centered in 160px → left edge x=47
        // Size 1: each char 6px wide. Centers: month=56, day=77, year=101
        auto month = registry.create();
        registry.emplace<Position>(month, int16_t(56), int16_t(78));
        registry.emplace<Label>(month, (const char *)bufs.month, uint16_t(TFT_LIGHTGREY), uint8_t(1));
        registry.emplace<ClockFieldLabel>(month, ClockFieldLabel::Field::Month, uint16_t(TFT_LIGHTGREY));

        auto day = registry.create();
        registry.emplace<Position>(day, int16_t(77), int16_t(78));
        registry.emplace<Label>(day, (const char *)bufs.day, uint16_t(TFT_LIGHTGREY), uint8_t(1));
        registry.emplace<ClockFieldLabel>(day, ClockFieldLabel::Field::Day, uint16_t(TFT_LIGHTGREY));

        auto year = registry.create();
        registry.emplace<Position>(year, int16_t(101), int16_t(78));
        registry.emplace<Label>(year, (const char *)bufs.year, uint16_t(TFT_LIGHTGREY), uint8_t(1));
        registry.emplace<ClockFieldLabel>(year, ClockFieldLabel::Field::Year, uint16_t(TFT_LIGHTGREY));
    }

    void unload(entt::registry &registry) override {
        registry.ctx<entt::dispatcher>().sink<ButtonEvent>().disconnect<&clockInputSystem>(&registry);
        registry.unset<ClockEditState>();
        registry.unset<ClockBuffers>();
        registry.clear();
    }

    void update(entt::registry &registry) override {
        pollInput(registry);
        syncClockBuffers(registry);
        highlightClockField(registry);
    }
};

inline ClockScene clockScene;
