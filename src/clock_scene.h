#pragma once

#include "components.h"
#include "scene.h"
#include "systems.h"
#include <entt/entity/registry.hpp>

// --- Components ---

struct ClockFieldLabel {
    enum class Field : uint8_t { Hours, Minutes, AmPm, Month, Day, Year };
    Field field;
    uint16_t normalColor;
};

struct ClockEditState {
    bool active{false};
    ClockFieldLabel::Field field{ClockFieldLabel::Field::Hours};
    int8_t hour{12};  // 1-12
    int8_t minute{0}; // 0-59
    bool pm{false};
    int8_t month{1}; // 1-12
    int8_t day{1};   // 1-31
    int16_t year{2025};
};

struct ClockBuffers {
    char hour[3];  // "12\0"
    char min[3];   // "34\0"
    char ampm[3];  // "AM\0"
    char month[4]; // "Jan\0"
    char day[3];   // "15\0"
    char year[5];  // "2025\0"
};

// --- Systems ---

static int8_t clockDaysInMonth(int8_t m, int16_t y) {
    if (m == 2)
        return (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 29 : 28;
    if (m == 4 || m == 6 || m == 9 || m == 11)
        return 30;
    return 31;
}

// Tomohiko Sakamoto's algorithm — returns 0=Sun ... 6=Sat
static int8_t clockWeekday(int16_t y, int8_t m, int8_t d) {
    static const int8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3)
        y--;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

static void applyClockEdit(entt::registry *registry) {
    auto &edit = registry->ctx<ClockEditState>();
    int8_t h24 = edit.hour % 12;
    if (edit.pm)
        h24 += 12;

    m5::rtc_datetime_t dt{};
    dt.time.hours = h24;
    dt.time.minutes = edit.minute;
    dt.time.seconds = 0;
    dt.date.year = edit.year;
    dt.date.month = edit.month;
    dt.date.date = edit.day;
    dt.date.weekDay = clockWeekday(edit.year, edit.month, edit.day);
    M5.Rtc.setDateTime(dt);

    auto &clock = registry->ctx<Clock>();
    clock.hours = h24;
    clock.minutes = edit.minute;
    clock.seconds = 0;
    clock.year = edit.year;
    clock.month = edit.month;
    clock.day = edit.day;
    clock.timestamp = toUnixTime(clock.year, clock.month, clock.day, clock.hours, clock.minutes, clock.seconds);
    edit.active = false;
}

inline void clockInputSystem(entt::registry *registry, const ButtonEvent &e) {
    auto &edit = registry->ctx<ClockEditState>();

    if (!edit.active) {
        if (e.action == ButtonState::LongPressed && e.button == ButtonEvent::Button::A) {
            const auto &clock = registry->ctx<Clock>();
            edit.pm = clock.hours >= 12;
            edit.hour = clock.hours % 12;
            if (edit.hour == 0)
                edit.hour = 12;
            edit.minute = clock.minutes;
            edit.month = clock.month;
            edit.day = clock.day;
            edit.year = clock.year;
            edit.field = ClockFieldLabel::Field::Hours;
            edit.active = true;
        } else if (e.button == ButtonEvent::Button::C) {
            registry->ctx<SceneManager>().transition(registry->ctx<GameMap>().homeScene);
        }
        return;
    }

    static constexpr uint8_t kFieldCount = uint8_t(ClockFieldLabel::Field::Year) + 1;

    switch (e.button) {
    case ButtonEvent::Button::A:
        switch (edit.field) {
        case ClockFieldLabel::Field::Hours:
            edit.hour = (edit.hour % 12) + 1;
            break;
        case ClockFieldLabel::Field::Minutes:
            edit.minute = (edit.minute + 1) % 60;
            break;
        case ClockFieldLabel::Field::AmPm:
            edit.pm = !edit.pm;
            break;
        case ClockFieldLabel::Field::Month:
            edit.month = (edit.month % 12) + 1;
            edit.day = min(edit.day, clockDaysInMonth(edit.month, edit.year));
            break;
        case ClockFieldLabel::Field::Day:
            edit.day = (edit.day % clockDaysInMonth(edit.month, edit.year)) + 1;
            break;
        case ClockFieldLabel::Field::Year:
            edit.year++;
            edit.day = min(edit.day, clockDaysInMonth(edit.month, edit.year));
            break;
        }
        break;
    case ButtonEvent::Button::B:
        if (uint8_t(edit.field) + 1 >= kFieldCount)
            applyClockEdit(registry);
        else
            edit.field = ClockFieldLabel::Field(uint8_t(edit.field) + 1);
        break;
    case ButtonEvent::Button::C:
        applyClockEdit(registry);
        break;
    }
}

inline void syncClockBuffers(entt::registry &registry) {
    static const char *kMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    auto &bufs = registry.ctx<ClockBuffers>();
    const auto &edit = registry.ctx<ClockEditState>();

    if (edit.active) {
        snprintf(bufs.hour, sizeof(bufs.hour), "%02d", edit.hour);
        snprintf(bufs.min, sizeof(bufs.min), "%02d", edit.minute);
        snprintf(bufs.ampm, sizeof(bufs.ampm), "%s", edit.pm ? "PM" : "AM");
        snprintf(bufs.month, sizeof(bufs.month), "%s", kMonths[edit.month - 1]);
        snprintf(bufs.day, sizeof(bufs.day), "%02d", edit.day);
        snprintf(bufs.year, sizeof(bufs.year), "%04d", edit.year);
    } else {
        const auto &clock = registry.ctx<Clock>();
        bool pm = clock.hours >= 12;
        int8_t h = clock.hours % 12;
        if (h == 0)
            h = 12;
        snprintf(bufs.hour, sizeof(bufs.hour), "%02d", h);
        snprintf(bufs.min, sizeof(bufs.min), "%02d", clock.minutes);
        snprintf(bufs.ampm, sizeof(bufs.ampm), "%s", pm ? "PM" : "AM");
        snprintf(bufs.month, sizeof(bufs.month), "%s", kMonths[clock.month - 1]);
        snprintf(bufs.day, sizeof(bufs.day), "%02d", clock.day);
        snprintf(bufs.year, sizeof(bufs.year), "%04d", clock.year);
    }
}

inline void highlightClockField(entt::registry &registry) {
    const auto &edit = registry.ctx<ClockEditState>();
    registry.view<ClockFieldLabel, Label>().each([&edit](const ClockFieldLabel &cfl, Label &label) {
        label.color = (edit.active && edit.field == cfl.field) ? uint16_t(TFT_YELLOW) : cfl.normalColor;
    });
}

class ClockScene : public Scene {
  public:
    void load(entt::registry &registry) override {
        registry.set<ClockEditState>();
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
